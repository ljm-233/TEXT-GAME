#include "player.h"
#include "game_constants.h"
#include "level.h"
#include <algorithm>
#include <cmath>
#include "animation.h"

Player::Player(Vec2 spawn)
      : pos_(spawn),
        spawn_(spawn) {
    body_.setSize({size_.x, size_.y});
    body_.setFillColor(sf::Color(80, 200, 120));
    body_.setOutlineThickness(2.f);
    body_.setOutlineColor(sf::Color(40, 120, 70));

    eye_.setSize({6.f, 6.f});
    eye_.setFillColor(sf::Color(240, 240, 250));
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
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        switch (kp->code) {
        case sf::Keyboard::Key::A:
        case sf::Keyboard::Key::Left:
            keyLeft_ = true;
            break;
        case sf::Keyboard::Key::D:
        case sf::Keyboard::Key::Right:
            keyRight_ = true;
            break;
        case sf::Keyboard::Key::W:
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::Space:
            if (!keyJump_)
                jumpBufferTimer_ = GameConst::kPlayerJumpBuffer;
            keyJump_ = true;
            break;
        default:
            break;
        }
    }
    if (const auto* kr = event.getIf<sf::Event::KeyReleased>()) {
        switch (kr->code) {
        case sf::Keyboard::Key::A:
        case sf::Keyboard::Key::Left:
            keyLeft_ = false;
            break;
        case sf::Keyboard::Key::D:
        case sf::Keyboard::Key::Right:
            keyRight_ = false;
            break;
        case sf::Keyboard::Key::W:
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::Space:
            keyJump_ = false;
            break;
        default:
            break;
        }
    }
}

void Player::update(float dt, const Level& level) {
    if (invincibleTimer_ > 0.f)
        invincibleTimer_ -= dt;

    float dir = 0.f;
    if (keyLeft_)
        dir -= 1.f;
    if (keyRight_)
        dir += 1.f;
    vel_.x = dir * GameConst::kPlayerMoveSpeed;

    if (onGround_)
        coyoteTimer_ = GameConst::kPlayerCoyoteTime;
    else
        coyoteTimer_ = std::max(0.f, coyoteTimer_ - dt);
    jumpBufferTimer_ = std::max(0.f, jumpBufferTimer_ - dt);

    if (jumpBufferTimer_ > 0.f && coyoteTimer_ > 0.f && !jumpConsumed_) {
        vel_.y = GameConst::kPlayerJumpVelocity;
        jumpBufferTimer_ = 0.f;
        coyoteTimer_ = 0.f;
        jumpConsumed_ = true;
        onGround_ = false;
        justJumped_ = true;
    }
    if (!keyJump_)
        jumpConsumed_ = false;

    if (!keyJump_ && vel_.y < 0.f)
        vel_.y *= 0.5f;

    vel_.y += GameConst::kPlayerGravity * dt;
    if (vel_.y > GameConst::kPlayerMaxFall)
        vel_.y = GameConst::kPlayerMaxFall;

    onGround_ = false;
    moveHorizontal(vel_.x * dt, level);
    moveVertical(vel_.y * dt, level);

    if (pos_.y > killY_) {
        fellOut_ = true;
        pos_ = spawn_;
        vel_ = {0.f, 0.f};
    }

    if (onGround_ && !prevOnGround_)
        justLanded_ = true;
    prevOnGround_ = onGround_;

        // ===== 弹性动画 =====
    // 根据状态设目标缩放
    if (justJumped_) {
        targetScale_ = {0.85f, 1.15f};   // 跳起：拉长
    } else if (justLanded_) {
        targetScale_ = {1.15f, 0.85f};   // 落地：压扁
    } else {
        targetScale_ = {1.f, 1.f};       // 默认
    }

    // 平滑逼近
    currentScale_.x = Anim::approachF(currentScale_.x, targetScale_.x, dt * 6.f);
    currentScale_.y = Anim::approachF(currentScale_.y, targetScale_.y, dt * 6.f);
}

void Player::moveHorizontal(float dx, const Level& level) {
    if (dx == 0.f)
        return;

    float targetX = pos_.x + dx;
    AABB box{targetX, pos_.y, size_.x, size_.y};

    int ts = level.tileSize();
    int tx0 = static_cast<int>(std::floor(box.left() / ts));
    int tx1 = static_cast<int>(std::floor((box.right() - 0.001f) / ts));
    int ty0 = static_cast<int>(std::floor(box.top() / ts));
    int ty1 = static_cast<int>(std::floor((box.bottom() - 0.001f) / ts));

    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (!level.isSolid(tx, ty))
                continue;
            if (dx > 0.f)
                pos_.x = static_cast<float>(tx * ts) - size_.x;
            else
                pos_.x = static_cast<float>((tx + 1) * ts);
            vel_.x = 0.f;
            return;
        }
    }
    pos_.x = targetX;
}

void Player::moveVertical(float dy, const Level& level) {
    if (dy == 0.f)
        return;

    float targetY = pos_.y + dy;
    AABB box{pos_.x, targetY, size_.x, size_.y};

    int ts = level.tileSize();
    int tx0 = static_cast<int>(std::floor(box.left() / ts));
    int tx1 = static_cast<int>(std::floor((box.right() - 0.001f) / ts));
    int ty0 = static_cast<int>(std::floor(box.top() / ts));
    int ty1 = static_cast<int>(std::floor((box.bottom() - 0.001f) / ts));

    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (!level.isSolid(tx, ty))
                continue;
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

    // 弹性：围绕底部中心缩放
    body_.setOrigin({size_.x * 0.5f, size_.y});
    body_.setScale(currentScale_);
    body_.setPosition({pos_.x + size_.x * 0.5f, pos_.y + size_.y});
    target.draw(body_);

    // 眼睛跟着缩放
    eye_.setScale(currentScale_);
    eye_.setPosition({
        pos_.x + size_.x * 0.5f + (size_.x * 0.5f - 12.f) * currentScale_.x,
        pos_.y + size_.y + (-size_.y + 8.f) * currentScale_.y
    });
    target.draw(eye_);
}