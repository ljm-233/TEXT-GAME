#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Coin : public GameObject {
public:
    explicit Coin(Vec2 pos);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Coin; }
    bool isRemovable() const override { return collected_; }

    void collect() { collected_ = true; }
    bool collected() const { return collected_; }

private:
    Vec2 pos_;
    bool collected_ = false;
    float animTimer_ = 0.f;

    mutable sf::CircleShape shape_;
};