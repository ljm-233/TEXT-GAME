#include "player.h"
#include "level.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float GRAVITY       = 2200.f;
constexpr float MOVE_SPEED    = 300.f;
constexpr float JUMP_VELOCITY = -720.f;
constexpr float MAX_FALL      = 1000.f;
constexpr float COYOTE_TIME   = 0.10f;
constexpr float JUMP_BUFFER   = 0.12f;
}

Player::Player(Vec2 spawn)
    : pos_(spawn), spawn_(spawn) {
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
    invincibleTimer_ = kInvincibleDuration;
}

void Player::bounce() {
    vel_.y = -500.f;
    onGround_ = false;
    jumpConsumed_ = true;
}

void Player::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        switch (kp->code) {
            case sf::Keyboard::Key::A:
            case sf::Keyboard::Key::Left:  keyLeft_  = true; break;
            case sf::Keyboard::Key::D:
            case sf::Keyboard::Key::Right: keyRight_ = true; break;
            case sf::Keyboard::Key::W:
            case sf::Keyboard::Key::Up:
            case sf::Keyboard::Key::Space:
                if (!keyJump_) jumpBufferTimer_ = JUMP_BUFFER;
                keyJump_ = true;
                break;
            default: break;
        }
    }
    if (const auto* kr = event.getIf<sf::Event::KeyReleased>()) {
        switch (kr->code) {
            case sf::Keyboard::Key::A:
            case sf::Keyboard::Key::Left:  keyLeft_  = false; break;
            case sf::Keyboard::Key::D:
            case sf::Keyboard::Key::Right: keyRight_ = false; break;
            case sf::Keyboard::Key::W:
            case sf::Keyboard::Key::Up:
            case sf::Keyboard::Key::Space: keyJump_ = false; break;
            default: break;
        }
    }
}

void Player::update(float dt, const Level& level) {
    if (invincibleTimer_ > 0.f) invincibleTimer_ -= dt;

    float dir = 0.f;
    if (keyLeft_)  dir -= 1.f;
    if (keyRight_) dir += 1.f;
    vel_.x = dir * MOVE_SPEED;

    if (onGround_) coyoteTimer_ = COYOTE_TIME;
    else           coyoteTimer_ = std::max(0.f, coyoteTimer_ - dt);
    jumpBufferTimer_ = std::max(0.f, jumpBufferTimer_ - dt);

    if (jumpBufferTimer_ > 0.f && coyoteTimer_ > 0.f && !jumpConsumed_) {
        vel_.y = JUMP_VELOCITY;
        jumpBufferTimer_ = 0.f;
        coyoteTimer_ = 0.f;
        jumpConsumed_ = true;
        onGround_ = false;
        justJumped_ = true;
    }
    if (!keyJump_) jumpConsumed_ = false;

    if (!keyJump_ && vel_.y < 0.f) vel_.y *= 0.5f;

    vel_.y += GRAVITY * dt;
    if (vel_.y > MAX_FALL) vel_.y = MAX_FALL;

    onGround_ = false;
    moveHorizontal(vel_.x * dt, level);
    moveVertical(vel_.y * dt, level);

    if (pos_.y > killY_) {
        fellOut_ = true;
        pos_ = spawn_;
        vel_ = {0.f, 0.f};
    }

    // 落地检测
    if (onGround_ && !prevOnGround_) justLanded_ = true;
    prevOnGround_ = onGround_;
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

    body_.setPosition({pos_.x, pos_.y});
    target.draw(body_);

    eye_.setPosition({pos_.x + size_.x - 12.f, pos_.y + 8.f});
    target.draw(eye_);
}