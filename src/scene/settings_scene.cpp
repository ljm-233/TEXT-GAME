#include "settings_scene.h"
#include "text_strings.h"
#include <string>
#include "utf8.h"
#include "ui_scale.h"
#include "button_style.h"
#include "animation.h"
#include "notification.h"
#include "sound_manager.h"
#include "focus_group.h"
#include "keybindings.h"
#include <algorithm>
#include <cmath>
#include "lang.h"

namespace {
// ===== 常量表 =====
const int kaaLevels[]     = {0, 4, 8, 16};
constexpr int kaaCount    = 4;
const char* kAALabels[]   = {"关", "4x", "8x", "16x"};

const LogLevel klogLevels[] = {
    LogLevel::Trace, LogLevel::Debug, LogLevel::Info,
    LogLevel::Warn,  LogLevel::Error
};
constexpr int klogCount = 5;
const char* kLogLabels[] = {"Trace", "Debug", "Info", "Warn", "Error"};

const int kfpsLimits[]  = {0, 30, 60, 120, 144};
constexpr int kfpsLimitCount = 5;
const char* kFpsLimitLabels[] = {"无", "30", "60", "120", "144"};

const float kanimSpeeds[] = {0.5f, 1.0f, 2.0f};
constexpr int kanimSpeedCount = 3;
const char* kAnimSpeedLabels[] = {"慢", "正常", "快"};

const char* kPosLabels[] = {"左上", "右上", "左下", "右下"};
constexpr int kPosCount  = 4;

const char* kFpsFormatLabels[] = {"纯数字", "60 FPS", "60.0 FPS"};
constexpr int kFpsFormatCount  = 3;

const float kUiScales[]     = {0.8f, 1.0f, 1.2f, 1.5f};
constexpr int kUiScaleCount = 4;
const char* kUiScaleLabels[] = {"0.8x", "1.0x", "1.2x", "1.5x"};

const int kConsoleFonts[]  = {14, 18, 22, 26};
constexpr int kConsoleFontCount = 4;
const char* kConsoleFontLabels[] = {"小", "中", "大", "特大"};

const int kConsoleHistory[] = {50, 100, 200, 500};
constexpr int kConsoleHistoryCount = 4;

const int kConsoleLineHeights[] = {20, 26, 32};
constexpr int kConsoleLineHeightCount = 3;
const char* kConsoleLineHeightLabels[] = {"紧凑", "正常", "宽松"};

const char* kConsolePromptLabels[] = {">", "$", "λ", "❯"};
constexpr int kConsolePromptCount  = 4;

const float kButtonCorners[]  = {0.f, 6.f, 14.f};
constexpr int kButtonCornerCount = 3;
const char* kButtonCornerLabels[] = {"直角", "小圆", "大圆"};

const float kButtonOutlines[]  = {0.f, 2.f, 4.f};
constexpr int kButtonOutlineCount = 3;
const char* kButtonOutlineLabels[] = {"无", "细", "粗"};

const size_t kLogRotateSizes[] = {0, 1*1024*1024, 5*1024*1024, 10*1024*1024};
constexpr int kLogRotateCount = 4;
const char* kLogRotateLabels[] = {"无限", "1MB", "5MB", "10MB"};

const int kLogKeeps[] = {1, 3, 5, 10};
constexpr int kLogKeepCount = 4;

// ===== 布局 =====
constexpr float kTabX     = 40.f;
constexpr float kTabY     = 90.f;
constexpr float kTabGap   = 62.f;
constexpr float kContentX = kTabX + 200.f;
constexpr float kCtrlX    = kContentX + 240.f;
constexpr float kRowH     = 50.f;
constexpr float kBtnW     = 280.f;
constexpr float kBtnH     = 46.f;
constexpr float kGapX     = 16.f;
constexpr float kGapY     = 10.f;

// ===== 索引查找 =====
int indexOfAA(int level) {
    for (int i = 0; i < kaaCount; ++i) if (kaaLevels[i] == level) return i;
    return 2;
}
int indexOfUiScale(float s) {
    for (int i = 0; i < kUiScaleCount; ++i)
        if (std::abs(kUiScales[i] - s) < 0.01f) return i;
    return 1;
}
int indexOfConsoleFont(int f) {
    for (int i = 0; i < kConsoleFontCount; ++i)
        if (kConsoleFonts[i] == f) return i;
    return 1;
}
int indexOfConsoleHistory(int n) {
    for (int i = 0; i < kConsoleHistoryCount; ++i)
        if (kConsoleHistory[i] == n) return i;
    return 2;
}
int indexOfConsoleLineHeight(int h) {
    for (int i = 0; i < kConsoleLineHeightCount; ++i)
        if (kConsoleLineHeights[i] == h) return i;
    return 1;
}
int indexOfLogLevel(int l) {
    for (int i = 0; i < klogCount; ++i)
        if (static_cast<int>(klogLevels[i]) == l) return i;
    return 2;
}
int indexOfFpsLimit(int l) {
    for (int i = 0; i < kfpsLimitCount; ++i)
        if (kfpsLimits[i] == l) return i;
    return 2;
}
int indexOfButtonCorner(float c) {
    for (int i = 0; i < kButtonCornerCount; ++i)
        if (std::abs(kButtonCorners[i] - c) < 0.5f) return i;
    return 0;
}
int indexOfButtonOutline(float o) {
    for (int i = 0; i < kButtonOutlineCount; ++i)
        if (std::abs(kButtonOutlines[i] - o) < 0.5f) return i;
    return 1;
}
int indexOfLogRotate(int idx) { if (idx < 0 || idx >= kLogRotateCount) return 0; return idx; }
int indexOfLogKeep(int idx)   { if (idx < 0 || idx >= kLogKeepCount)   return 1; return idx; }
int indexOfAnimSpeed(int idx) { if (idx < 0 || idx >= kanimSpeedCount) return 1; return idx; }
int indexOfPos(int idx)       { if (idx < 0 || idx >= kPosCount)       return 1; return idx; }
int indexOfFpsFormat(int idx) { if (idx < 0 || idx >= kFpsFormatCount) return 1; return idx; }
int indexOfConsolePrompt(int idx) { if (idx < 0 || idx >= kConsolePromptCount) return 0; return idx; }

// ⭐ 初始生命值（1/3/5/10/100）↔ 索引
int indexOfLives(int lives) {
    const int kLives[] = {1, 3, 5, 10, 100};
    for (int i = 0; i < 5; ++i)
        if (kLives[i] == lives) return i;
    return 0;
}

// ⭐ 布局缩放：小屏下压缩垂直间距
inline float contentScaleForHeight(float winH) {
    return std::clamp(winH / 720.f, 0.65f, 1.0f);
}

// ⭐ 计算动态值
struct DynLayout {
    float rowH;        // 每行高度
    float btnH;        // 按钮高度
    float tabGap;      // Tab 按钮间距
    float tabY;        // 第一个 Tab 的 y
};

inline DynLayout computeDynLayout(float winH) {
    float s = contentScaleForHeight(winH);
    return {
        /*rowH  */ 50.f * s,
        /*btnH  */ 46.f * s,
        /*tabGap*/ 62.f * s,
        /*tabY  */ 90.f * s,
    };
}
}

// ============================================================
// 构造
// ============================================================

SettingsScene::SettingsScene(std::shared_ptr<Background>    background,
                             std::shared_ptr<Preferences>   preferences,
                             std::shared_ptr<RuntimeConfig> runtimeConfig,
                             std::shared_ptr<Window>        window,
                             const sf::Font&                font,
                             std::shared_ptr<Logger>        logger)
    : background_(std::move(background)),
      preferences_(std::move(preferences)),
      runtimeConfig_(std::move(runtimeConfig)),
      window_(std::move(window)),
      logger_(std::move(logger)),
      font_(font),
      headingDisplay_  (font, toSf(Str::TabDisplay),   scaledFontSize(24)),
      headingInterface_(font, toSf(Str::TabInterface), scaledFontSize(24)),
      headingGraphics_ (font, toSf(Str::TabGraphics),  scaledFontSize(24)),
      headingAudio_    (font, toSf(Str::TabAudioLog),  scaledFontSize(24)),
      headingKeys_     (font, toSf(Str::TabKeys),            scaledFontSize(24)),
      headingOther_    (font, toSf(Str::TabOther),     scaledFontSize(24)),

      labelResolution_     (font, toSf(Str::LabelResolution),     scaledFontSize(20)),
      labelFullscreen_     (font, toSf(Str::LabelFullscreen),     scaledFontSize(20)),
      labelVsync_          (font, toSf(Str::LabelVsync),          scaledFontSize(20)),
      labelAntiAliasing_   (font, toSf(Str::LabelAntiAliasing),   scaledFontSize(20)),
      labelLogLevel_       (font, toSf(Str::LabelLogLevel),       scaledFontSize(20)),
      labelFpsLimit_       (font, toSf(Str::LabelFpsLimit),       scaledFontSize(20)),

      labelFps_            (font, toSf(Str::LabelFps),            scaledFontSize(20)),
      labelFpsPos_         (font, toSf(Str::LabelFpsPos),         scaledFontSize(20)),
      labelFpsFormat_      (font, toSf(Str::LabelFpsFormat),      scaledFontSize(20)),
      labelUiScale_        (font, toSf(Str::LabelUiScale),        scaledFontSize(20)),
      labelTheme_          (font, toSf(Str::LabelTheme),          scaledFontSize(20)),
      labelLanguage_       (font, toSf(Str::LabelLanguage),       scaledFontSize(20)),
      labelWallpaper_      (font, toSf(Str::LabelWallpaper),      scaledFontSize(20)),
      labelClock_          (font, toSf(Str::LabelClock),          scaledFontSize(20)),
      labelClockPos_       (font, toSf(Str::LabelClockPos),       scaledFontSize(20)),

      labelConsoleMask_    (font, toSf(Str::LabelConsoleMask),    scaledFontSize(20)),
      labelConsolePanelAlpha_(font, toSf(Str::LabelConsolePanelAlpha), scaledFontSize(20)),
      labelConsoleFont_    (font, toSf(Str::LabelConsoleFont),    scaledFontSize(20)),
      labelConsoleHistory_ (font, toSf(Str::LabelConsoleHistory), scaledFontSize(20)),
      labelConsoleLineHeight_(font, toSf(Str::LabelConsoleLineHeight), scaledFontSize(20)),
      labelConsoleAutoScroll_(font, toSf(Str::LabelConsoleAutoScroll), scaledFontSize(20)),
      labelConsoleBlink_   (font, toSf(Str::LabelConsoleBlink),   scaledFontSize(20)),
      labelConsolePrompt_  (font, toSf(Str::LabelConsolePrompt),  scaledFontSize(20)),

      labelAnimation_      (font, toSf(Str::LabelAnimation),      scaledFontSize(20)),
      labelAnimationSpeed_ (font, toSf(Str::LabelAnimationSpeed), scaledFontSize(20)),
      labelNotification_   (font, toSf(Str::LabelNotification),   scaledFontSize(20)),
      labelNotificationPos_(font, toSf(Str::LabelNotificationPos),scaledFontSize(20)),
      labelPseudo3D_       (font, toSf(Str::LabelPseudo3D),       scaledFontSize(20)),
      labelParallax_       (font, toSf(Str::LabelParallax),       scaledFontSize(20)),
      labelPlayerAnim_     (font, toSf(Str::LabelPlayerAnimation),scaledFontSize(20)),
      labelLevelIntro_     (font, toSf(Str::LabelLevelIntro),     scaledFontSize(20)),
      labelInitialLives_   (font, toSf(Str::LabelInitialLives),   scaledFontSize(20)),
      labelParticles_      (font, toSf(Str::LabelParticles),      scaledFontSize(20)),
      labelScreenShake_    (font, toSf(Str::LabelScreenShake),    scaledFontSize(20)),
      labelShowColliders_  (font, toSf(Str::LabelShowColliders),  scaledFontSize(20)),
      labelButtonCorner_   (font, toSf(Str::LabelButtonCorner),   scaledFontSize(20)),
      labelButtonOutline_  (font, toSf(Str::LabelButtonOutline),  scaledFontSize(20)),

      labelMasterVolume_   (font, toSf(Str::LabelMasterVolume),   scaledFontSize(20)),
      labelSound_          (font, toSf(Str::LabelSound),          scaledFontSize(20)),
      labelSoundVolume_    (font, toSf(Str::LabelSoundVolume),    scaledFontSize(20)),
      labelBGM_            (font, toSf(Str::LabelBGM),            scaledFontSize(20)),
      labelBGMVolume_      (font, toSf(Str::LabelBGMVolume),      scaledFontSize(20)),
      labelGamepad_        (font, toSf(Str::LabelGamepad),        scaledFontSize(20)),

      labelRememberSize_   (font, toSf(Str::LabelRememberSize),   scaledFontSize(20)),
      labelAutoPause_      (font, toSf(Str::LabelAutoPause),      scaledFontSize(20)),
      labelLogRotate_      (font, toSf(Str::LabelLogRotate),      scaledFontSize(20)),
      labelLogKeep_        (font, toSf(Str::LabelLogKeep),        scaledFontSize(20)),
      labelPlayerName_     (font, toSf(Str::LabelPlayerName),     scaledFontSize(20)),
      hintUiScale_         (font, toSf(Str::HintUiScale),         scaledFontSize(14)) {

    selectedResolution_ = clampResolutionIndex(preferences_->getInt("resolution_index", 0));
    fullscreen_          = preferences_->getBool("fullscreen", false);
    vsync_               = preferences_->getBool("vsync", true);
    antiAliasingLevel_   = preferences_->getInt("anti_aliasing", 8);
    logLevel_            = preferences_->getInt("log_level", static_cast<int>(LogLevel::Info));
    fpsLimit_            = preferences_->getInt("fps_limit", 60);

    showFps_             = preferences_->getBool("show_fps", false);
    fpsPosition_         = preferences_->getInt("fps_position", 1);
    fpsFormat_           = indexOfFpsFormat(preferences_->getInt("fps_format", 1));
    uiScale_             = static_cast<float>(preferences_->getDouble("ui_scale", 1.0));
    themeId_             = static_cast<ThemeId>(preferences_->getInt("theme", 0));
    {
        std::string langCode = preferences_->get("language", "zh");
        const auto& avail = Lang::instance().available();
        languageIdx_ = 0;
        for (std::size_t i = 0; i < avail.size(); ++i) {
            if (avail[i] == langCode) { languageIdx_ = static_cast<int>(i); break; }
        }
    }
    showClock_           = preferences_->getBool("show_clock", false);
    clockPosition_       = preferences_->getInt("clock_position", 0);
    consoleMask_         = std::clamp(preferences_->getInt("console_mask", 160), 0, 255);
    consolePanelAlpha_   = std::clamp(preferences_->getInt("console_panel_alpha", 220), 0, 255);
    consoleFontSize_     = preferences_->getInt("console_font_size", 18);
    consoleHistoryLines_ = preferences_->getInt("console_history_lines", 200);
    consoleLineHeight_   = preferences_->getInt("console_line_height", 26);
    consoleAutoScroll_   = preferences_->getBool("console_auto_scroll", true);
    consoleBlinkCursor_  = preferences_->getBool("console_blink_cursor", true);
    consolePrompt_       = indexOfConsolePrompt(preferences_->getInt("console_prompt", 0));

    animationEnabled_    = preferences_->getBool("animation_enabled", true);
    animationSpeedIndex_ = indexOfAnimSpeed(preferences_->getInt("animation_speed_index", 1));
    notificationEnabled_ = preferences_->getBool("notification_enabled", true);
    notificationPosition_= indexOfPos(preferences_->getInt("notification_position", 1));
    initialLives_        = preferences_->getInt("initial_lives", 1);
    if (initialLives_ != 1 && initialLives_ != 3 && initialLives_ != 5
        && initialLives_ != 10 && initialLives_ != 100) {
        initialLives_ = 1;
        preferences_->setInt("initial_lives", 1);
    }
    pseudo3D_            = preferences_->getBool("pseudo_3d", true);
    parallaxEnabled_     = preferences_->getBool("parallax", true);
    playerAnimEnabled_   = preferences_->getBool("player_animation", true);
    levelIntroEnabled_   = preferences_->getBool("level_intro", true);
    particlesEnabled_    = preferences_->getBool("particles", true);
    screenShake_         = preferences_->getBool("screen_shake", true);
    showColliders_       = preferences_->getBool("show_colliders", false);
    buttonCorner_        = static_cast<float>(preferences_->getDouble("button_corner", 6.0));
    buttonOutline_       = static_cast<float>(preferences_->getDouble("button_outline", 2.0));

    masterVolume_        = static_cast<float>(preferences_->getDouble("master_volume", 1.0));
    soundEnabled_        = preferences_->getBool("sound_enabled", true);
    soundVolume_         = static_cast<float>(preferences_->getDouble("sound_volume", 0.6));
    bgmEnabled_          = preferences_->getBool("bgm_enabled", true);
    bgmVolume_           = static_cast<float>(preferences_->getDouble("bgm_volume", 0.4));
    gamepadEnabled_      = preferences_->getBool("gamepad_enabled", true);

    rememberSize_        = preferences_->getBool("remember_window_size", true);
    autoPauseOnBlur_     = preferences_->getBool("auto_pause_on_blur", true);
    logRotateIndex_      = indexOfLogRotate(preferences_->getInt("log_rotate", 0));
    logKeepIndex_        = indexOfLogKeep(preferences_->getInt("log_keep", 1));

    playerNameInput_ = std::make_unique<TextInput>(
        font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 40.f},
        Str::PlayerNamePlaceholder, 18, 16);
    playerNameInput_->setText(preferences_->get("player_name", ""));
    playerNameInput_->setOnChanged([this](const std::string& s) {
        preferences_->set("player_name", s);
    });

    auto headingColor = sf::Color(160, 200, 240);
    headingDisplay_.setFillColor(headingColor);
    headingInterface_.setFillColor(headingColor);
    headingGraphics_.setFillColor(headingColor);
    headingAudio_.setFillColor(headingColor);
    headingKeys_.setFillColor(headingColor);
    headingOther_.setFillColor(headingColor);

    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelResolution_, &labelFullscreen_, &labelVsync_,
                    &labelAntiAliasing_, &labelLogLevel_, &labelFpsLimit_,
                    &labelFps_, &labelFpsPos_, &labelFpsFormat_, &labelUiScale_,
                    &labelTheme_, &labelLanguage_, &labelWallpaper_, &labelClock_, &labelClockPos_,
                    &labelConsoleMask_, &labelConsolePanelAlpha_,
                    &labelConsoleFont_, &labelConsoleHistory_,
                    &labelConsoleLineHeight_, &labelConsoleAutoScroll_,
                    &labelConsoleBlink_, &labelConsolePrompt_,
                    &labelAnimationSpeed_,
                    &labelNotificationPos_,
                    &labelInitialLives_,
                    &labelButtonCorner_, &labelButtonOutline_,
                    &labelMasterVolume_,
                    &labelSound_, &labelSoundVolume_, &labelBGM_, &labelBGMVolume_,
                    &labelGamepad_,
                    &labelRememberSize_, &labelAutoPause_,
                    &labelLogRotate_, &labelLogKeep_, &labelPlayerName_}) {
        t->setFillColor(labelColor);
    }
    hintUiScale_.setFillColor(sf::Color(180, 180, 200));

    const char* tabLabels[] = {
        Str::TabDisplay, Str::TabInterface, Str::TabGraphics,
        Str::TabAudioLog, Str::TabKeys, Str::TabOther
    };
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_.push_back(std::make_unique<Button>(
            tabLabels[i], font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 50.f}, 22));
    }

    auto makeToggle = [&](const std::string& onText, const std::string& offText) {
        auto on  = std::make_unique<Button>(onText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(offText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        return std::make_pair(std::move(on), std::move(off));
    };

    auto makeToggleRow = [&](const char* labelKey,
                             std::function<void(bool)> onChanged) {
        auto row = std::make_unique<ToggleRow>(font_, labelKey,
                                               std::move(onChanged));
        auto [on, off] = makeToggle(Str::On, Str::Off);
        row->onButton = std::move(on);
        row->offButton = std::move(off);
        return row;
    };

    // Display
    auto makeMultiRow = [&](const char* labelKey,
                            std::function<void(int)> onSelected) {
        return std::make_unique<MultiRow>(font_, labelKey, std::move(onSelected));
    };

    // Resolution: 2×2 网格，按钮 280×46
    {
        auto row = makeMultiRow(Str::LabelResolution, [this](int i) {
            selectedResolution_ = i; refreshSelection(); applyResolution();
        });
        row->columns = 2;
        row->stepX   = kBtnW + kGapX;   // 296
        row->stepY   = kBtnH + kGapY;   // 56
        for (int i = 0; i < kResolutionCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kResolutions[i].label, font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{kBtnW, kBtnH}, 18));
        }
        displayMultiRows_.push_back(std::move(row));
    }
    // AntiAliasing
    {
        auto row = makeMultiRow(Str::LabelAntiAliasing, [this](int i) {
            antiAliasingLevel_ = kaaLevels[i];
            refreshSelection(); applyAntiAliasing();
        });
        for (int i = 0; i < kaaCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kAALabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{86.f, 40.f}, 18));
        }
        displayMultiRows_.push_back(std::move(row));
    }
    // LogLevel
    {
        auto row = makeMultiRow(Str::LabelLogLevel, [this](int i) {
            logLevel_ = static_cast<int>(klogLevels[i]);
            refreshSelection(); applyLogLevel();
        });
        row->stepX = 106.f;
        for (int i = 0; i < klogCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kLogLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{96.f, 40.f}, 16));
        }
        displayMultiRows_.push_back(std::move(row));
    }
    // FpsLimit
    {
        auto row = makeMultiRow(Str::LabelFpsLimit, [this](int i) {
            fpsLimit_ = kfpsLimits[i];
            refreshSelection(); applyFpsLimit();
        });
        row->stepX = 86.f;
        for (int i = 0; i < kfpsLimitCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kFpsLimitLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{76.f, 40.f}, 16));
        }
        displayMultiRows_.push_back(std::move(row));
    }

    // ⭐ 2 个 Toggle 用 ToggleRow
    displayToggles_.push_back(makeToggleRow(Str::LabelFullscreen, [this](bool v) {
        fullscreen_ = v; refreshSelection(); applyFullscreen();
    }));
    displayToggles_.push_back(makeToggleRow(Str::LabelVsync, [this](bool v) {
        vsync_ = v; refreshSelection(); applyVsync();
    }));

    // Interface
    interfaceToggles_.push_back(makeToggleRow(Str::LabelFps, [this](bool v) {
        showFps_ = v; refreshSelection();
        preferences_->setBool("show_fps", v);
    }));
    // FpsPos
    {
        auto row = makeMultiRow(Str::LabelFpsPos, [this](int i) {
            fpsPosition_ = i; refreshSelection(); applyFpsPosition();
        });
        row->stepX = 86.f;
        for (int i = 0; i < kPosCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kPosLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{76.f, 40.f}, 16));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    // FpsFormat
    {
        auto row = makeMultiRow(Str::LabelFpsFormat, [this](int i) {
            fpsFormat_ = i; refreshSelection(); applyFpsFormat();
        });
        row->stepX = 114.f;
        for (int i = 0; i < kFpsFormatCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kFpsFormatLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{110.f, 40.f}, 16));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    // UiScale
    {
        auto row = makeMultiRow(Str::LabelUiScale, [this](int i) {
            uiScale_ = kUiScales[i];
            refreshSelection();
            setUiScale(uiScale_);
            preferences_->setDouble("ui_scale", uiScale_);
        });
        for (int i = 0; i < kUiScaleCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kUiScaleLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{86.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    // Theme
    {
        auto row = makeMultiRow(Str::LabelTheme, [this](int i) {
            themeId_ = static_cast<ThemeId>(i);
            refreshSelection(); applyTheme();
        });
        row->stepX = 110.f;
        for (int i = 0; i < kThemeCount; ++i) {
            row->addButton(std::make_unique<Button>(
                themeName(static_cast<ThemeId>(i)), font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    // Language（动态数量）
    {
        auto row = makeMultiRow(Str::LabelLanguage, [this](int i) {
            languageIdx_ = i; refreshSelection(); applyLanguage();
        });
        row->stepX = 110.f;
        for (const auto& code : Lang::instance().available()) {
            std::string label = code;
            if (code == "zh")         label = "中文";
            else if (code == "zh-TW") label = "繁體中文";
            else if (code == "en")    label = "English";
            else if (code == "ja")    label = "日本語";
            else if (code == "ko")    label = "한국어";
            row->addButton(std::make_unique<Button>(
                label, font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{100.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    wallpaperButton_ = std::make_unique<Button>(Str::NextWallpaper, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{150.f, 40.f}, 18);

    // Clock 是 Toggle（位置在 [1]）
    interfaceToggles_.push_back(makeToggleRow(Str::LabelClock, [this](bool v) {
        showClock_ = v; refreshSelection();
        preferences_->setBool("show_clock", v);
    }));
    // ClockPos
    {
        auto row = makeMultiRow(Str::LabelClockPos, [this](int i) {
            clockPosition_ = i; refreshSelection();
            preferences_->setInt("clock_position", clockPosition_);
        });
        row->stepX = 86.f;
        for (int i = 0; i < kPosCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kPosLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{76.f, 40.f}, 16));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }

    consoleMaskSlider_ = std::make_unique<Slider>(
        font_, 0.f, 255.f, static_cast<float>(consoleMask_),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    consolePanelAlphaSlider_ = std::make_unique<Slider>(
        font_, 0.f, 255.f, static_cast<float>(consolePanelAlpha_),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});

    // ConsoleFont
    {
        auto row = makeMultiRow(Str::LabelConsoleFont, [this](int i) {
            consoleFontSize_ = kConsoleFonts[i]; refreshSelection();
            preferences_->setInt("console_font_size", consoleFontSize_);
        });
        for (int i = 0; i < kConsoleFontCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kConsoleFontLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{86.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    // ConsoleHistory
    {
        auto row = makeMultiRow(Str::LabelConsoleHistory, [this](int i) {
            consoleHistoryLines_ = kConsoleHistory[i]; refreshSelection();
            preferences_->setInt("console_history_lines", consoleHistoryLines_);
        });
        for (int i = 0; i < kConsoleHistoryCount; ++i) {
            row->addButton(std::make_unique<Button>(
                std::to_string(kConsoleHistory[i]), font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    // ConsoleLineHeight
    {
        auto row = makeMultiRow(Str::LabelConsoleLineHeight, [this](int i) {
            consoleLineHeight_ = kConsoleLineHeights[i]; refreshSelection();
            preferences_->setInt("console_line_height", consoleLineHeight_);
        });
        for (int i = 0; i < kConsoleLineHeightCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kConsoleLineHeightLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }

    interfaceToggles_.push_back(makeToggleRow(Str::LabelConsoleAutoScroll, [this](bool v) {
        consoleAutoScroll_ = v; refreshSelection();
        preferences_->setBool("console_auto_scroll", v);
    }));
    interfaceToggles_.push_back(makeToggleRow(Str::LabelConsoleBlink, [this](bool v) {
        consoleBlinkCursor_ = v; refreshSelection();
        preferences_->setBool("console_blink_cursor", v);
    }));

    // ConsolePrompt
    {
        auto row = makeMultiRow(Str::LabelConsolePrompt, [this](int i) {
            consolePrompt_ = i; refreshSelection(); applyConsolePrompt();
        });
        row->stepX = 70.f;
        for (int i = 0; i < kConsolePromptCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kConsolePromptLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{60.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }

    // Graphics
    // InitialLives
    {
        const char* kLivesLabels[] = {"1", "3", "5", "10", "100"};
        auto row = makeMultiRow(Str::LabelInitialLives, [this](int i) {
            const int kLives[] = {1, 3, 5, 10, 99};
            initialLives_ = kLives[i];
            refreshSelection();
            preferences_->setInt("initial_lives", initialLives_);
        });
        for (int i = 0; i < 5; ++i) {
            row->addButton(std::make_unique<Button>(
                kLivesLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 18));
        }
        graphicsMultiRows_.push_back(std::move(row));
    }

    graphicsToggles_.push_back(makeToggleRow(Str::LabelAnimation, [this](bool v) {
        animationEnabled_ = v; refreshSelection(); applyAnimation();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelPseudo3D, [this](bool v) {
        pseudo3D_ = v; refreshSelection(); applyPseudo3D();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelParallax, [this](bool v) {
        parallaxEnabled_ = v; refreshSelection(); applyParallax();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelPlayerAnimation, [this](bool v) {
        playerAnimEnabled_ = v; refreshSelection(); applyPlayerAnimation();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelLevelIntro, [this](bool v) {
        levelIntroEnabled_ = v; refreshSelection(); applyLevelIntro();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelParticles, [this](bool v) {
        particlesEnabled_ = v; refreshSelection(); applyParticles();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelScreenShake, [this](bool v) {
        screenShake_ = v; refreshSelection(); applyScreenShake();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelNotification, [this](bool v) {
        notificationEnabled_ = v; refreshSelection(); applyNotification();
    }));
    graphicsToggles_.push_back(makeToggleRow(Str::LabelShowColliders, [this](bool v) {
        showColliders_ = v; refreshSelection(); applyShowColliders();
    }));

    // AnimationSpeed
    {
        auto row = makeMultiRow(Str::LabelAnimationSpeed, [this](int i) {
            animationSpeedIndex_ = i;
            refreshSelection(); applyAnimation();
        });
        for (int i = 0; i < kanimSpeedCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kAnimSpeedLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{86.f, 40.f}, 18));
        }
        graphicsMultiRows_.push_back(std::move(row));
    }
    // NotificationPos
    {
        auto row = makeMultiRow(Str::LabelNotificationPos, [this](int i) {
            notificationPosition_ = i;
            refreshSelection(); applyNotification();
        });
        row->stepX = 86.f;
        for (int i = 0; i < kPosCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kPosLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{76.f, 40.f}, 16));
        }
        graphicsMultiRows_.push_back(std::move(row));
    }
    // ButtonCorner
    {
        auto row = makeMultiRow(Str::LabelButtonCorner, [this](int i) {
            buttonCorner_ = kButtonCorners[i];
            refreshSelection(); applyButtonStyle();
        });
        for (int i = 0; i < kButtonCornerCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kButtonCornerLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{86.f, 40.f}, 18));
        }
        graphicsMultiRows_.push_back(std::move(row));
    }
    // ButtonOutline
    {
        auto row = makeMultiRow(Str::LabelButtonOutline, [this](int i) {
            buttonOutline_ = kButtonOutlines[i];
            refreshSelection(); applyButtonStyle();
        });
        for (int i = 0; i < kButtonOutlineCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kButtonOutlineLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{86.f, 40.f}, 18));
        }
        graphicsMultiRows_.push_back(std::move(row));
    }

    // Audio
    masterVolumeSlider_ = std::make_unique<Slider>(
        font_, 0.f, 100.f, masterVolume_ * 100.f,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    audioToggles_.push_back(makeToggleRow(Str::LabelSound, [this](bool v) {
        soundEnabled_ = v; refreshSelection(); applySound();
    }));
    soundVolumeSlider_ = std::make_unique<Slider>(
        font_, 0.f, 100.f, soundVolume_ * 100.f,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    audioToggles_.push_back(makeToggleRow(Str::LabelBGM, [this](bool v) {
        bgmEnabled_ = v; refreshSelection(); applyBGM();
    }));
    bgmVolumeSlider_ = std::make_unique<Slider>(
        font_, 0.f, 100.f, bgmVolume_ * 100.f,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    audioToggles_.push_back(makeToggleRow(Str::LabelGamepad, [this](bool v) {
        gamepadEnabled_ = v; refreshSelection(); applyGamepad();
    }));

    // Other
    otherToggles_.push_back(makeToggleRow(Str::LabelRememberSize, [this](bool v) {
        rememberSize_ = v; refreshSelection();
        preferences_->setBool("remember_window_size", v);
    }));
    otherToggles_.push_back(makeToggleRow(Str::LabelAutoPause, [this](bool v) {
        autoPauseOnBlur_ = v; refreshSelection(); applyAutoPause();
    }));
    // ⭐ 2 个 Multi 用 MultiRow
    {
        auto row = makeMultiRow(Str::LabelLogRotate, [this](int i) {
            logRotateIndex_ = i; refreshSelection(); applyLogRotation();
        });
        for (int i = 0; i < kLogRotateCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kLogRotateLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
        }
        otherMultiRows_.push_back(std::move(row));
    }
    {
        auto row = makeMultiRow(Str::LabelLogKeep, [this](int i) {
            logKeepIndex_ = i; refreshSelection(); applyLogRotation();
        });
        for (int i = 0; i < kLogKeepCount; ++i) {
            row->addButton(std::make_unique<Button>(
                std::to_string(kLogKeeps[i]), font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
        }
        otherMultiRows_.push_back(std::move(row));
    }
    aboutButton_ = std::make_unique<Button>(Str::ButtonAbout, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 46.f}, 20);
    resetButton_ = std::make_unique<Button>(Str::ResetDefault, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{220.f, 46.f}, 20);
    backButton_ = std::make_unique<Button>(Str::Back, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 50.f}, 22);

    {
        ButtonStyle bs;
        bs.cornerRadius = buttonCorner_;
        bs.outlineThickness = buttonOutline_;
        setButtonStyle(bs);
    }

    // 按键绑定按钮
    for (int i = 0; i < KeyBindings::Count; ++i) {
        auto btn = std::make_unique<Button>(
            KeyBindings::keyToString(KeyBindings::instance().get(
                static_cast<KeyBindings::Action>(i))),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 40.f}, 18);
        keyBindingButtons_.push_back(std::move(btn));
    }

    refreshSelection();
}

// ============================================================
// 选中状态
// ============================================================

void SettingsScene::refreshLabels() {
    // 只在语言变化时刷新
    int v = Lang::instance().version();
    if (v == lastLangVersion_) return;
    lastLangVersion_ = v;

    auto setLabel = [](sf::Text& t, const char* key) {
        t.setString(toSf(Str::T(key)));
    };

    // ===== Heading =====
    setLabel(headingDisplay_,   Str::TabDisplay);
    setLabel(headingInterface_, Str::TabInterface);
    setLabel(headingGraphics_,  Str::TabGraphics);
    setLabel(headingAudio_,     Str::TabAudioLog);
    setLabel(headingKeys_,      Str::TabKeys);
    setLabel(headingOther_,     Str::TabOther);

    // ===== Display tab =====
    // （分辨率/抗锯齿/日志级别/FPS 上限 由 displayMultiRows_ 接管）
    setLabel(labelFullscreen_,   Str::LabelFullscreen);
    setLabel(labelVsync_,        Str::LabelVsync);

    // ===== Interface tab =====
    setLabel(labelWallpaper_,            Str::LabelWallpaper);
    setLabel(labelConsoleMask_,          Str::LabelConsoleMask);
    setLabel(labelConsolePanelAlpha_,    Str::LabelConsolePanelAlpha);

    // ===== Graphics tab =====
    for (auto& row : graphicsToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }

    // ===== Audio tab =====
    setLabel(labelMasterVolume_, Str::LabelMasterVolume);
    setLabel(labelSoundVolume_,  Str::LabelSoundVolume);
    setLabel(labelBGMVolume_,    Str::LabelBGMVolume);
    for (auto& row : audioToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }

    // ===== Other tab =====
    setLabel(labelPlayerName_,   Str::LabelPlayerName);
    setLabel(hintUiScale_,       Str::HintUiScale);

    // ===== Tab 按钮 =====
    if (tabButtons_.size() >= 6) {
        tabButtons_[0]->setText(Str::T(Str::TabDisplay));
        tabButtons_[1]->setText(Str::T(Str::TabInterface));
        tabButtons_[2]->setText(Str::T(Str::TabGraphics));
        tabButtons_[3]->setText(Str::T(Str::TabAudioLog));
        tabButtons_[4]->setText(Str::T(Str::TabKeys));
        tabButtons_[5]->setText(Str::T(Str::TabOther));
    }

    for (auto& row : displayToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : interfaceToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : otherToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }

    // ===== 其他按钮 =====
    if (wallpaperButton_) wallpaperButton_->setText(Str::T(Str::NextWallpaper));
    if (aboutButton_)     aboutButton_->setText(Str::T(Str::ButtonAbout));
    if (resetButton_)     resetButton_->setText(Str::T(Str::ResetDefault));
    if (backButton_)      backButton_->setText(Str::T(Str::Back));

    // 语言按钮保持"中文"/"English"字样（不翻译，否则用户无法识别）
    for (auto& row : displayMultiRows_)  row->refreshLabel();
    for (auto& row : graphicsMultiRows_) row->refreshLabel();
    for (auto& row : interfaceMultiRows_) row->refreshLabel();
    for (auto& row : otherMultiRows_)    row->refreshLabel();

    // ⭐ 主题按钮的文字是 themeName()，语言按钮是动态码名，都特殊处理
    if (interfaceMultiRows_.size() >= 5) {
        auto& themeRow = *interfaceMultiRows_[3];
        for (int i = 0; i < kThemeCount && i < static_cast<int>(themeRow.buttons.size()); ++i) {
            themeRow.buttons[i]->setText(Str::T(themeName(static_cast<ThemeId>(i))));
        }
    }
}

void SettingsScene::syncFocus() {
    if (resetConfirm_ || aboutDialog_) {
        FocusGroup::instance().clear();
        return;
    }

    std::vector<Button*> items;
    for (auto& b : tabButtons_) items.push_back(b.get());

    switch (currentTab_) {
        case Tab::Display:
            for (auto& row : displayMultiRows_) {
                for (auto& btn : row->buttons) items.push_back(btn.get());
            }
            for (auto& row : displayToggles_) {
                items.push_back(row->onButton.get());
                items.push_back(row->offButton.get());
            }
            break;
        case Tab::Interface:
            for (auto& row : interfaceToggles_) {
                items.push_back(row->onButton.get());
                items.push_back(row->offButton.get());
            }
            for (auto& row : interfaceMultiRows_) {
                for (auto& btn : row->buttons) items.push_back(btn.get());
            }
            items.push_back(wallpaperButton_.get());
            break;
            case Tab::Graphics:
                for (auto& row : graphicsMultiRows_) {
                    for (auto& btn : row->buttons) items.push_back(btn.get());
                }
                for (auto& row : graphicsToggles_) {
                    items.push_back(row->onButton.get());
                    items.push_back(row->offButton.get());
                }
               break;
        case Tab::Audio:
            for (auto& row : audioToggles_) {
                items.push_back(row->onButton.get());
                items.push_back(row->offButton.get());
            }
            break;
        case Tab::Keys:
            for (auto& b : keyBindingButtons_) items.push_back(b.get());
            break;
        case Tab::Other:
            for (auto& row : otherToggles_) {
                items.push_back(row->onButton.get());
                items.push_back(row->offButton.get());
            }
            for (auto& row : otherMultiRows_) {
                for (auto& btn : row->buttons) items.push_back(btn.get());
            }
            items.push_back(aboutButton_.get());
            items.push_back(resetButton_.get());
            break;
    }
    items.push_back(backButton_.get());

    FocusGroup::instance().setItems(items);
}


void SettingsScene::refreshSelection() {
    for (int i = 0; i < kTabCount; ++i)
        tabButtons_[i]->setSelected(i == static_cast<int>(currentTab_));
    if (displayMultiRows_.size() == 4) {
        displayMultiRows_[0]->setSelected(selectedResolution_);
        displayMultiRows_[1]->setSelected(indexOfAA(antiAliasingLevel_));
        displayMultiRows_[2]->setSelected(indexOfLogLevel(logLevel_));
        displayMultiRows_[3]->setSelected(indexOfFpsLimit(fpsLimit_));
    }

    if (displayToggles_.size() == 2) {
        displayToggles_[0]->currentValue = fullscreen_;
        displayToggles_[0]->onButton->setSelected(fullscreen_);
        displayToggles_[0]->offButton->setSelected(!fullscreen_);
        displayToggles_[1]->currentValue = vsync_;
        displayToggles_[1]->onButton->setSelected(vsync_);
        displayToggles_[1]->offButton->setSelected(!vsync_);
    }

    if (interfaceToggles_.size() == 4) {
        auto setRow = [](ToggleRow& row, bool v) {
            row.currentValue = v;
            row.onButton->setSelected(v);
            row.offButton->setSelected(!v);
        };
        setRow(*interfaceToggles_[0], showFps_);
        setRow(*interfaceToggles_[1], showClock_);
        setRow(*interfaceToggles_[2], consoleAutoScroll_);
        setRow(*interfaceToggles_[3], consoleBlinkCursor_);
    }
    if (interfaceMultiRows_.size() == 10) {
        interfaceMultiRows_[0]->setSelected(fpsPosition_);
        interfaceMultiRows_[1]->setSelected(fpsFormat_);
        interfaceMultiRows_[2]->setSelected(indexOfUiScale(uiScale_));
        interfaceMultiRows_[3]->setSelected(static_cast<int>(themeId_));
        interfaceMultiRows_[4]->setSelected(languageIdx_);
        interfaceMultiRows_[5]->setSelected(clockPosition_);
        interfaceMultiRows_[6]->setSelected(indexOfConsoleFont(consoleFontSize_));
        interfaceMultiRows_[7]->setSelected(indexOfConsoleHistory(consoleHistoryLines_));
        interfaceMultiRows_[8]->setSelected(indexOfConsoleLineHeight(consoleLineHeight_));
        interfaceMultiRows_[9]->setSelected(consolePrompt_);
    }

    // ⭐ 9 个 Toggle 一次性刷新
    auto setToggleRow = [](ToggleRow& row, bool v) {
        row.currentValue = v;
        row.onButton->setSelected(v);
        row.offButton->setSelected(!v);
    };
    if (graphicsToggles_.size() == 9) {
        setToggleRow(*graphicsToggles_[0], animationEnabled_);
        setToggleRow(*graphicsToggles_[1], pseudo3D_);
        setToggleRow(*graphicsToggles_[2], parallaxEnabled_);
        setToggleRow(*graphicsToggles_[3], playerAnimEnabled_);
        setToggleRow(*graphicsToggles_[4], levelIntroEnabled_);
        setToggleRow(*graphicsToggles_[5], particlesEnabled_);
        setToggleRow(*graphicsToggles_[6], screenShake_);
        setToggleRow(*graphicsToggles_[7], notificationEnabled_);
        setToggleRow(*graphicsToggles_[8], showColliders_);
    }
    if (graphicsMultiRows_.size() == 5) {
        graphicsMultiRows_[0]->setSelected(indexOfLives(initialLives_));
        graphicsMultiRows_[1]->setSelected(animationSpeedIndex_);
        graphicsMultiRows_[2]->setSelected(notificationPosition_);
        graphicsMultiRows_[3]->setSelected(indexOfButtonCorner(buttonCorner_));
        graphicsMultiRows_[4]->setSelected(indexOfButtonOutline(buttonOutline_));
    }

    if (audioToggles_.size() == 3) {
        auto setRow = [](ToggleRow& row, bool v) {
            row.currentValue = v;
            row.onButton->setSelected(v);
            row.offButton->setSelected(!v);
        };
        setRow(*audioToggles_[0], soundEnabled_);
        setRow(*audioToggles_[1], bgmEnabled_);
        setRow(*audioToggles_[2], gamepadEnabled_);
    }

    if (otherToggles_.size() == 2) {
        auto setRow = [](ToggleRow& row, bool v) {
            row.currentValue = v;
            row.onButton->setSelected(v);
            row.offButton->setSelected(!v);
        };
        setRow(*otherToggles_[0], rememberSize_);
        setRow(*otherToggles_[1], autoPauseOnBlur_);
    }
    if (otherMultiRows_.size() == 2) {
        otherMultiRows_[0]->setSelected(logRotateIndex_);
        otherMultiRows_[1]->setSelected(logKeepIndex_);
    }
}

// ============================================================
// 应用状态
// ============================================================

void SettingsScene::applyResolution() {
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setInt("resolution_index", selectedResolution_);
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(res.width));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(res.height));
}
void SettingsScene::applyFullscreen() {
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setBool("fullscreen", fullscreen_);
}
void SettingsScene::applyVsync() {
    window_->setVsync(vsync_);
    preferences_->setBool("vsync", vsync_);
}
void SettingsScene::applyAntiAliasing() {
    window_->setAntiAliasing(static_cast<unsigned>(antiAliasingLevel_));
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setInt("anti_aliasing", antiAliasingLevel_);
}
void SettingsScene::applyLogLevel() {
    logger_->setMinLevel(static_cast<LogLevel>(logLevel_));
    preferences_->setInt("log_level", logLevel_);
}
void SettingsScene::applyTheme() {
    setTheme(themeId_);
    preferences_->setInt("theme", static_cast<int>(themeId_));
}
void SettingsScene::applyLanguage() {
    const auto& avail = Lang::instance().available();
    if (languageIdx_ < 0 || languageIdx_ >= static_cast<int>(avail.size())) return;

    const std::string& code = avail[languageIdx_];
    Lang::instance().load(code);
    preferences_->set("language", code);

    // 提示用户切换场景后生效
    NotificationSystem::instance().push(
        Str::T(Str::NotifLanguageChanged), NotificationType::Info, 4.f);
}

void SettingsScene::applyWallpaper() {
    if (!background_) return;
    if (background_->next()) {
        preferences_->set("current_wallpaper", background_->currentFile());
    }
}
void SettingsScene::applyFpsPosition() { preferences_->setInt("fps_position", fpsPosition_); }
void SettingsScene::applyFpsFormat()   { preferences_->setInt("fps_format", fpsFormat_); }
void SettingsScene::applyFpsLimit() {
    window_->setFramerateLimit(static_cast<unsigned>(fpsLimit_));
    preferences_->setInt("fps_limit", fpsLimit_);
}
void SettingsScene::applyButtonStyle() {
    ButtonStyle bs;
    bs.cornerRadius     = buttonCorner_;
    bs.outlineThickness = buttonOutline_;
    setButtonStyle(bs);
    preferences_->setDouble("button_corner",  buttonCorner_);
    preferences_->setDouble("button_outline", buttonOutline_);
}
void SettingsScene::applyLogRotation() {
    logger_->setRotation(kLogRotateSizes[logRotateIndex_], kLogKeeps[logKeepIndex_]);
    preferences_->setInt("log_rotate", logRotateIndex_);
    preferences_->setInt("log_keep",   logKeepIndex_);
}
void SettingsScene::applyAnimation() {
    Anim::setEnabled(animationEnabled_);
    Anim::setSpeed(kanimSpeeds[animationSpeedIndex_]);
    preferences_->setBool("animation_enabled", animationEnabled_);
    preferences_->setInt("animation_speed_index", animationSpeedIndex_);
}
void SettingsScene::applyNotification() {
    NotificationSystem::instance().setEnabled(notificationEnabled_);
    NotificationSystem::instance().setPosition(
        static_cast<NotificationPos>(notificationPosition_));
    preferences_->setBool("notification_enabled", notificationEnabled_);
    preferences_->setInt("notification_position", notificationPosition_);
}
void SettingsScene::applySound() {
    SoundManager::instance().setEnabled(soundEnabled_);
    SoundManager::instance().setSFXVolume(soundVolume_);
    preferences_->setBool("sound_enabled", soundEnabled_);
    preferences_->setDouble("sound_volume", soundVolume_);
    if (soundEnabled_) SoundManager::instance().playCoin();
}
void SettingsScene::applyBGM() {
    SoundManager::instance().setBGMEnabled(bgmEnabled_);
    SoundManager::instance().setMusicVolume(bgmVolume_);
    preferences_->setBool("bgm_enabled", bgmEnabled_);
    preferences_->setDouble("bgm_volume", bgmVolume_);
}
void SettingsScene::applyGamepad() {
    preferences_->setBool("gamepad_enabled", gamepadEnabled_);
    FocusGroup::instance().setEnabled(gamepadEnabled_);
}
void SettingsScene::applyAutoPause()      { preferences_->setBool("auto_pause_on_blur", autoPauseOnBlur_); }
void SettingsScene::applyConsolePrompt()  { preferences_->setInt("console_prompt", consolePrompt_); }
void SettingsScene::applyShowColliders()  { preferences_->setBool("show_colliders", showColliders_); }
void SettingsScene::applyScreenShake()    { preferences_->setBool("screen_shake", screenShake_); }
void SettingsScene::applyParticles()      { preferences_->setBool("particles", particlesEnabled_); }
void SettingsScene::applyPseudo3D()       { preferences_->setBool("pseudo_3d", pseudo3D_); }
void SettingsScene::applyParallax()       { preferences_->setBool("parallax", parallaxEnabled_); }
void SettingsScene::applyPlayerAnimation(){ preferences_->setBool("player_animation", playerAnimEnabled_); }
void SettingsScene::applyLevelIntro()     { preferences_->setBool("level_intro", levelIntroEnabled_); }
void SettingsScene::resetAllPreferences() { preferences_->resetAll(); }

// ============================================================
// 事件
// ============================================================

void SettingsScene::onEnter() {
    nextScene_ = SceneId::None;
    syncFocus();
}

void SettingsScene::onResume() {
    nextScene_ = SceneId::None;
    syncFocus();
}

void SettingsScene::handleEvent(const sf::Event& event) {
    if (resetConfirm_) { resetConfirm_->handleEvent(event); return; }
    if (aboutDialog_)  { aboutDialog_->handleEvent(event);  return; }

    bool inputFocused = playerNameInput_ && playerNameInput_->isFocused();
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape && !inputFocused) {
            nextScene_ = SceneId::Back;
            return;
        }
    }
    for (auto& b : tabButtons_) b->handleEvent(event);

    if (currentTab_ == Tab::Other && playerNameInput_)
        playerNameInput_->handleEvent(event);

    switch (currentTab_) {
        case Tab::Display:
            for (auto& row : displayMultiRows_) {
                for (auto& btn : row->buttons) btn->handleEvent(event);
            }
            for (auto& row : displayToggles_) {
                row->onButton->handleEvent(event);
                row->offButton->handleEvent(event);
            }
            break;
        case Tab::Interface:
            for (auto& row : interfaceToggles_) {
                row->onButton->handleEvent(event);
                row->offButton->handleEvent(event);
            }
            for (auto& row : interfaceMultiRows_) {
                for (auto& btn : row->buttons) btn->handleEvent(event);
            }
            wallpaperButton_->handleEvent(event);
            consoleMaskSlider_->handleEvent(event);
            consolePanelAlphaSlider_->handleEvent(event);
            break;
        case Tab::Graphics:
            for (auto& row : graphicsMultiRows_) {
                for (auto& btn : row->buttons) btn->handleEvent(event);
            }
            for (auto& row : graphicsToggles_) {
                row->onButton->handleEvent(event);
                row->offButton->handleEvent(event);
            }
            break;
        case Tab::Audio:
            masterVolumeSlider_->handleEvent(event);
            for (auto& row : audioToggles_) {
                row->onButton->handleEvent(event);
                row->offButton->handleEvent(event);
            }
            soundVolumeSlider_->handleEvent(event);
            bgmVolumeSlider_->handleEvent(event);
            break;
        case Tab::Keys: {
            // 监听模式：下一个非 ESC 键被绑定
            if (listeningAction_ >= 0) {
                if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
                    if (kp->code == sf::Keyboard::Key::Escape) {
                        listeningAction_ = -1;
                    } else {
                        auto act = static_cast<KeyBindings::Action>(listeningAction_);
                        KeyBindings::instance().set(act, kp->code);

                        // 保存到 preferences
                        const char* prefKey = nullptr;
                        switch (act) {
                            case KeyBindings::MoveLeft:  prefKey = "key_left"; break;
                            case KeyBindings::MoveRight: prefKey = "key_right"; break;
                            case KeyBindings::Jump:      prefKey = "key_jump"; break;
                            case KeyBindings::Pause:     prefKey = "key_pause"; break;
                            case KeyBindings::Restart:   prefKey = "key_restart"; break;
                            default: break;
                        }
                        if (prefKey) {
                            preferences_->setInt(prefKey,
                                static_cast<int>(kp->code));
                        }
                        listeningAction_ = -1;
                        refreshSelection();
                    }
                    return;
                }
            } else {
                for (auto& b : keyBindingButtons_) b->handleEvent(event);
            }
            break;
        }
        case Tab::Other:
            for (auto& row : otherToggles_) {
                row->onButton->handleEvent(event);
                row->offButton->handleEvent(event);
            }
            for (auto& row : otherMultiRows_) {
                for (auto& btn : row->buttons) btn->handleEvent(event);
            }
            aboutButton_->handleEvent(event);
            resetButton_->handleEvent(event);
            break;
    }
    backButton_->handleEvent(event);
}

// ============================================================
// 更新
// ============================================================

void SettingsScene::update(float /*dt*/) {
    if (resetConfirm_) {
        auto r = resetConfirm_->consumeResult();
        if (r == ConfirmDialog::Result::Yes) {
            resetAllPreferences();
            nextScene_ = SceneId::Exit;
        } else if (r == ConfirmDialog::Result::No) {
            resetConfirm_.reset();
            syncFocus();
        }
        return;
    }
    if (aboutDialog_) {
        auto r = aboutDialog_->consumeResult();
        if (r == ConfirmDialog::Result::Ok || r == ConfirmDialog::Result::No) {
            aboutDialog_.reset();
            syncFocus();
        }
        return;
    }

    for (int i = 0; i < kTabCount; ++i) {
        if (tabButtons_[i]->consumeClick()) {
            if (static_cast<int>(currentTab_) != i) {
                currentTab_ = static_cast<Tab>(i);
                refreshSelection();
                syncFocus();           // ⭐ Tab 变了，按钮列表也变
            }
            return;
        }
    }

    switch (currentTab_) {
        case Tab::Display: {
            // ⭐ 4 组 Multi
            for (auto& row : displayMultiRows_) {
                for (size_t i = 0; i < row->buttons.size(); ++i) {
                    if (row->buttons[i]->consumeClick()) {
                        if (row->currentIndex != static_cast<int>(i)
                            && row->onSelected) {
                            row->onSelected(static_cast<int>(i));
                        }
                        return;
                    }
                }
            }
            // ⭐ 2 个 Toggle
            for (auto& row : displayToggles_) {
                if (row->onButton->consumeClick() && !row->currentValue) {
                    if (row->onChanged) row->onChanged(true);
                    return;
                }
                if (row->offButton->consumeClick() && row->currentValue) {
                    if (row->onChanged) row->onChanged(false);
                    return;
                }
            }
            break;
        }
        case Tab::Interface: {
            // ⭐ 4 个 Toggle
            for (auto& row : interfaceToggles_) {
                if (row->onButton->consumeClick() && !row->currentValue) {
                    if (row->onChanged) row->onChanged(true);
                    return;
                }
                if (row->offButton->consumeClick() && row->currentValue) {
                    if (row->onChanged) row->onChanged(false);
                    return;
                }
            }
            // ⭐ 10 组 Multi
            for (auto& row : interfaceMultiRows_) {
                for (size_t i = 0; i < row->buttons.size(); ++i) {
                    if (row->buttons[i]->consumeClick()) {
                        if (row->currentIndex != static_cast<int>(i)
                            && row->onSelected) {
                            row->onSelected(static_cast<int>(i));
                        }
                        return;
                    }
                }
            }
            // Wallpaper
            if (wallpaperButton_->consumeClick()) { applyWallpaper(); return; }
            // ConsoleMask / PanelAlpha sliders
            if (consoleMaskSlider_->consumeChanged()) {
                consoleMask_ = static_cast<int>(consoleMaskSlider_->value());
                preferences_->setInt("console_mask", consoleMask_);
            }
            if (consolePanelAlphaSlider_->consumeChanged()) {
                consolePanelAlpha_ = static_cast<int>(consolePanelAlphaSlider_->value());
                preferences_->setInt("console_panel_alpha", consolePanelAlpha_);
            }
            break;
        }
        case Tab::Graphics: {
            // ⭐ 5 组 Multi
            for (auto& row : graphicsMultiRows_) {
                for (size_t i = 0; i < row->buttons.size(); ++i) {
                    if (row->buttons[i]->consumeClick()) {
                        if (row->currentIndex != static_cast<int>(i)
                            && row->onSelected) {
                            row->onSelected(static_cast<int>(i));
                        }
                        return;
                    }
                }
            }
            // ⭐ 9 个 Toggle
            for (auto& row : graphicsToggles_) {
                if (row->onButton->consumeClick() && !row->currentValue) {
                    if (row->onChanged) row->onChanged(true);
                    return;
                }
                if (row->offButton->consumeClick() && row->currentValue) {
                    if (row->onChanged) row->onChanged(false);
                    return;
                }
            }
            break;
        }
        case Tab::Audio: {
            if (masterVolumeSlider_->consumeChanged()) {
                masterVolume_ = masterVolumeSlider_->value() / 100.f;
                SoundManager::instance().setMasterVolume(masterVolume_);
                preferences_->setDouble("master_volume", masterVolume_);
            }
            for (auto& row : audioToggles_) {
                if (row->onButton->consumeClick() && !row->currentValue) {
                    if (row->onChanged) row->onChanged(true);
                    return;
                }
                if (row->offButton->consumeClick() && row->currentValue) {
                    if (row->onChanged) row->onChanged(false);
                    return;
                }
            }
            if (soundVolumeSlider_->consumeChanged()) {
                soundVolume_ = soundVolumeSlider_->value() / 100.f;
                SoundManager::instance().setSFXVolume(soundVolume_);
                preferences_->setDouble("sound_volume", soundVolume_);
            }
            if (bgmVolumeSlider_->consumeChanged()) {
                bgmVolume_ = bgmVolumeSlider_->value() / 100.f;
                SoundManager::instance().setMusicVolume(bgmVolume_);
                preferences_->setDouble("bgm_volume", bgmVolume_);
            }
            break;
        }
        case Tab::Keys: {
            if (listeningAction_ >= 0) break;   // 监听模式下不响应点击
            for (int i = 0; i < static_cast<int>(keyBindingButtons_.size()); ++i) {
                if (keyBindingButtons_[i]->consumeClick()) {
                    listeningAction_ = i;
                    keyBindingButtons_[i]->setText(Str::T(Str::KeyPressNew));
                    return;
                }
            }
            break;
        }
        case Tab::Other: {
            for (auto& row : otherToggles_) {
                if (row->onButton->consumeClick() && !row->currentValue) {
                    if (row->onChanged) row->onChanged(true);
                    return;
                }
                if (row->offButton->consumeClick() && row->currentValue) {
                    if (row->onChanged) row->onChanged(false);
                    return;
                }
            }
            for (auto& row : otherMultiRows_) {
                for (size_t i = 0; i < row->buttons.size(); ++i) {
                    if (row->buttons[i]->consumeClick()) {
                        if (row->currentIndex != static_cast<int>(i)
                            && row->onSelected) {
                            row->onSelected(static_cast<int>(i));
                        }
                        return;
                    }
                }
            }
            if (aboutButton_->consumeClick()) {
                std::string msg =
                    std::string(Str::T(Str::AboutTitle)) + "\n"
                    + Str::T(Str::AboutVersion) + PROJECT_VERSION + "\n"
                    + Str::T(Str::AboutBuild)   + BUILD_DATE + "\n"
                    + Str::T(Str::AboutAuthor)  + "ljm-233";
                aboutDialog_ = std::make_unique<ConfirmDialog>(
                    font_, msg, sf::Vector2f(1440.f, 900.f),
                    ConfirmDialog::Mode::Info);
                syncFocus();
                return;
            }
            if (resetButton_->consumeClick()) {
                resetConfirm_ = std::make_unique<ConfirmDialog>(
                    font_, Str::T(Str::ResetConfirm),
                    sf::Vector2f(1720.f, 720.f));
                syncFocus();
                return;
            }
            break;
        }
    }

    if (backButton_->consumeClick()) nextScene_ = SceneId::Back;
}

// ============================================================
// 渲染
// ============================================================

void SettingsScene::renderTabs(Window& window) {
    // ⭐ 根据缩放调整 Tab 按钮的位置
    float tabY   = 90.f  * layoutScale_;
    float tabGap = 62.f  * layoutScale_;
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i]->setPosition({40.f, tabY + i * tabGap});
        tabButtons_[i]->render(window.native());
    }
}

namespace {
struct RowDrawer {
    sf::RenderTarget& target;
    float contentX;
    float ctrlX;
    float y;
    float rowH = 50.f;              // ⭐ 动态行高
    float scrollOffset = 0.f;       // ⭐ 内容区滚动偏移（负值向上）

    void toggle(ToggleRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);
        row.onButton->setPosition ({ctrlX, y});
        row.offButton->setPosition({ctrlX + 96.f, y});
        row.onButton->render(target);
        row.offButton->render(target);
        y += rowH;
    }

    // 兼容旧接口（其他 Tab 还在用）
    void toggle(sf::Text& label,
                const std::unique_ptr<Button>& on,
                const std::unique_ptr<Button>& off) {
        label.setPosition({contentX, y + 8.f});
        target.draw(label);
        on->setPosition ({ctrlX, y});
        off->setPosition({ctrlX + 96.f, y});
        on->render(target);
        off->render(target);
        y += rowH;
    }

    void multi(sf::Text& label,
               std::vector<std::unique_ptr<Button>>& btns,
               float gap = 96.f) {
        label.setPosition({contentX, y + 8.f});
        target.draw(label);
        for (size_t i = 0; i < btns.size(); ++i) {
            btns[i]->setPosition({ctrlX + static_cast<float>(i) * gap, y});
            btns[i]->render(target);
        }
        y += rowH;
    }

    // ⭐ MultiRow 版本（单行 + 网格）
    void multi(MultiRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);

        if (row.columns <= 0) {
            // 单行
            for (size_t i = 0; i < row.buttons.size(); ++i) {
                row.buttons[i]->setPosition(
                    {ctrlX + static_cast<float>(i) * row.stepX, y});
                row.buttons[i]->render(target);
            }
            y += rowH;
        } else {
            // 多行网格
            const int cols = row.columns;
            float stepY = rowH;   // ⭐ 用动态行高
            for (size_t i = 0; i < row.buttons.size(); ++i) {
                int r = static_cast<int>(i) / cols;
                int c = static_cast<int>(i) % cols;
                row.buttons[i]->setPosition(
                    {ctrlX + static_cast<float>(c) * row.stepX,
                     y + static_cast<float>(r) * stepY});
                row.buttons[i]->render(target);
            }
            int rows = (static_cast<int>(row.buttons.size()) + cols - 1) / cols;
            y += static_cast<float>(rows) * stepY + 6.f;
        }
    }

    void slider(sf::Text& label, Slider* s) {
        label.setPosition({contentX, y + 4.f});
        target.draw(label);
        s->setPosition({ctrlX, y + 4.f});
        s->render(target);
        y += rowH;
    }
};
}

void SettingsScene::renderDisplayTab(Window& window, float contentX,
                                     float ctrlX, float y) {
    headingDisplay_.setPosition({contentX, y});
    window.native().draw(headingDisplay_);
    y += 36.f;

    RowDrawer r{window.native(), contentX, ctrlX, y};
    r.rowH = 50.f * layoutScale_;   // ⭐ 动态行高
    r.multi (*displayMultiRows_[0]);   // Resolution (2×2 网格)
    r.toggle(*displayToggles_[0]);     // Fullscreen
    r.toggle(*displayToggles_[1]);     // VSync
    r.multi (*displayMultiRows_[1]);   // AntiAliasing
    r.multi (*displayMultiRows_[2]);   // LogLevel
    r.multi (*displayMultiRows_[3]);   // FpsLimit
}

void SettingsScene::renderInterfaceTab(Window& window, float contentX,
                                       float ctrlX, float y) {
    headingInterface_.setPosition({contentX, y});
    window.native().draw(headingInterface_);
    y += 36.f;

    RowDrawer r{window.native(), contentX, ctrlX, y};
    r.rowH = 50.f * layoutScale_;

    r.toggle(*interfaceToggles_[0]);   // FPS 显示
    r.multi (*interfaceMultiRows_[0]); // FpsPos
    r.multi (*interfaceMultiRows_[1]); // FpsFormat
    r.multi (*interfaceMultiRows_[2]); // UiScale

    hintUiScale_.setPosition({contentX, r.y - 26.f});
    window.native().draw(hintUiScale_);

    r.multi (*interfaceMultiRows_[3]); // Theme
    r.multi (*interfaceMultiRows_[4]); // Language
    if (background_) {
        labelWallpaper_.setString(toSf(
            std::string(Str::LabelWallpaper) + "  ("
            + std::to_string(background_->currentIndex() + 1) + "/"
            + std::to_string(background_->totalWallpapers()) + ")"));
    }
    labelWallpaper_.setPosition({contentX, r.y + 8.f});
    window.native().draw(labelWallpaper_);
    wallpaperButton_->setPosition({ctrlX, r.y});
    wallpaperButton_->render(window.native());
    r.y += 50.f;

    r.toggle(*interfaceToggles_[1]);   // 时钟显示
    r.multi (*interfaceMultiRows_[5]); // ClockPos

    r.slider(labelConsoleMask_,       consoleMaskSlider_.get());
    r.slider(labelConsolePanelAlpha_, consolePanelAlphaSlider_.get());
    r.multi (*interfaceMultiRows_[6]); // ConsoleFont
    r.multi (*interfaceMultiRows_[7]); // ConsoleHistory
    r.multi (*interfaceMultiRows_[8]); // ConsoleLineHeight
    r.toggle(*interfaceToggles_[2]);   // 控制台自动滚动
    r.toggle(*interfaceToggles_[3]);   // 控制台光标闪烁
    r.multi (*interfaceMultiRows_[9]); // ConsolePrompt
}

void SettingsScene::renderGraphicsTab(Window& window, float contentX,
                                      float ctrlX, float y) {
    headingGraphics_.setPosition({contentX, y});
    window.native().draw(headingGraphics_);
    y += 36.f;

    RowDrawer r{window.native(), contentX, ctrlX, y};
    r.rowH = 50.f * layoutScale_;

    // ⭐ 顺序必须和构造里 push_back 一致
    r.multi (*graphicsMultiRows_[0]);   // InitialLives
    r.toggle(*graphicsToggles_[0]);     // Animation
    r.multi (*graphicsMultiRows_[1]);   // AnimationSpeed
    r.toggle(*graphicsToggles_[1]);     // Pseudo3D
    r.toggle(*graphicsToggles_[2]);     // Parallax
    r.toggle(*graphicsToggles_[3]);     // PlayerAnim
    r.toggle(*graphicsToggles_[4]);     // LevelIntro
    r.toggle(*graphicsToggles_[5]);     // Particles
    r.toggle(*graphicsToggles_[6]);     // ScreenShake
    r.toggle(*graphicsToggles_[7]);     // Notification
    r.multi (*graphicsMultiRows_[2]);   // NotificationPos
    r.multi (*graphicsMultiRows_[3]);   // ButtonCorner
    r.multi (*graphicsMultiRows_[4]);   // ButtonOutline
    r.toggle(*graphicsToggles_[8]);     // ShowColliders
}

void SettingsScene::renderAudioTab(Window& window, float contentX,
                                   float ctrlX, float y) {
    headingAudio_.setPosition({contentX, y});
    window.native().draw(headingAudio_);
    y += 36.f;

    RowDrawer r{window.native(), contentX, ctrlX, y};
    r.rowH = 50.f * layoutScale_;

    r.slider(labelMasterVolume_, masterVolumeSlider_.get());
    r.toggle(*audioToggles_[0]);   // Sound
    r.slider(labelSoundVolume_,  soundVolumeSlider_.get());
    r.toggle(*audioToggles_[1]);   // BGM
    r.slider(labelBGMVolume_,    bgmVolumeSlider_.get());
    r.toggle(*audioToggles_[2]);   // Gamepad
}

void SettingsScene::renderKeysTab(Window& window, float contentX,
                                  float ctrlX, float y) {
    headingKeys_.setPosition({contentX, y});
    window.native().draw(headingKeys_);
    y += 36.f;

    for (int i = 0; i < KeyBindings::Count; ++i) {
        auto act = static_cast<KeyBindings::Action>(i);

        // 动作名
        sf::Text label(font_, toSf(Str::T(KeyBindings::actionName(act))),
                       scaledFontSize(20));
        label.setFillColor(sf::Color(230, 230, 230));
        label.setPosition({contentX, y + 8.f});
        window.native().draw(label);

        // 当前键名
        if (i != listeningAction_) {
            keyBindingButtons_[i]->setText(
                KeyBindings::keyToString(KeyBindings::instance().get(act)));
        }
        keyBindingButtons_[i]->setPosition({ctrlX, y});
        keyBindingButtons_[i]->render(window.native());

        y += 50.f;
    }

    sf::Text hint(font_, toSf(Str::T(Str::KeyBindHint)),
                  scaledFontSize(14));
    hint.setFillColor(sf::Color(180, 180, 200));
    hint.setPosition({contentX, y + 8.f});
    window.native().draw(hint);
}

void SettingsScene::renderOtherTab(Window& window, float contentX,
                                   float ctrlX, float y) {
    headingOther_.setPosition({contentX, y});
    window.native().draw(headingOther_);
    y += 36.f;

    RowDrawer r{window.native(), contentX, ctrlX, y};
    r.rowH = 50.f * layoutScale_;

    r.toggle(*otherToggles_[0]);   // RememberSize
    r.toggle(*otherToggles_[1]);   // AutoPause
    r.multi (*otherMultiRows_[0]);   // LogRotate
    r.multi (*otherMultiRows_[1]);   // LogKeep

    labelPlayerName_.setPosition({contentX, r.y + 8.f});
    window.native().draw(labelPlayerName_);
    playerNameInput_->setPosition({ctrlX, r.y});
    playerNameInput_->setSize({240.f, 40.f});
    playerNameInput_->render(window.native());
    r.y += 50.f;

    r.y += 10.f;
    aboutButton_->setPosition({contentX, r.y});
    aboutButton_->render(window.native());

    resetButton_->setPosition({contentX + 220.f, r.y});
    resetButton_->render(window.native());
}

void SettingsScene::renderBackButton(Window& window) {
    auto size = window.native().getSize();
    backButton_->setPosition({static_cast<float>(size.x) - 200.f,
                              static_cast<float>(size.y) - 70.f});
    backButton_->render(window.native());
}

void SettingsScene::render(Window& window) {
    refreshLabels();

    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w  = static_cast<float>(size.x);
    float h  = static_cast<float>(size.y);

    // ⭐ 根据窗口高度更新缩放
    float newScale = std::clamp(h / 720.f, 0.65f, 1.0f);
    if (std::abs(newScale - layoutScale_) > 0.01f || h != lastWinH_) {
        layoutScale_ = newScale;
        lastWinH_ = h;
        contentScroll_ = 0.f;   // 尺寸变了重置滚动
    }

    renderTabs(window);

    switch (currentTab_) {
        case Tab::Display:
            renderDisplayTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Interface:
            renderInterfaceTab(window, kContentX, kCtrlX, 50.f);
            break;
        case Tab::Graphics:
            renderGraphicsTab(window, kContentX, kCtrlX, 50.f);
            break;
        case Tab::Audio:
            renderAudioTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Keys:
            renderKeysTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Other:
            renderOtherTab(window, kContentX, kCtrlX, 60.f);
            break;
    }

    renderBackButton(window);

    if (resetConfirm_) {
        resetConfirm_->relayout({w, h});
        resetConfirm_->render(window.native());
    }
    if (aboutDialog_) {
        aboutDialog_->relayout({w, h});
        aboutDialog_->render(window.native());
    }
}