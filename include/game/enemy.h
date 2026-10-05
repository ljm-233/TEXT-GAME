#pragma once
#include "game_object.h"
#include "utils/vec2.h"
#include "utils/animator.h"
#include <SFML/Graphics.hpp>
#include <memory>

/// 敌人种类。关卡字符 → 行为：
///
///   Basic  ('E') 现有敌人：左右走，撞墙或平台边缘掉头。
///   Patrol ('W') 巡逻：与 Basic 同一套走路逻辑，唯一区别是**掉头时在端点停一下**
///                      （Basic 掉头立刻反向；见 Enemy::updateWalk）。
///                      说明：现有 E 的 cliffAhead 已经会在平台边缘掉头（有测试
///                      test_entities.cpp「Enemy - 走到平台边缘前掉头」守着），
///                      所以 W 与 E 的差别不能落在"会不会掉下平台"上，
///                      只能额外给 W 一个可辨认的行为。
///   Flyer  ('F') 飞行：不受重力，按正弦上下浮动 + 水平缓慢巡游，撞墙/关卡边界反向。
///   Jumper ('B') 跳跃：受重力，趴在地上，周期性起跳，落地后歇一会儿再跳。
enum class EnemyKind { Basic, Patrol, Flyer, Jumper };

class Enemy : public GameObject {
public:
    // sheet 为 nullptr 时进入"无 sprite"模式：实体照常参与逻辑与碰撞，
    // 只是画不出东西。测试用它，生产代码请传 EnemySpriteFactory::getSheet()。
    Enemy(Vec2 pos, int tileSize, std::shared_ptr<sf::Texture> sheet = nullptr);

    // 带种类的构造。旧的三个参数的重载等价于 EnemyKind::Basic ——
    // E 的既有代码路径与行为一个字都没动。
    Enemy(EnemyKind kind, Vec2 pos, int tileSize,
          std::shared_ptr<sf::Texture> sheet = nullptr);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Enemy; }
    bool isRemovable() const override { return killed_; }

    EnemyKind kind() const { return kind_; }

    void kill() { killed_ = true; }
    bool killed() const { return killed_; }

private:
    bool wallAhead(const Level& level) const;
    bool cliffAhead(const Level& level) const;
    bool outsideLevelX(const Level& level) const;

    void setupSprite();
    void updateWalk(float dt, const Level& level); // Basic / Patrol 共用
    void updateFlyer(float dt, const Level& level);
    void updateJumper(float dt, const Level& level);

    EnemyKind kind_ = EnemyKind::Basic;
    Vec2 pos_;
    Vec2 vel_;
    Vec2 size_{28.f, 28.f};
    int tileSize_;
    bool killed_ = false;

    // ==== 分种类状态（Basic 一个都不碰，保证既有行为逐字不变）====
    bool turnAtLedge_ = true;  // Basic/Patrol：走到平台边缘掉头（现有 E 就是这样）
    bool pauseAtTurn_ = false; // Patrol：掉头后在端点停 kPatrolPauseTime
    float pauseTimer_ = 0.f;   // Patrol：> 0 时原地不动
    float bobBaseY_ = 0.f;     // Flyer：正弦中心（出生时的 y）
    float bobPhase_ = 0.f;     // Flyer：正弦相位
    bool grounded_ = false;    // Jumper：是否踩在地上
    float jumpTimer_ = 0.f;    // Jumper：落地后的起跳倒计时

    std::shared_ptr<sf::Texture> sheet_;
    Animator animator_;
    mutable std::unique_ptr<sf::Sprite> sprite_;
};
