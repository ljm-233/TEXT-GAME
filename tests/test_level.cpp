#include "doctest.h"
#include "level.h"

TEST_CASE("Level - 从字符串加载") {
    const char* text = R"(
#####
#P  #
#   #
#  G#
#####
)";

    Level level;
    REQUIRE(level.loadFromString(text));

    CHECK(level.width()  == 5);
    CHECK(level.height() == 5);

    SUBCASE("实体瓦片") {
        CHECK(level.isSolid(0, 0));   // 左上角墙
        CHECK(level.isSolid(4, 4));   // 右下角墙
        CHECK_FALSE(level.isSolid(2, 2));  // 中间空地
    }

    SUBCASE("出生点") {
        Vec2 p = level.playerSpawn();
        CHECK(p.x == 32.f);   // 第 1 列
        CHECK(p.y == 32.f);   // 第 1 行
    }

    SUBCASE("终点") {
        CHECK(level.hasGoal());
        Vec2 g = level.goalPos();
        CHECK(g.x == 96.f);   // 第 3 列
        CHECK(g.y == 96.f);   // 第 3 行
    }
}

TEST_CASE("Level - 敌人和金币") {
    const char* text = R"(
#######
#P  C #
#  E  #
#  C  #
#    G#
#######
)";

    Level level;
    REQUIRE(level.loadFromString(text));

    CHECK(level.enemySpawns().size() == 1);
    CHECK(level.coinSpawns().size() == 2);
    CHECK(level.hasGoal());
}

TEST_CASE("Level - 空地图") {
    Level level;
    CHECK_FALSE(level.loadFromString(""));
}

TEST_CASE("Level - 不等长行") {
    // 第二行更长，应该按最长的算宽度
    const char* text =
        "#####\n"
        "#P     #\n"
        "#####\n";

    Level level;
    REQUIRE(level.loadFromString(text));
    CHECK(level.width() == 7);
    CHECK(level.height() == 3);
}