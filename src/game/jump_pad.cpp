#include "jump_pad.h"
#include <cmath>

JumpPad::JumpPad(Vec2 pos, int tileSize)
      : pos_(pos),
        tileSize_(tileSize) {
    float ts = static_cast<float>(tileSize_);

    base_.setSize({ts, ts * 0.5f});
    base_.setFillColor(sf::Color(60, 200, 90));
    base_.setOutlineThickness(2.f);
    base_.setOutlineColor(sf::Color(30, 120, 50));

    arrow_.setSize({ts * 0.5f, ts * 0.35f});
    arrow_.setFillColor(sf::Color(180, 255, 180));
    arrow_.setOrigin({ts * 0.25f, ts * 0.175f});
}

void JumpPad::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
    if (cooldown_ > 0.f)
        cooldown_ -= dt;
}

AABB JumpPad::bounds() const {
    float ts = static_cast<float>(tileSize_);
    // ⭐ 碰撞盒往上延伸一个 tile 高度
    //    这样玩家站在跳板上（脚底 = 跳板顶部）时，盒子就有重叠
    return {pos_.x, pos_.y - ts, ts, ts * 2.f};
}

void JumpPad::render(sf::RenderTarget& target) const {
    float ts = static_cast<float>(tileSize_);

    if (cooldown_ > 0.f) {
        base_.setFillColor(sf::Color(40, 140, 60));
    } else {
        base_.setFillColor(sf::Color(60, 200, 90));
    }

    base_.setPosition({pos_.x, pos_.y + ts * 0.5f});
    target.draw(base_);

    float bob = std::sin(animTimer_ * 5.f) * 3.f;
    arrow_.setPosition({pos_.x + ts * 0.5f, pos_.y + ts * 0.35f + bob});
    target.draw(arrow_);
}