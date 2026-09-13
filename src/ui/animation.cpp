#include "animation.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

bool  g_enabled = true;
float g_speed   = 1.0f;

// 基础速率：每秒逼近比例。10 表示 0.1 秒内基本到位
constexpr float kBaseRate = 10.0f;

std::uint8_t lerp8(std::uint8_t a, std::uint8_t b, float t) {
    float v = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t + 0.5f;
    if (v < 0.f) v = 0.f;
    if (v > 255.f) v = 255.f;
    return static_cast<std::uint8_t>(v);
}

} // namespace

namespace Anim {

bool  isEnabled() { return g_enabled; }
void  setEnabled(bool e) { g_enabled = e; }

float getSpeed() { return g_speed; }
void  setSpeed(float s) { g_speed = std::clamp(s, 0.1f, 5.0f); }

float lerpF(float a, float b, float t) {
    return a + (b - a) * t;
}

sf::Color lerp(sf::Color a, sf::Color b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        lerp8(a.r, b.r, t),
        lerp8(a.g, b.g, t),
        lerp8(a.b, b.b, t),
        lerp8(a.a, b.a, t));
}

sf::Color approach(sf::Color current, sf::Color target, float dt) {
    if (!g_enabled) return target;
    float k = kBaseRate * g_speed;
    float t = 1.f - std::exp(-k * dt);
    return lerp(current, target, t);
}

float approachF(float current, float target, float dt) {
    if (!g_enabled) return target;
    float k = kBaseRate * g_speed;
    float t = 1.f - std::exp(-k * dt);
    return lerpF(current, target, t);
}

} // namespace Anim