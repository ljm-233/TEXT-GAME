#include "particle_system.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace {

float randf() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}
float randRange(float lo, float hi) {
    return lo + randf() * (hi - lo);
}
constexpr float kGravity = 800.f;

} // namespace

void ParticleSystem::emit(Vec2 pos, int count, sf::Color color, float speedMin,
                          float speedMax, float lifeMin, float lifeMax, float sizeMin,
                          float sizeMax) {
    for (int i = 0; i < count; ++i) {
        if (particles_.size() >= kMaxParticles)
            break;
        Particle p;
        p.pos = pos;
        float angle = randRange(0.f, 6.2831853f);
        float speed = randRange(speedMin, speedMax);
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed - 100.f};
        p.color = color;
        p.life = randRange(lifeMin, lifeMax);
        p.maxLife = p.life;
        p.size = randRange(sizeMin, sizeMax);
        particles_.push_back(p);
    }
}

void ParticleSystem::emitCoin(Vec2 pos) {
    emit(pos, 12, sf::Color(255, 210, 60), 80.f, 200.f, 0.4f, 0.7f, 3.f, 5.f);
}
void ParticleSystem::emitJump(Vec2 pos) {
    emit(pos, 6, sf::Color(200, 220, 255), 40.f, 100.f, 0.2f, 0.4f, 2.f, 4.f);
}
void ParticleSystem::emitLand(Vec2 pos) {
    emit(pos, 8, sf::Color(180, 180, 200), 60.f, 140.f, 0.2f, 0.4f, 3.f, 5.f);
}
void ParticleSystem::emitStomp(Vec2 pos) {
    emit(pos, 14, sf::Color(240, 80, 80), 100.f, 250.f, 0.4f, 0.7f, 3.f, 6.f);
}
void ParticleSystem::emitHurt(Vec2 pos) {
    emit(pos, 10, sf::Color(255, 100, 100), 100.f, 200.f, 0.4f, 0.7f, 3.f, 5.f);
}

void ParticleSystem::update(float dt) {
    for (auto& p : particles_) {
        p.vel.y += kGravity * dt;
        p.pos += p.vel * dt;
        p.life -= dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& p) { return p.life <= 0.f; }),
                     particles_.end());
}

void ParticleSystem::render(sf::RenderTarget& target) {
    sf::RectangleShape rect;
    for (const auto& p : particles_) {
        float alpha = p.life / p.maxLife;
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(alpha * 255.f);
        rect.setFillColor(c);
        rect.setSize({p.size, p.size});
        rect.setPosition({p.pos.x - p.size * 0.5f, p.pos.y - p.size * 0.5f});
        target.draw(rect);
    }
}