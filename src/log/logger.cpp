#include "log/logger.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <utility>

Logger::Logger(LogLevel minLevel) : minLevel_(minLevel) {}

// ---------------- 装配 ----------------

void Logger::addHandler(std::shared_ptr<LogHandler> handler) {
    if (!handler)
        return;
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.push_back(std::move(handler));
}

std::vector<std::shared_ptr<LogHandler>> Logger::handlers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return handlers_;
}

// ---------------- 级别 ----------------

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

LogLevel Logger::minLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return minLevel_;
}

bool Logger::isEnabled(LogLevel level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return logLevelAtLeast(level, minLevel_);
}

// ---------------- 轮转 ----------------

void Logger::setRotation(size_t maxFileSize, int keepFiles) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& handler : handlers_) {
        if (auto* rotatable = dynamic_cast<RotatableHandler*>(handler.get()))
            rotatable->setRotation(maxFileSize, keepFiles);
    }
}

// ---------------- 输出 ----------------

std::string Logger::timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!logLevelAtLeast(level, minLevel_))
        return;

    LogRecord record;
    record.level = level;
    record.timestamp = timestamp();
    record.message = message;

    for (const auto& handler : handlers_) {
        // 一个落点写失败（磁盘满、终端断开）不该把整个程序带走，
        // 也不该阻止其余落点写完这条日志。
        try {
            handler->handle(record);
        } catch (const std::exception& e) {
            std::cerr << "[text-game] 日志 handler 失败: " << e.what() << '\n';
        }
    }
}

void Logger::trace(const std::string& message) { log(LogLevel::Trace, message); }
void Logger::debug(const std::string& message) { log(LogLevel::Debug, message); }
void Logger::info(const std::string& message) { log(LogLevel::Info, message); }
void Logger::warn(const std::string& message) { log(LogLevel::Warn, message); }
void Logger::error(const std::string& message) { log(LogLevel::Error, message); }
void Logger::normal(const std::string& message) { log(LogLevel::Normal, message); }

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& handler : handlers_)
        handler->flush();
}

// ---------------- 调试 ----------------

std::string Logger::str() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string out = "Logger(minLevel=" + std::string(logLevelName(minLevel_)) +
                      ", handlers=" + std::to_string(handlers_.size()) + "\n";
    for (const auto& handler : handlers_)
        out += "  - " + handler->str() + "\n";
    out += ")";
    return out;
}
