#include "log/formatters/text_formatter.h"

std::string TextFormatter::format(const LogRecord& record) const {
    return record.timestamp + " [" + logLevelName(record.level) + "] " + record.message;
}

std::string TextFormatter::str() const {
    return "TextFormatter()";
}
