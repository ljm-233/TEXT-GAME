#include "camera.h"
#include "animation.h"
#include <algorithm>

namespace {
// 把目标位置夹到合法范围。如果关卡比视口小，就居中显示。
float clampAxis(float desired, float view, float level) {
    if (level <= 0.f)
        return desired;
    if (level <= view) {
        // 关卡比视口小：居中
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
}

void Camera::follow(Vec2 target, float dt) {
    float desiredX = target.x - viewW_ * 0.5f;
    float desiredY = target.y - viewH_ * 0.5f;

    desiredX = clampAxis(desiredX, viewW_, levelW_);
    desiredY = clampAxis(desiredY, viewH_, levelH_);

    pos_.x = Anim::approachF(pos_.x, desiredX, dt);
    pos_.y = Anim::approachF(pos_.y, desiredY, dt);
}