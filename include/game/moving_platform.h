#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class MovingPlatform : public GameObject {
public:
    MovingPlatform(Vec2 pos, int tileSize, float range,
                   bool horizontal, float speed = 80.f);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Platform; }

    // 上一帧的位移（用于带着玩家走）
    Vec2 lastDelta() const { return lastDelta_; }

private:
    Vec2 pos_;
    Vec2 origin_;
    Vec2 vel_;
    Vec2 size_;
    float range_;
    bool  horizontal_;
    int   tileSize_;

    Vec2 lastDelta_{0.f, 0.f};

    mutable sf::RectangleShape body_;
    mutable sf::RectangleShape stripe_;
};