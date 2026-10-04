#pragma once

struct Resolution {
    unsigned width;
    unsigned height;
    const char* label;
};

inline const Resolution kResolutions[] = {
    // ===== 老设备 / 小屏 =====
    { 800,  480, "800 x 480    (WVGA)"},
    {1024,  600, "1024 x 600   (WSVGA)"},
    {1280,  720, "1280 x 720   (720p)"},
    // ===== 常见笔记本 =====
    {1366,  768, "1366 x 768   (HD)"},
    {1600,  900, "1600 x 900   (HD+)"},
    // ===== 高分辨率 =====
    {1920, 1080, "1920 x 1080  (1080p)"},
    {2560, 1440, "2560 x 1440  (2K)"},
    {3840, 2160, "3840 x 2160  (4K UHD)"},
};
inline constexpr int kResolutionCount = 8;

inline int clampResolutionIndex(int idx) {
    if (idx < 0 || idx >= kResolutionCount)
        return 0;
    return idx;
}