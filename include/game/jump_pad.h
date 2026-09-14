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

    static constexpr float kLaunchSpeed = -1050.f;   // 比普通跳跃高

private:
    Vec2 pos_;
    int  tileSize_;
    float animTimer_ = 0.f;

    mutable sf::RectangleShape base_;
    mutable sf::RectangleShape arrow_;
};