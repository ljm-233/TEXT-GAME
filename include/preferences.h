#pragma once
#include "config.h"

// 用户偏好：用户在设置界面调整的
// 存放分辨率索引、全屏开关等
class Preferences : public Config {
public:
    explicit Preferences(const Paths& p) : Config(p, "preferences.conf") {}
};