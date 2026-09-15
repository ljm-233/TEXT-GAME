#include "coin.h"
#include "coin_sprite_factory.h"
#include <cmath>
#include <cstdlib>
#include <vector>

namespace {
constexpr float kBobAmplitude = 4.f;
constexpr float kBobSpeed     = 3.f;
}

Coin::Coin(Vec2 pos, int tileSize)
    : pos_(pos), tileSize_(tileSize) {

    sheet_ = CoinSpriteFactory::getSheet();
    sprite_ = std::make_unique<sf::Sprite>(*sheet_);

    sprite_->setOrigin({
        CoinSpriteFactory::kFrameW * 0.5f,
        CoinSpriteFactory::kFrameH * 0.5f
    });

    // 缩放到 tileSize
    float scale = static_cast<float>(tileSize_) / CoinSpriteFactory::kFrameW;
    sprite_->setScale({scale, scale});

    // 动画帧
    const int fw = CoinSpriteFactory::kFrameW;
    const int fh = CoinSpriteFactory::kFrameH;

    std::vector<sf::IntRect> frames;
    for (int i = 0; i < CoinSpriteFactory::kFrameCount; ++i) {
        frames.push_back(sf::IntRect({i * fw, 0}, {fw, fh}));
    }

    // ⭐ 每个金币随机起始帧，不同步
    animator_.addClip("spin", {frames, 12.f, true});
    animator_.play("spin");

    // 随机推进几帧
    int randFrames = std::rand() % CoinSpriteFactory::kFrameCount;
    for (int i = 0; i < randFrames; ++i) {
        animator_.update(1.f / 24.f);
    }

    animator_.applyTo(*sprite_);
}

void Coin::update(float dt, const Level& /*level*/) {
    animTimer_ += dt;
    animator_.update(dt);
}

AABB Coin::bounds() const {
    float ts = static_cast<float>(tileSize_);
    return {pos_.x + ts * 0.15f, pos_.y + ts * 0.15f,
            ts * 0.7f, ts * 0.7f};
}

void Coin::render(sf::RenderTarget& target) const {
    if (collected_) return;

    float ts = static_cast<float>(tileSize_);
    float cx = pos_.x + ts * 0.5f;
    float cy = pos_.y + ts * 0.5f
             + std::sin(animTimer_ * kBobSpeed) * kBobAmplitude;

    animator_.applyTo(*sprite_);
    sprite_->setPosition({cx, cy});
    target.draw(*sprite_);
}