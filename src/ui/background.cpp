#include "background.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace {
constexpr float kFadeSeconds = 0.5f;
} // namespace

Background::Background(WallpaperLibrary& library, WallpaperLoader& loader,
                       const std::string& initialFile, unsigned windowWidth,
                       unsigned windowHeight, std::shared_ptr<Logger> logger)
      : lib_(library),
        loader_(loader),
        logger_(std::move(logger)) {
    // 用户选中的那张立刻 prime —— 必须主线程同步解码，保证第一帧就能渲染。
    // 这是"启动别卡"的核心取舍：只解码 1 张最坏 250ms，可以接受。
    //
    // ⚠️ 顺序不能反：必须**先 prime（解码入缓存）再 uploadToFront（取走+上传）**。
    //    反过来写的话 take() 时候缓存和队列都是空的，会立刻返回 false，
    //    再被 && 短路掉 prime —— 结果是背景永远黑着，而且不报任何错。
    int idx = lib_.resolveIndex(initialFile);
    if (idx < 0 && !lib_.empty())
        idx = 0;

    const auto t0 = std::chrono::steady_clock::now();
    bool primed = false;
    if (idx >= 0) {
        primed = loader_.prime(idx, lib_.at(idx)) && uploadToFront(idx, lib_.at(idx));
    }

    if (primed) {
        selected_ = front_.index; // 选中与显示在这一刻是一致的
        lastW_ = windowWidth;
        lastH_ = windowHeight;
        fitToWindow(front_, windowWidth, windowHeight);

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0)
                            .count();
        logger_->info("背景加载: " + lib_.at(front_.index).path.string() + "  (" +
                      std::to_string(ms) + " ms, 首图同步解码; 库内共 " +
                      std::to_string(lib_.size()) + " 张)");
    } else {
        logger_->warn("Background: 没有可用的壁纸（library 为空或解码失败）");
    }

    // 其余的扔给后台慢慢预解码 —— 切图时大概率已 ready
    if (lib_.size() > 1) {
        for (int i = 0; i < lib_.size(); ++i) {
            if (i == front_.index)
                continue;
            loader_.request(i, lib_.at(i));
        }
    }
}

std::string Background::currentFile() const {
    // 注意是 selected_ 而不是 front_.index —— 见头文件里的说明：
    // 淡入要 0.5s，这期间画面还是旧图，但用户的选择必须立刻能读出来，
    // 否则点完"下一张"存进配置的还是旧名字（重启打回上一张，静默）。
    if (selected_ < 0)
        return {};
    return lib_.at(selected_).filename;
}

bool Background::uploadToFront(int index, const WallpaperInfo& info) {
    if (index < 0 || info.path.empty())
        return false;

    // prime 已经把这一张的 Image 准备好了；拿出来用
    sf::Image img;
    if (!loader_.take(index, img, std::chrono::milliseconds(500)))
        return false;

    // 先扔 sprite 再换纹理，避免旧 sprite 短暂指向已析构的纹理
    front_.sprite.reset();
    front_.tex = std::make_unique<sf::Texture>();
    if (!front_.tex->loadFromImage(img)) {
        front_.tex.reset();
        return false;
    }

    front_.tex->setSmooth(true);
    front_.sprite = std::make_unique<sf::Sprite>(*front_.tex);
    front_.index = index;
    return true;
}

void Background::uploadToBack(int index, const WallpaperInfo& info) {
    if (index < 0 || info.path.empty())
        return;

    sf::Image img;
    if (!loader_.take(index, img, std::chrono::milliseconds(2000)))
        return;

    back_.sprite.reset();
    back_.tex = std::make_unique<sf::Texture>();
    if (!back_.tex->loadFromImage(img)) {
        back_.tex.reset();
        return;
    }
    back_.tex->setSmooth(true);
    back_.sprite = std::make_unique<sf::Sprite>(*back_.tex);
    back_.index = index;

    // ⚠️ 新 sprite 必须**立刻**适配窗口。
    //    render() 里的重适配只在"尺寸变化"时触发（lw != lastW_），而切图时
    //    尺寸根本没变，那条路径永远走不到 —— 漏了这一步，新图会以**原生像素
    //    尺寸**（比如 3840x2160）从左上角画出来，淡入结束搬成 front_ 之后
    //    也没人再给它适配，于是"当前壁纸"永远处在错位状态。
    if (lastW_ != 0 && lastH_ != 0)
        fitToWindow(back_, lastW_, lastH_);

    logger_->info("背景切换: " + info.path.string() + "  (淡入 0.5s)");
}

void Background::startFade() {
    fading_ = true;
    fadeT_ = 0.f;
    fadeClock_.restart();
}

bool Background::loadByName(const std::string& filename) {
    int idx = lib_.resolveIndex(filename);
    if (idx < 0)
        return false;
    if (idx == selected_)
        return true; // 选中的已经是这一张

    // 先 request 一遍（可能还没在 queue 里），然后上传到 back_
    loader_.request(idx, lib_.at(idx));
    uploadToBack(idx, lib_.at(idx));
    if (back_.index != idx)
        return false; // 解码/上传失败：保持旧图，也别改选中项

    // 只有新图层真的准备好了才改选中项 —— 否则配置会指向一张加载失败的图，
    // 重启后又是一次静默回退。
    selected_ = idx;
    startFade();
    return true;
}

bool Background::next() {
    if (lib_.empty())
        return false;
    int n = lib_.size();
    // 从**选中项**往前推，而不是从正在显示的那张 —— 淡入期间连点两下
    // "下一张"时要真的前进两张，从 front_ 推会算成同一张、原地打转。
    int nextIdx = (selected_ + 1 + n) % n; // selected_ = -1 时正好落到 0
    if (nextIdx == selected_)
        return false; // 库只有 1 张，无变化

    loader_.request(nextIdx, lib_.at(nextIdx));
    uploadToBack(nextIdx, lib_.at(nextIdx));
    if (back_.index != nextIdx)
        return false;

    selected_ = nextIdx;
    startFade();
    return true;
}

void Background::fitToWindow(Layer& layer, unsigned windowWidth,
                             unsigned windowHeight) const {
    if (!layer.valid())
        return;
    auto texSize = layer.tex->getSize();
    if (texSize.x == 0 || texSize.y == 0)
        return;

    float scaleX = static_cast<float>(windowWidth) / texSize.x;
    float scaleY = static_cast<float>(windowHeight) / texSize.y;
    float scale = std::max(scaleX, scaleY); // cover：铺满，可能裁

    layer.sprite->setScale({scale, scale});
    float dispW = texSize.x * scale;
    float dispH = texSize.y * scale;
    layer.sprite->setPosition(
        {(windowWidth - dispW) / 2.f, (windowHeight - dispH) / 2.f});
}

unsigned Background::targetWidth(const sf::RenderTarget& target) const {
    auto vs = target.getView().getSize();
    return static_cast<unsigned>(vs.x);
}

unsigned Background::targetHeight(const sf::RenderTarget& target) const {
    auto vs = target.getView().getSize();
    return static_cast<unsigned>(vs.y);
}

void Background::render(sf::RenderTarget& target) {
    // 推进 fade
    if (fading_) {
        fadeT_ = fadeClock_.getElapsedTime().asSeconds() / kFadeSeconds;
        if (fadeT_ >= 1.f) {
            // 切完：back_ 整体搬成 front_。
            // 这一步是安全的 —— Layer 里纹理是 unique_ptr，搬的只是指针，
            // 纹理对象地址不变，sprite 内部的 const Texture* 仍然指得对。
            front_ = std::move(back_);
            back_.clear();
            fading_ = false;
            fadeT_ = 0.f;
        }
    }

    // 首图加载失败、之后用户又切了图的恢复路径：没有底图就没法淡入，
    // 直接把 back_ 顶上来当底图（否则下面的 `!front_` 早退会让壁纸永远出不来）
    if (!front_.valid() && back_.valid()) {
        front_ = std::move(back_);
        back_.clear();
        fading_ = false;
        fadeT_ = 0.f;
    }

    // 取当前帧的"逻辑尺寸"（render 缩放下 getSize 返回 framebuffer 像素）
    const unsigned lw = targetWidth(target);
    const unsigned lh = targetHeight(target);
    if (lw == 0 || lh == 0)
        return;

    if (lw != lastW_ || lh != lastH_) {
        lastW_ = lw;
        lastH_ = lh;
        if (front_.valid())
            fitToWindow(front_, lw, lh);
        if (back_.valid())
            fitToWindow(back_, lw, lh);
    }

    if (!front_.valid())
        return;

    // 旧图（fade 中 alpha 由 1 → 0；非 fade 时 alpha=1）
    if (fading_ && back_.valid()) {
        sf::Color c = sf::Color::White;
        c.a = static_cast<uint8_t>(255.f * std::clamp(1.f - fadeT_, 0.f, 1.f));
        front_.sprite->setColor(c);
        target.draw(*front_.sprite);

        // 新图 alpha 0 → 1
        sf::Color c2 = sf::Color::White;
        c2.a = static_cast<uint8_t>(255.f * std::clamp(fadeT_, 0.f, 1.f));
        back_.sprite->setColor(c2);
        target.draw(*back_.sprite);

        // 还原 alpha，避免下一次非 fade 帧画错（setColor 持久化）
        front_.sprite->setColor(sf::Color::White);
        back_.sprite->setColor(sf::Color::White);
    } else {
        // 非 fade：就一张全 alpha 画
        if (front_.sprite->getColor() != sf::Color::White)
            front_.sprite->setColor(sf::Color::White);
        target.draw(*front_.sprite);
    }
}
