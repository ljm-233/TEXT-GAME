#include "platform.h"
#include <cstdlib>
#include <vector>

namespace Platform {

namespace fs = std::filesystem;

namespace {
fs::path getHome() {
    const char* home = std::getenv("HOME");
    if (home) return fs::path(home);
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) return fs::path(userProfile);
#endif
    return fs::current_path();
}
}

fs::path userConfigDir() {
#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData) return fs::path(appData) / "text-game";
    return getHome() / "AppData" / "Roaming" / "text-game";
#elif defined(__APPLE__)
    return getHome() / "Library" / "Application Support" / "text-game";
#else
    // Linux: 遵循 XDG 规范
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg) return fs::path(xdg) / "text-game";
    return getHome() / ".config" / "text-game";
#endif
}

fs::path userCacheDir() {
#if defined(_WIN32)
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData) return fs::path(localAppData) / "text-game" / "cache";
    return getHome() / "AppData" / "Local" / "text-game" / "cache";
#elif defined(__APPLE__)
    return getHome() / "Library" / "Caches" / "text-game";
#else
    const char* xdg = std::getenv("XDG_CACHE_HOME");
    if (xdg) return fs::path(xdg) / "text-game";
    return getHome() / ".cache" / "text-game";
#endif
}

fs::path userDataDir() {
#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData) return fs::path(appData) / "text-game" / "saves";
    return getHome() / "AppData" / "Roaming" / "text-game" / "saves";
#elif defined(__APPLE__)
    return getHome() / "Library" / "Application Support" / "text-game" / "saves";
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg) return fs::path(xdg) / "text-game" / "saves";
    return getHome() / ".local" / "share" / "text-game" / "saves";
#endif
}

std::vector<fs::path> systemFontDirs() {
    std::vector<fs::path> dirs;
#if defined(_WIN32)
    dirs.push_back("C:/Windows/Fonts");
#elif defined(__APPLE__)
    dirs.push_back("/System/Library/Fonts");
    dirs.push_back("/Library/Fonts");
    dirs.push_back(getHome() / "Library" / "Fonts");
#else
    // Linux：常见字体目录
    dirs.push_back("/usr/share/fonts");
    dirs.push_back("/usr/local/share/fonts");
    const char* home = std::getenv("HOME");
    if (home) dirs.push_back(fs::path(home) / ".local" / "share" / "fonts");
    if (fs::exists("/usr/share/fonts/noto-cjk")) {
        dirs.push_back("/usr/share/fonts/noto-cjk");
    }
#endif
    return dirs;
}

} // namespace Platform