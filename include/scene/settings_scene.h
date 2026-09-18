#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "slider.h"
#include "text_input.h"
#include "preferences.h"
#include "runtime_config.h"
#include "text_strings.h"
#include "window.h"
#include "resolution.h"
#include "confirm_dialog.h"
#include "theme.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "ui_scale.h"
#include "utf8.h"
#include "toggle_row.h"

class SettingsScene : public Scene {
public:
    SettingsScene(std::shared_ptr<Background>    background,
                  std::shared_ptr<Preferences>   preferences,
                  std::shared_ptr<RuntimeConfig> runtimeConfig,
                  std::shared_ptr<Window>        window,
                  const sf::Font&                font,
                  std::shared_ptr<Logger>        logger);

    void onEnter() override;
    void onResume() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;
    SceneId nextScene() const override { return nextScene_; }

private:
    enum class Tab { Display = 0, Interface = 1, Graphics = 2, Audio = 3, Keys = 4, Other = 5 };
    static constexpr int kTabCount = 6;

    // ===== 应用状态 =====
    void refreshLabels();
    void refreshSelection();
    void syncFocus();              // ⭐ 新增
    void applyResolution();
    void applyFullscreen();
    void applyVsync();
    void applyAntiAliasing();
    void applyLogLevel();
    void applyTheme();
    void applyLanguage();
    void applyWallpaper();
    void applyFpsPosition();
    void applyFpsFormat();
    void applyFpsLimit();
    void applyButtonStyle();
    void applyLogRotation();
    void applyAnimation();
    void applyNotification();
    void applySound();
    void applyBGM();
    void applyGamepad();
    void applyAutoPause();
    void applyConsolePrompt();
    void applyShowColliders();
    void applyScreenShake();
    void applyParticles();
    void applyPseudo3D();
    void applyParallax();
    void applyPlayerAnimation();
    void applyLevelIntro();
    void resetAllPreferences();

    // ===== 渲染子函数 =====
    void renderTabs         (Window& window);
    void renderDisplayTab   (Window& window, float contentX, float ctrlX, float y);
    void renderInterfaceTab (Window& window, float contentX, float ctrlX, float y);
    void renderGraphicsTab  (Window& window, float contentX, float ctrlX, float y);
    void renderAudioTab     (Window& window, float contentX, float ctrlX, float y);
    void renderKeysTab      (Window& window, float contentX, float ctrlX, float y);
    void renderOtherTab     (Window& window, float contentX, float ctrlX, float y);
    void renderBackButton   (Window& window);

    // ===== 依赖 =====
    std::shared_ptr<Background>    background_;
    std::shared_ptr<Preferences>   preferences_;
    std::shared_ptr<RuntimeConfig> runtimeConfig_;
    std::shared_ptr<Window>        window_;
    std::shared_ptr<Logger>        logger_;
    const sf::Font&                font_;

    Tab currentTab_ = Tab::Display;
    std::vector<std::unique_ptr<Button>> tabButtons_;

    // ================= Display =================
    std::vector<std::unique_ptr<Button>> resolutionButtons_;
    std::vector<std::unique_ptr<ToggleRow>> displayToggles_;   // ⭐ 2 个 Toggle
    std::vector<std::unique_ptr<Button>> antiAliasingButtons_;
    std::vector<std::unique_ptr<Button>> logLevelButtons_;
    std::vector<std::unique_ptr<Button>> fpsLimitButtons_;

    // ================= Interface =================
    std::vector<std::unique_ptr<ToggleRow>> interfaceToggles_;   // ⭐ 4 个 Toggle
    std::vector<std::unique_ptr<Button>> fpsPosButtons_;
    std::vector<std::unique_ptr<Button>> fpsFormatButtons_;
    std::vector<std::unique_ptr<Button>> uiScaleButtons_;
    std::vector<std::unique_ptr<Button>> themeButtons_;
    std::vector<std::unique_ptr<Button>> languageButtons_;
    std::unique_ptr<Button> wallpaperButton_;
    std::vector<std::unique_ptr<Button>> clockPosButtons_;

    std::unique_ptr<Slider> consoleMaskSlider_;
    std::unique_ptr<Slider> consolePanelAlphaSlider_;
    std::vector<std::unique_ptr<Button>> consoleFontButtons_;
    std::vector<std::unique_ptr<Button>> consoleHistoryButtons_;
    std::vector<std::unique_ptr<Button>> consoleLineHeightButtons_;
    std::vector<std::unique_ptr<Button>> consolePromptButtons_;

    // ================= Graphics =================
    std::vector<std::unique_ptr<Button>> initialLivesButtons_;
    std::vector<std::unique_ptr<ToggleRow>> graphicsToggles_;   // ⭐ 新增
    std::vector<std::unique_ptr<Button>> animationSpeedButtons_;
    std::vector<std::unique_ptr<Button>> notificationPosButtons_;
    std::vector<std::unique_ptr<Button>> buttonCornerButtons_;
    std::vector<std::unique_ptr<Button>> buttonOutlineButtons_;

    // ================= Audio =================
    std::unique_ptr<Slider> masterVolumeSlider_;
    std::vector<std::unique_ptr<ToggleRow>> audioToggles_;   // ⭐ 3 个 Toggle
    std::unique_ptr<Slider> soundVolumeSlider_;
    std::unique_ptr<Slider> bgmVolumeSlider_;

    // ================= Keys =================
    std::vector<std::unique_ptr<Button>> keyBindingButtons_;   // 5 个动作的按钮
    int listeningAction_ = -1;                                 // -1 = 不在监听

    // ================= Other =================
    std::vector<std::unique_ptr<ToggleRow>> otherToggles_;   // ⭐ 2 个 Toggle
    std::vector<std::unique_ptr<Button>> logRotateButtons_;
    std::vector<std::unique_ptr<Button>> logKeepButtons_;
    std::unique_ptr<Button> aboutButton_;
    std::unique_ptr<Button> resetButton_;
    std::unique_ptr<TextInput> playerNameInput_;

    std::unique_ptr<Button> backButton_;
    std::unique_ptr<ConfirmDialog> resetConfirm_;
    std::unique_ptr<ConfirmDialog> aboutDialog_;

    // ================= 标签 =================
    sf::Text headingDisplay_, headingInterface_, headingGraphics_;
    sf::Text headingAudio_, headingKeys_, headingOther_;

    sf::Text labelResolution_, labelFullscreen_, labelVsync_;
    sf::Text labelAntiAliasing_, labelLogLevel_, labelFpsLimit_;

    sf::Text labelFps_, labelFpsPos_, labelFpsFormat_, labelUiScale_;
    sf::Text labelTheme_, labelLanguage_, labelWallpaper_;
    sf::Text labelClock_, labelClockPos_;
    sf::Text labelConsoleMask_, labelConsolePanelAlpha_;
    sf::Text labelConsoleFont_, labelConsoleHistory_;
    sf::Text labelConsoleLineHeight_, labelConsoleAutoScroll_;
    sf::Text labelConsoleBlink_, labelConsolePrompt_;

    sf::Text labelAnimation_, labelAnimationSpeed_;
    sf::Text labelNotification_, labelNotificationPos_;
    sf::Text labelPseudo3D_, labelParallax_;
    sf::Text labelPlayerAnim_, labelLevelIntro_;
    sf::Text labelInitialLives_;
    sf::Text labelParticles_, labelScreenShake_, labelShowColliders_;
    sf::Text labelButtonCorner_, labelButtonOutline_;

    sf::Text labelMasterVolume_;
    sf::Text labelSound_, labelSoundVolume_;
    sf::Text labelBGM_, labelBGMVolume_;
    sf::Text labelGamepad_;

    sf::Text labelRememberSize_, labelAutoPause_;
    sf::Text labelLogRotate_, labelLogKeep_, labelPlayerName_;
    sf::Text hintUiScale_;

    // ================= 状态 =================
    int     selectedResolution_;
    bool    fullscreen_;
    bool    vsync_;
    int     antiAliasingLevel_;
    int     logLevel_;
    int     fpsLimit_;

    bool    showFps_;
    int     fpsPosition_;
    int     fpsFormat_;
    float   uiScale_;
    ThemeId themeId_;
    int     languageIdx_ = 0;
    bool    showClock_;
    int     clockPosition_;
    int     consoleMask_;
    int     consolePanelAlpha_;
    int     consoleFontSize_;
    int     consoleHistoryLines_;
    int     consoleLineHeight_;
    bool    consoleAutoScroll_;
    bool    consoleBlinkCursor_;
    int     consolePrompt_;

    bool    animationEnabled_;
    int     animationSpeedIndex_;
    bool    notificationEnabled_;
    int     notificationPosition_;
    bool    pseudo3D_;
    bool    parallaxEnabled_;
    bool    playerAnimEnabled_;
    bool    levelIntroEnabled_;
    bool    particlesEnabled_;
    bool    screenShake_;
    bool    showColliders_;
    float   buttonCorner_;
    float   buttonOutline_;
    int     initialLives_;

    float   masterVolume_;
    bool    soundEnabled_;
    float   soundVolume_;
    bool    bgmEnabled_;
    float   bgmVolume_;
    bool    gamepadEnabled_;

    bool    rememberSize_;
    bool    autoPauseOnBlur_;
    int     logRotateIndex_;
    int     logKeepIndex_;

    SceneId nextScene_ = SceneId::None;

    // 语言版本追踪：语言变化时刷新所有 UI 文字
    int lastLangVersion_ = -1;
};