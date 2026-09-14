#include "coin.h"
#include <cmath>

namespace {
constexpr float kRadiusRatio = 0.30f; // 半径 = tileSize * 0.30
constexpr float kBobAmplitude = 4.f;
constexpr float kBobSpeed = 3.f;
} // namespace

Coin::Coin(Vec2 pos, int tileSize)
      : pos_(pos),
        tileSize_(tileSize) {
    float radius = static_cast<float>(tileSize_) * kRadiusRatio;
    shape_.setRadius(radius);
    shape_.setOrigin({radius, radius});
    shape_.setFillColor(sf::Color(255, 210, 60));
    shape_.setOutlineThickness(2.f);
    shape_.setOutlineColor(sf::Color(180, 130, 20));
}

void Coin::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
}

AABB Coin::bounds() const {
    float r = static_cast<float>(tileSize_) * kRadiusRatio;
    float cx = pos_.x + static_cast<float>(tileSize_) * 0.5f;
    float cy = pos_.y + static_cast<float>(tileSize_) * 0.5f;
    return {cx - r, cy - r, r * 2.f, r * 2.f};
}

void Coin::render(sf::RenderTarget& target) const {
    float cx = pos_.x + static_cast<float>(tileSize_) * 0.5f;
    float cy = pos_.y + static_cast<float>(tileSize_) * 0.5f +
               std::sin(animTimer_ * kBobSpeed) * kBobAmplitude;
    shape_.setPosition({cx, cy});
    target.draw(shape_);
}