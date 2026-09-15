#include "doctest.h"
#include "jump_pad.h"
#include "level.h"

TEST_CASE("JumpPad - 初始可触发") {
    JumpPad pad({100.f, 100.f}, 32);
    CHECK(pad.canTrigger());
}

TEST_CASE("JumpPad - 触发后进入冷却") {
    JumpPad pad({100.f, 100.f}, 32);
    pad.trigger();
    CHECK_FALSE(pad.canTrigger());
}

TEST_CASE("JumpPad - 冷却结束后可再次触发") {
    JumpPad pad({100.f, 100.f}, 32);
    Level level;

    pad.trigger();
    CHECK_FALSE(pad.canTrigger());

    // 冷却 0.3 秒，等 0.4 秒
    pad.update(0.4f, level);
    CHECK(pad.canTrigger());
}

TEST_CASE("JumpPad - 冷却期间不可重复触发") {
    JumpPad pad({100.f, 100.f}, 32);
    Level level;

    pad.trigger();
    pad.update(0.1f, level);
    CHECK_FALSE(pad.canTrigger());

    pad.update(0.1f, level);
    CHECK_FALSE(pad.canTrigger());

    pad.update(0.15f, level);   // 总共 0.35 秒，超过 0.3
    CHECK(pad.canTrigger());
}

TEST_CASE("JumpPad - 碰撞盒覆盖上方一格") {
    JumpPad pad({100.f, 100.f}, 32);
    AABB b = pad.bounds();

    // 盒子的 top 应该在 y = 100 - 32 = 68
    CHECK(b.y == 68.f);
    // 高度应该是 tileSize * 2 = 64
    CHECK(b.h == 64.f);
    // 宽度是 tileSize = 32
    CHECK(b.w == 32.f);
}

TEST_CASE("JumpPad - 弹跳速度足够") {
    // 弹跳速度应该比跳跃速度更快
    CHECK(JumpPad::kLaunchSpeed < -1000.f);
}