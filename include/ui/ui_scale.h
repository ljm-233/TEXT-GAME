#pragma once

// 全局 UI 缩放系数（0.5 ~ 2.0）
float getUiScale();
void setUiScale(float s);

// 根据缩放系数计算字号（最小 8）
unsigned scaledFontSize(unsigned base);