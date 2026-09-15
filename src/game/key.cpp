#include "key.h"
#include <cmath>

Key::Key(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    float ts = static_cast<float>(tileSize_);

    // 钥匙头：三角环
    keyShape_.setPointCount(3);
    keyShape_.setFillColor(sf::Color::Transparent);
    keyShape_.setOutlineThickness(3.f);
    keyShape_.setOutlineColor(sf::Color(255, 210, 60));

    // 钥匙杆
    barShape_.setSize({ts * 0.12f, ts * 0.5f});
    barShape_.setFillColor(sf::Color(255, 210, 60));
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

    float ts = static_cast<float>(tileSize_);
    float cx = pos_.x + ts * 0.5f;
    float cy = pos_.y + ts * 0.5f + std::sin(animTimer_ * 3.f) * 3.f;

    // 旋转动画
    float angle = animTimer_ * 60.f;

    // 钥匙头
    keyShape_.setPoint(0, {cx - ts * 0.15f, cy - ts * 0.25f});
    keyShape_.setPoint(1, {cx + ts * 0.15f, cy - ts * 0.25f});
    keyShape_.setPoint(2, {cx, cy - ts * 0.05f});
    target.draw(keyShape_);

    // 钥匙杆
    barShape_.setPosition({cx - ts * 0.06f, cy - ts * 0.05f});
    barShape_.setRotation(sf::degrees(0.f));
    target.draw(barShape_);

    (void)angle;
}