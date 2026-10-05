#include "config/keys.h"

#include <algorithm>
#include <array>
#include <string>

namespace ConfigKey {
namespace {

// 可以跨机器搬运的键。
//
// 加新键时问自己一句：**这个值换一台机器还成立吗？**
// 跟显示器、显卡、窗口系统绑定的答案都是"不成立"。
constexpr std::array kPortable = {
    // 界面外观
    kTheme,
    kButtonCorner,
    kButtonOutline,
    kAnimationEnabled,
    kAnimationSpeedIndex,
    kNotificationEnabled,
    kNotificationPosition,
    kLanguage,
    // 屏幕上的浮层
    kShowFps,
    kFpsFormat,
    kFpsPosition,
    kShowClock,
    kClockPosition,
    // 渲染质量
    kRenderScale,
    kUpscaleMode,
    kPostSaturation,
    kPostContrast,
    kPostBrightness,
    kPostGamma,
    kPostVignette,
    kPostBloomStrength,
    kPostBloomThreshold,
    kPostChromatic,
    kPostGrain,
    kPostScanline,
    kPostDither,
    // 特效开关
    kParallax,
    kParticles,
    kPseudo3d,
    kScreenShake,
    kPlayerAnimation,
    // 音频
    kAudioMasterVolume,
    kAudioSoundEnabled,
    kAudioSoundVolume,
    kAudioBgmEnabled,
    kAudioBgmVolume,
    kUiSoundEnabled,
    // 观感强度（换台机器同样成立）
    kNotificationDuration,
    kParticleDensity,
    kShakeIntensity,
    // 手柄与键位（个人习惯，跟机器无关）
    kGamepadEnabled,
    kGamepadVibrationEnabled,
    kGamepadVibrationIntensity,
    kKeyLeft,
    kKeyRight,
    kKeyJump,
    kKeyPause,
    kKeyRestart,
    // 玩法
    kInitialLives,
    kLevelIntro,
    kPlayerName,
    // 控制台外观
    kConsoleAutoScroll,
    kConsoleBlinkCursor,
    kConsoleFontSize,
    kConsoleHistoryLines,
    kConsoleLineHeight,
    kConsoleMask,
    kConsolePanelAlpha,
    kConsolePrompt,
    // 日志与调试（导入别人的日志级别只是多打点日志，无害）
    kLogLevel,
    kLogRotate,
    kLogKeep,
    kShowColliders,
};

// 下面这批**故意不在**名单里，理由逐条写清楚，
// 免得以后有人看到"少了个键"就顺手加回去：
//
//   kResolutionIndex / kFullscreen / kWindowMode
//       跟这台机器的显示器与窗口系统绑定。导入别人的值可能开出一个
//       超出屏幕、或者当前显示器不支持的窗口 —— 严重时游戏起不来。
//   kVsync / kFpsLimit / kAntiAliasing
//       跟显卡与显示器绑定（CLAUDE.md 的默认值表里专门列过这一条：
//       0.3.1~0.3.3 把开发机的 vsync=false + fps_limit=0 原样发出去，
//       新用户默认不锁帧、显卡满载空转）。
//   kUiScale / kFontScale
//       按屏幕尺寸与 DPI 调的。换台机器照搬可能让整个设置界面
//       大到点不到、或者小到看不清 —— 而设置界面正是用户用来改回来的地方。
//   kRememberWindowSize / kLastWindowWidth / kLastWindowHeight
//       纯本机窗口状态，没有跨机器语义。
//   kCurrentWallpaper
//       个人口味，而且换台机器那个文件名很可能不存在。壁纸本身是
//       内容不是设置，跟着分享码走只会让人以为"设置坏了"。

} // namespace

bool isPortable(const std::string& key) {
    return std::any_of(kPortable.begin(), kPortable.end(),
                       [&key](const char* k) { return key == k; });
}

int portableKeyCount() {
    return static_cast<int>(kPortable.size());
}

} // namespace ConfigKey
