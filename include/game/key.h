#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Key : public GameObject {
public:
    Key(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Key; }
    bool isRemovable() const override { return collected_; }

    void collect() { collected_ = true; }
    bool collected() const { return collected_; }

private:
    Vec2 pos_;
    int  tileSize_;
    bool collected_ = false;
    float animTimer_ = 0.f;

    mutable sf::ConvexShape keyShape_;
    mutable sf::RectangleShape barShape_;
};