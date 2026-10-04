#pragma once
//
// 配置键的唯一权威来源。
//
// 在此之前，键名是以字符串字面量的形式散在 120 个调用点上的，有两个后果：
//   - 改一个键名要全仓库 grep，漏掉的地方**静默回退默认值**，不报错；
//   - 打错字（写成 "log_lvl"）同样静默回退，行为诡异但没有任何提示。
//
// 收敛到这里之后，改键名与拼错都是编译期错误。
//
// 对应 albuswall dto/thumbnail.py 的写法：
//     "唯一权威定义：spec / column 映射 —— 仓储、服务、迁移脚本一律从这里
//      import，禁止就地重定义。"
// 新代码一律用这里的常量；存量调用点随各自所属的层一起迁移。
//
// 注意：**默认值刻意不放在这里**。默认值和读取处的语义绑在一起
// （"没有配置时该用什么"是业务判断），硬抽出来只会让两边对不上。
// 键名是事实，默认值是决策，两者不该混在一个文件里。
//
namespace ConfigKey {

// ---------------- 界面 ----------------
inline constexpr const char* kUiScale = "ui_scale";
inline constexpr const char* kFontScale = "font_scale";
inline constexpr const char* kTheme = "theme";
inline constexpr const char* kButtonCorner = "button_corner";
inline constexpr const char* kButtonOutline = "button_outline";
inline constexpr const char* kAnimationEnabled = "animation_enabled";
inline constexpr const char* kAnimationSpeedIndex = "animation_speed_index";
inline constexpr const char* kLanguage = "language";
inline constexpr const char* kNotificationEnabled = "notification_enabled";
inline constexpr const char* kNotificationPosition = "notification_position";

// ---------------- HUD / 叠加显示 ----------------
inline constexpr const char* kShowFps = "show_fps";
inline constexpr const char* kFpsFormat = "fps_format";
inline constexpr const char* kFpsPosition = "fps_position";
inline constexpr const char* kShowClock = "show_clock";
inline constexpr const char* kClockPosition = "clock_position";

// ---------------- 窗口与显示 ----------------
inline constexpr const char* kResolutionIndex = "resolution_index";
inline constexpr const char* kFullscreen = "fullscreen";
inline constexpr const char* kVsync = "vsync";
inline constexpr const char* kFpsLimit = "fps_limit";
inline constexpr const char* kAntiAliasing = "anti_aliasing";
inline constexpr const char* kWindowMode = "window_mode";
inline constexpr const char* kRememberWindowSize = "remember_window_size";
inline constexpr const char* kAutoPauseOnBlur = "auto_pause_on_blur";
inline constexpr const char* kLastWindowWidth = "last_window_width";
inline constexpr const char* kLastWindowHeight = "last_window_height";

// ---------------- 渲染与后处理 ----------------
inline constexpr const char* kRenderScale = "render_scale";
inline constexpr const char* kUpscaleMode = "upscale_mode";
inline constexpr const char* kPostSaturation = "post_saturation";
inline constexpr const char* kPostContrast = "post_contrast";
inline constexpr const char* kPostBrightness = "post_brightness";
inline constexpr const char* kPostGamma = "post_gamma";
inline constexpr const char* kPostVignette = "post_vignette";
inline constexpr const char* kPostBloomStrength = "post_bloom_strength";
inline constexpr const char* kPostBloomThreshold = "post_bloom_threshold";
inline constexpr const char* kPostChromatic = "post_chromatic";
inline constexpr const char* kPostGrain = "post_grain";
inline constexpr const char* kPostScanline = "post_scanline";
inline constexpr const char* kPostDither = "post_dither";

// ---------------- 画面表现（游戏内） ----------------
inline constexpr const char* kParallax = "parallax";
inline constexpr const char* kParticles = "particles";
inline constexpr const char* kPseudo3d = "pseudo_3d";
inline constexpr const char* kScreenShake = "screen_shake";
inline constexpr const char* kPlayerAnimation = "player_animation";

// ---------------- 音频 ----------------
inline constexpr const char* kAudioMasterVolume = "master_volume";
inline constexpr const char* kAudioSoundEnabled = "sound_enabled";
inline constexpr const char* kAudioSoundVolume = "sound_volume";
inline constexpr const char* kAudioBgmEnabled = "bgm_enabled";
inline constexpr const char* kAudioBgmVolume = "bgm_volume";

// ---------------- 日志 ----------------
inline constexpr const char* kLogLevel = "log_level";
inline constexpr const char* kLogRotate = "log_rotate";
inline constexpr const char* kLogKeep = "log_keep";

// ---------------- 控制 ----------------
inline constexpr const char* kGamepadEnabled = "gamepad_enabled";
inline constexpr const char* kGamepadVibrationEnabled = "gamepad_vibration_enabled";
inline constexpr const char* kGamepadVibrationIntensity = "gamepad_vibration_intensity";
inline constexpr const char* kKeyLeft = "key_left";
inline constexpr const char* kKeyRight = "key_right";
inline constexpr const char* kKeyJump = "key_jump";
inline constexpr const char* kKeyPause = "key_pause";
inline constexpr const char* kKeyRestart = "key_restart";

// ---------------- 游戏 ----------------
inline constexpr const char* kCurrentWallpaper = "current_wallpaper";
inline constexpr const char* kInitialLives = "initial_lives";
inline constexpr const char* kLevelIntro = "level_intro";
inline constexpr const char* kPlayerName = "player_name";
inline constexpr const char* kShowColliders = "show_colliders";

// ---------------- 控制台场景 ----------------
inline constexpr const char* kConsoleAutoScroll = "console_auto_scroll";
inline constexpr const char* kConsoleBlinkCursor = "console_blink_cursor";
inline constexpr const char* kConsoleFontSize = "console_font_size";
inline constexpr const char* kConsoleHistoryLines = "console_history_lines";
inline constexpr const char* kConsoleLineHeight = "console_line_height";
inline constexpr const char* kConsoleMask = "console_mask";
inline constexpr const char* kConsolePanelAlpha = "console_panel_alpha";
inline constexpr const char* kConsolePrompt = "console_prompt";

}  // namespace ConfigKey
