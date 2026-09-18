#include "key.h"
#include <cmath>

Key::Key(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    // 同上：不再用 ConvexShape，全部在 render 里用 VertexArray + RectangleShape
}

void Key::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
}

AABB Key::bounds() const {
    float ts = static_cast<float>(tileSize_);
    return {pos_.x + ts * 0.2f, pos_.y + ts * 0.2f,
            ts * 0.6f, ts * 0.6f};
}

void Key::render(sf::RenderTarget& target) const {
    if (collected_) return;

    const float ts = static_cast<float>(tileSize_);
    const float cx = pos_.x + ts * 0.5f;
    float cy = pos_.y + ts * 0.5f;
    cy += std::sin(animTimer_ * 3.f) * 3.f;

    const float swing  = std::cos(animTimer_ * 2.f);
    const float scaleX = 0.6f + 0.4f * std::abs(swing);
    const sf::Color gold(255, 210, 60);

    // 钥匙头（三角形）
    {
        const float headW   = ts * 0.15f * scaleX;
        const float headTop = cy - ts * 0.35f;
        sf::VertexArray va(sf::PrimitiveType::Triangles, 3);
        va[0] = sf::Vertex{{cx - headW, headTop             }, gold};
        va[1] = sf::Vertex{{cx + headW, headTop             }, gold};
        va[2] = sf::Vertex{{cx,        headTop + ts * 0.18f }, gold};
        target.draw(va);
    }

    // 钥匙杆
    {
        const float shaftW = ts * 0.12f * scaleX;
        sf::RectangleShape shaft({shaftW, ts * 0.5f});
        shaft.setFillColor(gold);
        shaft.setPosition({cx - shaftW * 0.5f, cy - ts * 0.35f + ts * 0.15f});
        target.draw(shaft);
    }

    // 钥匙齿（两个凸起）
    {
        const float shaftW = ts * 0.12f * scaleX;
        sf::RectangleShape tooth({ts * 0.18f * scaleX, ts * 0.08f});
        tooth.setFillColor(gold);
        tooth.setPosition({cx + shaftW * 0.4f, cy - ts * 0.35f + ts * 0.42f});
        target.draw(tooth);

        tooth.setSize({ts * 0.14f * scaleX, ts * 0.08f});
        tooth.setPosition({cx + shaftW * 0.4f, cy - ts * 0.35f + ts * 0.58f});
        target.draw(tooth);
    }
}