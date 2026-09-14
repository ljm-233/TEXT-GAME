#include "coin.h"
#include <cmath>

namespace {
constexpr float kRadius = 10.f;
constexpr float kBobAmplitude = 4.f;
constexpr float kBobSpeed = 3.f;
}

Coin::Coin(Vec2 pos) : pos_(pos) {
    shape_.setRadius(kRadius);
    shape_.setOrigin({kRadius, kRadius});
    shape_.setFillColor(sf::Color(255, 210, 60));
    shape_.setOutlineThickness(2.f);
    shape_.setOutlineColor(sf::Color(180, 130, 20));
}

void Coin::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
}

AABB Coin::bounds() const {
    float cx = pos_.x + 16.f;   // 半瓦片中心
    float cy = pos_.y + 16.f;
    return {cx - kRadius, cy - kRadius, kRadius * 2.f, kRadius * 2.f};
}

void Coin::render(sf::RenderTarget& target) const {
    float cx = pos_.x + 16.f;
    float cy = pos_.y + 16.f + std::sin(animTimer_ * kBobSpeed) * kBobAmplitude;
    shape_.setPosition({cx, cy});
    target.draw(shape_);
}