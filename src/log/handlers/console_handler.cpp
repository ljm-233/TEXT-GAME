#include "log/handlers/console_handler.h"

#include <ostream>

ConsoleHandler::ConsoleHandler(std::ostream& out) : out_(out) {}

void ConsoleHandler::write(const LogRecord& /*record*/, const std::string& text) {
    out_ << text << '\n';
    // 逐条 flush：日志的价值在于崩溃前那条还在，
    // 缓冲住的日志在排查时等于不存在。
    out_.flush();
}

void ConsoleHandler::flush() {
    out_.flush();
}

std::string ConsoleHandler::str() const {
    return "ConsoleHandler(formatter=" +
           std::string(formatter() ? formatter()->str() : "none") + ")";
}
