#pragma once
#include "vec2.h"
#include <SFML/Graphics.hpp>
#include <vector>

struct Particle {
    Vec2 pos;
    Vec2 vel;
    sf::Color color;
    float life    = 0.f;
    float maxLife = 1.f;
    float size    = 4.f;
    float rotation = 0.f;   // 角度（度）
    float rotSpeed = 0.f;   // 每秒旋转度数
    bool  isSquare = true;  // 方块 or 圆
};

class ParticleSystem {
public:
    void emit(Vec2 pos, int count, sf::Color color,
              float speedMin, float speedMax,
              float lifeMin, float lifeMax,
              float sizeMin, float sizeMax);

    void emitCoin(Vec2 pos);
    void emitJump(Vec2 pos);
    void emitLand(Vec2 pos, float intensity = 1.f);   // ⭐ 加强度
    void emitStomp(Vec2 pos);
    void emitHurt(Vec2 pos);
    void emitDust(Vec2 pos, float intensity);          // ⭐ 新：落地尘云

    void update(float dt);
    void render(sf::RenderTarget& target);
    void clear() { particles_.clear(); }
    int  count() const { return static_cast<int>(particles_.size()); }

private:
    std::vector<Particle> particles_;
    static constexpr std::size_t kMaxParticles = 500;
};