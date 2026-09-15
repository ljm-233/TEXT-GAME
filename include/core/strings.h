#pragma once

// 所有用户可见的字符串集中在这里
// 用 constexpr const char* 而非 std::string，避免静态初始化顺序问题
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

constexpr const char* LabelResolution = "分辨率";
constexpr const char* LabelFullscreen = "全屏";
constexpr const char* LabelVsync = "垂直同步";
constexpr const char* LabelAntiAliasing = "抗锯齿";
constexpr const char* LabelLogLevel = "日志级别";
constexpr const char* LabelFps = "帧率显示";
constexpr const char* LabelUiScale = "界面缩放";
constexpr const char* LabelConsoleMask = "控制台遮罩";
constexpr const char* LabelConsoleFont = "控制台字号";
constexpr const char* LabelConsoleHistory = "控制台历史";
constexpr const char* LabelTheme = "主题";
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
constexpr const char* LabelSound          = "音效";
constexpr const char* LabelSoundVolume    = "音效音量";
constexpr const char* LabelAutoPause      = "失焦自动暂停";
constexpr const char* LabelFpsFormat      = "FPS 格式";
constexpr const char* LabelConsolePrompt  = "控制台提示符";

constexpr const char* LabelShowColliders = "显示碰撞盒";
constexpr const char* LabelScreenShake   = "屏幕震动";
constexpr const char* LabelParticles     = "粒子效果";
// ===== 新 Tab =====
constexpr const char* TabGraphics = "画面";
constexpr const char* TabAudioLog = "音频";
constexpr const char* TabGame     = "游戏";

// ===== 新设置项 =====
constexpr const char* LabelPseudo3D        = "伪 3D 效果";
constexpr const char* LabelParallax        = "视差背景";
constexpr const char* LabelPlayerAnimation = "玩家动画";
constexpr const char* LabelLevelIntro      = "关卡开场";
constexpr const char* LabelBGM             = "背景音乐";
// ===== 关卡选择 =====
constexpr const char* LevelSelect      = "选关";
constexpr const char* LevelSelectTitle = "选择关卡";
constexpr const char* NoSaveHint       = "没有存档，请先创建存档";
constexpr const char* SaveLabel        = "当前存档: ";
constexpr const char* LockedHint       = "未解锁";

} // namespace Str