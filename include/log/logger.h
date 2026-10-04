#pragma once
//
// 日志门面：组装记录，分发给所有 handler。
//
// 职责边界刻意收窄 —— 它不认识文件、不认识终端、不认识颜色，
// 只做三件事：级别过滤、打时间戳、往 handler 列表里分发。
//
// 与 albuswall 的 log/logger.py 对应。旧 core/logging.h 里的
// "打开文件 + 轮转 + 上色 + 打印" 四件事被拆到 FileHandler /
// ConsoleHandler / ConsoleFormatter，Logger 只剩分发。
//
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "log/protocol.h"

class Logger {
public:
    explicit Logger(LogLevel minLevel = LogLevel::Info);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // ---------------- 装配 ----------------

    void addHandler(std::shared_ptr<LogHandler> handler);

    /// 当前挂载的 handler 快照。
    std::vector<std::shared_ptr<LogHandler>> handlers() const;

    // ---------------- 级别 ----------------

    void setMinLevel(LogLevel level);
    LogLevel minLevel() const;

    /// 该级别此刻是否会被输出。
    bool isEnabled(LogLevel level) const;

    // ---------------- 轮转 ----------------

    /// 转发给所有支持轮转的 handler（目前只有 FileHandler）。
    /// 保留这个方法是为了让设置界面的调用点不必认识具体 handler 类型。
    void setRotation(size_t maxFileSize, int keepFiles);

    // ---------------- 输出 ----------------

    void log(LogLevel level, const std::string& message);

    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    /// 不受级别阈值过滤的通道：给"必须让人看到"的输出用。
    /// 这是旧 Logger 的既有语义（LogLevel::Normal 永不被 minLevel 挡掉）。
    void normal(const std::string& message);

    void flush();

    /// 调试用快照，对应 albuswall 的 __str__。
    std::string str() const;

private:
    /// 统一的本地时间戳，保证同一条记录落在多个 handler 里完全一致。
    static std::string timestamp();

    mutable std::mutex mutex_;
    LogLevel minLevel_;
    std::vector<std::shared_ptr<LogHandler>> handlers_;
};
