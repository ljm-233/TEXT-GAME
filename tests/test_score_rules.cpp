#include "doctest.h"
#include "game/score_rules.h"

#include <initializer_list>

// 星级与目标时间规则。
//
// 这段逻辑原先在 GameScene 的私有方法里，而 GameScene 构造需要 sf::Font
// （GlResource），所以一直测不到。它是纯算术，抽出来之后就能直接测了。
//
// 这里重点钉两条容易反着写的语义：
//   - 目标时间与金币数**正相关**（金币多 = 给的时间多）
//   - 没金币的关卡"集齐"天然成立，不超时就是 3 星

TEST_CASE("ScoreRules - 目标时间随金币数增加") {
    CHECK(ScoreRules::targetTimeSeconds(0) == 30);    // 基础时间
    CHECK(ScoreRules::targetTimeSeconds(1) == 33);
    CHECK(ScoreRules::targetTimeSeconds(10) == 60);

    // 单调递增：金币越多，容错时间越长
    for (int coins = 0; coins < 40; ++coins)
        CHECK(ScoreRules::targetTimeSeconds(coins + 1) >
              ScoreRules::targetTimeSeconds(coins));
}

TEST_CASE("ScoreRules - 金币数为负时按 0 处理") {
    CHECK(ScoreRules::targetTimeSeconds(-5) == ScoreRules::targetTimeSeconds(0));
}

TEST_CASE("ScoreRules - 集齐且达标 = 3 星") {
    // 10 枚金币 -> 目标 60 秒
    CHECK(ScoreRules::starsFor(10, 10, 10.f) == 3);
    CHECK(ScoreRules::starsFor(10, 10, 59.9f) == 3);
}

TEST_CASE("ScoreRules - 正好卡在目标时间算达标") {
    const int target = ScoreRules::targetTimeSeconds(10);   // 60
    CHECK(ScoreRules::starsFor(10, 10, static_cast<float>(target)) == 3);
    // 超出一丁点就不算
    CHECK(ScoreRules::starsFor(10, 10, static_cast<float>(target) + 0.01f) == 2);
}

TEST_CASE("ScoreRules - 只满足一项 = 2 星") {
    // 集齐了但超时
    CHECK(ScoreRules::starsFor(10, 10, 999.f) == 2);
    // 没集齐但很快
    CHECK(ScoreRules::starsFor(3, 10, 5.f) == 2);
}

TEST_CASE("ScoreRules - 两项都不满足 = 1 星") {
    CHECK(ScoreRules::starsFor(3, 10, 999.f) == 1);
    CHECK(ScoreRules::starsFor(0, 10, 999.f) == 1);
}

TEST_CASE("ScoreRules - 没有金币的关卡：不超时就是 3 星") {
    // coins == totalCoins == 0，集齐天然成立
    CHECK(ScoreRules::starsFor(0, 0, 1.f) == 3);
    CHECK(ScoreRules::starsFor(0, 0, 30.f) == 3);
    // 超时则掉到 2 星（集齐仍成立）
    CHECK(ScoreRules::starsFor(0, 0, 31.f) == 2);
}

TEST_CASE("ScoreRules - 星级恒在 1~3 之间") {
    for (int total = 0; total <= 12; ++total) {
        for (int got = 0; got <= total; ++got) {
            for (float t : {0.f, 5.f, 100.f, 10000.f}) {
                const int s = ScoreRules::starsFor(got, total, t);
                CHECK(s >= 1);
                CHECK(s <= 3);
            }
        }
    }
}

// ============================================================
// PB（个人最佳）
// ============================================================

TEST_CASE("ScoreRules - 0 或负数视为无记录，一定算刷新") {
    CHECK(ScoreRules::isNewRecord(0.f, 100.f));
    CHECK(ScoreRules::isNewRecord(-1.f, 100.f));
}

TEST_CASE("ScoreRules - 只有更快才算刷新 PB") {
    CHECK(ScoreRules::isNewRecord(10.f, 9.9f));       // 更快
    CHECK_FALSE(ScoreRules::isNewRecord(10.f, 10.f)); // 一样快不算
    CHECK_FALSE(ScoreRules::isNewRecord(10.f, 10.1f));// 更慢不算
}
