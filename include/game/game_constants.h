#pragma once

// ============================================================
// 游戏物理常量
// ============================================================
//
// 命名空间是 GameConst，唯一的读者是 game/ 层与需要知道物理参数的
// scene/ 层。**音频采样率曾经也放在这里**，结果 ui 层的 sound_manager.cpp
// 为了一个常量反向 include 了 game 层 —— 那是个把"设备的属性"
// 错当成"游戏的属性"的例子。采样率现在归音频模块自己（sound_manager.cpp）。
//
namespace GameConst {

// ===== 玩家 =====
constexpr float kPlayerGravity = 2200.f;
constexpr float kPlayerMoveSpeed = 300.f;
constexpr float kPlayerJumpVelocity = -720.f;
constexpr float kPlayerMaxFall = 1000.f;
constexpr float kPlayerCoyoteTime = 0.10f;
constexpr float kPlayerJumpBuffer = 0.12f;
constexpr float kPlayerBounceSpeed = -500.f;
constexpr float kPlayerInvincibleDur = 1.5f;

// ===== 敌人 =====
constexpr float kEnemySpeed = 90.f;

// ===== 物理 =====
constexpr float kFixedTimeStep = 1.f / 120.f;
constexpr float kStompTolerance = 12.f; // 踩敌人的判定容差

} // namespace GameConst