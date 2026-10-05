#pragma once
//
// 焦点导航：手柄方向键在"一堆矩形按钮"之间怎么走。
//
// 从 FocusGroup 里抽出来的。原先它和 Button*、按键重复触发状态、待分发事件
// 挤在同一个类里，而 Button 的构造需要 sf::Font（也就是 GlResource），
// 整个 FocusGroup 因此在无界面环境里根本构造不了，算法再好也测不到。
//
// 剩下的这部分只依赖矩形几何，搬出来就是纯函数。
// 手感全靠它：每个场景每个设置 Tab 的手柄导航都走这里。
//
#include "infrastructure/gamepad_config.h"
#include <SFML/Graphics/Rect.hpp>
#include <vector>

namespace FocusNav {

enum class Direction { Up, Down, Left, Right };

/// 按住方向键的重复时序（秒）。数值取自 GamepadConfig（手柄手感参数的唯一
/// 来源），这里只是给它起一个在"重复触发"语境下读得懂的名字。
inline constexpr float kRepeatDelay = GamepadConfig::kNavInitialDelay;
inline constexpr float kRepeatRate = GamepadConfig::kNavRepeatEvery;

/// 方向键/摇杆"按住重复"的边沿 + 节拍状态机：
/// 第一次按下立刻触发一次，之后先等 kRepeatDelay，再按 kRepeatRate 重复。
///
/// 从 FocusGroup::handleDirection 原样搬出来的（数值与判定次序都没动）。
/// 它不碰 Button，所以能脱离 GL 直接测 —— 而"按一下方向键焦点到底走没走、
/// 走对了几次"恰恰是导航里最该被测住的部分。
struct Repeater {
    bool held = false; ///< 上一帧是否按住（= 原来的 last*）
    float timer = 0.f; ///< 倒计时：负数=还在等（-timer 秒后触发），累加 ≥ 0 就触发一次

    /// 推进一帧，返回本帧是否应当移动一次
    bool tick(bool now, float dt);
};

/// 主方向投影的阈值（像素）。小于它的候选算作"不在这个方向上"，
/// 避免正上/正下方的按钮被当成左右邻居。
inline constexpr float kMinProjection = 2.f;

/// 垂直偏移的惩罚权重。比 1 大得多，于是"同一行/同一列"的按钮优先于
/// "斜前方更近"的按钮。
inline constexpr float kPerpWeight = 3.f;

/// 几何导航：从 `cur` 出发，沿 `dir` 找"该方向上最近"的那个，返回其下标。
///
/// 规则（顺序即优先级）：
///   1. 尺寸非正（x 或 y ≤ 0）的矩形跳过 —— 那是还没布局的按钮
///   2. 主方向投影 < kMinProjection 的跳过 —— 它不在这个方向上
///   3. 剩下的按 `投影 + 垂直偏移 × kPerpWeight` 打分，取最小
///
/// 该方向上没有候选时返回 **-1**（由调用方决定停在原地还是绕回，
/// 当前 FocusGroup 是停在原地）。
///
/// `cur` 越界或 `rects` 为空时同样返回 -1。
int nextInDirection(const std::vector<sf::FloatRect>& rects, int cur, Direction dir);

/// 线性导航：在长度为 `count` 的列表里前后移动 `delta` 步，越界绕回。
///
/// `count ≤ 0` 时返回 -1。
int nextLinear(int cur, int delta, int count);

} // namespace FocusNav
