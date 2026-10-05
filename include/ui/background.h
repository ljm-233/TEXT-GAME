#pragma once
#include "log/logger.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

/// 在候选文件里找出 requested 对应的那个文件名（不含路径）。
///
/// 先按全名匹配；找不到时按**主名**（不含扩展名）匹配，好让素材换扩展名之后
/// 用户存在 current_wallpaper 里的设置还能接上 —— 不做这一步的话，改名会让
/// 壁纸静默打回默认，看着就像"设置自己丢了"。
///
/// 返回空串表示候选里没有对得上的。
/// 抽成自由函数是为了能测：Background 有按值的 sf::Texture（GlResource），
/// 无界面环境里连构造都做不到。
std::string resolveWallpaperName(const std::vector<std::filesystem::path>& files,
                                 const std::string& requested);

class Background {
public:
    Background(const std::filesystem::path& dir, const std::string& initialFile,
               unsigned windowWidth, unsigned windowHeight,
               std::shared_ptr<Logger> logger);

    void render(sf::RenderTarget& target);

    bool isLoaded() const { return loaded_; }
    std::string currentFile() const { return currentFile_; }
    int currentIndex() const { return currentIndex_; }
    int totalWallpapers() const { return static_cast<int>(files_.size()); }

    // 加载指定文件名（不含路径）
    bool loadByName(const std::string& filename);

    // 切换到下一张，循环
    bool next();

private:
    void scanDirectory();
    void fitToWindow(unsigned windowWidth, unsigned windowHeight);

    std::filesystem::path dir_;
    std::vector<std::filesystem::path> files_;
    int currentIndex_ = -1;
    std::string currentFile_;

    sf::Texture texture_;
    std::unique_ptr<sf::Sprite> sprite_;
    bool loaded_ = false;
    unsigned lastW_ = 0;
    unsigned lastH_ = 0;

    std::shared_ptr<Logger> logger_;
};