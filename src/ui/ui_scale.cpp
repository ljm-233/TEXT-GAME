#include "ui_scale.h"
#include <algorithm>
#include <cmath>

namespace {
float g_uiScale = 1.0f;
float g_fontScale = 1.0f;
} // namespace

float getUiScale() {
    return g_uiScale;
}
void setUiScale(float s) {
    g_uiScale = std::clamp(s, 0.5f, 2.0f);
}

float getFontScale() {
    return g_fontScale;
}
void setFontScale(float s) {
    g_fontScale = std::clamp(s, 0.5f, 2.0f);
}

unsigned scaledFontSize(unsigned base) {
    // 用 std::lround 而不是 static_cast<unsigned>(x + 0.5f)：后者的取整方向
    // 在负值上是错的，clang-tidy 的 bugprone-incorrect-roundings 报的就是它。
    unsigned r = static_cast<unsigned>(std::lround(base * g_fontScale));
    return std::max(8u, r);
}

unsigned fontSizeInView(unsigned base) {
    float uiS = (g_uiScale < 0.01f) ? 1.0f : g_uiScale;
    unsigned r = static_cast<unsigned>(std::lround(base * g_fontScale / uiS));
    return std::max(6u, r);
}