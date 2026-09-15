#include "platform.h"
#include <cstdlib>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <climits>
#include <mach-o/dyld.h>
#else
#include <climits>
#include <unistd.h>
#endif

namespace Platform {

namespace fs = std::filesystem;

namespace {
fs::path getHome() {
    const char* home = std::getenv("HOME");
    if (home)
        return fs::path(home);
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile)
        return fs::path(userProfile);
#endif
    return fs::current_path();
}
} // namespace

// ============================================================
// 可执行文件目录
// ============================================================
fs::path executableDir() {
#if defined(_WIN32)
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n > 0)
        return fs::path(buf).parent_path();
    return fs::current_path();
#elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return fs::path(buf).parent_path();
    }
    return fs::current_path();
#else
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return fs::path(buf).parent_path();
    }
    return fs::current_path();
#endif
}

// ============================================================
// 用户目录（遵循 XDG 规范）
// ============================================================
fs::path userConfigDir() {
#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData)
        return fs::path(appData) / "text-game";
    return getHome() / "AppData" / "Roaming" / "text-game";
#elif defined(__APPLE__)
    return getHome() / "Library" / "Application Support" / "text-game";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg)
        return fs::path(xdg) / "text-game";
    return getHome() / ".config" / "text-game";
#endif
}

fs::path userCacheDir() {
#if defined(_WIN32)
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData)
        return fs::path(localAppData) / "text-game" / "cache";
    return getHome() / "AppData" / "Local" / "text-game" / "cache";
#elif defined(__APPLE__)
    return getHome() / "Library" / "Caches" / "text-game";
#else
    const char* xdg = std::getenv("XDG_CACHE_HOME");
    if (xdg)
        return fs::path(xdg) / "text-game";
    return getHome() / ".cache" / "text-game";
#endif
}

fs::path userDataDir() {
#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData)
        return fs::path(appData) / "text-game";
    return getHome() / "AppData" / "Roaming" / "text-game";
#elif defined(__APPLE__)
    return getHome() / "Library" / "Application Support" / "text-game";
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg)
        return fs::path(xdg) / "text-game";
    return getHome() / ".local" / "share" / "text-game";
#endif
}

fs::path userTempDir() {
#if defined(_WIN32)
    const char* tmp = std::getenv("TEMP");
    if (tmp)
        return fs::path(tmp) / "text-game";
    return fs::temp_directory_path() / "text-game";
#else
    return fs::temp_directory_path() / "text-game";
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
    dirs.push_back("/usr/share/fonts");
    dirs.push_back("/usr/local/share/fonts");
    const char* home = std::getenv("HOME");
    if (home)
        dirs.push_back(fs::path(home) / ".local" / "share" / "fonts");
#endif
    return dirs;
}

} // namespace Platform