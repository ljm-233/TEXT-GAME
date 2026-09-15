#include "key.h"
#include <cmath>

Key::Key(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    float ts = static_cast<float>(tileSize_);

    headShape_.setPointCount(3);
    headShape_.setFillColor(sf::Color::Transparent);
    headShape_.setOutlineThickness(3.f);
    headShape_.setOutlineColor(sf::Color(255, 210, 60));

    shaftShape_.setSize({ts * 0.12f, ts * 0.5f});
    shaftShape_.setFillColor(sf::Color(255, 210, 60));

    toothShape1_.setSize({ts * 0.18f, ts * 0.1f});
    toothShape1_.setFillColor(sf::Color(255, 210, 60));

    toothShape2_.setSize({ts * 0.14f, ts * 0.1f});
    toothShape2_.setFillColor(sf::Color(255, 210, 60));
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
    float cy = pos_.y + ts * 0.5f;

    // 上下浮动
    cy += std::sin(animTimer_ * 3.f) * 3.f;

    // 左右摇摆（横向缩放）
    float swing = std::cos(animTimer_ * 2.f);
    float scaleX = 0.6f + 0.4f * std::abs(swing);

    // ===== 钥匙头（三角形环）=====
    float headW = ts * 0.15f;
    float headTop = cy - ts * 0.35f;
    headShape_.setPoint(0, {cx - headW * scaleX, headTop});
    headShape_.setPoint(1, {cx + headW * scaleX, headTop});
    headShape_.setPoint(2, {cx, headTop + ts * 0.18f});
    target.draw(headShape_);

    // ===== 钥匙杆 =====
    float shaftW = ts * 0.12f * scaleX;
    shaftShape_.setSize({shaftW, ts * 0.5f});
    shaftShape_.setPosition({cx - shaftW * 0.5f, headTop + ts * 0.15f});
    target.draw(shaftShape_);

    // ===== 钥匙齿（两个小凸起）=====
    float tooth1W = ts * 0.18f * scaleX;
    toothShape1_.setSize({tooth1W, ts * 0.08f});
    toothShape1_.setPosition({
        cx + shaftW * 0.4f,
        headTop + ts * 0.42f
    });
    target.draw(toothShape1_);

    float tooth2W = ts * 0.14f * scaleX;
    toothShape2_.setSize({tooth2W, ts * 0.08f});
    toothShape2_.setPosition({
        cx + shaftW * 0.4f,
        headTop + ts * 0.58f
    });
    target.draw(toothShape2_);
}