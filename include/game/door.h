#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Door : public GameObject {
public:
    Door(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Door; }

    void unlock() { unlocked_ = true; }
    bool isUnlocked() const { return unlocked_; }

private:
    Vec2 pos_;
    int  tileSize_;
    bool unlocked_ = false;
    float animTimer_ = 0.f;

    mutable sf::RectangleShape frame_;
    mutable sf::RectangleShape panel_;
    mutable sf::CircleShape    knob_;
};