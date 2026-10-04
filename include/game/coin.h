#pragma once
#include "game_object.h"
#include "utils/vec2.h"
#include "utils/animator.h"
#include <SFML/Graphics.hpp>
#include <memory>

class Coin : public GameObject {
public:
    // sheet 为 nullptr 时进入"无 sprite"模式：实体照常参与逻辑与碰撞，
    // 只是画不出东西。测试用它，生产代码请传 CoinSpriteFactory::getSheet()。
    Coin(Vec2 pos, int tileSize, std::shared_ptr<sf::Texture> sheet = nullptr);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Coin; }
    bool isRemovable() const override { return collected_; }

    void collect() { collected_ = true; }
    bool collected() const { return collected_; }

private:
    Vec2 pos_;
    int  tileSize_;
    bool collected_ = false;
    float animTimer_ = 0.f;

    std::shared_ptr<sf::Texture>       sheet_;
    Animator                           animator_;
    mutable std::unique_ptr<sf::Sprite> sprite_;
};