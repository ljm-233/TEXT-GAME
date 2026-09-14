#pragma once
#include "aabb.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Level;

// 所有游戏对象的基类
class GameObject {
public:
    virtual ~GameObject() = default;

    // 每帧更新（dt 是秒；level 用于地形碰撞查询）
    virtual void update(float dt, const Level& level) = 0;

    // 渲染（世界坐标，view 已经在做平移）
    virtual void render(sf::RenderTarget& target) const = 0;

    // 碰撞盒
    virtual AABB bounds() const = 0;

    // 类型标记（用于查找/区分）
    enum class Type { Player, Enemy, Coin, Goal, Platform };
    virtual Type type() const = 0;

    // 是否可以从世界移除（金币收集后、敌人死亡后）
    virtual bool isRemovable() const { return false; }
};