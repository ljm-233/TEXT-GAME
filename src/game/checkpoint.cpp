#include "checkpoint.h"
#include <cmath>

Checkpoint::Checkpoint(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    float ts = static_cast<float>(tileSize_);

    // 杆子阴影（伪 3D）
    poleShadow_.setSize({ts * 0.15f, ts * 0.9f});
    poleShadow_.setFillColor(sf::Color(100, 100, 110));

    pole_.setSize({ts * 0.15f, ts * 0.9f});
    pole_.setFillColor(sf::Color(150, 150, 160));
    pole_.setOutlineThickness(1.f);
    pole_.setOutlineColor(sf::Color(80, 80, 90));

    flag_.setSize({ts * 0.55f, ts * 0.35f});
    flag_.setFillColor(sf::Color(120, 120, 130));
    flag_.setOutlineThickness(1.f);
    flag_.setOutlineColor(sf::Color(80, 80, 90));
}

void Checkpoint::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
}

AABB Checkpoint::bounds() const {
    float ts = static_cast<float>(tileSize_);
    // 向上扩展一格，让玩家站在 checkpoint 上方时也能触发
    return {pos_.x, pos_.y - ts, ts, ts * 2.f};
}

Vec2 Checkpoint::respawnPos() const {
    float ts = static_cast<float>(tileSize_);
    return {pos_.x + ts * 0.1f, pos_.y + ts * 0.2f};
}

void Checkpoint::render(sf::RenderTarget& target) const {
    float ts = static_cast<float>(tileSize_);

    // ⭐ 伪 3D：杆子阴影偏移 3 像素
    poleShadow_.setPosition({pos_.x + ts * 0.4f + 3.f, pos_.y + ts * 0.1f + 3.f});
    target.draw(poleShadow_);

    pole_.setPosition({pos_.x + ts * 0.4f, pos_.y + ts * 0.1f});
    target.draw(pole_);

    if (active_) {
        flag_.setFillColor(sf::Color(80, 220, 100));
        flag_.setOutlineColor(sf::Color(40, 120, 60));
    } else {
        flag_.setFillColor(sf::Color(120, 120, 130));
        flag_.setOutlineColor(sf::Color(80, 80, 90));
    }

    float wave = active_ ? std::sin(animTimer_ * 6.f) * 2.f : 0.f;
    flag_.setPosition({pos_.x + ts * 0.55f, pos_.y + ts * 0.15f + wave});
    target.draw(flag_);
}