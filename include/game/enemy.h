#pragma once
#include "game_object.h"
#include "utils/vec2.h"
#include "utils/animator.h"
#include <SFML/Graphics.hpp>
#include <memory>

class Enemy : public GameObject {
public:
    // sheet 为 nullptr 时进入"无 sprite"模式：实体照常参与逻辑与碰撞，
    // 只是画不出东西。测试用它，生产代码请传 EnemySpriteFactory::getSheet()。
    Enemy(Vec2 pos, int tileSize, std::shared_ptr<sf::Texture> sheet = nullptr);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Enemy; }
    bool isRemovable() const override { return killed_; }

    void kill() { killed_ = true; }
    bool killed() const { return killed_; }

private:
    bool wallAhead(const Level& level) const;
    bool cliffAhead(const Level& level) const;

    Vec2 pos_;
    Vec2 vel_;
    Vec2 size_{28.f, 28.f};
    int  tileSize_;
    bool killed_ = false;

    std::shared_ptr<sf::Texture>       sheet_;
    Animator                           animator_;
    mutable std::unique_ptr<sf::Sprite> sprite_;
};