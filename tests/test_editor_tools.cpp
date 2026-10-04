#include "doctest.h"
#include "scene/editor_tools.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

// 编辑器的格子几何。
//
// 拖拽绘制最容易出的错就是差一格：往左上拖和往右下拖结果不一致、
// 快速拖动时连线漏掉端点、或者两个采样点之间留下断线。
// 这些是纯几何，从 EditorScene 里抽出来之后就能测了
// （编辑器场景构造需要字体，测不了）。
namespace {

bool contains(const std::vector<sf::Vector2i>& cells, int x, int y) {
    return std::any_of(cells.begin(), cells.end(),
                       [x, y](const sf::Vector2i& c) { return c.x == x && c.y == y; });
}

/// 把格子列表排序后转成可比较的字符串，用来看"画了哪些格子"
std::string dump(std::vector<sf::Vector2i> cells) {
    std::sort(cells.begin(), cells.end(), [](const sf::Vector2i& a, const sf::Vector2i& b) {
        return a.y != b.y ? a.y < b.y : a.x < b.x;
    });
    std::string out;
    for (const auto& c : cells)
        out += "(" + std::to_string(c.x) + "," + std::to_string(c.y) + ")";
    return out;
}

} // namespace

// ============================================================
// rectCells
// ============================================================

TEST_CASE("EditorTools - 矩形含两端且格子数正确") {
    const auto cells = EditorTools::rectCells({2, 3}, {4, 5});   // 3 列 × 3 行

    CHECK(cells.size() == 9);
    CHECK(contains(cells, 2, 3));   // 左上角
    CHECK(contains(cells, 4, 5));   // 右下角
    CHECK(contains(cells, 3, 4));   // 中间
    CHECK_FALSE(contains(cells, 1, 3));
    CHECK_FALSE(contains(cells, 5, 5));
}

TEST_CASE("EditorTools - 拖拽方向不影响矩形结果") {
    const auto a = EditorTools::rectCells({2, 3}, {4, 5});
    const auto b = EditorTools::rectCells({4, 5}, {2, 3});
    const auto c = EditorTools::rectCells({2, 5}, {4, 3});
    const auto d = EditorTools::rectCells({4, 3}, {2, 5});

    CHECK(dump(a) == dump(b));
    CHECK(dump(a) == dump(c));
    CHECK(dump(a) == dump(d));
}

TEST_CASE("EditorTools - 单格矩形就是那一格") {
    const auto cells = EditorTools::rectCells({7, 8}, {7, 8});

    CHECK(cells.size() == 1);
    CHECK(cells[0].x == 7);
    CHECK(cells[0].y == 8);
}

TEST_CASE("EditorTools - 一行的矩形") {
    const auto cells = EditorTools::rectCells({0, 5}, {3, 5});

    CHECK(cells.size() == 4);
    for (int x = 0; x <= 3; ++x)
        CHECK(contains(cells, x, 5));
}

TEST_CASE("EditorTools - 矩形是行优先顺序") {
    // 与原来编辑器里的双重循环（外 y 内 x）一致，便于保持落笔顺序不变
    const auto cells = EditorTools::rectCells({0, 0}, {1, 1});

    REQUIRE(cells.size() == 4);
    CHECK(cells[0].x == 0); CHECK(cells[0].y == 0);
    CHECK(cells[1].x == 1); CHECK(cells[1].y == 0);
    CHECK(cells[2].x == 0); CHECK(cells[2].y == 1);
    CHECK(cells[3].x == 1); CHECK(cells[3].y == 1);
}

TEST_CASE("EditorTools - 负坐标也能正常围矩形") {
    const auto cells = EditorTools::rectCells({-2, -1}, {0, 1});

    CHECK(cells.size() == 9);
    CHECK(contains(cells, -2, -1));
    CHECK(contains(cells, 0, 1));
}

// ============================================================
// lineCells
// ============================================================

TEST_CASE("EditorTools - 连线含两端") {
    const auto cells = EditorTools::lineCells({0, 0}, {5, 0});

    CHECK_FALSE(cells.empty());
    CHECK(cells.front().x == 0);
    CHECK(cells.front().y == 0);
    CHECK(cells.back().x == 5);
    CHECK(cells.back().y == 0);
}

TEST_CASE("EditorTools - 水平线与垂直线每一格都补上") {
    const auto h = EditorTools::lineCells({0, 0}, {4, 0});
    REQUIRE(h.size() == 5);
    for (int x = 0; x <= 4; ++x)
        CHECK(contains(h, x, 0));

    const auto v = EditorTools::lineCells({3, 0}, {3, 4});
    REQUIRE(v.size() == 5);
    for (int y = 0; y <= 4; ++y)
        CHECK(contains(v, 3, y));
}

TEST_CASE("EditorTools - 单点连线只画一格") {
    const auto cells = EditorTools::lineCells({4, 4}, {4, 4});

    REQUIRE(cells.size() == 1);
    CHECK(cells[0].x == 4);
    CHECK(cells[0].y == 4);
}

TEST_CASE("EditorTools - 45 度斜线一格不漏") {
    const auto cells = EditorTools::lineCells({0, 0}, {3, 3});

    REQUIRE(cells.size() == 4);
    for (int i = 0; i <= 3; ++i)
        CHECK(contains(cells, i, i));
}

TEST_CASE("EditorTools - 反向连线与正向格子集合相同") {
    const auto fwd = EditorTools::lineCells({0, 0}, {7, 3});
    const auto bwd = EditorTools::lineCells({7, 3}, {0, 0});

    CHECK(dump(fwd) == dump(bwd));
}

TEST_CASE("EditorTools - 连线是连续的（相邻格子最多差一格）") {
    // 这条是"快速拖动不留断线"的本质：每一步至多斜着走一格
    const std::vector<std::pair<sf::Vector2i, sf::Vector2i>> cases = {
        {{0, 0}, {17, 5}},
        {{0, 0}, {5, 17}},
        {{-9, 4}, {12, -30}},
        {{3, 3}, {-3, -3}},
    };

    for (const auto& [from, to] : cases) {
        CAPTURE(from.x); CAPTURE(from.y); CAPTURE(to.x); CAPTURE(to.y);
        const auto cells = EditorTools::lineCells(from, to);
        REQUIRE(cells.size() >= 2);

        for (std::size_t i = 1; i < cells.size(); ++i) {
            const int dx = std::abs(cells[i].x - cells[i - 1].x);
            const int dy = std::abs(cells[i].y - cells[i - 1].y);
            CHECK(dx <= 1);
            CHECK(dy <= 1);
            CHECK(dx + dy >= 1);   // 不允许原地踏步
        }
    }
}

TEST_CASE("EditorTools - 连线长度符合切比雪夫距离") {
    // Bresenham 的步数 = max(|dx|, |dy|)，格子数 = 步数 + 1
    const auto cells = EditorTools::lineCells({0, 0}, {10, 4});

    CHECK(cells.size() == 11);   // max(10, 4) + 1
}
