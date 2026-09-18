#include "player.h"
#include "level.h"
#include "animation.h"
#include "game_constants.h"
#include "player_sprite_factory.h"
#include "gamepad.h"
#include "gamepad_config.h"
#include "keybindings.h"
#include <algorithm>
#include <cmath>

Player::Player(Vec2 spawn, std::shared_ptr<sf::Texture> sheet)
    : pos_(spawn), spawn_(spawn) {

    // 无 sprite 模式（测试用）：跳过所有纹理/动画初始化
    if (!sheet)
        return;

    sheet_ = sheet;
    sprite_ = std::make_unique<sf::Sprite>(*sheet_);

    sprite_->setOrigin({PlayerSpriteFactory::kFrameW * 0.5f,
                        static_cast<float>(PlayerSpriteFactory::kFrameH)});

    const int fw = PlayerSpriteFactory::kFrameW;
    const int fh = PlayerSpriteFactory::kFrameH;

    auto makeFrame = [&](int index) {
        return sf::IntRect({index * fw, 0}, {fw, fh});
    };

    animator_.addClip("idle", {{makeFrame(0), makeFrame(1)}, 4.f,  true});
    animator_.addClip("run",  {{makeFrame(2), makeFrame(3),
                               makeFrame(4), makeFrame(5)}, 12.f, true});
    animator_.addClip("jump", {{makeFrame(6)}, 1.f, false});
    animator_.addClip("fall", {{makeFrame(7)}, 1.f, false});

    animator_.play("idle");
    animator_.applyTo(*sprite_);
}

void Player::respawn(Vec2 spawn) {
    pos_ = spawn;
    spawn_ = spawn;
    vel_ = {0.f, 0.f};
    onGround_ = false;
    coyoteTimer_ = 0.f;
    jumpBufferTimer_ = 0.f;
    jumpConsumed_ = false;
    fellOut_ = false;
    invincibleTimer_ = 0.f;
    justJumped_ = false;
    justLanded_ = false;
    prevOnGround_ = false;
    currentScale_ = {1.f, 1.f};
    targetScale_ = {1.f, 1.f};
    keyboardLeft_  = false;
    keyboardRight_ = false;
    keyboardJump_  = false;
    gamepadLeft_   = false;
    gamepadRight_  = false;
    gamepadJump_   = false;
}

void Player::takeDamage() {
    invincibleTimer_ = GameConst::kPlayerInvincibleDur;
}

void Player::bounce() {
    vel_.y = GameConst::kPlayerBounceSpeed;
    onGround_ = false;
    jumpConsumed_ = true;
}

void Player::handleEvent(const sf::Event& event) {
    const auto kbLeft  = KeyBindings::instance().get(KeyBindings::MoveLeft);
    const auto kbRight = KeyBindings::instance().get(KeyBindings::MoveRight);
    const auto kbJump  = KeyBindings::instance().get(KeyBindings::Jump);

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == kbLeft)  keyboardLeft_  = true;
        if (kp->code == kbRight) keyboardRight_ = true;
        if (kp->code == kbJump) {
            if (!keyboardJump_) jumpBufferTimer_ = GameConst::kPlayerJumpBuffer;
            keyboardJump_ = true;
        }
    }
    if (const auto* kr = event.getIf<sf::Event::KeyReleased>()) {
        if (kr->code == kbLeft)  keyboardLeft_  = false;
        if (kr->code == kbRight) keyboardRight_ = false;
        if (kr->code == kbJump)  keyboardJump_ = false;
    }
}

void Player::handleGamepad() {
    if (!gamepadEnabled_) {
        gamepadLeft_  = false;
        gamepadRight_ = false;
        gamepadJump_  = false;
        return;
    }

    auto& gp = Gamepad::instance();
    if (!gp.isConnected()) {
        gamepadLeft_  = false;
        gamepadRight_ = false;
        gamepadJump_  = false;
        return;
    }

    float x = gp.leftX();
    constexpr float T = GamepadConfig::kMoveStickThreshold;
    gamepadLeft_  = (x < -T) || gp.dpadLeft();
    gamepadRight_ = (x >  T) || gp.dpadRight();

    bool jumpNow = gp.jumpPressed();
    if (jumpNow && !gamepadJump_) {
        jumpBufferTimer_ = GameConst::kPlayerJumpBuffer;
    }
    gamepadJump_ = jumpNow;
}

void Player::update(float dt, const Level& level) {
    if (invincibleTimer_ > 0.f) invincibleTimer_ -= dt;

    // 合并输入：手柄方向优先，键盘作为后备
    float dir = 0.f;
    if (keyboardLeft_)  dir -= 1.f;
    if (keyboardRight_) dir += 1.f;
    if (gamepadLeft_)       dir = -1.f;
    else if (gamepadRight_) dir =  1.f;

    vel_.x = dir * GameConst::kPlayerMoveSpeed;

    if (onGround_) coyoteTimer_ = GameConst::kPlayerCoyoteTime;
    else           coyoteTimer_ = std::max(0.f, coyoteTimer_ - dt);
    jumpBufferTimer_ = std::max(0.f, jumpBufferTimer_ - dt);

    bool jumpHeld = keyboardJump_ || gamepadJump_;

    if (jumpBufferTimer_ > 0.f && coyoteTimer_ > 0.f && !jumpConsumed_) {
        vel_.y = GameConst::kPlayerJumpVelocity;
        jumpBufferTimer_ = 0.f;
        coyoteTimer_ = 0.f;
        jumpConsumed_ = true;
        onGround_ = false;
        justJumped_ = true;
    }
    if (!jumpHeld) jumpConsumed_ = false;

    if (!jumpHeld && vel_.y < 0.f) vel_.y *= 0.5f;

    vel_.y += GameConst::kPlayerGravity * dt;
    if (vel_.y > GameConst::kPlayerMaxFall) vel_.y = GameConst::kPlayerMaxFall;

    onGround_ = false;
    moveHorizontal(vel_.x * dt, level);
    moveVertical(vel_.y * dt, level);

    if (pos_.y > killY_) {
        fellOut_ = true;
        pos_ = spawn_;
        vel_ = {0.f, 0.f};
    }

    if (onGround_ && !prevOnGround_) justLanded_ = true;
    prevOnGround_ = onGround_;

    if (justJumped_) {
        targetScale_ = {0.85f, 1.15f};
    } else if (justLanded_) {
        targetScale_ = {1.15f, 0.85f};
    } else {
        targetScale_ = {1.f, 1.f};
    }
    currentScale_.x = Anim::approachF(currentScale_.x, targetScale_.x, dt * 6.f);
    currentScale_.y = Anim::approachF(currentScale_.y, targetScale_.y, dt * 6.f);

    updateAnimation(dt);
}

void Player::updateAnimation(float dt) {
    if (!onGround_) {
        if (vel_.y < 0.f) animator_.play("jump");
        else              animator_.play("fall");
    } else if (std::abs(vel_.x) > 1.f) {
        animator_.play("run");
    } else {
        animator_.play("idle");
    }
    animator_.update(dt);
}

void Player::moveHorizontal(float dx, const Level& level) {
    if (dx == 0.f) return;

    float targetX = pos_.x + dx;
    AABB box{targetX, pos_.y, size_.x, size_.y};

    int ts = level.tileSize();
    int tx0 = static_cast<int>(std::floor(box.left() / ts));
    int tx1 = static_cast<int>(std::floor((box.right() - 0.001f) / ts));
    int ty0 = static_cast<int>(std::floor(box.top() / ts));
    int ty1 = static_cast<int>(std::floor((box.bottom() - 0.001f) / ts));

    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (!level.isSolid(tx, ty)) continue;
            if (dx > 0.f) pos_.x = static_cast<float>(tx * ts) - size_.x;
            else          pos_.x = static_cast<float>((tx + 1) * ts);
            vel_.x = 0.f;
            return;
        }
    }
    pos_.x = targetX;
}

void Player::moveVertical(float dy, const Level& level) {
    if (dy == 0.f) return;

    float targetY = pos_.y + dy;
    AABB box{pos_.x, targetY, size_.x, size_.y};

    int ts = level.tileSize();
    int tx0 = static_cast<int>(std::floor(box.left() / ts));
    int tx1 = static_cast<int>(std::floor((box.right() - 0.001f) / ts));
    int ty0 = static_cast<int>(std::floor(box.top() / ts));
    int ty1 = static_cast<int>(std::floor((box.bottom() - 0.001f) / ts));

    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (!level.isSolid(tx, ty)) continue;
            if (dy > 0.f) {
                pos_.y = static_cast<float>(ty * ts) - size_.y;
                onGround_ = true;
            } else {
                pos_.y = static_cast<float>((ty + 1) * ts);
            }
            vel_.y = 0.f;
            return;
        }
    }
    pos_.y = targetY;
}

void Player::render(sf::RenderTarget& target) const {
    if (invincibleTimer_ > 0.f) {
        auto ms = static_cast<int>(invincibleTimer_ * 1000.f);
        if ((ms / 100) % 2 == 0) return;
    }

    if (!animationEnabled_ || !sprite_) {
        sf::RectangleShape body({size_.x, size_.y});
        body.setPosition({pos_.x, pos_.y});
        body.setFillColor(sf::Color(80, 200, 120));
        body.setOutlineThickness(2.f);
        body.setOutlineColor(sf::Color(40, 120, 70));
        target.draw(body);
        return;
    }

    animator_.applyTo(*sprite_);
    sprite_->setScale(currentScale_);
    sprite_->setPosition({
        pos_.x + size_.x * 0.5f,
        pos_.y + size_.y
    });
    target.draw(*sprite_);
}