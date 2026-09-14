#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Checkpoint : public GameObject {
public:
    Checkpoint(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Checkpoint; }

    void activate() { active_ = true; }
    bool isActive() const { return active_; }
    Vec2 respawnPos() const;

private:
    Vec2 pos_;
    int  tileSize_;
    bool active_ = false;
    float animTimer_ = 0.f;

    mutable sf::RectangleShape pole_;
    mutable sf::RectangleShape flag_;
};