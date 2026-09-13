#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "slider.h"
#include "preferences.h"
#include "runtime_config.h"
#include "window.h"
#include "resolution.h"
#include <memory>
#include <vector>

class SettingsScene : public Scene {
public:
    SettingsScene(std::shared_ptr<Background>    background,
                  std::shared_ptr<Preferences>   preferences,
                  std::shared_ptr<RuntimeConfig> runtimeConfig,
                  std::shared_ptr<Window>        window,
                  const sf::Font&                font,
                  std::shared_ptr<Logger>        logger);

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    void refreshSelection();
    void applyResolution();
    void applyFullscreen();
    void applyVsync();

    std::shared_ptr<Background>    background_;
    std::shared_ptr<Preferences>   preferences_;
    std::shared_ptr<RuntimeConfig> runtimeConfig_;
    std::shared_ptr<Window>        window_;
    std::shared_ptr<Logger>        logger_;

    // 分辨率
    std::vector<std::unique_ptr<Button>> resolutionButtons_;
    // 全屏
    std::unique_ptr<Button> fullscreenOn_;
    std::unique_ptr<Button> fullscreenOff_;
    // V-Sync
    std::unique_ptr<Button> vsyncOn_;
    std::unique_ptr<Button> vsyncOff_;
    // 帧率显示
    std::unique_ptr<Button> fpsOn_;
    std::unique_ptr<Button> fpsOff_;
    // 记住窗口大小
    std::unique_ptr<Button> rememberOn_;
    std::unique_ptr<Button> rememberOff_;
    // 控制台遮罩
    std::unique_ptr<Slider> consoleMaskSlider_;

    std::unique_ptr<Button> backButton_;

    // 标签
    sf::Text headingDisplay_;
    sf::Text headingInterface_;
    sf::Text labelResolution_;
    sf::Text labelFullscreen_;
    sf::Text labelVsync_;
    sf::Text labelFps_;
    sf::Text labelConsoleMask_;
    sf::Text labelRememberSize_;

    // 状态
    int  selectedResolution_;
    bool fullscreen_;
    bool vsync_;
    bool showFps_;
    bool rememberSize_;

    SceneId nextScene_ = SceneId::None;
};