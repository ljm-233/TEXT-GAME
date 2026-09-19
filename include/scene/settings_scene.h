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
#include "toggle_row.h"
#include "multi_row.h"
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
    void syncFocus();
    void resetGraphicsPost();
    void applyPreset(int idx);

    bool anySliderEditing() const {
        for (auto* s : {saturationSlider_.get(), contrastSlider_.get(),
                        brightnessSlider_.get(), gammaSlider_.get(),
                        vignetteSlider_.get(), bloomStrengthSlider_.get(),
                        bloomThresholdSlider_.get(), chromaticSlider_.get(),
                        grainSlider_.get(), scanlineSlider_.get(),
                        ditherSlider_.get(), consoleMaskSlider_.get(),
                        consolePanelAlphaSlider_.get(), masterVolumeSlider_.get(),
                        soundVolumeSlider_.get(), bgmVolumeSlider_.get()}) {
            if (s && s->isEditing()) return true;
        }
        return false;
    }
    void applyResolution();
    void applyFullscreen();
    void applyWindowMode();
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
    void applyGamepadVibration();
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
    float renderDisplayTab   (Window& window, float contentX, float ctrlX, float y);
    float renderInterfaceTab (Window& window, float contentX, float ctrlX, float y);
    float renderGraphicsTab  (Window& window, float contentX, float ctrlX, float y);
    float renderAudioTab     (Window& window, float contentX, float ctrlX, float y);
    float renderKeysTab      (Window& window, float contentX, float ctrlX, float y);
    float renderOtherTab     (Window& window, float contentX, float ctrlX, float y);

    // ⭐ 设计坐标系 View（UI 整体缩放）
    void updateDesignView();

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
    std::vector<std::unique_ptr<ToggleRow>> displayToggles_;
    std::vector<std::unique_ptr<MultiRow>>  displayMultiRows_;
    // [0] Resolution  [1] AntiAliasing  [2] LogLevel  [3] FpsLimit

    // ================= Interface =================
    std::vector<std::unique_ptr<ToggleRow>> interfaceToggles_;
    std::vector<std::unique_ptr<MultiRow>>  interfaceMultiRows_;
    // [0] FpsPos  [1] FpsFormat  [2] UiScale  [3] Theme  [4] Language
    // [5] ClockPos  [6] ConsoleFont  [7] ConsoleHistory
    // [8] ConsoleLineHeight  [9] ConsolePrompt
    std::unique_ptr<Button> wallpaperButton_;
    std::unique_ptr<Slider> consoleMaskSlider_;
    std::unique_ptr<Slider> consolePanelAlphaSlider_;

    // ================= Graphics =================
    std::vector<std::unique_ptr<ToggleRow>> graphicsToggles_;
    std::vector<std::unique_ptr<MultiRow>>  graphicsMultiRows_;
    // [0] InitialLives  [1] AnimationSpeed  [2] NotificationPos
    // [3] ButtonCorner  [4] ButtonOutline

    // ================= Graphics 预设 =================
    std::unique_ptr<MultiRow> presetRow_;

    // ================= Graphics 后处理 Slider =================
    std::unique_ptr<Slider> saturationSlider_;
    std::unique_ptr<Slider> contrastSlider_;
    std::unique_ptr<Slider> brightnessSlider_;
    std::unique_ptr<Slider> gammaSlider_;
    std::unique_ptr<Slider> vignetteSlider_;
    std::unique_ptr<Slider> bloomStrengthSlider_;
    std::unique_ptr<Slider> bloomThresholdSlider_;
    std::unique_ptr<Slider> chromaticSlider_;
    std::unique_ptr<Slider> grainSlider_;
    std::unique_ptr<Slider> scanlineSlider_;
    std::unique_ptr<Slider> ditherSlider_;

    // ================= Audio =================
    std::vector<std::unique_ptr<ToggleRow>> audioToggles_;
    // [0] Sound  [1] BGM  [2] Gamepad
    std::unique_ptr<Slider> masterVolumeSlider_;
    std::unique_ptr<Slider> soundVolumeSlider_;
    std::unique_ptr<Slider> bgmVolumeSlider_;
    std::unique_ptr<Slider> gamepadVibrationSlider_;

    // ================= Keys =================
    std::vector<std::unique_ptr<Button>> keyBindingButtons_;
    int listeningAction_ = -1;

    // ================= Other =================
    std::vector<std::unique_ptr<ToggleRow>> otherToggles_;
    // [0] RememberSize  [1] AutoPause
    std::vector<std::unique_ptr<MultiRow>>  otherMultiRows_;
    // [0] LogRotate  [1] LogKeep
    std::unique_ptr<TextInput> playerNameInput_;
    std::unique_ptr<Button> aboutButton_;
    std::unique_ptr<Button> resetButton_;
    std::unique_ptr<Button> resetGraphicsButton_;

    std::unique_ptr<Button> backButton_;
    std::unique_ptr<ConfirmDialog> resetConfirm_;
    std::unique_ptr<ConfirmDialog> aboutDialog_;

    // ================= 标题 / 标签 =================
    sf::Text headingDisplay_, headingInterface_, headingGraphics_;
    sf::Text headingAudio_, headingKeys_, headingOther_;

    sf::Text labelWallpaper_;
    sf::Text labelConsoleMask_, labelConsolePanelAlpha_;
    sf::Text labelMasterVolume_, labelSoundVolume_, labelBGMVolume_;
    sf::Text labelVibrationIntensity_;
    sf::Text labelPlayerName_;
    sf::Text hintUiScale_;
    // ⭐ 后处理
    sf::Text labelPostSaturation_;
    sf::Text labelPostContrast_;
    sf::Text labelPostBrightness_;
    sf::Text labelPostGamma_;
    sf::Text labelPostVignette_;
    sf::Text labelPostBloomStrength_;
    sf::Text labelPostBloomThreshold_;
    sf::Text labelPostChromatic_;
    sf::Text labelPostGrain_;
    sf::Text labelPostScanline_;
    sf::Text labelPostDither_;

    // ================= 状态 =================
    int     selectedResolution_ = 0;
    bool    fullscreen_         = false;
    bool    vsync_              = true;
    int     antiAliasingLevel_  = 8;
    int     logLevel_           = 2;
    int     fpsLimit_           = 60;

    bool    showFps_            = false;
    int     fpsPosition_        = 1;
    int     fpsFormat_          = 1;
    float   uiScale_            = 1.0f;
    float   fontScale_          = 1.0f;
    float   renderScale_        = 1.0f;
    ThemeId themeId_            = ThemeId::Dark;
    int     languageIdx_        = 0;
    bool    showClock_          = false;
    int     clockPosition_      = 0;
    int     consoleMask_        = 160;
    int     consolePanelAlpha_  = 220;
    int     consoleFontSize_    = 18;
    int     consoleHistoryLines_= 200;
    int     consoleLineHeight_  = 26;
    bool    consoleAutoScroll_  = true;
    bool    consoleBlinkCursor_ = true;
    int     consolePrompt_      = 0;

    bool    animationEnabled_   = true;
    int     animationSpeedIndex_= 1;
    bool    notificationEnabled_= true;
    int     notificationPosition_= 1;
    bool    pseudo3D_           = true;
    bool    parallaxEnabled_    = true;
    bool    playerAnimEnabled_  = true;
    bool    levelIntroEnabled_  = true;
    bool    particlesEnabled_   = true;
    bool    screenShake_        = true;
    bool    showColliders_      = false;
    float   buttonCorner_       = 6.f;
    float   buttonOutline_      = 2.f;
    int     initialLives_       = 1;

    float   masterVolume_       = 1.0f;
    bool    soundEnabled_       = true;
    float   soundVolume_        = 0.6f;
    bool    bgmEnabled_         = true;
    float   bgmVolume_          = 0.4f;
    bool    gamepadEnabled_     = true;
    bool    gamepadVibrationEnabled_   = true;
    float   gamepadVibrationIntensity_ = 1.0f;

    int     windowMode_         = 0;   // 0=窗口 1=最大化 2=全屏
    bool    rememberSize_       = true;
    bool    autoPauseOnBlur_    = true;
    int     logRotateIndex_     = 0;
    int     logKeepIndex_       = 1;

    SceneId nextScene_ = SceneId::None;
    int lastLangVersion_ = -1;

    // ⭐ 设计坐标系（固定 1280×720，由 View 缩放到窗口）
    sf::View designView_;
    static constexpr float kDesignW = 1280.f;
    static constexpr float kDesignH = 720.f;
    // ⭐ uiScale > 1.0 时内容溢出，滚轮可上下平移
    float contentScroll_ = 0.f;
    float contentTotalH_ = 0.f;   // 上一帧内容底部 y
    int upscaleMode_ = 1;   // 0=关 1=双三次
};