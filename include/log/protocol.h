#pragma once
//
// 日志层的两个抽象：格式化（怎么变成文本）与落点（文本写到哪）。
//
// 拆开的原因是这两件事的变化频率完全不同：
//   - 换格式（纯文本 / 带色 / 将来的 JSON）不该动写入逻辑；
//   - 换落点（控制台 / 文件 / 内存）不该动格式。
//
// 与 albuswall 的 log/formatters/ 与 log/handlers/ 对应。
// 那边靠鸭子类型，这边用抽象基类表达同样的协议。
//
#include <memory>
#include <string>

#include "log/type.h"

/// 一条日志记录。
///
/// Logger 只负责组装它；时间戳在 Logger 侧统一生成，
/// 保证同一条记录落在多个 handler 里时间完全一致。
struct LogRecord {
    LogLevel level = LogLevel::Info;
    std::string timestamp;
    std::string message;
};

/// 格式化抽象。
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual std::string format(const LogRecord& record) const = 0;

    /// 调试用快照，对应 albuswall 各处的 __str__。
    virtual std::string str() const = 0;
};

/// 落点抽象：控制台 / 文件 / 测试替身。
///
/// 刻意用"非虚 handle + 虚 write"的模板方法：子类只需要想清楚
/// "文本写到哪"，不必重复格式化、开关判断这些公共动作。
class LogHandler {
public:
    virtual ~LogHandler() = default;

    void setFormatter(std::shared_ptr<LogFormatter> formatter) {
        formatter_ = std::move(formatter);
    }
    const std::shared_ptr<LogFormatter>& formatter() const { return formatter_; }

    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }

    void handle(const LogRecord& record) {
        if (!enabled_)
            return;
        write(record, formatter_ ? formatter_->format(record) : record.message);
    }

    virtual void flush() {}

    /// 调试用快照。子类在这里说明"我是谁、写到哪、格式是什么"。
    virtual std::string str() const = 0;

protected:
    /// 子类唯一必须实现的东西。
    virtual void write(const LogRecord& record, const std::string& text) = 0;

private:
    std::shared_ptr<LogFormatter> formatter_;
    bool enabled_ = true;
};

/// 支持轮转的落点额外实现这个接口。
///
/// 这样 Logger::setRotation 就不需要认识 FileHandler 这个具体类型 ——
/// 目前只有文件需要轮转，将来内存 handler 想做环形覆盖也能挂上来。
class RotatableHandler {
public:
    virtual ~RotatableHandler() = default;
    virtual void setRotation(size_t maxFileSize, int keepFiles) = 0;
};
