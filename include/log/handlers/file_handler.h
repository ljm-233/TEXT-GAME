#pragma once
//
// 文件落点，带按大小轮转。
//
// 轮转规则沿用旧 logging.h 的实现（保留行为，不改语义）：
//   app.log -> app.log.1 -> app.log.2 -> ... -> app.log.<keepFiles>
//   超过 keepFiles 份的最老文件被删除。
// maxFileSize = 0 表示不轮转。
//
#include <cstddef>
#include <fstream>
#include <string>

#include "log/protocol.h"

class FileHandler : public LogHandler, public RotatableHandler {
public:
    /// 构造即打开文件；打不开抛 AppError（旧 Logger 的行为是抛 std::runtime_error）。
    explicit FileHandler(std::string path, size_t maxFileSize = 0, int keepFiles = 3);

    void setRotation(size_t maxFileSize, int keepFiles) override;

    void flush() override;
    std::string str() const override;

    const std::string& path() const { return path_; }

protected:
    void write(const LogRecord& record, const std::string& text) override;

private:
    void openFile();
    void rotate();

    std::string path_;
    std::ofstream out_;
    size_t maxFileSize_;
    int keepFiles_;
};
