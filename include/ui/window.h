#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "render_pipeline.h"

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

    // ⭐ 渲染管线（转发到 RenderPipeline）
    void setRenderScale(float s) { pipeline_.setRenderScale(s); }
    float getRenderScale() const { return pipeline_.getRenderScale(); }

    void setUpscaleMode(int mode) { pipeline_.setUpscaleMode(mode); }
    int  getUpscaleMode() const   { return pipeline_.getUpscaleMode(); }

    void loadUpscaler(const std::string& shaderDir) {
        pipeline_.init(shaderDir);
    }

    PostProcessor& postProcess() { return pipeline_.postProcess(); }
    const PostProcessor& postProcess() const { return pipeline_.postProcess(); }

    float upscalePostMs() const { return pipeline_.upscalePostMs(); }

    // ⭐ 绘制目标 + 每帧生命周期
    sf::RenderTarget& target() { return pipeline_.target(window_); }
    void beginFrame()          { pipeline_.beginFrame(window_); }
    void endFrame()            { pipeline_.endFrame(window_); }

private:
    void applyView();
    void applyIcon();

    sf::RenderWindow window_;
    std::string title_;
    bool vsync_ = true;
    unsigned antiAliasing_ = 8;
    unsigned framerateLimit_ = 0;

    RenderPipeline pipeline_;
};