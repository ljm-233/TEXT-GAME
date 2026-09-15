#include "doctest.h"
#include "camera.h"

TEST_CASE("Camera - 初始化") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);

    cam.snapTo({500.f, 500.f});

    Vec2 p = cam.position();
    // 目标在屏幕中心：(500 - 640, 500 - 360) = (-140, 140)
    // 但 x 会被 clamp 到 [0, 3000-1280=1720]，所以 x = 0
    CHECK(p.x == 0.f);
    CHECK(p.y == 140.f);
}

TEST_CASE("Camera - 边界夹紧（右边界）") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);

    cam.snapTo({2900.f, 500.f});

    Vec2 p = cam.position();
    // 目标在右边界，应该夹到 3000 - 1280 = 1720
    CHECK(p.x == 1720.f);
}

TEST_CASE("Camera - 关卡比视口小（居中）") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(800.f, 400.f);

    cam.snapTo({400.f, 200.f});

    Vec2 p = cam.position();
    // (800 - 1280) / 2 = -240
    // (400 - 720) / 2 = -160
    CHECK(p.x == -240.f);
    CHECK(p.y == -160.f);
}

TEST_CASE("Camera - 中心点计算") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);
    cam.snapTo({1000.f, 500.f});

    Vec2 c = cam.center();
    CHECK(c.x == cam.position().x + 640.f);
    CHECK(c.y == cam.position().y + 360.f);
}

TEST_CASE("Camera - 震动衰减") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);
    cam.snapTo({1000.f, 500.f});

    // 触发震动
    cam.shake(10.f, 0.3f);
    cam.updateShake(0.01f);
    // 震动中，effectivePosition 可能偏离 position，但不做精确断言（有随机性）

    // 时间过去足够久，震动应完全结束
    cam.updateShake(1.f);
    Vec2 eff = cam.effectivePosition();
    CHECK(eff.x == cam.position().x);
    CHECK(eff.y == cam.position().y);
}