// include/ui/gamepad_vibration.h
#pragma once

class GamepadVibration {
public:
    static GamepadVibration& instance();

    // 初始化/清理（在游戏启动和退出时调用）
    void init();
    void shutdown();

    // 设置振动强度 (low: 低频/左侧大马达, high: 高频/右侧小马达)，强度范围 0.0f ~ 1.0f
    void setVibration(float low, float high);

    // 停止振动
    void stop();

    // 检查振动功能是否可用
    bool isAvailable() const { return available_; }

private:
    GamepadVibration() = default;
    ~GamepadVibration();

    bool available_ = false;
    int  deviceIndex_ = -1; // 平台相关句柄，Linux 为 fd，Windows 为 controller index

    // 平台相关初始化
    void initPlatform();
    void shutdownPlatform();
};