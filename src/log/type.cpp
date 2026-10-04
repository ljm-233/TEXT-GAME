#include "log/type.h"

#include <algorithm>
#include <cctype>
#include <string>

const char* logLevelName(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
        return "TRACE";
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Normal:
        return "NORMAL";
    }
    return "UNKNOWN";
}

bool logLevelAtLeast(LogLevel level, LogLevel threshold) {
    // Normal 是"无论如何都要输出"的通道（旧 Logger 的既有语义），
    // 不参与严重度比较。
    if (level == LogLevel::Normal)
        return true;
    return static_cast<int>(level) >= static_cast<int>(threshold);
}

LogLevel logLevelFromString(const std::string& text, LogLevel fallback) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (lower == "trace") return LogLevel::Trace;
    if (lower == "debug") return LogLevel::Debug;
    if (lower == "info")  return LogLevel::Info;
    if (lower == "warn" || lower == "warning") return LogLevel::Warn;
    if (lower == "error") return LogLevel::Error;
    if (lower == "normal") return LogLevel::Normal;
    return fallback;
}
