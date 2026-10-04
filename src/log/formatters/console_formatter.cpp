#include "log/formatters/console_formatter.h"

const char* ConsoleFormatter::colorFor(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
        return "\033[90m";
    case LogLevel::Debug:
        return "\033[36m";
    case LogLevel::Info:
        return "\033[32m";
    case LogLevel::Warn:
        return "\033[33m";
    case LogLevel::Error:
        return "\033[31m";
    case LogLevel::Normal:
        return "\033[0m";
    }
    return "\033[0m";
}

std::string ConsoleFormatter::format(const LogRecord& record) const {
    return std::string(colorFor(record.level)) + record.timestamp + " [" +
           logLevelName(record.level) + "] " + record.message + "\033[0m";
}

std::string ConsoleFormatter::str() const {
    return "ConsoleFormatter()";
}
