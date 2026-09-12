#pragma once
#include <filesystem>

// 管理项目下的四个资源目录
class Paths {
public:
    Paths() {
        namespace fs = std::filesystem;

        // PROJECT_ROOT 由 CMake 注入
        root_      = fs::path(PROJECT_ROOT);
        configDir_ = root_ / "config";
        cacheDir_  = root_ / "cache";
        tempDir_   = root_ / "temp";
        savesDir_  = root_ / "saves";

        // 启动时确保目录都存在，其他地方就不用再判断了
        for (auto& d : {configDir_, cacheDir_, tempDir_, savesDir_}) {
            fs::create_directories(d);
        }
    }

    const std::filesystem::path& configDir() const { return configDir_; }
    const std::filesystem::path& cacheDir()  const { return cacheDir_; }
    const std::filesystem::path& tempDir()   const { return tempDir_; }
    const std::filesystem::path& savesDir()  const { return savesDir_; }

private:
    std::filesystem::path root_;
    std::filesystem::path configDir_;
    std::filesystem::path cacheDir_;
    std::filesystem::path tempDir_;
    std::filesystem::path savesDir_;
};