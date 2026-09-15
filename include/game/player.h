#pragma once
#include "game_object.h"
#include "vec2.h"
#include "aabb.h"
#include "animator.h"
#include <SFML/Graphics.hpp>
#include <memory>

class Player : public GameObject {
public:
    explicit Player(Vec2 spawn);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override {
        return {pos_.x, pos_.y, size_.x, size_.y};
    }
    Type type() const override { return Type::Player; }

    void handleEvent(const sf::Event& event);

    // 每帧从手柄读输入（在 update 前调用）
    void handleGamepad();

    Vec2 position() const { return pos_; }
    Vec2 velocity() const { return vel_; }
    bool onGround() const { return onGround_; }

    void respawn(Vec2 spawn);
    void setKillY(float y) { killY_ = y; }
    void setSpawn(Vec2 spawn) { spawn_ = spawn; }

    void takeDamage();
    bool isInvincible() const { return invincibleTimer_ > 0.f; }
    void bounce();

    void setVelocityY(float vy) { vel_.y = vy; onGround_ = false; jumpConsumed_ = true; }
    void setPositionY(float y) { pos_.y = y; onGround_ = false; }
    void moveBy(Vec2 delta) { pos_ += delta; }
    void landOnPlatform(float topY) {
        pos_.y = topY - size_.y;
        vel_.y = 0.f;
        onGround_ = true;
    }

    void setAnimationEnabled(bool e) { animationEnabled_ = e; }
    void setGamepadEnabled(bool e)   { gamepadEnabled_ = e; }

    bool consumeFellOut() { bool f = fellOut_; fellOut_ = false; return f; }
    bool consumeJustJumped() { bool j = justJumped_; justJumped_ = false; return j; }
    bool consumeJustLanded() { bool l = justLanded_; justLanded_ = false; return l; }

private:
    void moveHorizontal(float dx, const Level& level);
    void moveVertical(float dy, const Level& level);
    void updateAnimation(float dt);

    Vec2 pos_;
    Vec2 vel_;
    Vec2 spawn_;
    Vec2 size_{24.f, 32.f};

    bool onGround_ = false;
    bool keyLeft_  = false;
    bool keyRight_ = false;
    bool keyJump_  = false;
    bool fellOut_  = false;
    bool justJumped_  = false;
    bool justLanded_  = false;
    bool prevOnGround_ = false;

    float killY_ = 10000.f;
    float coyoteTimer_    = 0.f;
    float jumpBufferTimer_= 0.f;
    bool  jumpConsumed_   = false;

    float invincibleTimer_ = 0.f;

    bool animationEnabled_ = true;
    bool gamepadEnabled_   = true;

    // 弹性动画
    sf::Vector2f currentScale_{1.f, 1.f};
    sf::Vector2f targetScale_{1.f, 1.f};

    // 精灵动画
    std::shared_ptr<sf::Texture>       sheet_;
    Animator                           animator_;
    std::unique_ptr<sf::Sprite>        sprite_;
};