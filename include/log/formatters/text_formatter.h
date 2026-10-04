#pragma once
//
// 纯文本格式：`2026-10-04 18:20:31 [INFO] 消息`
//
// 不带任何控制字符，所以文件 handler 与"重定向到管道"的场景都能直接用。
//
#include "log/protocol.h"

class TextFormatter : public LogFormatter {
public:
    std::string format(const LogRecord& record) const override;
    std::string str() const override;
};
