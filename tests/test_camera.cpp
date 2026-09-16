#include "doctest.h"
#include "camera.h"

TEST_CASE("Camera - 初始化") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);

    cam.snapTo({500.f, 500.f});

    Vec2 p = cam.position();
    CHECK(p.x == 0.f);
    CHECK(p.y == 140.f);
}

TEST_CASE("Camera - 边界夹紧（右边界）") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);

    cam.snapTo({2900.f, 500.f});

    Vec2 p = cam.position();
    CHECK(p.x == 1720.f);
}

TEST_CASE("Camera - 关卡比视口小（居中）") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(800.f, 400.f);

    cam.snapTo({400.f, 200.f});

    Vec2 p = cam.position();
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

    cam.shake(10.f, 0.3f);
    cam.updateShake(0.01f);

    cam.updateShake(1.f);
    Vec2 eff = cam.effectivePosition();
    CHECK(eff.x == cam.position().x);
    CHECK(eff.y == cam.position().y);
}

TEST_CASE("Camera - 死区：小移动不触发镜头跟随") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);
    cam.snapTo({1000.f, 500.f});

    // 目标静止
    for (int i = 0; i < 30; ++i) {
        cam.follow({1000.f, 500.f}, {0.f, 0.f}, 0.1f);
    }
    Vec2 p0 = cam.position();

    // 目标移动 50 像素（小于死区宽度 192/2 = 96）
    for (int i = 0; i < 30; ++i) {
        cam.follow({1050.f, 500.f}, {0.f, 0.f}, 0.1f);
    }
    Vec2 p1 = cam.position();

    // 死区内的移动不应该导致镜头大幅移动
    CHECK(std::abs(p1.x - p0.x) < 20.f);
}

TEST_CASE("Camera - 前瞻：向右跑镜头右偏") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);
    cam.snapTo({1000.f, 500.f});

    // 目标向右移动，速度 300
    for (int i = 0; i < 60; ++i) {
        cam.follow({1000.f + i * 5.f, 500.f}, {300.f, 0.f}, 0.05f);
    }
    float xRight = cam.position().x;

    // 重置
    cam.snapTo({1000.f, 500.f});

    // 目标向左移动
    for (int i = 0; i < 60; ++i) {
        cam.follow({1000.f - i * 5.f, 500.f}, {-300.f, 0.f}, 0.05f);
    }
    float xLeft = cam.position().x;

    // 向右跑时镜头比向左跑时靠右
    CHECK(xRight > xLeft);
}

TEST_CASE("Camera - follow 不崩") {
    Camera cam;
    cam.setViewSize(1280.f, 720.f);
    cam.setLevelBounds(3000.f, 1000.f);
    cam.snapTo({1000.f, 500.f});

    for (int i = 0; i < 100; ++i) {
        cam.follow({1000.f + static_cast<float>(i), 500.f},
                   {100.f, 0.f}, 0.016f);
    }
    // 不崩就算过
    CHECK(true);
}