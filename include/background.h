#pragma once
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include "logging.h"

class Background {
public:
    Background(const std::filesystem::path& dir,
               unsigned windowWidth, unsigned windowHeight,
               std::shared_ptr<Logger> logger);

    // 每帧调用，内部检测目标尺寸变化后自动重新适配
    void render(sf::RenderTarget& target);

    bool isLoaded() const { return loaded_; }

private:
    void fitToWindow(unsigned windowWidth, unsigned windowHeight);

    sf::Texture texture_;
    std::unique_ptr<sf::Sprite> sprite_;
    bool loaded_ = false;
    unsigned lastW_ = 0;
    unsigned lastH_ = 0;
};