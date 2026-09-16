#pragma once
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Camera {
public:
    Camera() = default;

    void setViewSize(float w, float h) {
        viewW_ = w;
        viewH_ = h;
        // 死区：屏幕中心 15% x 10% 区域
        deadZoneW_ = w * 0.15f;
        deadZoneH_ = h * 0.10f;
    }
    void setLevelBounds(float w, float h) {
        levelW_ = w;
        levelH_ = h;
    }

    // 最大前瞻像素偏移
    void setLookAhead(float pixels) { lookAheadMax_ = pixels; }

    void snapTo(Vec2 target);
    // targetVel：玩家速度（像素/秒），用于计算前瞻方向
    void follow(Vec2 target, Vec2 targetVel, float dt);
    void shake(float intensity, float duration);
    void updateShake(float dt);

    Vec2 position() const { return pos_; }
    Vec2 effectivePosition() const { return pos_ + shakeOffset_; }
    Vec2 center() const {
        return {pos_.x + shakeOffset_.x + viewW_ * 0.5f,
                pos_.y + shakeOffset_.y + viewH_ * 0.5f};
    }
    float viewWidth() const { return viewW_; }
    float viewHeight() const { return viewH_; }

private:
    Vec2 pos_{0.f, 0.f};
    float viewW_ = 1280.f;
    float viewH_ = 720.f;
    float levelW_ = 0.f;
    float levelH_ = 0.f;

    // 死区尺寸
    float deadZoneW_ = 192.f;   // 1280 * 0.15
    float deadZoneH_ = 72.f;    // 720 * 0.10

    // 前瞻
    float lookAheadMax_ = 100.f;   // 最大像素偏移
    float lookOffset_ = 0.f;       // 当前平滑后的偏移

    Vec2 shakeOffset_{0.f, 0.f};
    float shakeTimer_ = 0.f;
    float shakeDuration_ = 0.f;
    float shakeIntensity_ = 0.f;
};