#include "enemy.h"
#include "level.h"
#include "enemy_sprite_factory.h"
#include "game_constants.h"
#include <cmath>
#include <utility>
#include <vector>

namespace {
constexpr float kSpeed = GameConst::kEnemySpeed;

/// 三种新敌人复用同一张贴图，只改色调，省一张 sheet。
sf::Color kindTint(EnemyKind kind) {
    switch (kind) {
    case EnemyKind::Basic:
        return sf::Color::White;
    case EnemyKind::Patrol:
        return sf::Color(255, 200, 150); // 偏橙
    case EnemyKind::Flyer:
        return sf::Color(170, 210, 255); // 偏蓝
    case EnemyKind::Jumper:
        return sf::Color(190, 255, 190); // 偏绿
    }
    return sf::Color::White; // 加了新 kind 却忘了这里时，别静默画成透明
}
} // namespace

Enemy::Enemy(Vec2 pos, int tileSize, std::shared_ptr<sf::Texture> sheet)
      : Enemy(EnemyKind::Basic, pos, tileSize, std::move(sheet)) {}

Enemy::Enemy(EnemyKind kind, Vec2 pos, int tileSize, std::shared_ptr<sf::Texture> sheet)
      : kind_(kind),
        pos_(pos),
        tileSize_(tileSize),
        sheet_(std::move(sheet)) {
    pos_.x += (tileSize_ - size_.x) * 0.5f;
    pos_.y += (tileSize_ - size_.y) * 0.5f;

    switch (kind_) {
    case EnemyKind::Basic:
        vel_ = Vec2{-kSpeed, 0.f}; // ★ 与旧代码逐字一致
        break;
    case EnemyKind::Patrol:
        vel_ = Vec2{-kSpeed, 0.f};
        pauseAtTurn_ = true; // ★ W 与 E 的唯一区别
        break;
    case EnemyKind::Flyer:
        vel_ = Vec2{-GameConst::kFlyerSpeed, 0.f};
        bobBaseY_ = pos_.y;   // 正弦围绕出生高度上下浮动，不受重力
        turnAtLedge_ = false; // 飞行不看脚下有没有地
        break;
    case EnemyKind::Jumper:
        vel_ = Vec2{0.f, 0.f}; // 只上下跳，不横向移动
        turnAtLedge_ = false;
        break;
    }

    setupSprite();
}

void Enemy::setupSprite() {
    // 同 Coin：sheet 为空就进"无 sprite"模式，逻辑照常跑。
    if (!sheet_)
        return;

    sprite_ = std::make_unique<sf::Sprite>(*sheet_);
    sprite_->setColor(kindTint(kind_)); // 复用贴图，用色调区分种类

    // 原点在帧底部中心
    sprite_->setOrigin({EnemySpriteFactory::kFrameW * 0.5f,
                        static_cast<float>(EnemySpriteFactory::kFrameH)});

    // 缩放到 tileSize
    float scale = static_cast<float>(tileSize_) / EnemySpriteFactory::kFrameW;
    sprite_->setScale({scale, scale});

    // 动画剪辑（四种敌人共用走路帧）
    const int fw = EnemySpriteFactory::kFrameW;
    const int fh = EnemySpriteFactory::kFrameH;

    std::vector<sf::IntRect> frames;
    for (int i = 0; i < EnemySpriteFactory::kFrameCount; ++i) {
        frames.push_back(sf::IntRect({i * fw, 0}, {fw, fh}));
    }

    animator_.addClip("walk", {frames, 8.f, true});
    animator_.play("walk");
    animator_.applyTo(*sprite_);
}

AABB Enemy::bounds() const {
    return {pos_.x, pos_.y, size_.x, size_.y};
}

bool Enemy::wallAhead(const Level& level) const {
    int ts = level.tileSize();
    AABB box = bounds();

    float probeX = (vel_.x > 0.f) ? box.right() + 1.f : box.left() - 1.f;
    int tx = static_cast<int>(std::floor(probeX / ts));
    int tyTop = static_cast<int>(std::floor(box.top() / ts));
    int tyBot = static_cast<int>(std::floor((box.bottom() - 0.001f) / ts));

    return level.isSolid(tx, tyTop) || level.isSolid(tx, tyBot);
}

bool Enemy::cliffAhead(const Level& level) const {
    int ts = level.tileSize();
    AABB box = bounds();

    float probeX = (vel_.x > 0.f) ? box.right() + 2.f : box.left() - 2.f;
    int tx = static_cast<int>(std::floor(probeX / ts));
    int ty = static_cast<int>(std::floor((box.bottom() + 2.f) / ts));

    return !level.isSolid(tx, ty);
}

bool Enemy::outsideLevelX(const Level& level) const {
    AABB box = bounds();
    return box.left() < 0.f || box.right() > static_cast<float>(level.pixelWidth());
}

void Enemy::update(float dt, const Level& level) {
    if (killed_)
        return;

    switch (kind_) {
    case EnemyKind::Basic:
    case EnemyKind::Patrol:
        updateWalk(dt, level);
        break;
    case EnemyKind::Flyer:
        updateFlyer(dt, level);
        break;
    case EnemyKind::Jumper:
        updateJumper(dt, level);
        break;
    }

    animator_.update(dt);
}

void Enemy::updateWalk(float dt, const Level& level) {
    if (wallAhead(level) || (turnAtLedge_ && cliffAhead(level))) {
        vel_.x = -vel_.x;
        if (pauseAtTurn_)
            pauseTimer_ = GameConst::kPatrolPauseTime;
    }

    // Patrol 在端点停顿：只对 pauseAtTurn_ 的 kind 生效，Basic 的 pauseTimer_
    // 永远是 0，于是这一步对 E 是纯粹的空操作。
    if (pauseTimer_ > 0.f) {
        pauseTimer_ -= dt;
        return;
    }

    pos_.x += vel_.x * dt;
}

void Enemy::updateFlyer(float dt, const Level& level) {
    if (wallAhead(level) || outsideLevelX(level))
        vel_.x = -vel_.x;

    pos_.x += vel_.x * dt;

    // 不受重力：y 完全由正弦决定，永远在出生高度上下浮动
    bobPhase_ += GameConst::kFlyerBobOmega * dt;
    pos_.y = bobBaseY_ + GameConst::kFlyerBobAmp * std::sin(bobPhase_);
}

void Enemy::updateJumper(float dt, const Level& level) {
    if (grounded_) {
        if (jumpTimer_ > 0.f)
            jumpTimer_ -= dt;
        if (jumpTimer_ <= 0.f) {
            vel_.y = -GameConst::kEnemyJumpVelocity; // 起跳
            grounded_ = false;
        }
    }

    vel_.y += GameConst::kEnemyGravity * dt;
    pos_.y += vel_.y * dt;

    if (vel_.y <= 0.f) // 上升中，不会穿地
        return;

    // 落地：脚下那一格是实心就吸附上去。
    // 固定步长 1/120 下一步最多移动 ~4px，远小于一格，不会穿过地板。
    const float ts = static_cast<float>(level.tileSize());
    const AABB box = bounds();
    const int tx = static_cast<int>(std::floor((box.x + box.w * 0.5f) / ts));
    const int ty = static_cast<int>(std::floor(box.bottom() / ts));
    if (!level.isSolid(tx, ty))
        return;

    pos_.y = static_cast<float>(ty) * ts - size_.y;
    vel_.y = 0.f;
    if (!grounded_) {
        grounded_ = true;
        jumpTimer_ = GameConst::kEnemyJumpRest; // 落地后歇一会儿再跳
    }
}

void Enemy::render(sf::RenderTarget& target) const {
    if (killed_ || !sprite_)
        return;

    animator_.applyTo(*sprite_);

    // 朝向：向左移动时水平翻转
    float scale = static_cast<float>(tileSize_) / EnemySpriteFactory::kFrameW;
    float dir = (vel_.x > 0.f) ? 1.f : -1.f;

    sprite_->setScale({scale * dir, scale});
    sprite_->setPosition({pos_.x + size_.x * 0.5f, pos_.y + size_.y});
    target.draw(*sprite_);
}
