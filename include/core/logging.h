#pragma once
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

enum class LogLevel { Trace, Debug, Info, Warn, Error, Normal };

class Logger {
public:
    // maxFileSize = 0 表示不轮转
    explicit Logger(const std::string& filename, LogLevel minLevel = LogLevel::Info,
                    size_t maxFileSize = 0, int keepFiles = 3)
          : path_(filename),
            minLevel_(minLevel),
            maxFileSize_(maxFileSize),
            keepFiles_(keepFiles) {
        openFile();
    }

    void setMinLevel(LogLevel l) { minLevel_ = l; }
    LogLevel getMinLevel() const { return minLevel_; }

    void setRotation(size_t maxSize, int keep) {
        maxFileSize_ = maxSize;
        keepFiles_ = keep;
    }

    void log(LogLevel level, const std::string& msg) {
        if (level != LogLevel::Normal && level < minLevel_)
            return;
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

        // 文件超过阈值就轮转
        if (maxFileSize_ > 0) {
            std::error_code ec;
            auto sz = std::filesystem::file_size(path_, ec);
            if (!ec && sz >= maxFileSize_)
                rotate();
        }

        const char* color = "\033[0m";
        switch (level) {
        case LogLevel::Debug:
            color = "\033[36m";
            break;
        case LogLevel::Info:
            color = "\033[32m";
            break;
        case LogLevel::Warn:
            color = "\033[33m";
            break;
        case LogLevel::Error:
            color = "\033[31m";
            break;
        case LogLevel::Trace:
            color = "\033[90m";
            break;
        case LogLevel::Normal:
            color = "\033[0m";
            break;
        }
        std::cout << color << timebuf << " [" << levelToString(level) << "] " << msg
                  << "\033[0m" << '\n';
    }

    void debug(const std::string& m) { log(LogLevel::Debug, m); }
    void info(const std::string& m) { log(LogLevel::Info, m); }
    void warn(const std::string& m) { log(LogLevel::Warn, m); }
    void error(const std::string& m) { log(LogLevel::Error, m); }
    void trace(const std::string& m) { log(LogLevel::Trace, m); }
    void normal(const std::string& m) { log(LogLevel::Normal, m); }

private:
    void openFile() {
        out_.open(path_, std::ios::app);
        if (!out_)
            throw std::runtime_error("无法打开日志文件: " + path_);
    }

    void rotate() {
        out_.close();

        namespace fs = std::filesystem;
        // path_ = app.log
        // 轮转: app.log -> app.log.1, app.log.1 -> app.log.2, ...
        // 删除超过 keepFiles_ 的

        std::string base = path_;
        // 删除最老的
        std::string oldest = base + "." + std::to_string(keepFiles_);
        std::error_code ec;
        fs::remove(oldest, ec);

        // 从后往前重命名
        for (int i = keepFiles_ - 1; i >= 1; --i) {
            std::string from = base + "." + std::to_string(i);
            std::string to = base + "." + std::to_string(i + 1);
            if (fs::exists(from, ec)) {
                fs::rename(from, to, ec);
            }
        }

        // app.log -> app.log.1
        if (fs::exists(base, ec)) {
            fs::rename(base, base + ".1", ec);
        }

        openFile();
    }

    std::string path_;
    std::ofstream out_;
    std::mutex mutex_;
    LogLevel minLevel_;
    size_t maxFileSize_;
    int keepFiles_;

    static const char* levelToString(LogLevel level) {
        switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Normal:
            return "NORMAL";
        }
        return "UNKNOWN";
    }
};