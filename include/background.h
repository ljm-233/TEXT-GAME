#pragma once
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <vector>
#include <string>
#include "logging.h"

class Background {
public:
    Background(const std::filesystem::path& dir,
               const std::string& initialFile,
               unsigned windowWidth, unsigned windowHeight,
               std::shared_ptr<Logger> logger);

    void render(sf::RenderTarget& target);

    bool isLoaded() const { return loaded_; }
    std::string currentFile() const { return currentFile_; }
    int  currentIndex() const { return currentIndex_; }
    int  totalWallpapers() const { return static_cast<int>(files_.size()); }

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