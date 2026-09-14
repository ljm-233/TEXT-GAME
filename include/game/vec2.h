#pragma once
#include <cmath>

struct Vec2 {
    float x = 0.f;
    float y = 0.f;

    Vec2() = default;
    Vec2(float xx, float yy)
          : x(xx),
            y(yy) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }

    Vec2& operator+=(const Vec2& o) {
        x += o.x;
        y += o.y;
        return *this;
    }
    Vec2& operator-=(const Vec2& o) {
        x -= o.x;
        y -= o.y;
        return *this;
    }
    Vec2& operator*=(float s) {
        x *= s;
        y *= s;
        return *this;
    }

    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }
};

inline float dot(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
}