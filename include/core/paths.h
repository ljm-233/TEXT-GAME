#pragma once
#include <filesystem>

// 管理项目下的六个资源目录
class Paths {
public:
    Paths() {
        namespace fs = std::filesystem;

        root_ = fs::path(PROJECT_ROOT);
        configDir_ = root_ / "config";
        cacheDir_ = root_ / "cache";
        tempDir_ = root_ / "temp";
        savesDir_ = root_ / "saves";
        wallpaperDir_ = root_ / "wallpaper";
        assetsDir_ = root_ / "assets";

        for (auto& d :
             {configDir_, cacheDir_, tempDir_, savesDir_, wallpaperDir_, assetsDir_}) {
            fs::create_directories(d);
        }
    }

    const std::filesystem::path& configDir() const { return configDir_; }
    const std::filesystem::path& cacheDir() const { return cacheDir_; }
    const std::filesystem::path& tempDir() const { return tempDir_; }
    const std::filesystem::path& savesDir() const { return savesDir_; }
    const std::filesystem::path& wallpaperDir() const { return wallpaperDir_; }
    const std::filesystem::path& assetsDir() const { return assetsDir_; }

private:
    std::filesystem::path root_;
    std::filesystem::path configDir_;
    std::filesystem::path cacheDir_;
    std::filesystem::path tempDir_;
    std::filesystem::path savesDir_;
    std::filesystem::path wallpaperDir_;
    std::filesystem::path assetsDir_;
};