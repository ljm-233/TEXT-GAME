#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>
#include <stdexcept>

// Trace < Debug < Info < Warn < Error；Normal 是给用户的普通提示，不被过滤
enum class LogLevel { Trace, Debug, Info, Warn, Error, Normal };

class Logger {
public:
    explicit Logger(const std::string& filename,
                    LogLevel minLevel = LogLevel::Info)
        : out_(filename, std::ios::app), minLevel_(minLevel) {
        if (!out_) throw std::runtime_error("无法打开日志文件: " + filename);
    }

    void setMinLevel(LogLevel l) { minLevel_ = l; }
    LogLevel getMinLevel() const { return minLevel_; }

    void log(LogLevel level, const std::string& msg) {
        // Normal 不受级别过滤，其余按 minLevel_ 比较
        if (level != LogLevel::Normal && level < minLevel_) return;

        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        char timebuf[32];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);

        out_ << timebuf << " [" << levelToString(level) << "] " << msg << '\n';
        out_.flush();

        const char* color = "\033[0m";
        switch (level) {
            case LogLevel::Debug:  color = "\033[36m"; break;
            case LogLevel::Info:   color = "\033[32m"; break;
            case LogLevel::Warn:   color = "\033[33m"; break;
            case LogLevel::Error:  color = "\033[31m"; break;
            case LogLevel::Trace:  color = "\033[90m"; break;
            case LogLevel::Normal: color = "\033[0m";  break;
        }
        std::cout << color << timebuf << " [" << levelToString(level) << "] "
                  << msg << "\033[0m" << '\n';
    }

    void debug(const std::string& m) { log(LogLevel::Debug, m); }
    void info (const std::string& m) { log(LogLevel::Info,  m); }
    void warn (const std::string& m) { log(LogLevel::Warn,  m); }
    void error(const std::string& m) { log(LogLevel::Error, m); }
    void trace(const std::string& m) { log(LogLevel::Trace, m); }
    void normal(const std::string& m){ log(LogLevel::Normal,m); }

private:
    std::ofstream out_;
    std::mutex mutex_;
    LogLevel minLevel_;

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:  return "DEBUG";
            case LogLevel::Info:   return "INFO";
            case LogLevel::Warn:   return "WARN";
            case LogLevel::Error:  return "ERROR";
            case LogLevel::Trace:  return "TRACE";
            case LogLevel::Normal: return "NORMAL";
        }
        return "UNKNOWN";
    }
};