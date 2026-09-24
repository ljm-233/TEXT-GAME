#include "gamepad.h"
#include <algorithm>
#include <cmath>

Gamepad& Gamepad::instance() {
    static Gamepad inst;
    return inst;
}

void Gamepad::update() {
    // 扫描 0~3 号手柄，找第一个连上的
    connected_ = false;
    for (unsigned i = 0; i < 4; ++i) {
        if (sf::Joystick::isConnected(i)) {
            connected_ = true;
            joystickId_ = i;
            break;
        }
    }

    // ⭐ 振动包络：正弦曲线，0 → 1 → 0
    if (vibrationActive_) {
        float elapsed = vibrationClock_.getElapsedTime().asSeconds();
        if (elapsed >= vibrationDuration_) {
            stopVibration();
        } else {
            float t = elapsed / vibrationDuration_;
            float envelope = std::sin(t * 3.14159265f);
            float l = vibrationLow_  * envelope * vibrationIntensity_;
            float h = vibrationHigh_ * envelope * vibrationIntensity_;
            GamepadVibration::instance().setVibration(l, h);
        }
    }
}

float Gamepad::applyDeadzone(float value) const {
    if (std::abs(value) < deadzone_) return 0.f;
    float sign = value > 0.f ? 1.f : -1.f;
    return sign * (std::abs(value) - deadzone_) / (1.f - deadzone_);
}

float Gamepad::leftX() const {
    if (!connected_) return 0.f;
    return applyDeadzone(sf::Joystick::getAxisPosition(
        joystickId_, sf::Joystick::Axis::X) / 100.f);
}

float Gamepad::leftY() const {
    if (!connected_) return 0.f;
    return applyDeadzone(sf::Joystick::getAxisPosition(
        joystickId_, sf::Joystick::Axis::Y) / 100.f);
}

float Gamepad::rightX() const {
    if (!connected_) return 0.f;
    return applyDeadzone(sf::Joystick::getAxisPosition(
        joystickId_, sf::Joystick::Axis::U) / 100.f);
}

float Gamepad::rightY() const {
    if (!connected_) return 0.f;
    return applyDeadzone(sf::Joystick::getAxisPosition(
        joystickId_, sf::Joystick::Axis::V) / 100.f);
}

bool Gamepad::isButtonPressed(unsigned button) const {
    if (!connected_) return false;
    return sf::Joystick::isButtonPressed(joystickId_, button);
}

bool Gamepad::jumpPressed() const {
    return isButtonPressed(0) || dpadUp();
}

bool Gamepad::confirmPressed() const {
    return isButtonPressed(0) || isButtonPressed(7);
}

bool Gamepad::backPressed() const {
    return isButtonPressed(1) || isButtonPressed(6);
}

bool Gamepad::dpadUp() const {
    if (!connected_) return false;
    float y = sf::Joystick::getAxisPosition(joystickId_, sf::Joystick::Axis::PovY);
    return y < -50.f;
}

bool Gamepad::dpadDown() const {
    if (!connected_) return false;
    float y = sf::Joystick::getAxisPosition(joystickId_, sf::Joystick::Axis::PovY);
    return y > 50.f;
}

bool Gamepad::dpadLeft() const {
    if (!connected_) return false;
    float x = sf::Joystick::getAxisPosition(joystickId_, sf::Joystick::Axis::PovX);
    return x < -50.f;
}

bool Gamepad::dpadRight() const {
    if (!connected_) return false;
    float x = sf::Joystick::getAxisPosition(joystickId_, sf::Joystick::Axis::PovX);
    return x > 50.f;
}

// ===== 振动 =====

void Gamepad::setVibrationEnabled(bool e) {
    vibrationEnabled_ = e;
    if (!e) stopVibration();
}

void Gamepad::setVibrationIntensity(float i) {
    vibrationIntensity_ = std::clamp(i, 0.f, 1.f);
}

void Gamepad::vibrate(float low, float high, float duration) {
    if (!vibrationEnabled_) return;
    if (duration <= 0.f) return;

    vibrationLow_  = std::clamp(low,  0.f, 1.f);
    vibrationHigh_ = std::clamp(high, 0.f, 1.f);
    vibrationDuration_ = duration;
    vibrationActive_ = true;
    vibrationClock_.restart();

    // 首帧立即给一个起振
    float l = vibrationLow_  * vibrationIntensity_;
    float h = vibrationHigh_ * vibrationIntensity_;
    GamepadVibration::instance().setVibration(l, h);
}

void Gamepad::stopVibration() {
    GamepadVibration::instance().stop();
    vibrationActive_ = false;
}