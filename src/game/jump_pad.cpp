#include "jump_pad.h"
#include <algorithm>
#include <cmath>

JumpPad::JumpPad(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    float ts = static_cast<float>(tileSize_);

    base_.setSize({ts, ts * 0.5f});
    base_.setFillColor(sf::Color(60, 200, 90));
    base_.setOutlineThickness(2.f);
    base_.setOutlineColor(sf::Color(30, 120, 50));

    arrow_.setSize({ts * 0.5f, ts * 0.35f});
    arrow_.setFillColor(sf::Color(180, 255, 180));
    arrow_.setOrigin({ts * 0.25f, ts * 0.175f});
}

void JumpPad::trigger() {
    cooldown_ = kCooldown;
    compressRatio_ = 1.f;   // ⭐ 立即进入压缩状态
}

void JumpPad::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
    if (cooldown_ > 0.f) cooldown_ -= dt;

    // ⭐ 压缩回弹：每秒衰减 4 倍
    if (compressRatio_ > 0.f) {
        compressRatio_ = std::max(0.f, compressRatio_ - 4.f * dt);
    }
}

AABB JumpPad::bounds() const {
    float ts = static_cast<float>(tileSize_);
    return {pos_.x, pos_.y - ts, ts, ts * 2.f};
}

void JumpPad::render(sf::RenderTarget& target) const {
    float ts = static_cast<float>(tileSize_);

    // ⭐ 压缩系数：1 → 0
    //    高度 = 正常高度 × (1 - 0.6 * compressRatio)
    //    ratio=0 → 100% 高度
    //    ratio=1 → 40% 高度（被压扁）
    float heightScale = 1.f - 0.6f * compressRatio_;

    // 冷却时变暗
    if (cooldown_ > 0.f) {
        base_.setFillColor(sf::Color(40, 140, 60));
    } else {
        base_.setFillColor(sf::Color(60, 200, 90));
    }

    // 底座（压缩）
    float baseH = ts * 0.5f * heightScale;
    float baseY = pos_.y + ts * 0.5f + (ts * 0.5f - baseH);
    base_.setSize({ts, baseH});
    base_.setPosition({pos_.x, baseY});
    target.draw(base_);

    // 箭头（被压缩时往下偏移）
    float bob = std::sin(animTimer_ * 5.f) * 3.f;
    float arrowY = pos_.y + ts * 0.35f + bob
                 + (ts * 0.5f - baseH);   // 跟着底座下沉
    arrow_.setPosition({pos_.x + ts * 0.5f, arrowY});
    target.draw(arrow_);
}