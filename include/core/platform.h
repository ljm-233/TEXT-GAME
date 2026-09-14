#pragma once
#include <string>
#include <vector>
#include <filesystem>

// ============================================================
// 平台识别
// ============================================================
namespace Platform {

#if defined(_WIN32) || defined(_WIN64)
    constexpr bool isWindows = true;
    constexpr bool isMac     = false;
    constexpr bool isLinux   = false;
    constexpr const char* name = "Windows";
#elif defined(__APPLE__)
    constexpr bool isWindows = false;
    constexpr bool isMac     = true;
    constexpr bool isLinux   = false;
    constexpr const char* name = "macOS";
#elif defined(__linux__)
    constexpr bool isWindows = false;
    constexpr bool isMac     = false;
    constexpr bool isLinux   = true;
    constexpr const char* name = "Linux";
#else
    constexpr bool isWindows = false;
    constexpr bool isMac     = false;
    constexpr bool isLinux   = false;
    constexpr const char* name = "Unknown";
#endif

// ===== 路径分隔符 =====
#if defined(_WIN32)
    constexpr char pathSeparator = '\\';
#else
    constexpr char pathSeparator = '/';
#endif

// ============================================================
// 用户配置目录（跨平台）
// ============================================================
// Windows: %APPDATA%/text-game
// macOS:   ~/Library/Application Support/text-game
// Linux:   ~/.config/text-game  (XDG)
std::filesystem::path userConfigDir();

// 用户缓存目录
std::filesystem::path userCacheDir();

// 用户数据目录（存档）
std::filesystem::path userDataDir();

// 系统字体目录（用于查找可用字体）
std::vector<std::filesystem::path> systemFontDirs();

} // namespace Platform