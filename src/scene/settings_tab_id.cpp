#include "scene/settings_tab_id.h"

#include "config/keys.h"
#include "utils/text_strings.h"

#include <algorithm>

namespace {

// 「恢复本页默认」的做法是**把这些键从配置里删掉**，而不是往配置里写一份默认值 ——
// 默认值只存在于读取处（`prefs_->getInt(key, default)`），删掉之后自然回落。
// 好处是默认值永远只有一个定义，不会出现"这里改了那里没改"。
//
// ⚠️ 新加配置键时**必须**归到某一页，否则 tests/test_settings_tabs.cpp 会报出来
//    （"有键没归属"正是这次重做要消灭的问题：日志级别曾经在「显示」、
//      轮转保留在「其他」，同一个功能劈成两半）。
const std::vector<const char*> kDisplayKeys = {
    ConfigKey::kResolutionIndex,    ConfigKey::kFullscreen,
    ConfigKey::kWindowMode,         ConfigKey::kVsync,
    ConfigKey::kFpsLimit,           ConfigKey::kAntiAliasing,
    ConfigKey::kRememberWindowSize, ConfigKey::kAutoPauseOnBlur,
};

const std::vector<const char*> kInterfaceKeys = {
    ConfigKey::kTheme,
    ConfigKey::kUiScale,
    ConfigKey::kFontScale,
    ConfigKey::kLanguage,
    ConfigKey::kButtonCorner,
    ConfigKey::kButtonOutline,
    ConfigKey::kAnimationEnabled,
    ConfigKey::kAnimationSpeedIndex,
    ConfigKey::kNotificationEnabled,
    ConfigKey::kNotificationPosition,
    ConfigKey::kNotificationDuration,
    ConfigKey::kShowFps,
    ConfigKey::kFpsFormat,
    ConfigKey::kFpsPosition,
    ConfigKey::kShowClock,
    ConfigKey::kClockPosition,
};

const std::vector<const char*> kWallpaperKeys = {
    ConfigKey::kCurrentWallpaper,
};

const std::vector<const char*> kGraphicsKeys = {
    ConfigKey::kRenderScale,        ConfigKey::kUpscaleMode,
    ConfigKey::kPostSaturation,     ConfigKey::kPostContrast,
    ConfigKey::kPostBrightness,     ConfigKey::kPostGamma,
    ConfigKey::kPostVignette,       ConfigKey::kPostBloomStrength,
    ConfigKey::kPostBloomThreshold, ConfigKey::kPostChromatic,
    ConfigKey::kPostGrain,          ConfigKey::kPostScanline,
    ConfigKey::kPostDither,         ConfigKey::kParallax,
    ConfigKey::kParticles,          ConfigKey::kParticleDensity,
    ConfigKey::kPseudo3d,           ConfigKey::kScreenShake,
    ConfigKey::kShakeIntensity,     ConfigKey::kPlayerAnimation,
};

const std::vector<const char*> kAudioKeys = {
    ConfigKey::kAudioMasterVolume, ConfigKey::kAudioSoundEnabled,
    ConfigKey::kAudioSoundVolume,  ConfigKey::kAudioBgmEnabled,
    ConfigKey::kAudioBgmVolume,    ConfigKey::kUiSoundEnabled,
};

const std::vector<const char*> kControlsKeys = {
    ConfigKey::kKeyLeft,
    ConfigKey::kKeyRight,
    ConfigKey::kKeyJump,
    ConfigKey::kKeyPause,
    ConfigKey::kKeyRestart,
    ConfigKey::kGamepadEnabled,
    ConfigKey::kGamepadVibrationEnabled,
    ConfigKey::kGamepadVibrationIntensity,
};

const std::vector<const char*> kGameKeys = {
    ConfigKey::kInitialLives,
    ConfigKey::kLevelIntro,
    ConfigKey::kPlayerName,
};

const std::vector<const char*> kConsoleKeys = {
    ConfigKey::kConsoleAutoScroll, ConfigKey::kConsoleBlinkCursor,
    ConfigKey::kConsoleFontSize,   ConfigKey::kConsoleHistoryLines,
    ConfigKey::kConsoleLineHeight, ConfigKey::kConsoleMask,
    ConfigKey::kConsolePanelAlpha, ConfigKey::kConsolePrompt,
};

const std::vector<const char*> kAdvancedKeys = {
    ConfigKey::kLogLevel,
    ConfigKey::kLogRotate,
    ConfigKey::kLogKeep,
    ConfigKey::kShowColliders,
};

} // namespace

// 不在任何一页的键。目前只有窗口几何状态 —— 它由窗口逻辑自己写，
// 不是用户能"设置"的东西，所以不该出现在设置界面里，也不需要"恢复默认"。
// 注意它**故意放在匿名 namespace 外面**：测试要拿它区分
// "忘了归类"（报错）与"有意不归类"（放行）。
const std::vector<const char*>& unownedKeys() {
    static const std::vector<const char*> kUnowned = {
        ConfigKey::kLastWindowWidth,
        ConfigKey::kLastWindowHeight,
    };
    return kUnowned;
}

const char* settingsTabLabel(SettingsTab t) {
    switch (t) {
    case SettingsTab::Display:
        return Str::TabDisplay;
    case SettingsTab::Interface:
        return Str::TabInterface;
    case SettingsTab::Wallpaper:
        return Str::LabelWallpaper;
    case SettingsTab::Graphics:
        return Str::TabGraphics;
    case SettingsTab::Audio:
        return Str::TabAudioLog;
    case SettingsTab::Controls:
        return Str::TabControls;
    case SettingsTab::Game:
        return Str::TabGame;
    case SettingsTab::Console:
        return Str::TabConsole;
    case SettingsTab::Advanced:
        return Str::TabAdvanced;
    default:
        return "";
    }
}

const std::vector<const char*>& keysForTab(SettingsTab t) {
    switch (t) {
    case SettingsTab::Display:
        return kDisplayKeys;
    case SettingsTab::Interface:
        return kInterfaceKeys;
    case SettingsTab::Wallpaper:
        return kWallpaperKeys;
    case SettingsTab::Graphics:
        return kGraphicsKeys;
    case SettingsTab::Audio:
        return kAudioKeys;
    case SettingsTab::Controls:
        return kControlsKeys;
    case SettingsTab::Game:
        return kGameKeys;
    case SettingsTab::Console:
        return kConsoleKeys;
    case SettingsTab::Advanced:
        return kAdvancedKeys;
    default:
        return unownedKeys();
    }
}

std::optional<SettingsTab> tabForKey(const std::string& key) {
    for (int i = 0; i < static_cast<int>(kSettingsTabCount); ++i) {
        const auto t = static_cast<SettingsTab>(i);
        const auto& keys = keysForTab(t);
        if (std::find(keys.begin(), keys.end(), key) != keys.end())
            return t;
    }
    return std::nullopt;
}
