#include "spike.h"
#include <cmath>

Spike::Spike(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    // 不再使用 sf::ConvexShape，所有绘制在 render 里用 VertexArray 完成。
    // 原因：SFML 3 的 ConvexShape::updateOutline 在相邻顶点重合时会做除零计算，
    //       会触发 Vector2::operator/ 断言或 index 越界断言。
}

void Spike::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
}

AABB Spike::bounds() const {
    float ts = static_cast<float>(tileSize_);
    return {pos_.x + ts * 0.1f, pos_.y + ts * 0.3f,
            ts * 0.8f, ts * 0.7f};
}

void Spike::render(sf::RenderTarget& target) const {
    const float ts = static_cast<float>(tileSize_);
    const float x  = pos_.x;
    const float y  = pos_.y;
    const sf::Color c(200, 200, 210);

    sf::VertexArray va(sf::PrimitiveType::Triangles, 9);
    for (int i = 0; i < 3; ++i) {
        const float sx = x + static_cast<float>(i) * ts * 0.33f;
        va[i * 3 + 0] = sf::Vertex{{sx,                  y + ts        }, c};
        va[i * 3 + 1] = sf::Vertex{{sx + ts * 0.165f,    y + ts * 0.15f}, c};
        va[i * 3 + 2] = sf::Vertex{{sx + ts * 0.33f,     y + ts        }, c};
    }
    target.draw(va);
}