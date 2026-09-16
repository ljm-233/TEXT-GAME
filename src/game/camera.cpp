#include "camera.h"
#include "animation.h"
#include <algorithm>
#include <cstdlib>

namespace {
float clampAxis(float desired, float view, float level) {
    if (level <= 0.f)
        return desired;
    if (level <= view) {
        return (level - view) * 0.5f;
    }
    return std::clamp(desired, 0.f, level - view);
}
} // namespace

void Camera::snapTo(Vec2 target) {
    float desiredX = target.x - viewW_ * 0.5f;
    float desiredY = target.y - viewH_ * 0.5f;
    pos_.x = clampAxis(desiredX, viewW_, levelW_);
    pos_.y = clampAxis(desiredY, viewH_, levelH_);
    shakeOffset_ = {0.f, 0.f};
    shakeTimer_ = 0.f;
    lookOffset_ = 0.f;
}

void Camera::follow(Vec2 target, Vec2 targetVel, float dt) {
    // ===== 前瞻 =====
    // 目标偏移量 = 速度 * 系数，clamp 到最大值
    float desiredLook = std::clamp(targetVel.x * 0.35f,
                                   -lookAheadMax_, lookAheadMax_);
    // 平滑（避免瞬间切换方向）
    lookOffset_ = Anim::approachF(lookOffset_, desiredLook, dt * 3.f);

    // 加前瞻偏移后的目标位置
    float aimedX = target.x + lookOffset_;

    // ===== 死区 =====
    // 当前镜头中心
    float camCenterX = pos_.x + viewW_ * 0.5f;
    float camCenterY = pos_.y + viewH_ * 0.5f;

    float halfDeadW = deadZoneW_ * 0.5f;
    float halfDeadH = deadZoneH_ * 0.5f;

    // 目标离开死区时，镜头中心向目标推近，使目标刚好在死区边缘
    float desiredCamCenterX = camCenterX;
    float desiredCamCenterY = camCenterY;

    if (aimedX < camCenterX - halfDeadW) {
        desiredCamCenterX = aimedX + halfDeadW;
    } else if (aimedX > camCenterX + halfDeadW) {
        desiredCamCenterX = aimedX - halfDeadW;
    }

    if (target.y < camCenterY - halfDeadH) {
        desiredCamCenterY = target.y + halfDeadH;
    } else if (target.y > camCenterY + halfDeadH) {
        desiredCamCenterY = target.y - halfDeadH;
    }

    // 转换成镜头左上角坐标
    float desiredX = desiredCamCenterX - viewW_ * 0.5f;
    float desiredY = desiredCamCenterY - viewH_ * 0.5f;

    // 边界夹紧
    desiredX = clampAxis(desiredX, viewW_, levelW_);
    desiredY = clampAxis(desiredY, viewH_, levelH_);

    // ===== 平滑 =====
    pos_.x = Anim::approachF(pos_.x, desiredX, dt);
    pos_.y = Anim::approachF(pos_.y, desiredY, dt);
}

void Camera::shake(float intensity, float duration) {
    if (intensity <= 0.f || duration <= 0.f)
        return;
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
    float t = shakeTimer_ / shakeDuration_; // 1 → 0
    float amp = shakeIntensity_ * t;
    float rx = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.f * amp;
    float ry = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.f * amp;
    shakeOffset_ = {rx, ry};
}