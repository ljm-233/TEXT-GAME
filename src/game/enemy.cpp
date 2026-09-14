#include "enemy.h"
#include "level.h"
#include <cmath>

namespace {
constexpr float kSpeed = 90.f;
}

Enemy::Enemy(Vec2 pos, int tileSize)
    : pos_(pos), vel_(-kSpeed, 0.f), tileSize_(tileSize) {
    pos_.x += (tileSize_ - size_.x) * 0.5f;
    pos_.y += (tileSize_ - size_.y) * 0.5f;

    body_.setSize({size_.x, size_.y});
    body_.setFillColor(sf::Color(220, 80, 80));
    body_.setOutlineThickness(2.f);
    body_.setOutlineColor(sf::Color(140, 40, 40));

    eye_.setSize({5.f, 5.f});
    eye_.setFillColor(sf::Color(255, 255, 255));
}

AABB Enemy::bounds() const {
    return {pos_.x, pos_.y, size_.x, size_.y};
}

bool Enemy::wallAhead(const Level& level) const {
    int ts = level.tileSize();
    AABB box = bounds();

    float probeX = (vel_.x > 0.f) ? box.right() + 1.f : box.left() - 1.f;
    int tx = static_cast<int>(std::floor(probeX / ts));
    int tyTop = static_cast<int>(std::floor(box.top() / ts));
    int tyBot = static_cast<int>(std::floor((box.bottom() - 0.001f) / ts));

    return level.isSolid(tx, tyTop) || level.isSolid(tx, tyBot);
}

bool Enemy::cliffAhead(const Level& level) const {
    int ts = level.tileSize();
    AABB box = bounds();

    float probeX = (vel_.x > 0.f) ? box.right() + 2.f : box.left() - 2.f;
    int tx = static_cast<int>(std::floor(probeX / ts));
    int ty = static_cast<int>(std::floor((box.bottom() + 2.f) / ts));

    return !level.isSolid(tx, ty);
}

void Enemy::update(float dt, const Level& level) {
    if (killed_) return;
    if (wallAhead(level) || cliffAhead(level)) {
        vel_.x = -vel_.x;
    }
    pos_.x += vel_.x * dt;
}

void Enemy::render(sf::RenderTarget& target) const {
    if (killed_) return;

    body_.setPosition({pos_.x, pos_.y});
    target.draw(body_);

    float eyeX = (vel_.x > 0.f)
        ? pos_.x + size_.x - 9.f
        : pos_.x + 4.f;
    eye_.setPosition({eyeX, pos_.y + 8.f});
    target.draw(eye_);
}