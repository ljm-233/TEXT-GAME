// src/ui/gamepad_vibration_windows.cpp
#ifdef _WIN32
#include "gamepad_vibration.h"
#include <windows.h>
#include <xinput.h> // 需要链接 XInput.lib
#include <iostream>

// 需要链接 xinput 库
#pragma comment(lib, "xinput.lib")

static int g_controllerIndex = -1;
static bool g_available = false;

void GamepadVibration::initPlatform() {
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        if (XInputGetState(i, &state) == ERROR_SUCCESS) {
            g_controllerIndex = static_cast<int>(i);
            g_available = true;
            std::cout << "[Vibration] XInput controller found at index " << i << std::endl;
            break;
        }
    }
    if (!g_available) {
        std::cerr << "[Vibration] No XInput controller found." << std::endl;
    }
}

void GamepadVibration::shutdownPlatform() {
    if (g_available) {
        stop(); // 确保停止振动
    }
    g_controllerIndex = -1;
    g_available = false;
}

void GamepadVibration::setVibration(float low, float high) {
    if (!g_available || g_controllerIndex < 0) return;

    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
    // 将 0.0f-1.0f 映射到 0-65535
    vibration.wLeftMotorSpeed  = static_cast<WORD>(low * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(high * 65535.0f);

    XInputSetState(static_cast<DWORD>(g_controllerIndex), &vibration);
}

void GamepadVibration::stop() {
    if (!g_available) return;
    XINPUT_VIBRATION vibration = {0, 0};
    XInputSetState(static_cast<DWORD>(g_controllerIndex), &vibration);
}

#endif // _WIN32