#include "core/main_loop.h"

#include <chrono>
#include <csignal>

namespace {

/// 信号处理器只能碰 sig_atomic_t / volatile，不能加锁、不能碰条件变量，
/// 所以这里只置一个标志，真正的唤醒交给 run() 的轮询。
volatile std::sig_atomic_t g_stopRequested = 0;

extern "C" void onStopSignal(int /*signum*/) {
    g_stopRequested = 1;
}

/// 只在主线程安装一次。重复安装会覆盖别人的处理器，所以用静态标记挡住。
void installStopSignals() {
    static bool installed = false;
    if (installed)
        return;
    installed = true;

    std::signal(SIGINT, onStopSignal);
    std::signal(SIGTERM, onStopSignal);
}

}  // namespace

std::string MainLoop::str() const {
    return "MainLoop(exitCode=" + std::to_string(exitCode_) + ")";
}

int HeadlessMainLoop::run() {
    installStopSignals();

    std::unique_lock<std::mutex> lock(mutex_);
    // 用带超时的等待而不是无脑 wait()：这样即使没有 UI，
    // Ctrl+C 也能在一个轮询周期内退出来。
    while (!stopped_ && g_stopRequested == 0) {
        cv_.wait_for(lock, std::chrono::milliseconds(100));
    }

    if (g_stopRequested != 0) {
        // 128 + signum 是 Unix shell 的约定，这里只区分"被信号叫停"。
        exitCode_ = 130;
    } else {
        exitCode_ = 0;
    }
    return exitCode_;
}

void HeadlessMainLoop::quit() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    cv_.notify_all();
}

std::string HeadlessMainLoop::str() const {
    return "HeadlessMainLoop(exitCode=" + std::to_string(exitCode_) + ")";
}
