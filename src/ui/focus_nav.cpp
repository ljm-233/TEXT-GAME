#include "ui/focus_nav.h"

#include <cmath>

namespace FocusNav {

namespace {

/// 方向单位向量
void directionVector(Direction dir, float& dx, float& dy) {
    dx = (dir == Direction::Right) ? 1.f
       : (dir == Direction::Left)  ? -1.f : 0.f;
    dy = (dir == Direction::Down)  ? 1.f
       : (dir == Direction::Up)    ? -1.f : 0.f;
}

sf::Vector2f centerOf(const sf::FloatRect& r) {
    return {r.position.x + r.size.x * 0.5f,
            r.position.y + r.size.y * 0.5f};
}

} // namespace

int nextInDirection(const std::vector<sf::FloatRect>& rects, int cur, Direction dir) {
    if (rects.empty()) return -1;
    if (cur < 0 || cur >= static_cast<int>(rects.size())) return -1;

    float dx = 0.f, dy = 0.f;
    directionVector(dir, dx, dy);
    const sf::Vector2f ccenter = centerOf(rects[static_cast<std::size_t>(cur)]);

    int bestIdx = -1;
    float bestScore = 0.f;

    for (std::size_t i = 0; i < rects.size(); ++i) {
        if (static_cast<int>(i) == cur) continue;
        const sf::FloatRect& r = rects[i];

        // 跳过未定位的按钮（位置为 0 且尺寸为 0）
        if (r.size.x <= 0.f || r.size.y <= 0.f) continue;

        const sf::Vector2f bc = centerOf(r);
        const sf::Vector2f delta = {bc.x - ccenter.x, bc.y - ccenter.y};

        // 主方向投影：必须为正，否则该按钮不在 dir 方向上
        const float proj = delta.x * dx + delta.y * dy;
        if (proj < kMinProjection) continue;

        // 垂直偏移（叉积绝对值）
        const float perp = std::abs(delta.x * dy - delta.y * dx);

        // 评分：主方向距离 + 垂直偏移 × kPerpWeight（垂直偏移惩罚更重，
        // 让同行/同列优先）
        const float score = proj + perp * kPerpWeight;

        if (bestIdx < 0 || score < bestScore) {
            bestScore = score;
            bestIdx = static_cast<int>(i);
        }
    }

    return bestIdx;
}

int nextLinear(int cur, int delta, int count) {
    if (count <= 0) return -1;
    return ((cur + delta) % count + count) % count;
}

} // namespace FocusNav
