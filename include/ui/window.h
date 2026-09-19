#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "upscaler.h"

class Window {
public:
    Window(unsigned width, unsigned height, const std::string& title,
           bool fullscreen = false, unsigned antiAliasing = 8);

    bool isOpen() const;
    void close();

    void clear();
    void display();

    void pollEvents(const std::function<void(const sf::Event&)>& handler = nullptr);
    bool isFocused() const;

    bool isKeyPressed(sf::Keyboard::Key key) const;
    void draw(const sf::Drawable& drawable);

    void recreate(unsigned width, unsigned height, bool fullscreen);

    void setTitle(const std::string& title);
    const std::string& getTitle() const { return title_; }

    void setVsync(bool enabled);
    bool getVsync() const { return vsync_; }

    void setAntiAliasing(unsigned level) { antiAliasing_ = level; }
    unsigned getAntiAliasing() const { return antiAliasing_; }

    // 0 = 无限
    void setFramerateLimit(unsigned limit);
    unsigned getFramerateLimit() const { return framerateLimit_; }

    sf::RenderWindow& native();
    void requestMaximize();

    // ⭐ 渲染缩放（0.25 ~ 1.0）——软渲染弱机用
    void setRenderScale(float s);
    float getRenderScale() const { return renderScale_; }

    // ⭐ 绘制目标：renderScale >= 1 时返回窗口，否则返回中间 RenderTexture
    sf::RenderTarget& target();

    // ⭐ 每帧：beginFrame() → Scene 用 target() 画 → endFrame()
    void beginFrame();
    void endFrame();

    // ⭐ 加载超分 shader（shaderDir 是 upscale.vert/frag 所在目录）
    void loadUpscaler(const std::string& shaderDir);

    void setUpscaleMode(int mode);
    int  getUpscaleMode() const;

private:
    void applyView();
    void applyIcon();

    sf::RenderWindow window_;
    std::string title_;
    bool vsync_ = true;
    unsigned antiAliasing_ = 8;
    unsigned framerateLimit_ = 0;

    // ⭐ 渲染缩放
    sf::RenderTexture rt_;
    float renderScale_ = 1.0f;
    // ⭐ 超分辨率
    Upscaler upscaler_;
    bool     upscaleLoaded_ = false;
    bool  rtNeedsResize_ = true;
};