#include "camera.h"
#include "animation.h"
#include <algorithm>
#include <cstdlib>

namespace {
float clampAxis(float desired, float view, float level) {
    if (level <= 0.f) return desired;
    if (level <= view) {
        return (level - view) * 0.5f;
    }
    return std::clamp(desired, 0.f, level - view);
}
}

void Camera::snapTo(Vec2 target) {
    float desiredX = target.x - viewW_ * 0.5f;
    float desiredY = target.y - viewH_ * 0.5f;
    pos_.x = clampAxis(desiredX, viewW_, levelW_);
    pos_.y = clampAxis(desiredY, viewH_, levelH_);
    shakeOffset_ = {0.f, 0.f};
    shakeTimer_ = 0.f;
}

void Camera::follow(Vec2 target, float dt) {
    float desiredX = target.x - viewW_ * 0.5f;
    float desiredY = target.y - viewH_ * 0.5f;

    desiredX = clampAxis(desiredX, viewW_, levelW_);
    desiredY = clampAxis(desiredY, viewH_, levelH_);

    pos_.x = Anim::approachF(pos_.x, desiredX, dt);
    pos_.y = Anim::approachF(pos_.y, desiredY, dt);
}

void Camera::shake(float intensity, float duration) {
    if (intensity <= 0.f || duration <= 0.f) return;
    // 更强的震动会覆盖更弱的
    if (intensity >= shakeIntensity_ || shakeTimer_ <= 0.f) {
        shakeIntensity_ = intensity;
        shakeDuration_ = duration;
        shakeTimer_ = duration;
    }
}

void Camera::updateShake(float dt) {
    if (shakeTimer_ <= 0.f) {
        shakeOffset_ = {0.f, 0.f};
        return;
    }
    shakeTimer_ -= dt;
    if (shakeTimer_ <= 0.f) {
        shakeOffset_ = {0.f, 0.f};
        return;
    }
    float t = shakeTimer_ / shakeDuration_;   // 1 → 0
    float amp = shakeIntensity_ * t;
    float rx = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.f * amp;
    float ry = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.f * amp;
    shakeOffset_ = {rx, ry};
}