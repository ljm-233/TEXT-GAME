#pragma once
#include "vec2.h"
#include "aabb.h"
#include <SFML/Graphics.hpp>

class Level;

class Player {
public:
    explicit Player(Vec2 spawn);

    void handleEvent(const sf::Event& event);
    void update(float dt, const Level& level);

    // 直接用世界坐标绘制
    void render(sf::RenderTarget& target) const;

    AABB bounds() const { return {pos_.x, pos_.y, size_.x, size_.y}; }
    Vec2 position() const { return pos_; }
    Vec2 velocity() const { return vel_; }
    bool onGround() const { return onGround_; }

    void respawn(Vec2 spawn);

private:
    void moveHorizontal(float dx, const Level& level);
    void moveVertical(float dy, const Level& level);

    Vec2 pos_;
    Vec2 vel_;
    Vec2 size_{24.f, 32.f};

    bool onGround_ = false;
    bool keyLeft_  = false;
    bool keyRight_ = false;
    bool keyJump_  = false;

    float coyoteTimer_    = 0.f;
    float jumpBufferTimer_= 0.f;
    bool  jumpConsumed_   = false;

    mutable sf::RectangleShape body_;
    mutable sf::RectangleShape eye_;
};