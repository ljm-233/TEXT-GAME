#include "doctest.h"
#include "stats.h"

// 成绩/统计页的纯聚合逻辑。
//
// 界面本身（StatsScene）构造要 sf::Font，无头环境测不了；所以把"存档 → 每关一行
// + 总计"这段搬出来测。这里最容易写错、也最值得钉住的是**缺数据时怎么办**：
// 老存档没有 levelBestCoins、手改坏的存档数组短一截、星级被改成 7 之类。

namespace {

SaveInfo makeSave(std::vector<int> stars, std::vector<float> times,
                  std::vector<int> coins) {
    SaveInfo s;
    s.filename = "t.conf";
    s.levelStars = std::move(stars);
    s.levelBestTimes = std::move(times);
    s.levelBestCoins = std::move(coins);
    return s;
}

} // namespace

TEST_CASE("成绩 - 全新存档：有一行一关，但什么都没记录") {
    const auto sum = Stats::summarize(makeSave({}, {}, {}));

    CHECK(sum.levels.empty());
    CHECK(sum.clearedLevels == 0);
    CHECK(sum.totalStars == 0);
    CHECK(sum.maxStars == 0);
    CHECK(sum.anyRecord == false);
    CHECK(sum.progress() == doctest::Approx(0.f)); // 没有关卡也不能除零
}

TEST_CASE("成绩 - 九关全空：九行、满星 27、进度 0") {
    const auto sum = Stats::summarize(makeSave(std::vector<int>(9, 0),
                                               std::vector<float>(9, 0.f),
                                               std::vector<int>(9, 0)));

    CHECK(sum.levels.size() == 9);
    CHECK(sum.maxStars == 27);
    CHECK(sum.clearedLevels == 0);
    CHECK(sum.anyRecord == false);
    CHECK(sum.progress() == doctest::Approx(0.f));
    CHECK(sum.levels[0].level == 1); // 行号从 1 开始（关卡序号也是）
    CHECK(sum.levels[8].level == 9);
}

TEST_CASE("成绩 - 部分通关：总计按有记录的累加") {
    auto save = makeSave({3, 2, 0, 1, 0, 0, 0, 0, 0},
                         {10.f, 20.f, 0.f, 30.f, 0.f, 0.f, 0.f, 0.f, 0.f},
                         {5, 0, 0, 7, 0, 0, 0, 0, 0});
    const auto sum = Stats::summarize(save);

    CHECK(sum.clearedLevels == 3); // 星级 > 0 的三关
    CHECK(sum.totalStars == 6);
    CHECK(sum.maxStars == 27);
    // 没打过的那几关是 0，不该把总时间拉低（只累加有记录的）
    CHECK(sum.totalBestTime == doctest::Approx(60.f));
    CHECK(sum.totalBestCoins == 12);
    CHECK(sum.anyRecord == true);
    CHECK(sum.progress() == doctest::Approx(3.f / 9.f));

    CHECK(sum.levels[0].cleared == true);
    CHECK(sum.levels[2].cleared == false);
    CHECK(sum.levels[2].bestTime == doctest::Approx(0.f));
}

TEST_CASE("成绩 - 数组长度不齐时按最长的算，缺的当 0") {
    // 老存档：有星级、有时间，但没有 best coins（那个字段是新加的）
    const auto sum = Stats::summarize(makeSave({3, 1, 0, 0, 0, 0, 0, 0, 0},
                                               {10.f, 20.f}, {5}));

    CHECK(sum.levels.size() == 9); // 以最长的 levelStars 为准
    CHECK(sum.levels[0].bestCoins == 5);
    CHECK(sum.levels[1].bestCoins == 0); // 越过了 coins 数组
    CHECK(sum.levels[1].bestTime == doctest::Approx(20.f));
    CHECK(sum.levels[4].bestTime == doctest::Approx(0.f)); // 越过了 times 数组
    CHECK(sum.levels[4].stars == 0);
}

TEST_CASE("成绩 - 星级越界会被夹回 0~3，不会算出 100% 以上") {
    // 手改坏 / 未来版本加了更多星级：进度条不能算出 >100%
    const auto sum = Stats::summarize(makeSave({7, -2, 3}, {}, {}));

    CHECK(sum.levels[0].stars == 3);
    CHECK(sum.levels[1].stars == 0);
    CHECK(sum.levels[2].stars == 3);
    CHECK(sum.totalStars == 6);
    CHECK(sum.clearedLevels == 2);
    CHECK(sum.progress() <= 1.f);
}

TEST_CASE("成绩 - 只有最佳时间没有星级也算有记录") {
    // 理论上不会出现（通关才有时间），但存档是可以被手改的
    const auto sum = Stats::summarize(makeSave({}, {12.f}, {}));

    CHECK(sum.levels.size() == 1);
    CHECK(sum.clearedLevels == 0);
    CHECK(sum.anyRecord == true); // 有记录，界面不该显示"还没玩过"
    CHECK(sum.totalBestTime == doctest::Approx(12.f));
}

TEST_CASE("成绩 - 只有金币也算有记录") {
    const auto sum = Stats::summarize(makeSave({}, {}, {4}));
    CHECK(sum.levels.size() == 1);
    CHECK(sum.totalBestCoins == 4);
    CHECK(sum.anyRecord == true);
    CHECK(sum.clearedLevels == 0);
}
