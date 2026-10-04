#include "log/handlers/file_handler.h"

#include "common/exceptions.h"

#include <filesystem>
#include <system_error>
#include <utility>

FileHandler::FileHandler(std::string path, size_t maxFileSize, int keepFiles)
    : path_(std::move(path)), maxFileSize_(maxFileSize), keepFiles_(keepFiles) {
    openFile();
}

void FileHandler::openFile() {
    out_.open(path_, std::ios::app);
    if (!out_)
        throw AppError("无法打开日志文件: " + path_);
}

void FileHandler::setRotation(size_t maxFileSize, int keepFiles) {
    maxFileSize_ = maxFileSize;
    keepFiles_ = keepFiles;
}

void FileHandler::write(const LogRecord& /*record*/, const std::string& text) {
    out_ << text << '\n';
    out_.flush();

    if (maxFileSize_ == 0)
        return;

    std::error_code ec;
    auto size = std::filesystem::file_size(path_, ec);
    if (!ec && size >= maxFileSize_)
        rotate();
}

void FileHandler::rotate() {
    namespace fs = std::filesystem;

    out_.close();

    std::error_code ec;

    // 先删最老的一份，再整体后移，避免中间出现重名冲突。
    fs::remove(path_ + "." + std::to_string(keepFiles_), ec);

    for (int i = keepFiles_ - 1; i >= 1; --i) {
        const std::string from = path_ + "." + std::to_string(i);
        const std::string to = path_ + "." + std::to_string(i + 1);
        if (fs::exists(from, ec))
            fs::rename(from, to, ec);
    }

    if (fs::exists(path_, ec))
        fs::rename(path_, path_ + ".1", ec);

    openFile();
}

void FileHandler::flush() {
    out_.flush();
}

std::string FileHandler::str() const {
    return "FileHandler(path=" + path_ +
           ", maxFileSize=" + std::to_string(maxFileSize_) +
           ", keepFiles=" + std::to_string(keepFiles_) +
           ", formatter=" + std::string(formatter() ? formatter()->str() : "none") + ")";
}
