#pragma once
#include "platform.h"
#include <filesystem>

// 管理资源目录：
//   - 开发模式（项目目录里有 assets/）：所有目录在项目里
//   - 打包模式（可执行文件旁边有 assets/）：资源跟着可执行文件，用户数据在 XDG 目录
class Paths {
public:
    Paths() {
        namespace fs = std::filesystem;

        fs::path execDir = Platform::executableDir();

        // 判定模式：可执行文件旁边有没有 assets/
        bool packaged = fs::exists(execDir / "assets");

        if (packaged) {
            assetRoot_ = execDir;
            configDir_ = Platform::userConfigDir();
            cacheDir_ = Platform::userCacheDir();
            tempDir_ = Platform::userTempDir();
            savesDir_ = Platform::userDataDir() / "saves";
            wallpaperDir_ = execDir / "wallpaper";
            assetsDir_ = execDir / "assets";
            mode_ = "packaged";
        } else {
            // 开发模式
            fs::path root = fs::path(PROJECT_ROOT);
            configDir_ = root / "config";
            cacheDir_ = root / "cache";
            tempDir_ = root / "temp";
            savesDir_ = root / "saves";
            wallpaperDir_ = root / "wallpaper";
            assetsDir_ = root / "assets";
            mode_ = "development";
        }

        for (auto& d :
             {configDir_, cacheDir_, tempDir_, savesDir_, wallpaperDir_, assetsDir_}) {
            std::error_code ec;
            fs::create_directories(d, ec);
        }
    }

    const std::filesystem::path& configDir() const { return configDir_; }
    const std::filesystem::path& cacheDir() const { return cacheDir_; }
    const std::filesystem::path& tempDir() const { return tempDir_; }
    const std::filesystem::path& savesDir() const { return savesDir_; }
    const std::filesystem::path& wallpaperDir() const { return wallpaperDir_; }
    const std::filesystem::path& assetsDir() const { return assetsDir_; }

    const std::string& mode() const { return mode_; }

private:
    std::filesystem::path assetRoot_;
    std::filesystem::path configDir_;
    std::filesystem::path cacheDir_;
    std::filesystem::path tempDir_;
    std::filesystem::path savesDir_;
    std::filesystem::path wallpaperDir_;
    std::filesystem::path assetsDir_;
    std::string mode_ = "development";
};