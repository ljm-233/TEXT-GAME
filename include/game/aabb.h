#pragma once
#include "vec2.h"

struct AABB {
    float x = 0.f; // 左上角 x
    float y = 0.f; // 左上角 y
    float w = 0.f;
    float h = 0.f;

    AABB() = default;
    AABB(float xx, float yy, float ww, float hh)
          : x(xx),
            y(yy),
            w(ww),
            h(hh) {}

    float left() const { return x; }
    float right() const { return x + w; }
    float top() const { return y; }
    float bottom() const { return y + h; }

    Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
    Vec2 topLeft() const { return {x, y}; }

    bool intersects(const AABB& o) const {
        return !(right() <= o.left() || o.right() <= left() || bottom() <= o.top() ||
                 o.bottom() <= top());
    }
};