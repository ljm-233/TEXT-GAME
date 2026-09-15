#include "doctest.h"
#include "vec2.h"
#include <cmath>

TEST_CASE("Vec2 - 基础运算") {
    Vec2 a{1.f, 2.f};
    Vec2 b{3.f, 4.f};

    SUBCASE("加法") {
        Vec2 c = a + b;
        CHECK(c.x == 4.f);
        CHECK(c.y == 6.f);
    }

    SUBCASE("减法") {
        Vec2 c = b - a;
        CHECK(c.x == 2.f);
        CHECK(c.y == 2.f);
    }

    SUBCASE("缩放") {
        Vec2 c = a * 2.f;
        CHECK(c.x == 2.f);
        CHECK(c.y == 4.f);
    }

    SUBCASE("复合赋值") {
        Vec2 c = a;
        c += b;
        CHECK(c.x == 4.f);
        CHECK(c.y == 6.f);
    }
}

TEST_CASE("Vec2 - 长度") {
    Vec2 v{3.f, 4.f};
    CHECK(v.lengthSq() == 25.f);
    CHECK(v.length() == 5.f);
}

TEST_CASE("Vec2 - 点积") {
    Vec2 a{1.f, 0.f};
    Vec2 b{0.f, 1.f};
    CHECK(dot(a, b) == 0.f);

    Vec2 c{2.f, 3.f};
    Vec2 d{4.f, 5.f};
    CHECK(dot(c, d) == 23.f);
}