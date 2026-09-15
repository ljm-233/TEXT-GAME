#include "moving_platform.h"

MovingPlatform::MovingPlatform(Vec2 pos, int tileSize, float range, bool horizontal,
                               float speed)
      : pos_(pos),
        origin_(pos),
        range_(range),
        horizontal_(horizontal),
        tileSize_(tileSize) {
    float ts = static_cast<float>(tileSize_);
    // 平台尺寸：2.5 格宽 × 0.6 格高
    size_ = {ts * 2.5f, ts * 0.6f};

    if (horizontal_) {
        vel_ = {speed, 0.f};
    } else {
        vel_ = {0.f, speed};
    }

    // ⭐ 显式转成
    body_.setSize({size_.x, size_.y});
    body_.setFillColor(sf::Color(160, 120, 80));
    body_.setOutlineThickness(2.f);
    body_.setOutlineColor(sf::Color(90, 60, 30));

    // 顶部的装饰条纹
    stripe_.setSize({size_.x, 4.f});
    stripe_.setFillColor(sf::Color(220, 180, 120));
}

void MovingPlatform::update(float dt, const Level& /*level*/) {
    Vec2 oldPos = pos_;

    if (horizontal_) {
        pos_.x += vel_.x * dt;

        if (pos_.x > origin_.x + range_) {
            pos_.x = origin_.x + range_;
            vel_.x = -vel_.x;
        } else if (pos_.x < origin_.x) {
            pos_.x = origin_.x;
            vel_.x = -vel_.x;
        }
    } else {
        pos_.y += vel_.y * dt;

        if (pos_.y > origin_.y + range_) {
            pos_.y = origin_.y + range_;
            vel_.y = -vel_.y;
        } else if (pos_.y < origin_.y) {
            pos_.y = origin_.y;
            vel_.y = -vel_.y;
        }
    }

    lastDelta_ = pos_ - oldPos;
}

AABB MovingPlatform::bounds() const {
    return {pos_.x, pos_.y, size_.x, size_.y};
}

void MovingPlatform::render(sf::RenderTarget& target) const {
    body_.setPosition({pos_.x, pos_.y});
    target.draw(body_);

    stripe_.setPosition({pos_.x, pos_.y + 4.f});
    target.draw(stripe_);
}