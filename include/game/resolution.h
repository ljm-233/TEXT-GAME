#pragma once

struct Resolution {
    unsigned width;
    unsigned height;
    const char* label;
};

inline const Resolution kResolutions[] = {
    {1280,  720, "1280 x 720  (720p)"},
    {1600,  900, "1600 x 900"},
    {1920, 1080, "1920 x 1080 (1080p)"},
    {2560, 1440, "2560 x 1440 (2K)"},
};
inline constexpr int kResolutionCount = 4;

inline int clampResolutionIndex(int idx) {
    if (idx < 0 || idx >= kResolutionCount) return 0;
    return idx;
}