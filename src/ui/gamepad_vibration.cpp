// src/ui/gamepad_vibration.cpp
#include "gamepad_vibration.h"
#include <cstdio>

GamepadVibration& GamepadVibration::instance() {
    static GamepadVibration inst;
    return inst;
}

GamepadVibration::~GamepadVibration() {
    shutdown();
}

void GamepadVibration::init() {
    if (available_) return;
    fprintf(stderr, "[Vibration] init() called\n");
    initPlatform();
    available_ = true;
}

void GamepadVibration::shutdown() {
    if (!available_) return;
    shutdownPlatform();
    available_ = false;
}