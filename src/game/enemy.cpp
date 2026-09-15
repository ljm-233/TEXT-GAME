#include "enemy.h"
#include "level.h"
#include "enemy_sprite_factory.h"
#include "game_constants.h"
#include <cmath>
#include <vector>

namespace {
constexpr float kSpeed = GameConst::kEnemySpeed;
}

Enemy::Enemy(Vec2 pos, int tileSize)
    : pos_(pos), vel_(-kSpeed, 0.f), tileSize_(tileSize) {
    pos_.x += (tileSize_ - size_.x) * 0.5f;
    pos_.y += (tileSize_ - size_.y) * 0.5f;

    sheet_ = EnemySpriteFactory::getSheet();
    sprite_ = std::make_unique<sf::Sprite>(*sheet_);

    // 原点在帧底部中心
    sprite_->setOrigin({
        EnemySpriteFactory::kFrameW * 0.5f,
        static_cast<float>(EnemySpriteFactory::kFrameH)
    });

    // 缩放到 tileSize
    float scale = static_cast<float>(tileSize_) / EnemySpriteFactory::kFrameW;
    sprite_->setScale({scale, scale});

    // 动画剪辑
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

void Enemy::update(float dt, const Level& level) {
    if (killed_) return;

    if (wallAhead(level) || cliffAhead(level)) {
        vel_.x = -vel_.x;
    }
    pos_.x += vel_.x * dt;

    animator_.update(dt);
}

void Enemy::render(sf::RenderTarget& target) const {
    if (killed_) return;

    animator_.applyTo(*sprite_);

    // 朝向：向左移动时水平翻转
    float scale = static_cast<float>(tileSize_) / EnemySpriteFactory::kFrameW;
    float dir = (vel_.x > 0.f) ? 1.f : -1.f;

    sprite_->setScale({scale * dir, scale});
    sprite_->setPosition({
        pos_.x + size_.x * 0.5f,
        pos_.y + size_.y
    });
    target.draw(*sprite_);
}