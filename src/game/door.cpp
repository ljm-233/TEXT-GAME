#include "door.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

Door::Door(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {
    float ts = static_cast<float>(tileSize_);

    frame_.setSize({ts, ts});
    frame_.setFillColor(sf::Color(90, 60, 30));
    frame_.setOutlineThickness(2.f);
    frame_.setOutlineColor(sf::Color(50, 30, 15));

    panel_.setSize({ts * 0.8f, ts * 0.85f});
    panel_.setFillColor(sf::Color(140, 90, 50));
    panel_.setOutlineThickness(2.f);
    panel_.setOutlineColor(sf::Color(70, 40, 20));

    knob_.setRadius(ts * 0.08f);
    knob_.setFillColor(sf::Color(255, 210, 60));
    knob_.setOutlineThickness(1.f);
    knob_.setOutlineColor(sf::Color(120, 90, 20));

    // 开启时的闪光小方块
    sparkle1_.setSize({4.f, 4.f});
    sparkle1_.setFillColor(sf::Color(255, 240, 150));

    sparkle2_.setSize({3.f, 3.f});
    sparkle2_.setFillColor(sf::Color(255, 220, 100));
}

void Door::unlock() {
    if (unlocked_) return;
    unlocked_ = true;
    openTimer_ = kOpenDuration;   // ⭐ 开始淡出动画
}

void Door::update(float dt, const Level& /*level*/) {
    if (openTimer_ > 0.f) openTimer_ -= dt;
}

AABB Door::bounds() const {
    float ts = static_cast<float>(tileSize_);
    return {pos_.x, pos_.y, ts, ts};
}

void Door::render(sf::RenderTarget& target) const {
    if (isGone()) return;   // 已经消失

    float ts = static_cast<float>(tileSize_);

    // ⭐ 淡出 + 缩放（0.4 秒）
    float alpha = 1.f;
    float scale = 1.f;
    if (unlocked_) {
        float t = std::clamp(openTimer_ / kOpenDuration, 0.f, 1.f);  // 1 → 0
        alpha = t;
        scale = 0.7f + 0.3f * t;   // 0.7 → 1.0
    }

    auto a8 = static_cast<std::uint8_t>(alpha * 255.f);

    // 中心点（缩放围绕）
    float cx = pos_.x + ts * 0.5f;
    float cy = pos_.y + ts * 0.5f;

    // ===== 外框 =====
    {
        sf::Color c = frame_.getFillColor();
        frame_.setFillColor(sf::Color(c.r, c.g, c.b, a8));
        c = frame_.getOutlineColor();
        frame_.setOutlineColor(sf::Color(c.r, c.g, c.b, a8));

        float w = ts * scale;
        float h = ts * scale;
        frame_.setSize({w, h});
        frame_.setPosition({cx - w * 0.5f, cy - h * 0.5f});
        target.draw(frame_);
    }

    // ===== 门板 =====
    {
        sf::Color c = panel_.getFillColor();
        panel_.setFillColor(sf::Color(c.r, c.g, c.b, a8));
        c = panel_.getOutlineColor();
        panel_.setOutlineColor(sf::Color(c.r, c.g, c.b, a8));

        float w = ts * 0.8f * scale;
        float h = ts * 0.85f * scale;
        panel_.setSize({w, h});
        panel_.setPosition({cx - w * 0.5f, cy - h * 0.5f});
        target.draw(panel_);
    }

    // ===== 门把手 =====
    {
        sf::Color c = knob_.getFillColor();
        knob_.setFillColor(sf::Color(c.r, c.g, c.b, a8));
        c = knob_.getOutlineColor();
        knob_.setOutlineColor(sf::Color(c.r, c.g, c.b, a8));

        knob_.setPosition({
            cx + ts * 0.2f * scale,
            cy
        });
        target.draw(knob_);
    }

    // ===== 闪光（解锁瞬间）=====
    if (unlocked_ && openTimer_ > 0.f) {
        float t = openTimer_ / kOpenDuration;   // 1 → 0
        float sparkleAlpha = t;

        sparkle1_.setPosition({
            cx + std::sin(t * 12.f) * 15.f - 2.f,
            cy + std::cos(t * 8.f) * 10.f - 2.f
        });
        sparkle1_.setFillColor(sf::Color(255, 240, 150,
            static_cast<std::uint8_t>(sparkleAlpha * 255.f)));
        target.draw(sparkle1_);

        sparkle2_.setPosition({
            cx - std::sin(t * 10.f) * 12.f - 1.5f,
            cy - std::cos(t * 14.f) * 8.f - 1.5f
        });
        sparkle2_.setFillColor(sf::Color(255, 220, 100,
            static_cast<std::uint8_t>(sparkleAlpha * 255.f)));
        target.draw(sparkle2_);
    }
}