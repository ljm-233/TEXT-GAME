#include "ui_scale.h"
#include <algorithm>

namespace {
float g_uiScale   = 1.0f;
float g_fontScale = 1.0f;
}

float getUiScale() { return g_uiScale; }
void setUiScale(float s) { g_uiScale = std::clamp(s, 0.5f, 2.0f); }

float getFontScale() { return g_fontScale; }
void setFontScale(float s) { g_fontScale = std::clamp(s, 0.5f, 2.0f); }

unsigned scaledFontSize(unsigned base) {
    unsigned r = static_cast<unsigned>(base * g_fontScale + 0.5f);
    return std::max(8u, r);
}

unsigned fontSizeInView(unsigned base) {
    float uiS = (g_uiScale < 0.01f) ? 1.0f : g_uiScale;
    unsigned r = static_cast<unsigned>(base * g_fontScale / uiS + 0.5f);
    return std::max(6u, r);
}