#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class JumpPad : public GameObject {
public:
    JumpPad(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::JumpPad; }

    // 冷却：触发后一段时间内不再触发
    bool canTrigger() const { return cooldown_ <= 0.f; }
    void trigger() { cooldown_ = kCooldown; }

    static constexpr float kLaunchSpeed = -1200.f;

private:
    Vec2 pos_;
    int  tileSize_;
    float animTimer_ = 0.f;
    float cooldown_  = 0.f;

    static constexpr float kCooldown = 0.3f;

    mutable sf::RectangleShape base_;
    mutable sf::RectangleShape arrow_;
};