#include "doctest.h"
#include "aabb.h"

TEST_CASE("AABB - 边缘") {
    AABB box{10.f, 20.f, 30.f, 40.f};
    CHECK(box.left()   == 10.f);
    CHECK(box.right()  == 40.f);
    CHECK(box.top()    == 20.f);
    CHECK(box.bottom() == 60.f);

    Vec2 c = box.center();
    CHECK(c.x == 25.f);
    CHECK(c.y == 40.f);
}

TEST_CASE("AABB - 相交") {
    AABB a{0.f, 0.f, 10.f, 10.f};

    SUBCASE("完全重叠") {
        AABB b{2.f, 2.f, 5.f, 5.f};
        CHECK(a.intersects(b));
        CHECK(b.intersects(a));
    }

    SUBCASE("部分重叠") {
        AABB b{5.f, 5.f, 10.f, 10.f};
        CHECK(a.intersects(b));
    }

    SUBCASE("分离") {
        AABB b{20.f, 20.f, 5.f, 5.f};
        CHECK_FALSE(a.intersects(b));
    }

    SUBCASE("边缘接触不算相交") {
        AABB b{10.f, 0.f, 5.f, 5.f};   // 左边缘 = a 的右边缘
        CHECK_FALSE(a.intersects(b));
    }

    SUBCASE("角重叠") {
        AABB b{9.f, 9.f, 5.f, 5.f};
        CHECK(a.intersects(b));
    }
}