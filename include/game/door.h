#pragma once
#include "game_object.h"
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Door : public GameObject {
public:
    Door(Vec2 pos, int tileSize);

    void update(float dt, const Level& level) override;
    void render(sf::RenderTarget& target) const override;
    AABB bounds() const override;
    Type type() const override { return Type::Door; }

    void unlock();
    bool isUnlocked() const { return unlocked_; }
    // 只有"已解锁 + 淡出结束"才算真正消失
    bool isGone() const { return unlocked_ && openTimer_ <= 0.f; }

private:
    Vec2 pos_;
    int  tileSize_;
    bool unlocked_ = false;
    float openTimer_ = 0.f;    // 0.4 秒淡出

    static constexpr float kOpenDuration = 0.4f;

    mutable sf::RectangleShape frame_;
    mutable sf::RectangleShape panel_;
    mutable sf::CircleShape    knob_;
    mutable sf::RectangleShape sparkle1_;
    mutable sf::RectangleShape sparkle2_;
};