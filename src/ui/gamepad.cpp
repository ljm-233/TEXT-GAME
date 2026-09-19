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

    // ⭐ 振动超时归零
    if (vibrationActive_) {
        if (vibrationClock_.getElapsedTime().asSeconds() >= vibrationDuration_) {
            stopVibration();
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

void Gamepad::setVibrationIntensity(float i) {
    vibrationIntensity_ = std::clamp(i, 0.f, 1.f);
}

void Gamepad::vibrate(float low, float high, float duration) {
    if (!vibrationEnabled_ || !connected_) return;
    if (duration <= 0.f) return;

    float l = std::clamp(low  * vibrationIntensity_, 0.f, 1.f);
    float h = std::clamp(high * vibrationIntensity_, 0.f, 1.f);

    vibrationDuration_ = duration;
    vibrationClock_.restart();
    vibrationActive_ = true;

    // ⚠️ SFML 3 移除了振动 API，此处为空实现。
    //    接口保留，设置项保留，等以后接入平台原生 API 或换库再填。
    (void)l;
    (void)h;
}

void Gamepad::stopVibration() {
    vibrationActive_ = false;
    // ⚠️ SFML 3 无振动 API，空实现
}