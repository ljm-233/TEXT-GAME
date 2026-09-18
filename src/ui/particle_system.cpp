#include "particle_system.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>

namespace {

float randf() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}
float randRange(float lo, float hi) {
    return lo + randf() * (hi - lo);
}
constexpr float kGravity = 800.f;

}

void ParticleSystem::emit(Vec2 pos, int count, sf::Color color,
                          float speedMin, float speedMax,
                          float lifeMin, float lifeMax,
                          float sizeMin, float sizeMax) {
    for (int i = 0; i < count; ++i) {
        if (particles_.size() >= kMaxParticles) break;
        Particle p;
        p.pos = pos;
        float angle = randRange(0.f, 6.2831853f);
        float speed = randRange(speedMin, speedMax);
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed - 100.f};
        p.color = color;
        p.life = randRange(lifeMin, lifeMax);
        p.maxLife = p.life;
        p.size = randRange(sizeMin, sizeMax);
        p.rotation = randRange(0.f, 360.f);
        p.rotSpeed = randRange(-180.f, 180.f);
        p.isSquare = true;
        particles_.push_back(p);
    }
}

void ParticleSystem::emitCoin(Vec2 pos) {
    emit(pos, 12, sf::Color(255, 210, 60),
         80.f, 200.f, 0.4f, 0.7f, 3.f, 5.f);
}

void ParticleSystem::emitJump(Vec2 pos) {
    emit(pos, 6, sf::Color(200, 220, 255),
         40.f, 100.f, 0.2f, 0.4f, 2.f, 4.f);
}

void ParticleSystem::emitStomp(Vec2 pos) {
    emit(pos, 14, sf::Color(240, 80, 80),
         100.f, 250.f, 0.4f, 0.7f, 3.f, 6.f);
}

void ParticleSystem::emitHurt(Vec2 pos) {
    emit(pos, 10, sf::Color(255, 100, 100),
         100.f, 200.f, 0.4f, 0.7f, 3.f, 5.f);
}

// ============================================================
// ⭐ 落地扬尘（增强版）
// ============================================================
void ParticleSystem::emitLand(Vec2 pos, float intensity) {
    // intensity 范围 0.5 ~ 1.5（轻度落地 ~ 高速落地）

    // ===== ① 两侧碎石（向左右上方散开）=====
    int pebbleCount = static_cast<int>(6 * intensity);
    for (int i = 0; i < pebbleCount; ++i) {
        if (particles_.size() >= kMaxParticles) break;
        Particle p;
        p.pos = pos;

        // 随机决定向左还是向右
        bool left = (i % 2 == 0);
        float angle = left
            ? randRange(-2.9f, -1.4f)   // 左上方向（弧度）
            : randRange(-1.7f, -0.2f);  // 右上方向

        float speed = randRange(60.f, 160.f) * intensity;
        p.vel = {std::cos(angle) * speed,
                 std::sin(angle) * speed - 30.f};

        // 灰白碎石
    std::uint8_t shade = static_cast<std::uint8_t>(randRange(140.f, 200.f));
    p.color = sf::Color(shade, shade,
                    static_cast<std::uint8_t>(shade + 10));

        p.life = randRange(0.25f, 0.45f);
        p.maxLife = p.life;
        p.size = randRange(2.f, 4.f);
        p.rotation = randRange(0.f, 360.f);
        p.rotSpeed = randRange(-360.f, 360.f);
        p.isSquare = true;

        particles_.push_back(p);
    }

    // ===== ② 尘云（大的淡色圆）=====
    int dustCount = static_cast<int>(4 * intensity);
    for (int i = 0; i < dustCount; ++i) {
        if (particles_.size() >= kMaxParticles) break;
        Particle p;
        p.pos = pos;
        p.pos.x += randRange(-8.f, 8.f);

        float angle = randRange(-2.8f, -0.35f);
        float speed = randRange(30.f, 70.f) * intensity;
        p.vel = {std::cos(angle) * speed,
                 std::sin(angle) * speed - 20.f};

        // 淡灰色尘云
        p.color = sf::Color(180, 180, 190, 180);

        p.life = randRange(0.4f, 0.7f);
        p.maxLife = p.life;
        p.size = randRange(6.f, 12.f) * intensity;
        p.rotation = 0.f;
        p.rotSpeed = 0.f;
        p.isSquare = false;   // 圆形

        particles_.push_back(p);
    }
}

// ⭐ 兼容旧接口
void ParticleSystem::emitDust(Vec2 pos, float intensity) {
    emitLand(pos, intensity);
}

void ParticleSystem::update(float dt) {
    for (auto& p : particles_) {
        p.vel.y += kGravity * dt;
        p.pos += p.vel * dt;
        p.life -= dt;
        p.rotation += p.rotSpeed * dt;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [](const Particle& p) { return p.life <= 0.f; }),
        particles_.end());
}

void ParticleSystem::render(sf::RenderTarget& target) {
    sf::RectangleShape rect;
    sf::CircleShape circle;

    for (const auto& p : particles_) {
        float alpha = p.life / p.maxLife;
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(
            std::min(255.f, alpha * p.color.a));

        if (p.isSquare) {
            // 方块（碎石）：带旋转
            rect.setFillColor(c);
            rect.setSize({p.size, p.size});
            rect.setOrigin({p.size * 0.5f, p.size * 0.5f});
            rect.setPosition({p.pos.x, p.pos.y});
            rect.setRotation(sf::degrees(p.rotation));
            target.draw(rect);
        } else {
            // 圆形（尘云）：随生命变大（扩散效果）
            float grow = 1.f + (1.f - alpha) * 0.8f;
            float r = p.size * 0.5f * grow;
            circle.setFillColor(c);
            circle.setRadius(r);
            circle.setOrigin({r, r});
            circle.setPosition({p.pos.x, p.pos.y});
            target.draw(circle);
        }
    }
}