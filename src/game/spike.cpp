#include "spike.h"
#include <cmath>

Spike::Spike(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    // 三角形的尖刺
    spike_.setPointCount(3);
    spike_.setFillColor(sf::Color(200, 200, 210));
    spike_.setOutlineThickness(2.f);
    spike_.setOutlineColor(sf::Color(120, 120, 140));
}

void Spike::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
}

AABB Spike::bounds() const {
    float ts = static_cast<float>(tileSize_);
    // 尖刺碰撞盒比瓦片小，让玩家能擦边过
    return {pos_.x + ts * 0.1f, pos_.y + ts * 0.3f,
            ts * 0.8f, ts * 0.7f};
}

void Spike::render(sf::RenderTarget& target) const {
    float ts = static_cast<float>(tileSize_);
    float x = pos_.x;
    float y = pos_.y;

    // 3 根小尖刺
    for (int i = 0; i < 3; ++i) {
        float sx = x + static_cast<float>(i) * ts * 0.33f;
        spike_.setPoint(0, {sx, y + ts});
        spike_.setPoint(1, {sx + ts * 0.165f, y + ts * 0.15f});
        spike_.setPoint(2, {sx + ts * 0.33f, y + ts});
        target.draw(spike_);
    }
}