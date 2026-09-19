#pragma once
#include <SFML/Window/Joystick.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include "gamepad_config.h"

// 手柄封装：自动检测、死区、按钮查询、振动
class Gamepad {
public:
    static Gamepad& instance();

    // 每帧调用，更新连接状态 + 振动超时
    void update();

    bool isConnected() const { return connected_; }
    unsigned id() const { return joystickId_; }

    // ===== 轴（已应用死区，返回 -1 ~ 1）=====
    float leftX() const;
    float leftY() const;
    float rightX() const;
    float rightY() const;

    // ===== 按钮 =====
    bool isButtonPressed(unsigned button) const;

    // 便捷方法
    bool jumpPressed() const;
    bool confirmPressed() const;
    bool backPressed() const;

    // 方向键（D-Pad）
    bool dpadUp() const;
    bool dpadDown() const;
    bool dpadLeft() const;
    bool dpadRight() const;

    void setDeadzone(float dz) { deadzone_ = dz; }
    float deadzone() const { return deadzone_; }

    // ⭐ 振动
    //   low  : 低频马达（左侧大马达），0~1
    //   high : 高频马达（右侧小马达），0~1
    //   duration: 秒
    void vibrate(float low, float high, float duration);
    void stopVibration();

    void setVibrationEnabled(bool e) { vibrationEnabled_ = e; if (!e) stopVibration(); }
    bool isVibrationEnabled() const { return vibrationEnabled_; }

    void setVibrationIntensity(float i);
    float vibrationIntensity() const { return vibrationIntensity_; }

private:
    Gamepad() = default;

    bool     connected_ = false;
    unsigned joystickId_ = 0;
    float    deadzone_ = GamepadConfig::kStickDeadzone;

    // 死区处理
    float applyDeadzone(float value) const;

    // ⭐ 振动状态
    bool  vibrationEnabled_   = true;
    float vibrationIntensity_ = 1.0f;
    bool  vibrationActive_    = false;
    float vibrationDuration_  = 0.f;
    sf::Clock vibrationClock_;
};