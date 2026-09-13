#pragma once
#include "vec2.h"
#include <SFML/Graphics.hpp>

class Camera {
public:
    Camera() = default;

    void setViewSize(float w, float h) { viewW_ = w; viewH_ = h; }
    void setLevelBounds(float w, float h) { levelW_ = w; levelH_ = h; }

    void snapTo(Vec2 target);
    void follow(Vec2 target, float dt);

    Vec2 position() const { return pos_; }   // 左上角
    Vec2 center() const {
        return {pos_.x + viewW_ * 0.5f, pos_.y + viewH_ * 0.5f};
    }
    float viewWidth()  const { return viewW_; }
    float viewHeight() const { return viewH_; }

private:
    Vec2 pos_{0.f, 0.f};
    float viewW_ = 1280.f;
    float viewH_ = 720.f;
    float levelW_ = 0.f;
    float levelH_ = 0.f;
};