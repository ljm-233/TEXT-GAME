#pragma once
//
// 控制台落点。
//
// 允许替换 ostream 是为了测试：把输出接进 std::ostringstream
// 就能断言日志内容，不需要重定向进程的 stdout。
//
#include <iosfwd>

#include "log/protocol.h"

class ConsoleHandler : public LogHandler {
public:
    explicit ConsoleHandler(std::ostream& out);

    void flush() override;
    std::string str() const override;

protected:
    void write(const LogRecord& record, const std::string& text) override;

private:
    std::ostream& out_;
};
