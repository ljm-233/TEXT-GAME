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

const float kUiScales[]     = {0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f};
constexpr int kUiScaleCount = 10;
const char* kUiScaleLabels[] = {"0.5x", "0.6x", "0.7x", "0.8x", "0.9x",
                                "1.0x", "1.1x", "1.25x", "1.5x", "2.0x"};

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

// ===== 布局（设计坐标系 1280×720）=====
constexpr float kTabX     = 40.f;
constexpr float kTabY     = 90.f;
constexpr float kTabGap   = 62.f;
constexpr float kContentX = kTabX + 200.f;
constexpr float kCtrlX    = kContentX + 240.f;
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
    return 5;
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

// ⭐ 初始生命值（1/3/5/10/99）↔ 索引
int indexOfLives(int lives) {
    const int kLives[] = {1, 3, 5, 10, 99};
    for (int i = 0; i < 5; ++i)
        if (kLives[i] == lives) return i;
    return 0;
}
} // namespace

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
      headingDisplay_  (font, toSf(Str::TabDisplay),   fontSizeInView(24)),
      headingInterface_(font, toSf(Str::TabInterface), fontSizeInView(24)),
      headingGraphics_ (font, toSf(Str::TabGraphics),  fontSizeInView(24)),
      headingAudio_    (font, toSf(Str::TabAudioLog),  fontSizeInView(24)),
      headingKeys_     (font, toSf(Str::TabKeys),      fontSizeInView(24)),
      headingOther_    (font, toSf(Str::TabOther),     fontSizeInView(24)),

      labelWallpaper_      (font, toSf(Str::LabelWallpaper),      fontSizeInView(20)),
      labelConsoleMask_    (font, toSf(Str::LabelConsoleMask),    fontSizeInView(20)),
      labelConsolePanelAlpha_(font, toSf(Str::LabelConsolePanelAlpha), fontSizeInView(20)),
      labelMasterVolume_   (font, toSf(Str::LabelMasterVolume),   fontSizeInView(20)),
      labelSoundVolume_    (font, toSf(Str::LabelSoundVolume),    fontSizeInView(20)),
      labelBGMVolume_      (font, toSf(Str::LabelBGMVolume),      fontSizeInView(20)),
      labelPlayerName_     (font, toSf(Str::LabelPlayerName),     fontSizeInView(20)),
      hintUiScale_         (font, toSf(Str::HintUiScale),         fontSizeInView(14)) {

    selectedResolution_ = clampResolutionIndex(preferences_->getInt("resolution_index", 0));
    fullscreen_          = preferences_->getBool("fullscreen", false);
    windowMode_          = preferences_->getInt("window_mode", 0);
    vsync_               = preferences_->getBool("vsync", true);
    antiAliasingLevel_   = preferences_->getInt("anti_aliasing", 8);
    logLevel_            = preferences_->getInt("log_level", static_cast<int>(LogLevel::Info));
    fpsLimit_            = preferences_->getInt("fps_limit", 60);

    showFps_             = preferences_->getBool("show_fps", false);
    fpsPosition_         = preferences_->getInt("fps_position", 1);
    fpsFormat_           = indexOfFpsFormat(preferences_->getInt("fps_format", 1));
    uiScale_             = static_cast<float>(preferences_->getDouble("ui_scale", 1.0));
    fontScale_           = static_cast<float>(preferences_->getDouble("font_scale", 1.0));
    renderScale_         = static_cast<float>(preferences_->getDouble("render_scale", 1.0));
    upscaleMode_         = preferences_->getInt("upscale_mode", 1);
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
        && initialLives_ != 10 && initialLives_ != 99) {
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
    autoPauseOnBlur_     = preferences_->getBool("auto_pause_on_blue", true);
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

    // 标题颜色
    auto headingColor = sf::Color(160, 200, 240);
    headingDisplay_.setFillColor(headingColor);
    headingInterface_.setFillColor(headingColor);
    headingGraphics_.setFillColor(headingColor);
    headingAudio_.setFillColor(headingColor);
    headingKeys_.setFillColor(headingColor);
    headingOther_.setFillColor(headingColor);

    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelWallpaper_,
                    &labelConsoleMask_, &labelConsolePanelAlpha_,
                    &labelMasterVolume_, &labelSoundVolume_, &labelBGMVolume_,
                    &labelPlayerName_}) {
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
        row->onButton  = std::move(on);
        row->offButton = std::move(off);
        return row;
    };

    auto makeMultiRow = [&](const char* labelKey,
                            std::function<void(int)> onSelected) {
        return std::make_unique<MultiRow>(font_, labelKey, std::move(onSelected));
    };

    // ───────────── Display ─────────────
    // Resolution: 2×2 网格
    {
        auto row = makeMultiRow(Str::LabelResolution, [this](int i) {
            selectedResolution_ = i; refreshSelection(); applyResolution();
        });
        row->columns = 2;
        row->stepX   = kBtnW + kGapX;
        row->stepY   = kBtnH + kGapY;
        for (int i = 0; i < kResolutionCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kResolutions[i].label, font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{kBtnW, kBtnH}, 18));
        }
        displayMultiRows_.push_back(std::move(row));
    }
    // WindowMode
    {
        auto row = makeMultiRow(Str::LabelWindowMode, [this](int i) {
            windowMode_ = i;
            preferences_->setInt("window_mode", i);
            applyWindowMode();
        });
        row->stepX = 110.f;
        row->addButton(std::make_unique<Button>(Str::T(Str::WinModeWindowed),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{100.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::WinModeMaximized),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{100.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::WinModeFullscreen),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{100.f,40.f}, 18));
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
    // 2 个 Toggle
    displayToggles_.push_back(makeToggleRow(Str::LabelFullscreen, [this](bool v) {
        fullscreen_ = v; refreshSelection(); applyFullscreen();
    }));
    displayToggles_.push_back(makeToggleRow(Str::LabelVsync, [this](bool v) {
        vsync_ = v; refreshSelection(); applyVsync();
    }));

    // ───────────── Interface ─────────────
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
    // FontScale
    {
        auto row = makeMultiRow(Str::LabelFontScale, [this](int i) {
            fontScale_ = kUiScales[i];
            refreshSelection();
            setFontScale(fontScale_);
            preferences_->setDouble("font_scale", fontScale_);
        });
        for (int i = 0; i < kUiScaleCount; ++i) {
            row->addButton(std::make_unique<Button>(
                kUiScaleLabels[i], font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{86.f, 40.f}, 18));
        }
        interfaceMultiRows_.push_back(std::move(row));
    }
    // RenderScale
    {
        auto row = makeMultiRow(Str::LabelRenderScale, [this](int i) {
            static const float kScales[] = {
                1.0f, 0.75f, 0.5f, 1.0f/3.0f, 0.25f, 0.10f
            };
            renderScale_ = kScales[i];
            refreshSelection();
            window_->setRenderScale(renderScale_);
            preferences_->setDouble("render_scale", renderScale_);
        });
        row->stepX = 96.f;
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale100),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale75),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale50),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale33),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale25),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale10),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        interfaceMultiRows_.push_back(std::move(row));
    }
    // UpscaleMode
    {
        auto row = makeMultiRow(Str::LabelUpscaleMode, [this](int i) {
            upscaleMode_ = i;
            refreshSelection();
            window_->setUpscaleMode(upscaleMode_);
            preferences_->setInt("upscale_mode", upscaleMode_);
        });
        row->stepX = 100.f;
        row->addButton(std::make_unique<Button>(Str::T(Str::UpscaleOff),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::UpscaleBicubic),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::UpscaleFsr1),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
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
    // Language（动态）
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

    // Clock Toggle
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

    // ───────────── Graphics ─────────────
    // InitialLives
    {
        const char* kLivesLabels[] = {"1", "3", "5", "10", "99"};
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

    // 9 个 Toggle
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

    // ───────────── Audio ─────────────
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

    // ───────────── Other ─────────────
    otherToggles_.push_back(makeToggleRow(Str::LabelRememberSize, [this](bool v) {
        rememberSize_ = v; refreshSelection();
        preferences_->setBool("remember_window_size", v);
    }));
    otherToggles_.push_back(makeToggleRow(Str::LabelAutoPause, [this](bool v) {
        autoPauseOnBlur_ = v; refreshSelection(); applyAutoPause();
    }));
    // LogRotate
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
    // LogKeep
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

    // Keys Tab
    for (int i = 0; i < KeyBindings::Count; ++i) {
        auto btn = std::make_unique<Button>(
            KeyBindings::keyToString(KeyBindings::instance().get(
                static_cast<KeyBindings::Action>(i))),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 40.f}, 18);
        keyBindingButtons_.push_back(std::move(btn));
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

    refreshSelection();
    updateDesignView();
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

    // ===== 静态标签 =====
    setLabel(labelWallpaper_,        Str::LabelWallpaper);
    setLabel(labelConsoleMask_,      Str::LabelConsoleMask);
    setLabel(labelConsolePanelAlpha_, Str::LabelConsolePanelAlpha);
    setLabel(labelMasterVolume_,     Str::LabelMasterVolume);
    setLabel(labelSoundVolume_,      Str::LabelSoundVolume);
    setLabel(labelBGMVolume_,        Str::LabelBGMVolume);
    setLabel(labelPlayerName_,       Str::LabelPlayerName);
    setLabel(hintUiScale_,           Str::HintUiScale);

    // ===== Tab 按钮 =====
    if (tabButtons_.size() >= 6) {
        tabButtons_[0]->setText(Str::T(Str::TabDisplay));
        tabButtons_[1]->setText(Str::T(Str::TabInterface));
        tabButtons_[2]->setText(Str::T(Str::TabGraphics));
        tabButtons_[3]->setText(Str::T(Str::TabAudioLog));
        tabButtons_[4]->setText(Str::T(Str::TabKeys));
        tabButtons_[5]->setText(Str::T(Str::TabOther));
    }

    // ===== Toggle / Multi 刷新 =====
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
    for (auto& row : graphicsToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : audioToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : otherToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : displayMultiRows_)  row->refreshLabel();
    for (auto& row : interfaceMultiRows_) row->refreshLabel();
    for (auto& row : graphicsMultiRows_) row->refreshLabel();
    for (auto& row : otherMultiRows_)    row->refreshLabel();

    // 主题按钮文字特殊处理
    // 索引：0 FpsPos  1 FpsFormat  2 UiScale  3 FontScale  4 Theme
    if (interfaceMultiRows_.size() >= 5) {
        auto& themeRow = *interfaceMultiRows_[6];   // ⭐ RenderScale 加进来后挪到 5
        for (int i = 0; i < kThemeCount && i < static_cast<int>(themeRow.buttons.size()); ++i) {
            themeRow.buttons[i]->setText(Str::T(themeName(static_cast<ThemeId>(i))));
        }
    }

    // ===== 其他按钮 =====
    if (wallpaperButton_) wallpaperButton_->setText(Str::T(Str::NextWallpaper));
    if (aboutButton_)     aboutButton_->setText(Str::T(Str::ButtonAbout));
    if (resetButton_)     resetButton_->setText(Str::T(Str::ResetDefault));
    if (backButton_)      backButton_->setText(Str::T(Str::Back));
}

void SettingsScene::refreshSelection() {
    for (int i = 0; i < kTabCount; ++i)
        tabButtons_[i]->setSelected(i == static_cast<int>(currentTab_));

    auto setToggleRow = [](ToggleRow& row, bool v) {
        row.currentValue = v;
        row.onButton->setSelected(v);
        row.offButton->setSelected(!v);
    };

    // Display
    if (displayMultiRows_.size() == 5) {
        displayMultiRows_[0]->setSelected(selectedResolution_);
        displayMultiRows_[1]->setSelected(windowMode_);
        displayMultiRows_[2]->setSelected(indexOfAA(antiAliasingLevel_));
        displayMultiRows_[3]->setSelected(indexOfLogLevel(logLevel_));
        displayMultiRows_[4]->setSelected(indexOfFpsLimit(fpsLimit_));
    }
    if (displayToggles_.size() == 2) {
        setToggleRow(*displayToggles_[0], fullscreen_);
        setToggleRow(*displayToggles_[1], vsync_);
    }

    // Interface
    if (interfaceToggles_.size() == 4) {
        setToggleRow(*interfaceToggles_[0], showFps_);
        setToggleRow(*interfaceToggles_[1], showClock_);
        setToggleRow(*interfaceToggles_[2], consoleAutoScroll_);
        setToggleRow(*interfaceToggles_[3], consoleBlinkCursor_);
    }
    if (interfaceMultiRows_.size() == 13) {
        interfaceMultiRows_[0]->setSelected(fpsPosition_);
        interfaceMultiRows_[1]->setSelected(fpsFormat_);
        interfaceMultiRows_[2]->setSelected(indexOfUiScale(uiScale_));
        interfaceMultiRows_[3]->setSelected(indexOfUiScale(fontScale_));
        // renderScale: 1.0→0, 0.75→1, 0.5→2, 1/3→3, 0.25→4, 0.10→5
        {
            static const float kScales[] = {
                1.0f, 0.75f, 0.5f, 1.0f/3.0f, 0.25f, 0.10f
            };
            int idx = 0;
            for (int i = 0; i < 6; ++i)
                if (std::abs(kScales[i] - renderScale_) < 0.01f) idx = i;
            interfaceMultiRows_[4]->setSelected(idx);
        }
        interfaceMultiRows_[5]->setSelected(upscaleMode_);   // ⭐ 新增
        interfaceMultiRows_[6]->setSelected(static_cast<int>(themeId_));
        interfaceMultiRows_[7]->setSelected(languageIdx_);
        interfaceMultiRows_[8]->setSelected(clockPosition_);
        interfaceMultiRows_[9]->setSelected(indexOfConsoleFont(consoleFontSize_));
        interfaceMultiRows_[10]->setSelected(indexOfConsoleHistory(consoleHistoryLines_));
        interfaceMultiRows_[11]->setSelected(indexOfConsoleLineHeight(consoleLineHeight_));
        interfaceMultiRows_[12]->setSelected(consolePrompt_);
    }

    // Graphics
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

    // Audio
    if (audioToggles_.size() == 3) {
        setToggleRow(*audioToggles_[0], soundEnabled_);
        setToggleRow(*audioToggles_[1], bgmEnabled_);
        setToggleRow(*audioToggles_[2], gamepadEnabled_);
    }

    // Other
    if (otherToggles_.size() == 2) {
        setToggleRow(*otherToggles_[0], rememberSize_);
        setToggleRow(*otherToggles_[1], autoPauseOnBlur_);
    }
    if (otherMultiRows_.size() == 2) {
        otherMultiRows_[0]->setSelected(logRotateIndex_);
        otherMultiRows_[1]->setSelected(logKeepIndex_);
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
            for (auto& row : displayMultiRows_)
                for (auto& btn : row->buttons) items.push_back(btn.get());
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
            for (auto& row : interfaceMultiRows_)
                for (auto& btn : row->buttons) items.push_back(btn.get());
            items.push_back(wallpaperButton_.get());
            break;
        case Tab::Graphics:
            for (auto& row : graphicsMultiRows_)
                for (auto& btn : row->buttons) items.push_back(btn.get());
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
            for (auto& row : otherMultiRows_)
                for (auto& btn : row->buttons) items.push_back(btn.get());
            // 关于/恢复默认在屏幕坐标系，不参与焦点导航
            break;
    }
    // ⭐ 返回按钮在窗口坐标系固定右下角，不参与设计坐标系几何导航
    //    用 ESC 或手柄 B 键返回
    FocusGroup::instance().setItems(items);
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
void SettingsScene::applyWindowMode() {
    if (windowMode_ == 1 && !fullscreen_) {
        window_->requestMaximize();
    } else if (windowMode_ == 0) {
        // 恢复到正常窗口（可能需要重建窗口，这里简化处理）
        // 用户想切回"窗口"模式可手动拖
    }
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
    // ⭐ 鼠标事件坐标转换：屏幕像素 → 设计坐标
    updateDesignView();

    sf::Event ev = event;
    auto& native = window_->native();

    auto convert = [&](sf::Vector2i pixel) -> sf::Vector2i {
        auto p = native.mapPixelToCoords(pixel, designView_);
        return {static_cast<int>(p.x), static_cast<int>(p.y)};
    };

    if (auto* mm = ev.getIf<sf::Event::MouseMoved>()) {
        mm->position = convert(mm->position);
    } else if (auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
        mb->position = convert(mb->position);
    } else if (auto* mr = ev.getIf<sf::Event::MouseButtonReleased>()) {
        mr->position = convert(mr->position);
    } else if (auto* ws = ev.getIf<sf::Event::MouseWheelScrolled>()) {
        ws->position = convert(ws->position);

        // ⭐ 内容高 > View 高时允许滚动
        float uiS = getUiScale();
        float viewH = kDesignH / uiS;
        constexpr float kBottomReserve = 60.f;
        float maxScroll = std::max(0.f, contentTotalH_ - viewH + kBottomReserve);
        if (maxScroll > 1.f) {
            contentScroll_ -= ws->delta * 40.f;
            contentScroll_ = std::clamp(contentScroll_, 0.f, maxScroll);
        }
        return;   // 不往下传
    }

    // ⭐ 底部一行按钮在窗口坐标系（不缩放），用原始 event
    backButton_->handleEvent(event);
    if (currentTab_ == Tab::Other) {
        aboutButton_->handleEvent(event);
        resetButton_->handleEvent(event);
    }

    if (resetConfirm_) { resetConfirm_->handleEvent(ev); return; }
    if (aboutDialog_)  { aboutDialog_->handleEvent(ev);  return; }

    bool inputFocused = playerNameInput_ && playerNameInput_->isFocused();
    if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape && !inputFocused) {
            nextScene_ = SceneId::Back;
            return;
        }
    }
    for (auto& b : tabButtons_) b->handleEvent(ev);

    if (currentTab_ == Tab::Other && playerNameInput_)
        playerNameInput_->handleEvent(ev);

    switch (currentTab_) {
        case Tab::Display:
            for (auto& row : displayMultiRows_)
                for (auto& btn : row->buttons) btn->handleEvent(ev);
            for (auto& row : displayToggles_) {
                row->onButton->handleEvent(ev);
                row->offButton->handleEvent(ev);
            }
            break;
        case Tab::Interface:
            for (auto& row : interfaceToggles_) {
                row->onButton->handleEvent(ev);
                row->offButton->handleEvent(ev);
            }
            for (auto& row : interfaceMultiRows_)
                for (auto& btn : row->buttons) btn->handleEvent(ev);
            wallpaperButton_->handleEvent(ev);
            consoleMaskSlider_->handleEvent(ev);
            consolePanelAlphaSlider_->handleEvent(ev);
            break;
        case Tab::Graphics:
            for (auto& row : graphicsMultiRows_)
                for (auto& btn : row->buttons) btn->handleEvent(ev);
            for (auto& row : graphicsToggles_) {
                row->onButton->handleEvent(ev);
                row->offButton->handleEvent(ev);
            }
            break;
        case Tab::Audio:
            masterVolumeSlider_->handleEvent(ev);
            for (auto& row : audioToggles_) {
                row->onButton->handleEvent(ev);
                row->offButton->handleEvent(ev);
            }
            soundVolumeSlider_->handleEvent(ev);
            bgmVolumeSlider_->handleEvent(ev);
            break;
        case Tab::Keys: {
            if (listeningAction_ >= 0) {
                if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
                    if (kp->code == sf::Keyboard::Key::Escape) {
                        listeningAction_ = -1;
                    } else {
                        auto act = static_cast<KeyBindings::Action>(listeningAction_);
                        KeyBindings::instance().set(act, kp->code);

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
                for (auto& b : keyBindingButtons_) b->handleEvent(ev);
            }
            break;
        }
        case Tab::Other:
            for (auto& row : otherToggles_) {
                row->onButton->handleEvent(ev);
                row->offButton->handleEvent(ev);
            }
            for (auto& row : otherMultiRows_)
                for (auto& btn : row->buttons) btn->handleEvent(ev);
            break;
    }
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
                contentScroll_ = 0.f;      // ⭐ 切 Tab 重置滚动
                refreshSelection();
                syncFocus();
            }
            return;
        }
    }

    switch (currentTab_) {
        case Tab::Display: {
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
            if (wallpaperButton_->consumeClick()) { applyWallpaper(); return; }
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
            if (listeningAction_ >= 0) break;
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
                    std::string(Str::T(Str::AboutTitle)) + "\n\n"
                    + Str::T(Str::AboutVersion) + PROJECT_VERSION + "\n"
                    + Str::T(Str::AboutBuild)   + BUILD_DATE + "\n"
                    + Str::T(Str::AboutAuthor)  + "ljm-233";
                aboutDialog_ = std::make_unique<ConfirmDialog>(
                    font_, msg, sf::Vector2f(kDesignW, kDesignH),
                    ConfirmDialog::Mode::Info);
                syncFocus();
                return;
            }
            if (resetButton_->consumeClick()) {
                resetConfirm_ = std::make_unique<ConfirmDialog>(
                    font_, Str::T(Str::ResetConfirm),
                    sf::Vector2f(kDesignW, kDesignH));
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
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i]->setPosition({kTabX, kTabY + i * kTabGap});
        tabButtons_[i]->render(window.target());
    }
}

namespace {
struct RowDrawer {
    sf::RenderTarget& target;
    float contentX;
    float ctrlX;
    float y;
    float rowH = 50.f;
    float availableWidth = 800.f;

    void toggle(ToggleRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);
        row.onButton->setPosition ({ctrlX, y});
        row.offButton->setPosition({ctrlX + 96.f, y});
        row.onButton->render(target);
        row.offButton->render(target);
        y += rowH;
    }

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

    // MultiRow 版本（单行 + 自动折行 + 网格）
    void multi(MultiRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);

        if (row.buttons.empty()) { y += rowH; return; }

        if (row.columns <= 0) {
            const float btnW = row.buttons[0]->size().x;
            const float totalW = row.stepX * (row.buttons.size() - 1) + btnW;

            if (totalW <= availableWidth) {
                for (size_t i = 0; i < row.buttons.size(); ++i) {
                    row.buttons[i]->setPosition(
                        {ctrlX + static_cast<float>(i) * row.stepX, y});
                    row.buttons[i]->render(target);
                }
                y += rowH;
            } else {
                float usable = availableWidth - btnW;
                if (usable < 0.f) usable = 0.f;
                int perRow = 1 + static_cast<int>(usable / row.stepX);
                if (perRow < 1) perRow = 1;
                if (perRow > static_cast<int>(row.buttons.size()))
                    perRow = static_cast<int>(row.buttons.size());

                for (size_t i = 0; i < row.buttons.size(); ++i) {
                    int r = static_cast<int>(i) / perRow;
                    int c = static_cast<int>(i) % perRow;
                    row.buttons[i]->setPosition(
                        {ctrlX + static_cast<float>(c) * row.stepX,
                         y + static_cast<float>(r) * rowH});
                    row.buttons[i]->render(target);
                }
                int rows = (static_cast<int>(row.buttons.size()) + perRow - 1)
                           / perRow;
                y += static_cast<float>(rows) * rowH + 6.f;
            }
        } else {
            const int cols = row.columns;
            float stepY = rowH;
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
} // namespace

float SettingsScene::renderDisplayTab(Window& window, float contentX,
                                     float ctrlX, float y) {
    headingDisplay_.setPosition({contentX, y});
    window.target().draw(headingDisplay_);
    y += 36.f;

    RowDrawer r{window.target(), contentX, ctrlX, y};
    r.availableWidth = kDesignW - ctrlX - 40.f;

    r.multi (*displayMultiRows_[0]);   // Resolution
    r.multi (*displayMultiRows_[1]);   // WindowMode
    r.toggle(*displayToggles_[0]);     // Fullscreen
    r.toggle(*displayToggles_[1]);     // VSync
    r.multi (*displayMultiRows_[2]);   // AntiAliasing
    r.multi (*displayMultiRows_[3]);   // LogLevel
    r.multi (*displayMultiRows_[4]);   // FpsLimit
    return r.y;
}

float SettingsScene::renderInterfaceTab(Window& window, float contentX,
                                       float ctrlX, float y) {
    headingInterface_.setPosition({contentX, y});
    window.target().draw(headingInterface_);
    y += 36.f;

    RowDrawer r{window.target(), contentX, ctrlX, y};
    r.availableWidth = kDesignW - ctrlX - 40.f;

    r.toggle(*interfaceToggles_[0]);   // FPS
    r.multi (*interfaceMultiRows_[0]); // FpsPos
    r.multi (*interfaceMultiRows_[1]); // FpsFormat
    r.multi (*interfaceMultiRows_[2]); // UiScale
    r.multi (*interfaceMultiRows_[3]); // FontScale
    r.multi (*interfaceMultiRows_[4]); // RenderScale
    r.multi (*interfaceMultiRows_[5]); // UpscaleMode

    hintUiScale_.setPosition({contentX, r.y - 26.f});
    window.target().draw(hintUiScale_);

    r.multi (*interfaceMultiRows_[6]); // Theme
    r.multi (*interfaceMultiRows_[7]); // Language
    if (background_) {
        labelWallpaper_.setString(toSf(
            std::string(Str::LabelWallpaper) + "  ("
            + std::to_string(background_->currentIndex() + 1) + "/"
            + std::to_string(background_->totalWallpapers()) + ")"));
    }
    labelWallpaper_.setPosition({contentX, r.y + 8.f});
    window.target().draw(labelWallpaper_);
    wallpaperButton_->setPosition({ctrlX, r.y});
    wallpaperButton_->render(window.target());
    r.y += 50.f;

    r.toggle(*interfaceToggles_[1]);   // Clock
    r.multi (*interfaceMultiRows_[8]); // ClockPos

    r.slider(labelConsoleMask_,       consoleMaskSlider_.get());
    r.slider(labelConsolePanelAlpha_, consolePanelAlphaSlider_.get());
    r.multi (*interfaceMultiRows_[9]); // ConsoleFont
    r.multi (*interfaceMultiRows_[10]); // ConsoleHistory
    r.multi (*interfaceMultiRows_[11]); // ConsoleLineHeight
    r.toggle(*interfaceToggles_[2]);   // AutoScroll
    r.toggle(*interfaceToggles_[3]);   // Blink
    r.multi (*interfaceMultiRows_[12]); // ConsolePrompt
    return r.y;
}

float SettingsScene::renderGraphicsTab(Window& window, float contentX,
                                      float ctrlX, float y) {
    headingGraphics_.setPosition({contentX, y});
    window.target().draw(headingGraphics_);
    y += 36.f;

    RowDrawer r{window.target(), contentX, ctrlX, y};
    r.availableWidth = kDesignW - ctrlX - 40.f;

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
    return r.y;
}

float SettingsScene::renderAudioTab(Window& window, float contentX,
                                   float ctrlX, float y) {
    headingAudio_.setPosition({contentX, y});
    window.target().draw(headingAudio_);
    y += 36.f;

    RowDrawer r{window.target(), contentX, ctrlX, y};
    r.availableWidth = kDesignW - ctrlX - 40.f;

    r.slider(labelMasterVolume_, masterVolumeSlider_.get());
    r.toggle(*audioToggles_[0]);   // Sound
    r.slider(labelSoundVolume_,  soundVolumeSlider_.get());
    r.toggle(*audioToggles_[1]);   // BGM
    r.slider(labelBGMVolume_,    bgmVolumeSlider_.get());
    r.toggle(*audioToggles_[2]);   // Gamepad
    return r.y;
}

float SettingsScene::renderKeysTab(Window& window, float contentX,
                                  float ctrlX, float y) {
    headingKeys_.setPosition({contentX, y});
    window.target().draw(headingKeys_);
    y += 36.f;

    for (int i = 0; i < KeyBindings::Count; ++i) {
        auto act = static_cast<KeyBindings::Action>(i);

        sf::Text label(font_, toSf(Str::T(KeyBindings::actionName(act))),
                       scaledFontSize(20));
        label.setFillColor(sf::Color(230, 230, 230));
        label.setPosition({contentX, y + 8.f});
        window.target().draw(label);

        if (i != listeningAction_) {
            keyBindingButtons_[i]->setText(
                KeyBindings::keyToString(KeyBindings::instance().get(act)));
        }
        keyBindingButtons_[i]->setPosition({ctrlX, y});
        keyBindingButtons_[i]->render(window.target());

        y += 50.f;
    }

    sf::Text hint(font_, toSf(Str::T(Str::KeyBindHint)),
                  scaledFontSize(14));
    hint.setFillColor(sf::Color(180, 180, 200));
    hint.setPosition({contentX, y + 8.f});
    window.target().draw(hint);
    return y + 30.f;
}

float SettingsScene::renderOtherTab(Window& window, float contentX,
                                   float ctrlX, float y) {
    headingOther_.setPosition({contentX, y});
    window.target().draw(headingOther_);
    y += 36.f;

    RowDrawer r{window.target(), contentX, ctrlX, y};
    r.availableWidth = kDesignW - ctrlX - 40.f;

    r.toggle(*otherToggles_[0]);   // RememberSize
    r.toggle(*otherToggles_[1]);   // AutoPause
    r.multi (*otherMultiRows_[0]); // LogRotate
    r.multi (*otherMultiRows_[1]); // LogKeep

    labelPlayerName_.setPosition({contentX, r.y + 8.f});
    window.target().draw(labelPlayerName_);
    playerNameInput_->setPosition({ctrlX, r.y});
    playerNameInput_->setSize({240.f, 40.f});
    playerNameInput_->render(window.target());
    r.y += 50.f;

    // ⭐ 关于/恢复默认按钮已改到屏幕坐标系（固定底部一行）
    //    此处把它们的设计坐标移到屏幕外，避免残留绘制
    aboutButton_->setPosition({-1000.f, -1000.f});
    resetButton_->setPosition({-1000.f, -1000.f});
    return r.y + 40.f;
}

// ============================================================
// 设计坐标系 View
// ============================================================

void SettingsScene::updateDesignView() {
    auto size = window_->native().getSize();
    float winW = static_cast<float>(size.x);
    float winH = static_cast<float>(size.y);
    if (winW <= 0.f || winH <= 0.f) return;

    // ⭐ uiScale 决定设计区域的"多少"映射到窗口
    //   uiScale = 1.0 → 1280×720 设计区域占满窗口
    //   uiScale = 0.5 → 2560×1440 设计区域缩到窗口（UI 缩小一半）
    //   uiScale = 2.0 → 640×360 设计区域放大到窗口（UI 放大两倍）
    float uiS = getUiScale();
    if (uiS < 0.01f) uiS = 1.0f;

    float viewW = kDesignW / uiS;
    float viewH = kDesignH / uiS;

    designView_.setSize({viewW, viewH});

    // ⭐ 应用滚动偏移
    // 底部多留 60 设计像素，让内容能滚到返回按钮上方
    constexpr float kBottomReserve = 60.f;
    float maxScroll = std::max(0.f, contentTotalH_ - viewH + kBottomReserve);
    contentScroll_ = std::clamp(contentScroll_, 0.f, maxScroll);
    designView_.setCenter({viewW * 0.5f, viewH * 0.5f + contentScroll_});

    // Viewport 保持设计宽高比
    float winAspect    = winW / winH;
    float designAspect = viewW / viewH;

    sf::FloatRect vp;
    if (winAspect > designAspect) {
        float vpW = designAspect / winAspect;
        vp = sf::FloatRect(sf::Vector2f{(1.f - vpW) * 0.5f, 0.f},
                           sf::Vector2f{vpW, 1.f});
    } else {
        float vpH = winAspect / designAspect;
        vp = sf::FloatRect(sf::Vector2f{0.f, (1.f - vpH) * 0.5f},
                           sf::Vector2f{1.f, vpH});
    }
    designView_.setViewport(vp);
}

void SettingsScene::render(Window& window) {
    refreshLabels();
    updateDesignView();

    auto& native = window.target();

    // 阶段 1：窗口坐标系画背景
    native.setView(native.getDefaultView());
    native.clear(sf::Color::Black);
    if (background_) background_->render(native);

    // 阶段 2a：Tab 栏用不滚动的 View（固定左侧）
    {
        float uiS = getUiScale();
        if (uiS < 0.01f) uiS = 1.0f;
        sf::View tabView = designView_;
        tabView.setCenter({kDesignW / uiS * 0.5f, kDesignH / uiS * 0.5f});
        native.setView(tabView);
        renderTabs(window);
    }

    // 阶段 2b：内容用带滚动的 View
    native.setView(designView_);

    float contentBottom = 0.f;
    switch (currentTab_) {
        case Tab::Display:
            contentBottom = renderDisplayTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Interface:
            contentBottom = renderInterfaceTab(window, kContentX, kCtrlX, 50.f);
            break;
        case Tab::Graphics:
            contentBottom = renderGraphicsTab(window, kContentX, kCtrlX, 50.f);
            break;
        case Tab::Audio:
            contentBottom = renderAudioTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Keys:
            contentBottom = renderKeysTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Other:
            contentBottom = renderOtherTab(window, kContentX, kCtrlX, 60.f);
            break;
    }
    contentTotalH_ = contentBottom;

    if (resetConfirm_) {
        resetConfirm_->relayout({kDesignW, kDesignH});
        resetConfirm_->render(native);
    }
    if (aboutDialog_) {
        aboutDialog_->relayout({kDesignW, kDesignH});
        aboutDialog_->render(native);
    }

    // ⭐ 滚动条指示器（在 designView 下画）
    {
        float uiS = getUiScale();
        if (uiS > 1.01f) {
            float viewH = kDesignH / uiS;
            constexpr float kBottomReserve = 60.f;
            float maxScroll = std::max(0.f, contentTotalH_ - viewH + kBottomReserve);
            if (maxScroll > 1.f) {
                float viewW = kDesignW / uiS;
                float ratio = contentScroll_ / maxScroll;
                float barH = std::max(40.f, viewH * 0.25f);
                float barY = 10.f + ratio * (viewH - barH - 20.f);

                sf::RectangleShape bar({6.f, barH});
                bar.setFillColor(sf::Color(255, 255, 255, 120));
                bar.setPosition({viewW - 14.f, barY});
                native.draw(bar);
            }
        }
    }

    // ⭐ 阶段 3：窗口坐标系底部一行（不随 UI 缩放）
    native.setView(native.getDefaultView());
    {
        auto size = window.native().getSize();
        float winW = static_cast<float>(size.x);
        float winH = static_cast<float>(size.y);

        const float gap    = 20.f;
        const float margin = 20.f;

        float bw = backButton_->size().x;
        float bh = backButton_->size().y;
        float y  = winH - bh - margin;

        float backX = winW - bw - margin;

        // ⭐ 先画关于/重置，再画返回（保证返回在最上层）
        if (currentTab_ == Tab::Other) {
            float rw = resetButton_->size().x;
            float aw = aboutButton_->size().x;
            float resetX = backX - gap - rw;
            float aboutX = resetX - gap - aw;

            aboutButton_->setPosition({aboutX, y});
            aboutButton_->render(native);

            resetButton_->setPosition({resetX, y});
            resetButton_->render(native);
        }

        // 返回按钮最后画，确保在最上层
        backButton_->setPosition({backX, y});
        backButton_->render(native);
    }
}