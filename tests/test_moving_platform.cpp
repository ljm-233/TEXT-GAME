#include "doctest.h"
#include "moving_platform.h"
#include "level.h"

TEST_CASE("MovingPlatform - 水平往返") {
    Level level;   // 空关卡（宽高都是 0）

    MovingPlatform mp({100.f, 100.f}, 32, 200.f, true, 100.f);

    // 初始位置
    AABB b0 = mp.bounds();
    CHECK(b0.x == 100.f);

    // 前进 1 秒（应该走 100 像素）
    mp.update(1.f, level);
    AABB b1 = mp.bounds();
    CHECK(b1.x > b0.x);

    // 继续走，应该到达右边界后反向
    for (int i = 0; i < 30; ++i) {
        mp.update(0.1f, level);
    }
    AABB b2 = mp.bounds();
    // 位置应该在 [100, 300] 范围内
    CHECK(b2.x >= 100.f - 0.01f);
    CHECK(b2.x <= 300.f + 0.01f);
}

TEST_CASE("MovingPlatform - 垂直往返") {
    Level level;

    MovingPlatform mp({100.f, 100.f}, 32, 150.f, false, 80.f);

    AABB b0 = mp.bounds();
    CHECK(b0.y == 100.f);

    mp.update(1.f, level);
    AABB b1 = mp.bounds();
    CHECK(b1.y > b0.y);

    for (int i = 0; i < 30; ++i) {
        mp.update(0.1f, level);
    }
    AABB b2 = mp.bounds();
    CHECK(b2.y >= 100.f - 0.01f);
    CHECK(b2.y <= 250.f + 0.01f);
}

TEST_CASE("MovingPlatform - lastDelta") {
    Level level;
    MovingPlatform mp({100.f, 100.f}, 32, 500.f, true, 100.f);

    // 初始 lastDelta 为 0
    Vec2 d0 = mp.lastDelta();
    CHECK(d0.x == 0.f);
    CHECK(d0.y == 0.f);

    // 前进 1 秒后，lastDelta 应该接近 (10, 0)（每帧 0.1 秒 × 100 像素/秒）
    // 但 update(1.f) 是一次性走 100 像素
    mp.update(1.f, level);
    Vec2 d1 = mp.lastDelta();
    CHECK(d1.x == doctest::Approx(100.f).epsilon(0.01));
}