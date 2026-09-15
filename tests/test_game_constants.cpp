#include "doctest.h"
#include "game_constants.h"

TEST_CASE("GameConst - 重力方向") {
    // 重力应该向下（正值，因为 y 轴向下）
    CHECK(GameConst::kPlayerGravity > 0.f);
}

TEST_CASE("GameConst - 跳跃速度方向") {
    // 跳跃速度应该向上（负值）
    CHECK(GameConst::kPlayerJumpVelocity < 0.f);
}

TEST_CASE("GameConst - 跳跃足够克服重力") {
    // 跳跃初速度的平方 / (2 * 重力) = 最大跳跃高度
    // 应该能跳到至少 2 格（64 像素）
    float jumpHeight = (GameConst::kPlayerJumpVelocity *
                        GameConst::kPlayerJumpVelocity) /
                       (2.f * GameConst::kPlayerGravity);
    CHECK(jumpHeight > 64.f);
}

TEST_CASE("GameConst - 土狼时间合理") {
    // 0.05 ~ 0.3 秒
    CHECK(GameConst::kPlayerCoyoteTime > 0.05f);
    CHECK(GameConst::kPlayerCoyoteTime < 0.3f);
}

TEST_CASE("GameConst - 固定步长合理") {
    CHECK(GameConst::kFixedTimeStep > 0.f);
    CHECK(GameConst::kFixedTimeStep < 0.05f);   // 小于 50ms
}

TEST_CASE("GameConst - 音效采样率") {
    CHECK(GameConst::kSoundSampleRate == 44100u);
}

TEST_CASE("GameConst - 弹跳是轻轻反弹（比跳跃矮）") {
    // 数值更小 = 弹得更矮
    // -500 > -720，所以反弹比跳跃矮
    CHECK(GameConst::kPlayerBounceSpeed > GameConst::kPlayerJumpVelocity);
    // 但仍应该向上（负值）
    CHECK(GameConst::kPlayerBounceSpeed < 0.f);
}