#pragma once
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"
#include "log/logger.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

/// 全屏背景。
///
/// 持有 wallpaper 层的两个服务：
///   - `WallpaperLibrary` 负责"目录下有几张图、哪张是当前选中的"
///   - `WallpaperLoader` 负责"异步把图解成 sf::Image"
///
/// 自身只管 GL 那部分：sf::Texture 上传、sprite 平铺、窗口适配、淡入淡出。
///
/// **淡入淡出**用两张纹理：front_ 是底图（一直 alpha=1），back_ 是正在淡入
/// 的新图（alpha 从 0 到 1）。两层都画，front_ 的 alpha 同时从 1 降到
/// (1 - fadeT_)，达到 crossfade 的效果。fadeT_ 由 render() 内部按 sf::Clock
/// 推进 —— 场景常驻 + 单例 Background 共享意味着没有跨场景的 fade，
/// 同场景一帧只 render 一次，所以"内部 clock 推进"是安全的。
class Background {
public:
    /// 构造时立即在主线程同步解码当前选中的那张（`prime`），避免首屏黑屏。
    /// 其余壁纸由 loader 后台预解码，切图时几乎都已 ready。
    Background(WallpaperLibrary& library, WallpaperLoader& loader,
               const std::string& initialFile, unsigned windowWidth,
               unsigned windowHeight, std::shared_ptr<Logger> logger);

    void render(sf::RenderTarget& target);

    bool isLoaded() const { return front_.index >= 0; }
    std::string currentFile() const;
    int currentIndex() const { return front_.index; }
    int totalWallpapers() const { return lib_.size(); }

    /// 切到指定文件名。异步：从 loader 等图（默认 2s 超时），
    /// 拿到后开始 0.5s 淡入淡出；超时则保持当前图。
    bool loadByName(const std::string& filename);

    /// 切到下一张（循环）。
    bool next();

private:
    /// 一层纹理 + sprite + 关联的 library index。
    ///
    /// ⚠️ 纹理必须用 `unique_ptr` 持有，**不能按值**。
    ///    `sf::Sprite` 内部存的是 `const sf::Texture*` —— 它记的是**地址**。
    ///    淡入淡出结束时要把 back_ 整体搬成 front_（`front_ = std::move(back_)`），
    ///    如果纹理是按值成员，这一搬会让纹理对象换地址，而 sprite 里的指针
    ///    还指着旧地址（已经变成 moved-from 的空纹理）—— 表现是**每次切图
    ///    淡入结束后壁纸直接消失**。用 unique_ptr 后搬的只是指针，
    ///    纹理对象本身地址不变，sprite 就始终指得对。
    struct Layer {
        std::unique_ptr<sf::Texture> tex;
        std::unique_ptr<sf::Sprite> sprite;
        int index = -1; ///< -1 表示这层空着

        bool valid() const { return tex != nullptr && sprite != nullptr; }
        void clear() {
            sprite.reset();
            tex.reset();
            index = -1;
        }
    };

    bool uploadToFront(int index, const WallpaperInfo& info);
    void uploadToBack(int index, const WallpaperInfo& info);
    void startFade();
    void fitToWindow(Layer& layer, unsigned windowWidth, unsigned windowHeight) const;
    unsigned targetWidth(const sf::RenderTarget& target) const;
    unsigned targetHeight(const sf::RenderTarget& target) const;

    WallpaperLibrary& lib_;
    WallpaperLoader& loader_;
    std::shared_ptr<Logger> logger_;

    Layer front_;
    Layer back_;

    bool fading_ = false;
    float fadeT_ = 0.f; ///< 0 = 刚开切；1 = 切完
    sf::Clock fadeClock_;

    unsigned lastW_ = 0;
    unsigned lastH_ = 0;
};
