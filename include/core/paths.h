#pragma once
#include "platform.h"
#include <filesystem>

// 管理资源目录：
//   - 开发模式（项目目录里有 assets/）：所有目录在项目里
//   - 打包模式（可执行文件旁边有 assets/）：资源跟着可执行文件，用户数据在 XDG 目录
//   - 测试模式（显式传根目录）：全部挂在给定根目录下
//
// 三种模式的区别只在于"根在哪"，所以下面把目录布局抽成 layoutUnder()，
// 免得三份路径拼装各写一遍、各自漂移。
class Paths {
public:
    Paths() {
        namespace fs = std::filesystem;

        const fs::path execDir = Platform::executableDir();
        const fs::path resRoot = resourceRootFor(execDir);

        // 判定模式：资源根旁边有没有 assets/
        bool packaged = fs::exists(resRoot / "assets");

        if (packaged) {
            configDir_ = Platform::userConfigDir();
            cacheDir_ = Platform::userCacheDir();
            tempDir_ = Platform::userTempDir();
            savesDir_ = Platform::userDataDir() / "saves";
            wallpaperDir_ = resRoot / "wallpaper";
            assetsDir_ = resRoot / "assets";
            mode_ = "packaged";
        } else {
            // 开发模式
            layoutUnder(fs::path(PROJECT_ROOT));
            mode_ = "development";
        }

        createAll();
    }

    /// 测试用：把所有目录挂到给定根目录下。
    ///
    /// 原先不行 —— 目录一律从 PROJECT_ROOT 或可执行文件位置推导，于是任何
    /// "写文件"的模块（SaveManager 等）都没法在临时目录里被测，只能去动真实的
    /// 存档目录。有了这个构造函数，测试可以造一个只属于自己的沙箱。
    explicit Paths(const std::filesystem::path& root) {
        layoutUnder(root);
        mode_ = "test";
        createAll();
    }

    /// 资源根目录：平常就是可执行文件所在目录。
    ///
    /// macOS 的 .app 是个例外 —— 可执行文件在 <X>.app/Contents/MacOS/，
    /// 而资源按 macOS 的规矩放在 <X>.app/Contents/Resources/。
    ///
    /// 资源**不能**塞进 Contents/MacOS/：codesign 给 bundle 盖章时会把那里的
    /// 目录当成嵌套代码去验签，直接报 "code object is not signed at all"
    /// （实际报在 wallpaper/CREDITS.md 上），签名失败、包都做不出来。
    ///
    /// 纯函数，所以能直接单元测试 —— 见 tests/test_config_seeding.cpp。
    static std::filesystem::path resourceRootFor(
            const std::filesystem::path& execDir) {
        if (execDir.filename() == "MacOS" &&
            execDir.parent_path().filename() == "Contents") {
            return execDir.parent_path() / "Resources";
        }
        return execDir;
    }

    const std::filesystem::path& configDir() const { return configDir_; }
    const std::filesystem::path& cacheDir() const { return cacheDir_; }
    const std::filesystem::path& tempDir() const { return tempDir_; }
    const std::filesystem::path& savesDir() const { return savesDir_; }
    const std::filesystem::path& wallpaperDir() const { return wallpaperDir_; }
    const std::filesystem::path& assetsDir() const { return assetsDir_; }

    const std::string& mode() const { return mode_; }

private:
    void layoutUnder(const std::filesystem::path& root) {
        configDir_ = root / "config";
        cacheDir_ = root / "cache";
        tempDir_ = root / "temp";
        savesDir_ = root / "saves";
        wallpaperDir_ = root / "wallpaper";
        assetsDir_ = root / "assets";
    }

    void createAll() {
        for (auto& d :
             {configDir_, cacheDir_, tempDir_, savesDir_, wallpaperDir_, assetsDir_}) {
            std::error_code ec;
            std::filesystem::create_directories(d, ec);
        }
    }

    std::filesystem::path configDir_;
    std::filesystem::path cacheDir_;
    std::filesystem::path tempDir_;
    std::filesystem::path savesDir_;
    std::filesystem::path wallpaperDir_;
    std::filesystem::path assetsDir_;
    std::string mode_ = "development";
};
