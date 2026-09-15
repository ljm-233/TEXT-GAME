#include "door.h"
#include <cmath>

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
}

void Door::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
}

AABB Door::bounds() const {
    float ts = static_cast<float>(tileSize_);
    return {pos_.x, pos_.y, ts, ts};
}

void Door::render(sf::RenderTarget& target) const {
    if (unlocked_) return;   // 解锁后消失

    float ts = static_cast<float>(tileSize_);

    frame_.setPosition({pos_.x, pos_.y});
    target.draw(frame_);

    panel_.setPosition({pos_.x + ts * 0.1f, pos_.y + ts * 0.075f});
    target.draw(panel_);

    knob_.setPosition({pos_.x + ts * 0.7f, pos_.y + ts * 0.5f});
    target.draw(knob_);
}