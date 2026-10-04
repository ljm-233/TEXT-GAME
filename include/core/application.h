#pragma once
//
// 生命周期编排者。
//
// exec() 走五个 phase：
//
//     boot     -- _boot_fn 把所有东西 register 进容器
//     setup    -- 确保前置单例就绪，抓前端的 main_loop
//     wire     -- 跑 on_boot 钩子（连信号、初始化子系统）
//     run      -- 跑 on_loop 钩子 + main_loop.run()
//     teardown -- 跑 on_final 钩子
//
// 四类钩子全部挂在 Application 上，Container 只做注册表。
// 组件（前端 / 服务）只暴露方法，不需要知道自己是第几个被调的。
//
// 与 albuswall 的 core/application.py 对应。两点 C++ 化差异：
//   1. 那边用"单例门面 Application + 内部 _Application"两层，
//      是为了绕开 Python 的模块级构造与 __getattr__ 委托；
//      C++ 里没有这个约束，合并成一个类，API 形状保持一致。
//   2. 那边的 sys.excepthook / threading.excepthook 换成
//      std::set_terminate，语义仍是"先把异常写出去，再二次委托"。
//
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/container.h"
#include "core/main_loop.h"

class Application {
public:
    /// 由入口提供：只做注册，不产生副作用。
    using BootFn = std::function<void(Application&)>;

    using LifecycleHook = std::function<void()>;
    using LoopHook      = std::function<void(MainLoop&)>;
    using QuitHook      = std::function<void(const std::string&)>;

    /// 异常处理钩子。返回 false 表示"我处理不了，请继续往下传"，
    /// 由 Application 二次委托给默认处理（写 stderr）。
    using ExceptionHandler = std::function<bool(const std::exception&)>;

    static Application& instance();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // ============================================================
    // 钩子注册（先注册先执行）
    // ============================================================

    /// 进入主循环前的接线钩子。
    Application& onBoot(LifecycleHook fn);

    /// 主循环启动前的钩子，可以拿到 main_loop。
    Application& onLoop(LoopHook fn);

    /// 主动退出时的钩子（主循环之前）。
    Application& onQuit(QuitHook fn);

    /// 退出收尾钩子。注册顺序 = 执行顺序，所以"后注册的先被依赖"的
    /// 组件应该后注册。
    Application& onFinal(LifecycleHook fn);

    // ============================================================
    // 编排
    // ============================================================

    /// 提供注册函数。此时不构造任何实例。
    void boot(BootFn fn);

    /// 跑完整生命周期，返回退出码。
    int exec();

    /// 请求退出：先跑 on_quit 钩子，再让主循环停下。
    void requestQuit(const std::string& reason = "");

    // ============================================================
    // 访问器
    // ============================================================

    Container& container() { return container_; }

    /// 未设置时惰性回退 HeadlessMainLoop：核心脱离界面也能跑完生命周期。
    MainLoop& mainLoop();

    /// 前端是否已经交出主循环（false 表示将走 Headless 兜底）。
    bool hasMainLoop() const { return static_cast<bool>(mainLoop_); }

    // ============================================================
    // 异常处理
    // ============================================================

    void setExceptionHandler(ExceptionHandler fn);

    /// 返回 true 表示已被处理，false 表示调用方应继续往上抛。
    bool handleException(const std::exception& e);

    /// 把当前容器状态 dump 成多行文本（调试用）。
    std::string str() const;

private:
    Application();

    // ---------- 五个 phase ----------
    void phaseBoot();
    void phaseSetup();
    void phaseWire();
    int  phaseRun();
    void phaseTeardown();

    /// 单个钩子失败不阻断其余钩子，对应 albuswall 的
    /// "某个钩子失败不阻断其余钩子"。
    void runLifecycleHook(const char* phase, const LifecycleHook& fn);

    void installTerminateHandler();

    Container container_;
    std::shared_ptr<MainLoop> mainLoop_;

    BootFn bootFn_;
    std::vector<LifecycleHook> bootHooks_;
    std::vector<LifecycleHook> finalHooks_;
    std::vector<QuitHook>      quitHooks_;
    std::vector<LoopHook>      loopHooks_;

    ExceptionHandler exceptionHandler_;
};
