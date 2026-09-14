#pragma once
#include "game_object.h"
#include "vec2.h"
#include "aabb.h"
#include <SFML/Graphics.hpp>

class Player : public GameObject {
public:
    explicit Player(Vec2 spawn);

    // ===== GameObject 接口 =====
    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override {
        return {pos_.x, pos_.y, size_.x, size_.y};
    }
    Type type() const override { return Type::Player; }

    // ===== Player 独有 =====
    void handleEvent(const sf::Event& event);
    Vec2 position() const { return pos_; }
    Vec2 velocity() const { return vel_; }
    bool onGround() const { return onGround_; }

    void respawn(Vec2 spawn);
    void setKillY(float y) { killY_ = y; }

    bool consumeFellOut() { bool f = fellOut_; fellOut_ = false; return f; }

        // ===== 受伤 / 无敌 =====
    void takeDamage() { invincibleTimer_ = kInvincibleDuration; }
    bool isInvincible() const { return invincibleTimer_ > 0.f; }

private:
    void moveHorizontal(float dx, const Level& level);
    void moveVertical(float dy, const Level& level);

    Vec2 pos_;
    Vec2 vel_;
    Vec2 spawn_;
    Vec2 size_{24.f, 32.f};

    bool onGround_ = false;
    bool keyLeft_  = false;
    bool keyRight_ = false;
    bool keyJump_  = false;
    bool fellOut_  = false;

    float killY_ = 10000.f;
    float coyoteTimer_    = 0.f;
    float jumpBufferTimer_= 0.f;
    bool  jumpConsumed_   = false;

    float invincibleTimer_ = 0.f;
    static constexpr float kInvincibleDuration = 1.5f;

    mutable sf::RectangleShape body_;
    mutable sf::RectangleShape eye_;
};