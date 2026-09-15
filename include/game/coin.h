#pragma once
#include "game_object.h"
#include "vec2.h"
#include "animator.h"
#include <SFML/Graphics.hpp>
#include <memory>

class Coin : public GameObject {
public:
    Coin(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Coin; }
    bool isRemovable() const override { return collected_; }

    void collect() { collected_ = true; }
    bool collected() const { return collected_; }

private:
    Vec2 pos_;
    int  tileSize_;
    bool collected_ = false;
    float animTimer_ = 0.f;

    std::shared_ptr<sf::Texture>       sheet_;
    Animator                           animator_;
    mutable std::unique_ptr<sf::Sprite> sprite_;
};