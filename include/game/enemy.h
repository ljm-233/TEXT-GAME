#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Enemy : public GameObject {
public:
    Enemy(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Enemy; }
    bool isRemovable() const override { return killed_; }

    void kill() { killed_ = true; }
    bool killed() const { return killed_; }

private:
    bool wallAhead(const Level& level) const;
    bool cliffAhead(const Level& level) const;

    Vec2 pos_;
    Vec2 vel_;
    Vec2 size_{28.f, 28.f};
    int tileSize_;
    bool killed_ = false;

    mutable sf::RectangleShape body_;
    mutable sf::RectangleShape eye_;
};