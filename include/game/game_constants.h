#pragma once

// ============================================================
// 游戏物理常量
// ============================================================

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

// ===== 音效 =====
constexpr unsigned kSoundSampleRate = 44100;

} // namespace GameConst