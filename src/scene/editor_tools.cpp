#include "scene/editor_tools.h"

#include <algorithm>
#include <cstdlib>

namespace EditorTools {

std::vector<sf::Vector2i> rectCells(sf::Vector2i a, sf::Vector2i b) {
    const int x0 = std::min(a.x, b.x);
    const int x1 = std::max(a.x, b.x);
    const int y0 = std::min(a.y, b.y);
    const int y1 = std::max(a.y, b.y);

    std::vector<sf::Vector2i> cells;
    cells.reserve(static_cast<std::size_t>(x1 - x0 + 1) *
                  static_cast<std::size_t>(y1 - y0 + 1));

    // 行优先：与原来编辑器里的双重循环顺序一致
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            cells.push_back({x, y});

    return cells;
}

std::vector<sf::Vector2i> lineCells(sf::Vector2i from, sf::Vector2i to) {
    // 这段是原 EditorScene::paintLine 里的 Bresenham，逐字搬过来。
    // 抽取只是把"画哪几格"与"怎么画"分开，算法本身一个字没改。
    int x0 = from.x, y0 = from.y;
    int x1 = to.x,   y1 = to.y;
    const int dx =  std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    std::vector<sf::Vector2i> cells;
    while (true) {
        cells.push_back({x0, y0});
        if (x0 == x1 && y0 == y1) break;

        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return cells;
}

} // namespace EditorTools
