#include "theme.h"

namespace {
const Theme kThemes[kThemeCount] = {
    // Dark
    {
        {20, 20, 30},
        {35, 35, 50},
        {70, 70, 80},
        {105, 105, 125},
        {45, 45, 55},
        {70, 140, 220},
        {230, 230, 230},
        {160, 160, 180},
        {140, 140, 160},
    },
    // Blue
    {
        {15, 25, 45},
        {25, 40, 65},
        {40, 70, 110},
        {60, 100, 150},
        {30, 55, 90},
        {80, 160, 240},
        {230, 240, 255},
        {150, 180, 210},
        {90, 130, 180},
    },
    // Light
    {
        {230, 230, 235},
        {245, 245, 250},
        {200, 200, 210},
        {220, 220, 230},
        {180, 180, 190},
        {100, 160, 230},
        {40, 40, 50},
        {100, 100, 110},
        {150, 150, 160},
    },
};

ThemeId g_current = ThemeId::Dark;
}

const Theme& getTheme() { return kThemes[static_cast<int>(g_current)]; }
ThemeId      getThemeId() { return g_current; }
void         setTheme(ThemeId id) { g_current = id; }

const char* themeName(ThemeId id) {
    switch (id) {
        case ThemeId::Dark:  return "深色";
        case ThemeId::Blue:  return "蓝色";
        case ThemeId::Light: return "浅色";
    }
    return "未知";
}