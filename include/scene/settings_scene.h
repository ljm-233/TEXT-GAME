#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "slider.h"
#include "text_input.h"
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
    void applyFpsPosition();
    void applyFpsLimit();
    void applyButtonStyle();
    void applyLogRotation();
    void resetAllPreferences();

    void renderTabs        (Window& window);
    void renderDisplayTab  (Window& window, float contentX, float ctrlX, float y);
    void renderInterfaceTab(Window& window, float contentX, float ctrlX, float y);
    void renderOtherTab    (Window& window, float contentX, float ctrlX, float y);
    void renderBackButton  (Window& window);

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
    std::unique_ptr<Button> fullscreenOn_, fullscreenOff_;
    std::unique_ptr<Button> vsyncOn_, vsyncOff_;
    std::vector<std::unique_ptr<Button>> antiAliasingButtons_;
    std::vector<std::unique_ptr<Button>> logLevelButtons_;
    std::vector<std::unique_ptr<Button>> fpsLimitButtons_;

    // Interface
    std::unique_ptr<Button> fpsOn_, fpsOff_;
    std::vector<std::unique_ptr<Button>> fpsPosButtons_;
    std::vector<std::unique_ptr<Button>> uiScaleButtons_;
    std::unique_ptr<Slider> consoleMaskSlider_;
    std::unique_ptr<Slider> consolePanelAlphaSlider_;
    std::vector<std::unique_ptr<Button>> consoleFontButtons_;
    std::vector<std::unique_ptr<Button>> consoleHistoryButtons_;
    std::vector<std::unique_ptr<Button>> consoleLineHeightButtons_;
    std::unique_ptr<Button> consoleAutoScrollOn_, consoleAutoScrollOff_;
    std::unique_ptr<Button> consoleBlinkOn_, consoleBlinkOff_;
    std::vector<std::unique_ptr<Button>> themeButtons_;
    std::unique_ptr<Button> wallpaperButton_;
    std::unique_ptr<Button> clockOn_, clockOff_;
    std::vector<std::unique_ptr<Button>> clockPosButtons_;

    // Other
    std::unique_ptr<Button> rememberOn_, rememberOff_;
    std::vector<std::unique_ptr<Button>> logRotateButtons_;
    std::vector<std::unique_ptr<Button>> logKeepButtons_;
    std::vector<std::unique_ptr<Button>> buttonCornerButtons_;
    std::vector<std::unique_ptr<Button>> buttonOutlineButtons_;
    std::unique_ptr<Button> aboutButton_;
    std::unique_ptr<Button> resetButton_;

    std::unique_ptr<Button> backButton_;
    std::unique_ptr<ConfirmDialog> resetConfirm_;
    std::unique_ptr<ConfirmDialog> aboutDialog_;

    // ⭐ 玩家名输入框
    std::unique_ptr<TextInput> playerNameInput_;

    // 标签
    sf::Text headingDisplay_, headingInterface_, headingOther_;
    sf::Text labelResolution_, labelFullscreen_, labelVsync_;
    sf::Text labelAntiAliasing_, labelLogLevel_, labelFpsLimit_;
    sf::Text labelFps_, labelFpsPos_, labelUiScale_;
    sf::Text labelConsoleMask_, labelConsolePanelAlpha_;
    sf::Text labelConsoleFont_, labelConsoleHistory_;
    sf::Text labelConsoleLineHeight_, labelConsoleAutoScroll_;
    sf::Text labelConsoleBlink_, labelTheme_, labelWallpaper_;
    sf::Text labelClock_, labelClockPos_;
    sf::Text labelRememberSize_, labelLogRotate_, labelLogKeep_;
    sf::Text labelButtonCorner_, labelButtonOutline_;
    sf::Text labelPlayerName_;
    sf::Text hintUiScale_;

    // 状态
    int     selectedResolution_;
    bool    fullscreen_;
    bool    vsync_;
    int     antiAliasingLevel_;
    int     logLevel_;
    int     fpsLimit_;
    bool    showFps_;
    int     fpsPosition_;
    float   uiScale_;
    int     consoleMask_;
    int     consolePanelAlpha_;
    int     consoleFontSize_;
    int     consoleHistoryLines_;
    int     consoleLineHeight_;
    bool    consoleAutoScroll_;
    bool    consoleBlinkCursor_;
    ThemeId themeId_;
    float   buttonCorner_;
    float   buttonOutline_;
    bool    showClock_;
    int     clockPosition_;
    bool    rememberSize_;
    int     logRotateIndex_;
    int     logKeepIndex_;

    SceneId nextScene_ = SceneId::None;
};