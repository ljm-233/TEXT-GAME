#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "slider.h"
#include "preferences.h"
#include "runtime_config.h"
#include "window.h"
#include "resolution.h"
#include "confirm_dialog.h"
#include "theme.h"
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
    enum class Tab { Display = 0, Interface = 1, Other = 2 };
    static constexpr int kTabCount = 3;

    void refreshSelection();
    void applyResolution();
    void applyFullscreen();
    void applyVsync();
    void applyAntiAliasing();
    void applyLogLevel();
    void applyTheme();
    void applyWallpaper();
    void resetAllPreferences();

    std::shared_ptr<Background>    background_;
    std::shared_ptr<Preferences>   preferences_;
    std::shared_ptr<RuntimeConfig> runtimeConfig_;
    std::shared_ptr<Window>        window_;
    std::shared_ptr<Logger>        logger_;
    const sf::Font&                font_;

    Tab currentTab_ = Tab::Display;
    std::vector<std::unique_ptr<Button>> tabButtons_;

    // Display
    std::vector<std::unique_ptr<Button>> resolutionButtons_;
    std::unique_ptr<Button> fullscreenOn_;
    std::unique_ptr<Button> fullscreenOff_;
    std::unique_ptr<Button> vsyncOn_;
    std::unique_ptr<Button> vsyncOff_;
    std::vector<std::unique_ptr<Button>> antiAliasingButtons_;
    std::vector<std::unique_ptr<Button>> logLevelButtons_;

    // Interface
    std::unique_ptr<Button> fpsOn_;
    std::unique_ptr<Button> fpsOff_;
    std::vector<std::unique_ptr<Button>> uiScaleButtons_;
    std::unique_ptr<Slider> consoleMaskSlider_;
    std::vector<std::unique_ptr<Button>> consoleFontButtons_;
    std::vector<std::unique_ptr<Button>> consoleHistoryButtons_;
    std::vector<std::unique_ptr<Button>> themeButtons_;
    std::unique_ptr<Button> wallpaperButton_;

    // Other
    std::unique_ptr<Button> rememberOn_;
    std::unique_ptr<Button> rememberOff_;
    std::unique_ptr<Button> resetButton_;

    std::unique_ptr<Button> backButton_;
    std::unique_ptr<ConfirmDialog> resetConfirm_;

    // 标签
    sf::Text headingDisplay_;
    sf::Text headingInterface_;
    sf::Text headingOther_;
    sf::Text labelResolution_;
    sf::Text labelFullscreen_;
    sf::Text labelVsync_;
    sf::Text labelAntiAliasing_;
    sf::Text labelLogLevel_;
    sf::Text labelFps_;
    sf::Text labelUiScale_;
    sf::Text labelConsoleMask_;
    sf::Text labelConsoleFont_;
    sf::Text labelConsoleHistory_;
    sf::Text labelTheme_;
    sf::Text labelWallpaper_;
    sf::Text labelRememberSize_;
    sf::Text hintUiScale_;

    // 状态
    int     selectedResolution_;
    bool    fullscreen_;
    bool    vsync_;
    int     antiAliasingLevel_;
    int     logLevel_;
    bool    showFps_;
    float   uiScale_;
    int     consoleFontSize_;
    int     consoleHistoryLines_;
    ThemeId themeId_;
    bool    rememberSize_;

    SceneId nextScene_ = SceneId::None;
};