#pragma once

// 全局 UI 缩放系数（0.5 ~ 2.0）—— 控制布局（按钮位置/大小/间距）
float getUiScale();
void setUiScale(float s);

// 全局字体缩放系数（0.5 ~ 2.0）—— 单独控制字号
float getFontScale();
void setFontScale(float s);

// 根据 fontScale 计算字号（最小 8）—— 用于非 View 场景
unsigned scaledFontSize(unsigned base);

// 设置页专用：设计区字号 = base * fontScale / uiScale
// View 缩放 uiScale 后，屏幕上实际显示 base * fontScale
unsigned fontSizeInView(unsigned base);