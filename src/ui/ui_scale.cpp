#include "ui_scale.h"
#include <algorithm>

namespace {
float g_scale = 1.0f;
}

float getUiScale() {
    return g_scale;
}

void setUiScale(float s) {
    g_scale = std::clamp(s, 0.5f, 2.0f);
}

unsigned scaledFontSize(unsigned base) {
    unsigned r = static_cast<unsigned>(base * g_scale + 0.5f);
    return std::max(8u, r);
}