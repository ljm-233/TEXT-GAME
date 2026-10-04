#pragma once
//
// 主循环抽象。
//
// 这是"前端"与"核心"之间唯一的边界：核心只知道有人会跑一个阻塞循环，
// 并且能被请求退出；至于循环体里是 SFML 窗口、Qt 事件循环还是什么都没有，
// 核心不关心。
//
// 与 albuswall 的 core/main_loop.py 对应：那边的 MainLoop 是 ABC，
// 这边是纯虚基类；HeadlessMainLoop 的语义完全一致。
//
#include <condition_variable>
#include <mutex>
#include <string>

class MainLoop {
public:
    virtual ~MainLoop() = default;

    /// 阻塞直到退出，返回进程退出码。
    virtual int run() = 0;

    /// 请求退出。可能被任意线程调用。
    virtual void quit() = 0;

    int exitCode() const { return exitCode_; }

    /// 调试用快照，对应 albuswall 各处的 __str__。
    virtual std::string str() const;

protected:
    /// 约定：-1 表示"还没跑完"，0 表示正常退出，其余为错误码。
    int exitCode_ = -1;
};

/// 无前端：条件变量阻塞主线程，CPU 占用为 0。
///
/// 没有 UI 插件时 Application 会惰性回退到它，这样"核心能脱离界面单独跑"
/// 不是一句口号，而是默认行为。
class HeadlessMainLoop : public MainLoop {
public:
    HeadlessMainLoop() = default;

    int run() override;
    void quit() override;
    std::string str() const override;

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false;
};
