#pragma once
#include "lang.h"

// 所有用户可见的字符串集中在这里
// 用 constexpr const char* 而非 std::string，避免静态初始化顺序问题
// 迁移到多语言时：把 Str::Xxx 改成 Str::T(Str::Xxx)
namespace Str {

// ===== 通用 =====
constexpr const char* Yes = "是";
constexpr const char* No = "否";
constexpr const char* Back = "返回";
constexpr const char* On = "开";
constexpr const char* Off = "关";

// ===== 主菜单 =====
constexpr const char* StartGame = "启动游戏";
constexpr const char* Calculator = "控制台";
constexpr const char* LevelEditor = "关卡编辑器";
constexpr const char* Achievements = "成就";
constexpr const char* Settings = "设置";
constexpr const char* ExitGame = "退出游戏";

// ===== 存档 =====
constexpr const char* NewSave = "＋ 新建存档";
constexpr const char* DeleteMark = "×";
constexpr const char* DeleteConfirmHead = "确定删除存档「";
constexpr const char* DeleteConfirmTail = "」？";
constexpr const char* UnnamedSave = "未命名存档";
constexpr const char* SaveNamePrefix = "存档 ";

// ===== 游戏场景 =====
constexpr const char* GameNotImpl = "游戏尚未实现";
constexpr const char* CurrentSave = "当前存档: ";
constexpr const char* EscToSaveSelect = "按 ESC 返回存档选择";

// ===== 设置 =====
constexpr const char* TabDisplay = "显示";
constexpr const char* TabInterface = "界面";
constexpr const char* TabOther = "其他";
constexpr const char* TabKeys = "按键";

constexpr const char* LabelResolution = "分辨率";
constexpr const char* LabelFullscreen = "全屏";
constexpr const char* LabelVsync = "垂直同步";
constexpr const char* LabelAntiAliasing = "抗锯齿";
constexpr const char* LabelLogLevel = "日志级别";
constexpr const char* LabelFps = "帧率显示";
constexpr const char* LabelUiScale = "界面缩放";
constexpr const char* LabelFontScale = "字体缩放";
constexpr const char* LabelRenderScale = "渲染缩放";
constexpr const char* LabelUpscaleMode = "超分辨率";
constexpr const char* UpscaleOff      = "关";
constexpr const char* UpscaleBicubic  = "双三次";
constexpr const char* UpscaleFsr1     = "FSR1";
constexpr const char* RenderScale100   = "100%";
constexpr const char* RenderScale75    = "75%";
constexpr const char* RenderScale50    = "50%";
constexpr const char* RenderScale33    = "33%";
constexpr const char* RenderScale25    = "25%";
constexpr const char* RenderScale10    = "10%";
// ⭐ 超采样
constexpr const char* RenderScale200   = "200%";
constexpr const char* RenderScale150   = "150%";
constexpr const char* RenderScale125   = "125%";
constexpr const char* LabelConsoleMask = "控制台遮罩";
constexpr const char* LabelConsoleFont = "控制台字号";
constexpr const char* LabelConsoleHistory = "控制台历史";
constexpr const char* LabelTheme = "主题";
constexpr const char* LabelLanguage = "语言";
constexpr const char* LabelWallpaper = "壁纸";
constexpr const char* LabelRememberSize = "记住窗口大小";
constexpr const char* LabelFpsPos = "FPS 位置";
constexpr const char* LabelFpsLimit = "帧率上限";
constexpr const char* LabelConsoleAutoScroll = "自动滚动";
constexpr const char* LabelConsoleBlink = "光标闪烁";
constexpr const char* LabelButtonCorner = "按钮圆角";
constexpr const char* LabelButtonOutline = "按钮边框";

constexpr const char* HintUiScale = "* 修改后返回主菜单再进入生效";
constexpr const char* ResetDefault = "恢复默认设置";
constexpr const char* ResetConfirm = "恢复默认设置？所有自定义将被清除";
constexpr const char* NextWallpaper = "下一张 →";

// ===== 控制台 =====
constexpr const char* ConsoleTitle = "=== TEXT-GAME 控制台 ===";
constexpr const char* ConsoleHint = "提示: ↑/↓ 翻历史，ESC 返回";
constexpr const char* ConsolePrompt = "> ";

// ===== 计算器 =====
constexpr const char* CalcFirstNum = "请输入第一个数字·Enter 1st number:";
constexpr const char* CalcSecondNum = "请输入第二个数字·Enter 2nd number:";
constexpr const char* CalcWhatWant =
    "你想要什么·What do you want?\n1+ 2- 3x 4/ 5幂函数\n";
constexpr const char* CalcDivZero = "不能除以零·Cannot divide by zero!";
constexpr const char* CalcOverflow = "结果过大，无法显示!";
constexpr const char* CalcResult = "结果·End Number: ";
constexpr const char* CalcRetry = "[再试·Retry]";
constexpr const char* CalcInvalid = "输入无效";
constexpr const char* CalcFail = " Fail! :( ";

// ===== 主题名 =====
constexpr const char* ThemeDark = "深色";
constexpr const char* ThemeBlue = "蓝色";
constexpr const char* ThemeLight = "浅色";

constexpr const char* LabelLogRotate = "日志轮转";
constexpr const char* LabelLogKeep = "日志保留";
constexpr const char* LabelConsolePanelAlpha = "面板透明度";
constexpr const char* LabelConsoleLineHeight = "控制台行高";
constexpr const char* LabelClock = "时间显示";
constexpr const char* LabelClockPos = "时钟位置";
constexpr const char* ButtonAbout = "关于";
constexpr const char* AboutTitle = "TEXT-GAME";

// ===== 玩家名 =====
constexpr const char* LabelPlayerName = "玩家名";
constexpr const char* PlayerNamePlaceholder = "输入玩家名";

// ===== 新建存档对话框 =====
constexpr const char* NewSaveTitle = "新建存档";
constexpr const char* NewSaveHint = "输入存档名：";
constexpr const char* NewSavePlaceholder = "我的存档";
constexpr const char* BtnCreate = "创建";
constexpr const char* BtnCancel = "取消";

// ===== 动画 =====
constexpr const char* LabelAnimation = "动画效果";
constexpr const char* LabelAnimationSpeed = "动画速度";
constexpr const char* AnimSpeedSlow = "慢";
constexpr const char* AnimSpeedNormal = "正常";
constexpr const char* AnimSpeedFast = "快";

// ===== 通知 =====
constexpr const char* LabelNotification = "屏幕通知";
constexpr const char* LabelNotificationPos = "通知位置";

// ===== 新设置项 =====
constexpr const char* LabelMasterVolume = "总音量";
constexpr const char* LabelSound = "音效";
constexpr const char* LabelSoundVolume = "音效音量";
constexpr const char* LabelBGMVolume = "BGM 音量";
constexpr const char* LabelAutoPause = "失焦自动暂停";
constexpr const char* LabelFpsFormat = "FPS 格式";
constexpr const char* LabelConsolePrompt = "控制台提示符";

constexpr const char* LabelShowColliders = "显示碰撞盒";
constexpr const char* LabelScreenShake = "屏幕震动";
constexpr const char* LabelParticles = "粒子效果";
// ===== 新 Tab =====
constexpr const char* TabGraphics = "画面";
constexpr const char* TabAudioLog = "音频";
constexpr const char* TabGame = "游戏";

// ===== 新设置项 =====
constexpr const char* LabelPseudo3D = "伪 3D 效果";
constexpr const char* LabelParallax = "视差背景";
constexpr const char* LabelPlayerAnimation = "玩家动画";
constexpr const char* LabelLevelIntro = "关卡开场";
constexpr const char* LabelBGM = "背景音乐";
// ===== 关卡选择 =====
constexpr const char* LevelSelect = "选关";
constexpr const char* LevelSelectTitle = "选择关卡";
constexpr const char* NoSaveHint = "没有存档，请先创建存档";
constexpr const char* SaveLabel = "当前存档: ";
constexpr const char* LockedHint = "未解锁";

// ===== 手柄 =====
constexpr const char* LabelGamepad        = "手柄支持";
constexpr const char* LabelGamepadDeadzone = "摇杆死区";
constexpr const char* LabelGamepadVibration = "手柄振动";
constexpr const char* LabelVibrationIntensity = "振动强度";

// ===== 成就系统 =====
constexpr const char* AchvUnlockedPrefix = "★ 成就解锁：";
constexpr const char* AchvTitle = "成就";
constexpr const char* AchvProgressPrefix = "已解锁 ";
constexpr const char* AchvProgressSeparator = " / ";

constexpr const char* AchvFirstCoin     = "初次收获";
constexpr const char* AchvFirstCoinDesc = "收集第一个金币";
constexpr const char* AchvFirstLevel     = "初出茅庐";
constexpr const char* AchvFirstLevelDesc = "通关第一个关卡";
constexpr const char* AchvAllLevels     = "通关大师";
constexpr const char* AchvAllLevelsDesc = "通关全部关卡";
constexpr const char* AchvPerfectLevel     = "完美主义";
constexpr const char* AchvPerfectLevelDesc = "单关收集全部金币并通关";
constexpr const char* AchvNoDamage     = "毫发无伤";
constexpr const char* AchvNoDamageDesc = "无伤通关一个关卡";
constexpr const char* AchvSpeedrun     = "疾风步";
constexpr const char* AchvSpeedrunDesc = "30 秒内通关一个关卡";
constexpr const char* AchvThreeStars     = "三星大师";
constexpr const char* AchvThreeStarsDesc = "在一个关卡拿到 3 星";
constexpr const char* AchvAllStars     = "全星达成";
constexpr const char* AchvAllStarsDesc = "全部关卡都拿到 3 星";
constexpr const char* AchvEditorUsed     = "关卡设计师";
constexpr const char* AchvEditorUsedDesc = "在编辑器里保存一次关卡";
constexpr const char* AchvEditorExport     = "分享达人";
constexpr const char* AchvEditorExportDesc = "导出一次关卡分享码";
constexpr const char* AchvConsoleUsed     = "命令行勇士";
constexpr const char* AchvConsoleUsedDesc = "进入控制台场景";
constexpr const char* AchvDebugMode     = "内部人士";
constexpr const char* AchvDebugModeDesc = "使用任意调试快捷键";

// ===== 生命值 =====
constexpr const char* LabelInitialLives = "初始生命";

// ===== 游戏场景 HUD =====
constexpr const char* HudSave   = "存档: ";
constexpr const char* HudLevel  = "关卡: ";
constexpr const char* HudLives  = "生命: ";
constexpr const char* HudCoins  = "金币: ";
constexpr const char* HudTime   = "时间: ";
constexpr const char* HudKeys   = "钥匙: ";
constexpr const char* HudHelp   = "WASD 移动，Space 跳跃，ESC 暂停";

// ===== 关卡完成 / 游戏失败 =====
constexpr const char* LevelCompleteTitle = "完成！";
constexpr const char* GameOverTitle      = "游戏失败";
constexpr const char* OverlayCoins       = "金币: ";
constexpr const char* OverlayTime        = "时间: ";
constexpr const char* OverlayTarget      = "目标: ";
constexpr const char* OverlaySeconds     = " 秒";
constexpr const char* OverlayHintComplete = "Enter 继续    R 重玩本关    ESC 返回";
constexpr const char* OverlayHintFailed   = "R 重试    ESC 返回";
constexpr const char* OverlayBestTime     = "最佳: ";
constexpr const char* OverlayNewRecord    = "★ 新纪录！";

// ===== 通知 =====
constexpr const char* NotifLevelLoadFailed    = "关卡加载失败";
constexpr const char* NotifLevelCompleteStars = "关卡完成！获得 ";
constexpr const char* NotifStarSuffix         = " 星";
constexpr const char* NotifGameOverRetry      = "游戏失败，按 R 重试";
constexpr const char* NotifEnterLevel         = "进入第 ";
constexpr const char* NotifLevelSuffix        = " 关";
constexpr const char* NotifAllClear           = "全部通关！";
constexpr const char* NotifRespawned          = "已重生";
constexpr const char* NotifLifeExhausted      = "生命耗尽，重新开始";

// ===== 关卡开场 =====
constexpr const char* IntroLevelPrefix = "关卡 ";
constexpr const char* IntroCollect     = "收集 ";
constexpr const char* IntroCollectTail = " 个金币，到达终点";

// ===== 关卡编辑器 =====
constexpr const char* EditorHudFile  = "文件: ";
constexpr const char* EditorHudBrush = "笔刷: ";
constexpr const char* EditorHudSize  = "尺寸: ";
constexpr const char* EditorHudZoom  = "缩放: ";
constexpr const char* EditorHudGrid  = "网格: ";
constexpr const char* EditorHudUndo  = "撤销: ";

constexpr const char* EditorHint =
    "左键画/拖动连画  Shift+拖动=矩形  右键擦  T可达性  G网格  Ctrl+滚轮缩放  WASD移动  Ctrl+S保存  Ctrl+Z撤销  Ctrl+N切文件  Ctrl+E导出  Ctrl+I导入  Ctrl+0重置缩放";

constexpr const char* EditorSaved      = "✓ 已保存";
constexpr const char* EditorSaveFailed = "保存失败！";
constexpr const char* EditorUndone     = "已撤销";
constexpr const char* EditorNoUndo     = "无可撤销";
constexpr const char* EditorGridOn     = "网格: 开";
constexpr const char* EditorGridOff    = "网格: 关";
constexpr const char* EditorZoomReset  = "缩放: 100%";
constexpr const char* EditorSwitchTo   = "切换到 ";
constexpr const char* EditorReachOn    = "可达性: 开";
constexpr const char* EditorReachOff   = "可达性: 关";

// ===== 编辑器笔刷名 =====
constexpr const char* BrushWall     = "墙";
constexpr const char* BrushErase    = "橡皮";
constexpr const char* BrushPlayer   = "玩家";
constexpr const char* BrushEnemy    = "敌人";
constexpr const char* BrushCoin     = "金币";
constexpr const char* BrushGoal     = "终点";
constexpr const char* BrushSpike    = "尖刺";
constexpr const char* BrushJumpPad  = "跳台";
constexpr const char* BrushCheckpoint = "存档";
constexpr const char* BrushKey      = "钥匙";
constexpr const char* BrushDoor     = "门";
constexpr const char* BrushPlatformH = "横台";
constexpr const char* BrushPlatformV = "竖台";
// ===== 编辑器元素统计 =====
constexpr const char* StatPlatform   = "移动平台";

// ===== 控制台 =====
constexpr const char* ConsoleWelcomeTitle = "TEXT-GAME 控制台";
constexpr const char* ConsoleWelcomeHint  = "输入 help 查看可用命令";

constexpr const char* ConsoleHelpText =
    "可用命令:\n"
    "  help              显示帮助\n"
    "  clear             清空屏幕\n"
    "  echo <text>       回显文本\n"
    "  version           显示版本\n"
    "  calc              启动计算器\n"
    "  scene <name>      切换场景 (main/save/settings/quit)\n"
    "  log <level>       设置日志级别 (trace/debug/info/warn/error)\n"
    "  theme <name>      切换主题 (dark/blue/light)\n"
    "  save list         列出所有存档\n"
    "  exit              关闭控制台\n";

constexpr const char* ConsoleUsageScene       = "用法: scene <main|save|settings|quit>";
constexpr const char* ConsoleUnknownScene     = "未知场景: ";
constexpr const char* ConsoleUsageLog         = "用法: log <trace|debug|info|warn|error>";
constexpr const char* ConsoleUnknownLevel     = "未知级别: ";
constexpr const char* ConsoleLogLevelChanged  = "日志级别已切换: ";
constexpr const char* ConsoleUsageTheme       = "用法: theme <dark|blue|light>";
constexpr const char* ConsoleUnknownTheme     = "未知主题: ";
constexpr const char* ConsoleThemeChanged     = "主题已切换。返回主菜单再进入生效。";
constexpr const char* ConsoleNoSaves          = "没有存档。";
constexpr const char* ConsoleSavesPrefix      = "共 ";
constexpr const char* ConsoleSavesSuffix      = " 个存档:";
constexpr const char* ConsoleUsageSave        = "用法: save list";
constexpr const char* ConsoleUnknownCmd1      = "未知命令: ";
constexpr const char* ConsoleUnknownCmd2      = "。输入 help 查看帮助。";
constexpr const char* ConsoleCalcStart        = "[启动计算器...]";
constexpr const char* ConsoleCalcEnd          = "[计算器已退出]";

// ===== 设置页下拉选项 =====
constexpr const char* OptOff       = "关";
constexpr const char* OptNone      = "无";
constexpr const char* OptSlow      = "慢";
constexpr const char* OptNormal    = "正常";
constexpr const char* OptFast      = "快";
constexpr const char* OptTL        = "左上";
constexpr const char* OptTR        = "右上";
constexpr const char* OptBL        = "左下";
constexpr const char* OptBR        = "右下";
constexpr const char* OptNumber    = "纯数字";
constexpr const char* OptSmall     = "小";
constexpr const char* OptMedium    = "中";
constexpr const char* OptLarge     = "大";
constexpr const char* OptXLarge    = "特大";
constexpr const char* OptTight     = "紧凑";
constexpr const char* OptLoose     = "宽松";
constexpr const char* OptSharp     = "直角";
constexpr const char* OptSmallRound = "小圆";
constexpr const char* OptLargeRound = "大圆";
constexpr const char* OptThin      = "细";
constexpr const char* OptThick     = "粗";
constexpr const char* OptInfinite  = "无限";

// ===== 设置页对话框 =====
constexpr const char* NotifLanguageChanged = "语言已切换，返回主菜单后生效";
constexpr const char* AboutVersion = "版本: ";
constexpr const char* AboutBuild   = "构建: ";
constexpr const char* AboutAuthor  = "作者: ";

// ===== 关卡完成 / 失败按钮 =====
constexpr const char* BtnNextLevel = "下一关";
constexpr const char* BtnReplay    = "重玩";
constexpr const char* BtnRetry     = "重试";

// ============================================================
// 多语言辅助函数
// ============================================================
// 用法：Str::T(Str::Yes) —— 返回当前语言的翻译
// 未提供翻译时返回中文原文
inline std::string T(const char* key) {
    return Lang::instance().tr(key);
}

// ===== 窗口标题 =====
constexpr const char* WinTitleSaveSelect  = "存档选择";
constexpr const char* WinTitleLevelSelect = "选关";
constexpr const char* WinTitleSettings    = "设置";
constexpr const char* WinTitleConsole     = "控制台";
constexpr const char* WinTitleEditor      = "关卡编辑器";
constexpr const char* WinTitleAchievements = "成就";
constexpr const char* WinTitleLevelPrefix = "第 ";
constexpr const char* WinTitleLevelSuffix = " 关";
constexpr const char* WinTitlePausedSuffix = " 关 (已暂停)";

// ===== 按键绑定 =====
constexpr const char* KeyActionLeft    = "左移";
constexpr const char* KeyActionRight   = "右移";
constexpr const char* KeyActionJump    = "跳跃";
constexpr const char* KeyActionPause   = "暂停";
constexpr const char* KeyActionRestart = "重开";
constexpr const char* KeyPressNew      = "按下新键...";
constexpr const char* KeyBindHint      = "点击按钮后按新键绑定，Esc 取消";

constexpr const char* LabelWindowMode = "窗口模式";
constexpr const char* WinModeWindowed = "窗口";
constexpr const char* WinModeMaximized = "最大化";
constexpr const char* WinModeFullscreen = "全屏";
// ===== 后处理 =====
constexpr const char* LabelPostSaturation = "饱和度";
constexpr const char* LabelPostContrast   = "对比度";
constexpr const char* LabelPostBrightness = "亮度";
constexpr const char* LabelPostGamma      = "伽马";
constexpr const char* LabelPostVignette   = "暗角";
constexpr const char* LabelPostBloomStrength  = "泛光强度";
constexpr const char* LabelPostBloomThreshold = "泛光阈值";
constexpr const char* LabelPostChromatic      = "色差";
constexpr const char* LabelPostGrain          = "胶片颗粒";
constexpr const char* LabelPostScanline       = "扫描线";
constexpr const char* LabelPostDither         = "抖动";
constexpr const char* ResetGraphics           = "重置画面";

// ===== 画面预设 =====
constexpr const char* LabelPreset       = "预设";
constexpr const char* PresetDefault     = "原版";
constexpr const char* PresetCRT         = "复古 CRT";
constexpr const char* PresetCinematic   = "电影感";
constexpr const char* PresetPixel8      = "像素 8-bit";
constexpr const char* PresetNight       = "夜晚";

} // namespace Str