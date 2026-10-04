#pragma once
//
// 日志级别。
//
// 枚举值与顺序沿用旧 logging.h：显示设置里存的是 int，
// 动顺序会让老配置文件的 log_level 含义漂移。
//
#include <string>

enum class LogLevel { Trace, Debug, Info, Warn, Error, Normal };

/// 级别 -> 固定的大写短名，用于日志行。
const char* logLevelName(LogLevel level);

/// 级别是否比另一个更严重。Normal 视为"永远输出"，不参与比较。
bool logLevelAtLeast(LogLevel level, LogLevel threshold);

/// 解析配置里的字符串级别（"info" / "warn" ...），无法识别时返回 fallback。
LogLevel logLevelFromString(const std::string& text, LogLevel fallback);
