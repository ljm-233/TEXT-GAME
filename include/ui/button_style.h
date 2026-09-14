#pragma once

struct ButtonStyle {
    float cornerRadius = 0.f;     // 0 = 直角
    float outlineThickness = 2.f; // 0 = 无边框
};

const ButtonStyle& getButtonStyle();
void setButtonStyle(const ButtonStyle& s);