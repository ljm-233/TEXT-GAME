#pragma once
//
// 控制台格式：与 TextFormatter 同构，额外按级别加 ANSI 颜色。
//
// 单独一个类而不是给 TextFormatter 加开关，是因为"往终端写"和
// "往文件写"对颜色的需求是相反的：文件里混进转义序列就是脏数据。
//
#include "log/protocol.h"

class ConsoleFormatter : public LogFormatter {
public:
    std::string format(const LogRecord& record) const override;
    std::string str() const override;

    /// 级别 -> ANSI 颜色码。公开是为了让别的地方（如控制台场景）复用同一套配色。
    static const char* colorFor(LogLevel level);
};
