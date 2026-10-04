#pragma once
//
// 星级与目标时间的规则。
//
// 这段逻辑原先埋在 GameScene 的私有方法里。它是纯算术，却因为宿主类构造
// 需要 sf::Font（GlResource）而完全测不到 —— 属于"改一个数字就会影响
// 所有关卡手感、却没有任何东西守着"的那类代码。抽成纯函数之后可以脱离界面测。
//
namespace ScoreRules {

/// 基础时间（秒）
inline constexpr float kBaseTime = 30.f;

/// 每枚金币额外给的时间（秒）—— 集齐金币本身要多绕路
inline constexpr float kPerCoinTime = 3.f;

/// 三星目标时间（秒）。
///
/// 注意**与金币数正相关**：金币越多，容错时间越长。
/// 直觉上容易反着写（以为金币多应该更严格）。
int targetTimeSeconds(int totalCoins);

/// 按"集齐金币 + 时间达标"给星，取值 1~3：
///
///     两项都满足 -> 3 星
///     满足任意一项 -> 2 星
///     都不满足 -> 1 星
///
/// 没有金币的关卡（coins == totalCoins == 0）"集齐"天然成立，
/// 所以只要不超时就是 3 星。
int starsFor(int coins, int totalCoins, float levelTime);

/// 是否刷新了个人最佳（0 或负数视为无记录）。
bool isNewRecord(float previousBest, float levelTime);

} // namespace ScoreRules
