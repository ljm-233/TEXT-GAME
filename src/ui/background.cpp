#include "background.h"
#include <algorithm>

Background::Background(const std::filesystem::path& dir, const std::string& initialFile,
                       unsigned windowWidth, unsigned windowHeight,
                       std::shared_ptr<Logger> logger)
      : dir_(dir),
        logger_(std::move(logger)) {
    scanDirectory();
    if (files_.empty()) {
        logger_->warn("wallpaper 目录中没有图片，使用纯黑背景");
        return;
    }

    std::string toLoad = initialFile;
    if (toLoad.empty() || std::find_if(files_.begin(), files_.end(), [&](const auto& p) {
                              return p.filename().string() == toLoad;
                          }) == files_.end()) {
        toLoad = files_[0].filename().string();
    }
    loadByName(toLoad);
    fitToWindow(windowWidth, windowHeight);
    lastW_ = windowWidth;
    lastH_ = windowHeight;
}

void Background::scanDirectory() {
    namespace fs = std::filesystem;
    files_.clear();
    if (!fs::exists(dir_))
        return;

    for (const auto& entry : fs::directory_iterator(dir_)) {
        if (!entry.is_regular_file())
            continue;
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
            files_.push_back(entry.path());
        }
    }
    std::sort(files_.begin(), files_.end());
}

bool Background::loadByName(const std::string& filename) {
    auto path = dir_ / filename;
    if (!std::filesystem::exists(path))
        return false;
    if (!texture_.loadFromFile(path.string()))
        return false;

    texture_.setSmooth(true);
    sprite_ = std::make_unique<sf::Sprite>(texture_);
    loaded_ = true;
    currentFile_ = filename;

    // 更新索引
    for (size_t i = 0; i < files_.size(); ++i) {
        if (files_[i].filename().string() == filename) {
            currentIndex_ = static_cast<int>(i);
            break;
        }
    }

    logger_->info("背景加载: " + path.string());
    return true;
}

bool Background::next() {
    if (files_.empty())
        return false;
    int nextIdx = (currentIndex_ + 1) % static_cast<int>(files_.size());
    return loadByName(files_[nextIdx].filename().string());
}

void Background::fitToWindow(unsigned windowWidth, unsigned windowHeight) {
    if (!sprite_)
        return;
    auto texSize = texture_.getSize();
    if (texSize.x == 0 || texSize.y == 0)
        return;

    float scaleX = static_cast<float>(windowWidth) / texSize.x;
    float scaleY = static_cast<float>(windowHeight) / texSize.y;
    float scale = std::max(scaleX, scaleY);

    sprite_->setScale({scale, scale});
    float dispW = texSize.x * scale;
    float dispH = texSize.y * scale;
    sprite_->setPosition({(windowWidth - dispW) / 2.f, (windowHeight - dispH) / 2.f});
}

void Background::render(sf::RenderTarget& target) {
    if (!loaded_ || !sprite_)
        return;
    auto size = target.getSize();
    if (size.x != lastW_ || size.y != lastH_) {
        fitToWindow(size.x, size.y);
        lastW_ = size.x;
        lastH_ = size.y;
    }
    target.draw(*sprite_);
}