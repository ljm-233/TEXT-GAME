#pragma once
#include <SFML/Window/Joystick.hpp>
#include "gamepad_config.h"

// 手柄封装：自动检测、死区、按钮查询
class Gamepad {
public:
    static Gamepad& instance();

    // 每帧调用，更新连接状态
    void update();

    bool isConnected() const { return connected_; }
    unsigned id() const { return joystickId_; }

    // ===== 轴（已应用死区，返回 -1 ~ 1）=====
    float leftX() const;
    float leftY() const;
    float rightX() const;
    float rightY() const;

    // ===== 按钮 =====
    // 通用映射（Xbox 布局为准）：
    //   0 = A（下）  1 = B（右）  2 = X（左）  3 = Y（上）
    //   4 = LB       5 = RB
    //   6 = Back     7 = Start
    bool isButtonPressed(unsigned button) const;

    // 便捷方法
    bool jumpPressed() const;   // A 或 十字键上
    bool confirmPressed() const;// A 或 Start
    bool backPressed() const;   // B 或 Back

    // 方向键（D-Pad）
    bool dpadUp() const;
    bool dpadDown() const;
    bool dpadLeft() const;
    bool dpadRight() const;

    void setDeadzone(float dz) { deadzone_ = dz; }
    float deadzone() const { return deadzone_; }

private:
    Gamepad() = default;

    bool     connected_ = false;
    unsigned joystickId_ = 0;
    float    deadzone_ = GamepadConfig::kStickDeadzone;

    // 死区处理
    float applyDeadzone(float value) const;
};