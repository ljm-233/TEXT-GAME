#pragma once
#include <optional>
#include <string>
#include <vector>

/// 设置页的身份与归属。
///
/// 把这个枚举和"哪一页管哪些键"的表从 SettingsScene 里抽出来，是因为有**三处**
/// 需要它，而且必须完全一致：
///   1. SettingsScene 自己：建 Tab 按钮、分发事件与渲染
///   2. 「恢复本页默认」：把本页的键从配置里删掉（默认值只存在于读取处，
///      删掉之后自然回落 —— 见 Config::resetKeys）
///   3. 冒烟测试：点某一页的控件，只能改这一页的键。以前控件靠数字下标引用，
///      搬一行就可能"点这个改了那个"，而且完全不报错
///
/// ⚠️ 顺序变了要同步 SettingsScene 的 tabLabels 与逐项 setText
///    （那里有 static_assert 挡着数量，但挡不住顺序）。
enum class SettingsTab {
    Display = 0,
    Interface,
    Wallpaper,
    Graphics,
    Audio,
    Controls,
    Game,
    Console,
    Advanced,
};

/// Tab 总数。
///
/// ⚠️ **刻意不放进枚举**：放进去了每个 switch 都得补一个 `case Count:`
///    才能过 -Wswitch，而那只是噪音 —— 更糟的是它会让"加了新 Tab 却忘了
///    处理"这个错误**不再报警**（因为 Count 把 switch 补全了）。
///    独立常量 + 穷尽 switch 才是对的组合。
inline constexpr int kSettingsTabCount = 9;

/// Tab 按钮上的文字（返回 Str 常量，随语言切换）
const char* settingsTabLabel(SettingsTab t);

/// 这一页负责哪些配置键（用于「恢复本页默认」与归属检查）
const std::vector<const char*>& keysForTab(SettingsTab t);

/// 这个键归哪一页；找不到返回 nullopt
std::optional<SettingsTab> tabForKey(const std::string& key);

/// **有意**不属于任何设置页的键。
///
/// 存在的意义是让"忘了归类"和"有意不归类"能被区分开 ——
/// 否则测试只能二者都不报，或者把有意的也一起报成错。
/// 目前只有窗口几何状态：那是窗口逻辑自己写的，不是用户能设的东西。
const std::vector<const char*>& unownedKeys();
