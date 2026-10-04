#include "doctest.h"
#include "ui/focus_nav.h"

#include <vector>

// 手柄焦点导航的算法。
//
// 从 FocusGroup 里抽出来的：那个类持有 Button*，而 Button 构造需要 sf::Font
// （GlResource），在无界面环境里根本构造不了，算法再好也测不到。
// 抽出来之后只剩矩形几何，纯函数。
//
// 这段决定手感 —— 每个场景、每个设置 Tab 的手柄导航都走它。
namespace {

using FocusNav::Direction;

sf::FloatRect rect(float x, float y, float w = 100.f, float h = 40.f) {
    return sf::FloatRect({x, y}, {w, h});
}

/// 2×2 网格：
///     [0] (0,0)     [1] (200,0)
///     [2] (0,100)   [3] (200,100)
std::vector<sf::FloatRect> grid2x2() {
    return {rect(0.f, 0.f), rect(200.f, 0.f),
            rect(0.f, 100.f), rect(200.f, 100.f)};
}

} // namespace

// ============================================================
// 几何导航
// ============================================================

TEST_CASE("FocusNav - 网格里四个方向都走到正确的邻居") {
    const auto g = grid2x2();

    CHECK(FocusNav::nextInDirection(g, 0, Direction::Right) == 1);
    CHECK(FocusNav::nextInDirection(g, 0, Direction::Down)  == 2);
    CHECK(FocusNav::nextInDirection(g, 1, Direction::Left)  == 0);
    CHECK(FocusNav::nextInDirection(g, 1, Direction::Down)  == 3);
    CHECK(FocusNav::nextInDirection(g, 2, Direction::Up)    == 0);
    CHECK(FocusNav::nextInDirection(g, 2, Direction::Right) == 3);
    CHECK(FocusNav::nextInDirection(g, 3, Direction::Up)    == 1);
    CHECK(FocusNav::nextInDirection(g, 3, Direction::Left)  == 2);
}

TEST_CASE("FocusNav - 该方向上没有按钮时返回 -1（停在原地）") {
    const auto g = grid2x2();

    CHECK(FocusNav::nextInDirection(g, 0, Direction::Up)    == -1);   // 已经在顶上
    CHECK(FocusNav::nextInDirection(g, 0, Direction::Left)  == -1);   // 已经在最左
    CHECK(FocusNav::nextInDirection(g, 3, Direction::Right) == -1);
    CHECK(FocusNav::nextInDirection(g, 3, Direction::Down)  == -1);
}

TEST_CASE("FocusNav - 同一行/列的按钮优先于斜前方的") {
    // [0] 在原点；[1] 正右方；[2] 斜右下方但直线距离更近
    std::vector<sf::FloatRect> r = {
        rect(0.f, 0.f),
        rect(300.f, 0.f),      // 正右，主方向 300，垂直 0
        rect(120.f, 90.f),     // 斜右下，直线更近但垂直偏移大
    };

    // 往右应该选正右方那个
    CHECK(FocusNav::nextInDirection(r, 0, Direction::Right) == 1);
}

TEST_CASE("FocusNav - 同方向多个候选时取评分最低的") {
    std::vector<sf::FloatRect> r = {
        rect(0.f, 0.f),
        rect(400.f, 0.f),    // 远
        rect(150.f, 0.f),    // 近 —— 应该选它
    };

    CHECK(FocusNav::nextInDirection(r, 0, Direction::Right) == 2);
}

TEST_CASE("FocusNav - 未布局的按钮（尺寸为 0）被跳过") {
    std::vector<sf::FloatRect> r = {
        rect(0.f, 0.f),
        rect(150.f, 0.f, 0.f, 0.f),   // 还没布局
        rect(400.f, 0.f),
    };

    // 跳过尺寸为 0 的那个，选更远但有效的
    CHECK(FocusNav::nextInDirection(r, 0, Direction::Right) == 2);
}

TEST_CASE("FocusNav - 垂直偏移在阈值内的算「同一行」") {
    // 正右方与"几乎正右方"（偏移 < 阈值）—— 应该仍能选中
    std::vector<sf::FloatRect> r = {
        rect(0.f, 0.f),
        rect(200.f, 1.f),   // 中心差不多同高
    };

    CHECK(FocusNav::nextInDirection(r, 0, Direction::Right) == 1);
}

TEST_CASE("FocusNav - 重叠/同心的按钮不会被选中") {
    std::vector<sf::FloatRect> r = {
        rect(0.f, 0.f),
        rect(0.f, 0.f),     // 完全重合：投影为 0，小于阈值
    };

    CHECK(FocusNav::nextInDirection(r, 0, Direction::Right) == -1);
    CHECK(FocusNav::nextInDirection(r, 0, Direction::Down) == -1);
}

TEST_CASE("FocusNav - 只有一个按钮时任何方向都返回 -1") {
    std::vector<sf::FloatRect> r = {rect(0.f, 0.f)};

    CHECK(FocusNav::nextInDirection(r, 0, Direction::Up)    == -1);
    CHECK(FocusNav::nextInDirection(r, 0, Direction::Down)  == -1);
    CHECK(FocusNav::nextInDirection(r, 0, Direction::Left)  == -1);
    CHECK(FocusNav::nextInDirection(r, 0, Direction::Right) == -1);
}

// ============================================================
// 非法输入
// ============================================================

TEST_CASE("FocusNav - 空列表返回 -1 而不是崩") {
    const std::vector<sf::FloatRect> empty;

    CHECK(FocusNav::nextInDirection(empty, 0, Direction::Up) == -1);
}

TEST_CASE("FocusNav - 当前下标越界返回 -1") {
    const auto g = grid2x2();

    CHECK(FocusNav::nextInDirection(g, -1, Direction::Right) == -1);
    CHECK(FocusNav::nextInDirection(g, 4, Direction::Right) == -1);
    CHECK(FocusNav::nextInDirection(g, 999, Direction::Up) == -1);
}

TEST_CASE("FocusNav - 返回的下标永远合法（或 -1）") {
    const auto g = grid2x2();
    const Direction dirs[] = {Direction::Up, Direction::Down,
                              Direction::Left, Direction::Right};

    for (int cur = 0; cur < 4; ++cur) {
        for (Direction d : dirs) {
            const int r = FocusNav::nextInDirection(g, cur, d);
            CHECK(r >= -1);
            CHECK(r < 4);
            CHECK(r != cur);   // 永远不会选中自己
        }
    }
}

// ============================================================
// 线性导航
// ============================================================

TEST_CASE("FocusNav - 线性移动越界绕回") {
    CHECK(FocusNav::nextLinear(0, 1, 3) == 1);
    CHECK(FocusNav::nextLinear(2, 1, 3) == 0);   // 末尾往后绕回开头
    CHECK(FocusNav::nextLinear(0, -1, 3) == 2);  // 开头往前绕回末尾
    CHECK(FocusNav::nextLinear(1, -1, 3) == 0);
}

TEST_CASE("FocusNav - 线性移动支持跨多步与负数") {
    CHECK(FocusNav::nextLinear(0, 5, 3) == 2);
    CHECK(FocusNav::nextLinear(0, -5, 3) == 1);
    CHECK(FocusNav::nextLinear(0, 0, 3) == 0);
    CHECK(FocusNav::nextLinear(0, 300, 3) == 0);   // 整圈回到原地
}

TEST_CASE("FocusNav - 空列表的线性移动返回 -1") {
    CHECK(FocusNav::nextLinear(0, 1, 0) == -1);
    CHECK(FocusNav::nextLinear(0, 1, -3) == -1);
}

TEST_CASE("FocusNav - 单个元素的线性移动永远停在原地") {
    CHECK(FocusNav::nextLinear(0, 1, 1) == 0);
    CHECK(FocusNav::nextLinear(0, -7, 1) == 0);
}

TEST_CASE("FocusNav - 线性移动的结果永远在范围内") {
    for (int n = 1; n <= 8; ++n) {
        for (int cur = 0; cur < n; ++cur) {
            for (int delta = -20; delta <= 20; ++delta) {
                const int r = FocusNav::nextLinear(cur, delta, n);
                CHECK(r >= 0);
                CHECK(r < n);
            }
        }
    }
}
