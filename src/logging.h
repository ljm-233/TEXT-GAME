#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>
#include <stdexcept>

enum class LogLevel { Debug, Info, Warn, Error, Trace};

class Logger{
public:
    explicit Logger(const std::string& filename)
        : out_(filename, std::ios::app) {
        if (!out_) throw std::runtime_error("无法打开日志文件: " + filename);
    }

    void log(LogLevel level, const std::string& msg) {
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

        std::cout << timebuf << " [" << levelToString(level) << "] " << msg << '\n';
    }

    void debug(const std::string& msg) { log(LogLevel::Debug, msg); }
    void info(const std::string& msg)  { log(LogLevel::Info, msg); }
    void warn(const std::string& msg)  { log(LogLevel::Warn, msg); }
    void error(const std::string& msg) { log(LogLevel::Error, msg); }
    void trace(const std::string& msg) { log(LogLevel::Trace, msg); }

private:
    std::ofstream out_;
    std::mutex mutex_;

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO";
            case LogLevel::Warn:  return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Trace: return "TRACE";
        }
        return "UNKNOWN";
    }
};