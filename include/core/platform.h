#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace Platform {

#if defined(_WIN32) || defined(_WIN64)
constexpr bool isWindows = true;
constexpr bool isMac = false;
constexpr bool isLinux = false;
constexpr const char* name = "Windows";
#elif defined(__APPLE__)
constexpr bool isWindows = false;
constexpr bool isMac = true;
constexpr bool isLinux = false;
constexpr const char* name = "macOS";
#elif defined(__linux__)
constexpr bool isWindows = false;
constexpr bool isMac = false;
constexpr bool isLinux = true;
constexpr const char* name = "Linux";
#else
constexpr bool isWindows = false;
constexpr bool isMac = false;
constexpr bool isLinux = false;
constexpr const char* name = "Unknown";
#endif

#if defined(_WIN32)
constexpr char pathSeparator = '\\';
#else
constexpr char pathSeparator = '/';
#endif

// ============================================================
// 路径检测
// ============================================================

// 当前可执行文件所在的目录（打包后资源就在这旁边）
std::filesystem::path executableDir();

// 用户目录（跨平台）
std::filesystem::path userConfigDir(); // ~/.config/text-game
std::filesystem::path userCacheDir();  // ~/.cache/text-game
std::filesystem::path userDataDir();   // ~/.local/share/text-game
std::filesystem::path userTempDir();   // /tmp/text-game

// 系统字体目录
std::vector<std::filesystem::path> systemFontDirs();

} // namespace Platform