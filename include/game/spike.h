#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Spike : public GameObject {
public:
    Spike(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Spike; }

private:
    Vec2 pos_;
    int  tileSize_;
    float animTimer_ = 0.f;

    mutable sf::ConvexShape spike_;
};