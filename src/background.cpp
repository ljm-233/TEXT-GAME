#include "background.h"

#include <algorithm>

Background::Background(const std::filesystem::path& dir,
                       unsigned windowWidth, unsigned windowHeight,
                       std::shared_ptr<Logger> logger) {
    namespace fs = std::filesystem;

    // 先试 jpg，再试 png
    fs::path jpgPath = dir / "wallpaper.jpg";
    fs::path pngPath = dir / "wallpaper.png";

    bool ok = false;
    if (fs::exists(jpgPath) && texture_.loadFromFile(jpgPath.string())) {
        logger->info("背景加载: " + jpgPath.string());
        texture_.setSmooth(true);   // 开启纹理平滑，缩放时不会出现像素块
        ok = true;
    } else if (fs::exists(pngPath) && texture_.loadFromFile(pngPath.string())) {
        logger->info("背景加载: " + pngPath.string());
        texture_.setSmooth(true);   // 开启纹理平滑
        ok = true;
    }

    if (!ok) {
        logger->warn("找不到 wallpaper.jpg 或 wallpaper.png，使用纯黑背景");
        return;
    }

    sprite_ = std::make_unique<sf::Sprite>(texture_);
    loaded_ = true;
    fitToWindow(windowWidth, windowHeight);
    lastW_ = windowWidth;
    lastH_ = windowHeight;
}

void Background::fitToWindow(unsigned windowWidth, unsigned windowHeight) {
    if (!sprite_) return;

    auto texSize = texture_.getSize();
    if (texSize.x == 0 || texSize.y == 0) return;

    // 裁剪模式：取宽、高缩放比中较大的那个，保证图片盖满窗口
    float scaleX = static_cast<float>(windowWidth)  / texSize.x;
    float scaleY = static_cast<float>(windowHeight) / texSize.y;
    float scale  = std::max(scaleX, scaleY);

    sprite_->setScale({scale, scale});

    // 居中：图片比窗口大，偏移是负数，超出窗口的部分被自动裁掉
    float dispW = texSize.x * scale;
    float dispH = texSize.y * scale;
    sprite_->setPosition({(windowWidth  - dispW) / 2.f,
                          (windowHeight - dispH) / 2.f});
}

void Background::render(sf::RenderTarget& target) {
    if (!loaded_ || !sprite_) return;

    // 每帧检测目标尺寸是否变了，变了就重新适配
    auto size = target.getSize();
    if (size.x != lastW_ || size.y != lastH_) {
        fitToWindow(size.x, size.y);
        lastW_ = size.x;
        lastH_ = size.y;
    }

    target.draw(*sprite_);
}