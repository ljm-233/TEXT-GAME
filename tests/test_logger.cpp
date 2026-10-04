#include "doctest.h"
#include "common/exceptions.h"
#include "log/formatters/console_formatter.h"
#include "log/formatters/text_formatter.h"
#include "log/handlers/file_handler.h"
#include "log/logger.h"
#include "log/rotation.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

/// 测试替身：把落点收到的文本记下来。
///
/// 这正是 LogHandler 抽象存在的意义 —— 断言日志内容不需要重定向进程 stdout，
/// 也不需要去磁盘上翻文件。
class RecordingHandler : public LogHandler {
public:
    std::vector<std::string> lines;

    std::string str() const override { return "RecordingHandler()"; }

protected:
    void write(const LogRecord& /*record*/, const std::string& text) override {
        lines.push_back(text);
    }
};

/// 落点写失败不该把整个程序带走，也不该影响其它落点。
class ThrowingHandler : public LogHandler {
public:
    std::string str() const override { return "ThrowingHandler()"; }

protected:
    void write(const LogRecord&, const std::string&) override {
        throw std::runtime_error("落点故障");
    }
};

/// 测试用临时目录：构造时创建，析构时删除。
struct TempDir {
    fs::path root;

    TempDir() {
        static int counter = 0;
        root = fs::temp_directory_path() /
               ("textgame_log_test_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(root, ec);
        fs::create_directories(root);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

std::string readFile(const fs::path& path) {
    std::ifstream in(path);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

} // namespace

TEST_CASE("TextFormatter - 输出 时间 [级别] 消息") {
    LogRecord record;
    record.level = LogLevel::Warn;
    record.timestamp = "2026-10-04 18:30:00";
    record.message = "磁盘快满了";

    TextFormatter formatter;

    CHECK(formatter.format(record) == "2026-10-04 18:30:00 [WARN] 磁盘快满了");
}

TEST_CASE("ConsoleFormatter - 按级别加 ANSI 颜色，文件格式不带转义") {
    LogRecord record;
    record.level = LogLevel::Error;
    record.timestamp = "2026-10-04 18:30:00";
    record.message = "崩了";

    ConsoleFormatter console;
    const std::string colored = console.format(record);

    CHECK(colored.find("\033[31m") != std::string::npos);  // 错误色
    CHECK(colored.find("\033[0m") != std::string::npos);   // 收尾复位
    CHECK(colored.find("崩了") != std::string::npos);

    // 纯文本格式化器绝不能吐控制字符，否则文件里就是脏数据
    TextFormatter text;
    CHECK(text.format(record).find('\033') == std::string::npos);
}

TEST_CASE("Logger - 低于阈值的级别被丢弃") {
    Logger logger(LogLevel::Warn);
    auto recorder = std::make_shared<RecordingHandler>();
    logger.addHandler(recorder);

    logger.info("这条不该出现");
    logger.debug("这条也不该");
    logger.warn("这条应该出现");
    logger.error("这条也应该");

    REQUIRE(recorder->lines.size() == 2);
    CHECK(recorder->lines[0].find("这条应该出现") != std::string::npos);
    CHECK(recorder->lines[1].find("这条也应该") != std::string::npos);
}

TEST_CASE("Logger - Normal 通道不受阈值过滤") {
    Logger logger(LogLevel::Error);
    auto recorder = std::make_shared<RecordingHandler>();
    logger.addHandler(recorder);

    logger.normal("无论如何都要看到");
    logger.info("这条会被挡掉");

    REQUIRE(recorder->lines.size() == 1);
    CHECK(recorder->lines[0].find("无论如何都要看到") != std::string::npos);
}

TEST_CASE("Logger - setMinLevel 立刻生效") {
    Logger logger(LogLevel::Error);
    auto recorder = std::make_shared<RecordingHandler>();
    logger.addHandler(recorder);

    logger.info("被挡");
    CHECK(recorder->lines.empty());

    logger.setMinLevel(LogLevel::Info);
    logger.info("放行");

    REQUIRE(recorder->lines.size() == 1);
    CHECK(logger.minLevel() == LogLevel::Info);
}

TEST_CASE("Logger - 同一条记录分发给所有 handler") {
    Logger logger(LogLevel::Trace);
    auto first = std::make_shared<RecordingHandler>();
    auto second = std::make_shared<RecordingHandler>();
    logger.addHandler(first);
    logger.addHandler(second);

    logger.info("广播");

    REQUIRE(first->lines.size() == 1);
    REQUIRE(second->lines.size() == 1);
    CHECK(first->lines[0] == second->lines[0]);  // 时间戳同源，两边完全一致
    CHECK(logger.handlers().size() == 2);
}

TEST_CASE("Logger - 一个 handler 失败不影响其余 handler") {
    Logger logger(LogLevel::Info);
    logger.addHandler(std::make_shared<ThrowingHandler>());
    auto recorder = std::make_shared<RecordingHandler>();
    logger.addHandler(recorder);

    logger.info("坏落点不该拖垮好落点");

    REQUIRE(recorder->lines.size() == 1);
    CHECK(recorder->lines[0].find("坏落点不该拖垮好落点") != std::string::npos);
}

TEST_CASE("Logger - isEnabled 与阈值一致") {
    Logger logger(LogLevel::Warn);

    CHECK_FALSE(logger.isEnabled(LogLevel::Info));
    CHECK(logger.isEnabled(LogLevel::Warn));
    CHECK(logger.isEnabled(LogLevel::Error));
    CHECK(logger.isEnabled(LogLevel::Normal));
}

TEST_CASE("Logger - str 是给人看的调试快照") {
    Logger logger(LogLevel::Debug);
    auto recorder = std::make_shared<RecordingHandler>();
    recorder->setFormatter(std::make_shared<TextFormatter>());
    logger.addHandler(recorder);

    const std::string dump = logger.str();

    CHECK(dump.find("DEBUG") != std::string::npos);
    CHECK(dump.find("RecordingHandler") != std::string::npos);
}

TEST_CASE("FileHandler - 落盘并自动补换行") {
    TempDir dir;
    const fs::path logPath = dir.root / "app.log";

    {
        FileHandler handler(logPath.string());
        handler.setFormatter(std::make_shared<TextFormatter>());

        LogRecord record;
        record.level = LogLevel::Info;
        record.timestamp = "2026-10-04 18:30:00";
        record.message = "写入一行";
        handler.handle(record);
        handler.flush();
    }

    const std::string content = readFile(logPath);
    CHECK(content == "2026-10-04 18:30:00 [INFO] 写入一行\n");
}

TEST_CASE("FileHandler - 超过阈值触发轮转") {
    TempDir dir;
    const fs::path logPath = dir.root / "app.log";

    // 阈值给得很小，写几条就会触发
    FileHandler handler(logPath.string(), /*maxFileSize=*/60, /*keepFiles=*/2);
    handler.setFormatter(std::make_shared<TextFormatter>());

    for (int i = 0; i < 6; ++i) {
        LogRecord record;
        record.level = LogLevel::Info;
        record.timestamp = "2026-10-04 18:30:00";
        record.message = "第 " + std::to_string(i) + " 条比较长的日志内容";
        handler.handle(record);
    }
    handler.flush();

    CHECK(fs::exists(logPath));
    CHECK(fs::exists(logPath.string() + ".1"));
}

TEST_CASE("FileHandler - 打不开的路径抛 AppError") {
    CHECK_THROWS_AS(FileHandler("/definitely/not/a/dir/app.log"), AppError);
}

TEST_CASE("logRotation - 档位取值与越界夹回") {
    CHECK(logRotationSizeAt(0) == 0);  // 0 = 不轮转
    CHECK(logRotationSizeAt(1) == 1 * 1024 * 1024);
    CHECK(logRotationKeepAt(0) == 1);

    // 越界不能越出数组，且要夹回旧实现的默认档
    CHECK(clampLogRotationIndex(-1) == 0);
    CHECK(clampLogRotationIndex(99) == 0);
    CHECK(clampLogKeepIndex(-1) == 1);
    CHECK(clampLogKeepIndex(99) == 1);

    CHECK(logRotationSizeAt(99) == logRotationSizeAt(0));
    CHECK(logRotationKeepAt(99) == logRotationKeepAt(1));
}

TEST_CASE("Logger - setRotation 转发给支持轮转的落点") {
    TempDir dir;
    const fs::path logPath = dir.root / "app.log";

    Logger logger(LogLevel::Info);
    auto file = std::make_shared<FileHandler>(logPath.string());
    file->setFormatter(std::make_shared<TextFormatter>());
    logger.addHandler(file);

    // 不经过 FileHandler 的具体类型，只走 Logger 的转发
    logger.setRotation(40, 2);

    for (int i = 0; i < 6; ++i)
        logger.info("一条足够长的日志内容 " + std::to_string(i));
    logger.flush();

    CHECK(fs::exists(logPath.string() + ".1"));
}
