#include "core/application.h"

#include <cstdlib>
#include <iostream>

namespace {

/// std::set_terminate 只接受无捕获的函数指针，所以"上一个处理器"
/// 只能放在文件作用域。语义对应 albuswall 里保存的 _origin_sys_excepthook。
std::terminate_handler g_previousTerminate = nullptr;

/// 兜底输出：故意不依赖 Application / Logger 的任何状态。
/// terminate 可能发生在构造失败的过程中，此时碰单例只会二次崩溃。
/// 这一点与 albuswall 把 bootstrap hook 放在"任何可能失败的 import 之前"同理。
void reportTerminate() {
    if (auto current = std::current_exception()) {
        try {
            std::rethrow_exception(current);
        } catch (const std::exception& e) {
            std::cerr << "[text-game][bootstrap] 未捕获异常: " << e.what() << '\n';
        } catch (...) {
            std::cerr << "[text-game][bootstrap] 未捕获异常（非 std::exception）\n";
        }
    } else {
        std::cerr << "[text-game][bootstrap] std::terminate（无活动异常）\n";
    }
    std::cerr.flush();
}

}  // namespace

// ============================================================
// 单例
// ============================================================

Application& Application::instance() {
    static Application inst;
    return inst;
}

Application::Application() {
    installTerminateHandler();
}

void Application::installTerminateHandler() {
    g_previousTerminate = std::set_terminate([]() {
        reportTerminate();
        // 二次委托：先写出去，再把处置权交回原来的处理器（默认是 abort）
        if (g_previousTerminate != nullptr) {
            g_previousTerminate();
            return;
        }
        std::abort();
    });
}

// ============================================================
// 钩子注册
// ============================================================

Application& Application::onBoot(LifecycleHook fn) {
    bootHooks_.push_back(std::move(fn));
    return *this;
}

Application& Application::onLoop(LoopHook fn) {
    loopHooks_.push_back(std::move(fn));
    return *this;
}

Application& Application::onQuit(QuitHook fn) {
    quitHooks_.push_back(std::move(fn));
    return *this;
}

Application& Application::onFinal(LifecycleHook fn) {
    finalHooks_.push_back(std::move(fn));
    return *this;
}

// ============================================================
// 编排
// ============================================================

void Application::boot(BootFn fn) {
    bootFn_ = std::move(fn);
}

int Application::exec() {
    phaseBoot();

    // teardown 必须跑：哪怕 setup / wire / run 中途抛出来也要收尾。
    // C++ 里用 RAII 守卫表达 Python 的 try/finally。
    struct TeardownGuard {
        Application* self;
        ~TeardownGuard() { self->phaseTeardown(); }
    } guard{this};

    phaseSetup();
    phaseWire();
    return phaseRun();
}

MainLoop& Application::mainLoop() {
    if (!mainLoop_)
        mainLoop_ = std::make_shared<HeadlessMainLoop>();
    return *mainLoop_;
}

void Application::requestQuit(const std::string& reason) {
    for (const auto& fn : quitHooks_) {
        try {
            fn(reason);
        } catch (const std::exception& e) {
            handleException(e);
        }
    }

    if (mainLoop_)
        mainLoop_->quit();
}

// ---------- phase 1: boot ----------

void Application::phaseBoot() {
    // 只跑注册函数。此时容器里只有工厂，没有实例。
    if (bootFn_)
        bootFn_(*this);
}

// ---------- phase 2: setup ----------

void Application::phaseSetup() {
    // 构造顺序显式化：路径 -> 配置。后面的 logger / window 工厂都依赖这两者，
    // 由这里决定谁先落地，而不是靠"谁先被解析"碰运气。
    // 用 contains() 守卫是因为"注册了什么"由入口的 boot 决定。
    for (const char* name : {"paths", "preferences"}) {
        if (container_.contains(name))
            container_.touch(name);
    }

    // 前端可选：注册了 main_loop 就抓过来；
    // 没注册则等 mainLoop() 惰性回退 HeadlessMainLoop。
    if (auto loop = container_.tryGet<MainLoop>("main_loop"))
        mainLoop_ = loop;
}

// ---------- phase 3: wire ----------

void Application::phaseWire() {
    for (const auto& fn : bootHooks_)
        runLifecycleHook("on_boot", fn);
}

// ---------- phase 4: run ----------

int Application::phaseRun() {
    MainLoop& loop = mainLoop();

    for (const auto& fn : loopHooks_) {
        try {
            fn(loop);
        } catch (const std::exception& e) {
            handleException(e);
        }
    }

    return loop.run();
}

// ---------- phase 5: teardown ----------

void Application::phaseTeardown() {
    for (const auto& fn : finalHooks_)
        runLifecycleHook("on_final", fn);
}

void Application::runLifecycleHook(const char* phase, const LifecycleHook& fn) {
    // 某个钩子失败不阻断其余钩子：收尾阶段尤其不能让一个组件
    // 的异常把别的组件的释放动作一起吞掉。
    try {
        fn();
    } catch (const std::exception& e) {
        std::cerr << "[text-game] " << phase << " 钩子失败: " << e.what() << '\n';
        handleException(e);
    }
}

// ============================================================
// 异常处理
// ============================================================

void Application::setExceptionHandler(ExceptionHandler fn) {
    exceptionHandler_ = std::move(fn);
}

bool Application::handleException(const std::exception& e) {
    if (exceptionHandler_ && exceptionHandler_(e))
        return true;

    std::cerr << "[text-game] 未处理异常: " << e.what() << '\n';
    return false;
}

// ============================================================
// 调试
// ============================================================

std::string Application::str() const {
    std::string out = "Application(\n";

    out += "  main_loop: ";
    out += mainLoop_ ? mainLoop_->str() : std::string("(未设置，将回退 Headless)");
    out += '\n';

    out += "  hooks: boot=" + std::to_string(bootHooks_.size()) +
           " loop=" + std::to_string(loopHooks_.size()) +
           " quit=" + std::to_string(quitHooks_.size()) +
           " final=" + std::to_string(finalHooks_.size()) + '\n';

    out += "  " + container_.str() + "\n)";
    return out;
}
