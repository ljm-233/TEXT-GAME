#include "platform.h"
#include <cstdlib>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h> // shellapi.h 依赖它，必须先来
#include <shellapi.h>
#elif defined(__APPLE__)
#include <climits>
#include <mach-o/dyld.h>
#include <spawn.h>
#else
#include <climits>
#include <spawn.h>
#include <unistd.h>
#endif

#if !defined(_WIN32)
// posix_spawn 要显式把环境传下去（shell 版的 execvp 是自己去拿）。
// 已经在 unistd.h 里声明过就重复声明一次，不冲突。
extern char** environ;
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

// ============================================================
// 在文件管理器里打开目录
// ============================================================
#if defined(_WIN32)

bool openDirectory(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
        return false;

    // ShellExecuteW 而不是 system()：它按 shell 关联去找「打开」这个动作，
    // 而且只认宽字符路径 —— 窄字符入口会先按 ANSI 代码页把中文路径截断。
    // 返回值 >32 才是成功，<=32 是 SE_ERR_* 错误码，别当 HRESULT 看。
    // 它不等待目标程序结束，点完立刻返回。
    const INT_PTR result = reinterpret_cast<INT_PTR>(ShellExecuteW(
        nullptr, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    return result > 32;
}

#else // POSIX

namespace {

/// 起一个进程就立刻返回，不关心它跑成什么样。
///
/// posix_spawn 而不是 system()：system() 会**等子进程结束**（文件管理器一开，
/// 主循环就冻在那儿），而且它把命令交给 /bin/sh 拼字符串，空格和中文全靠引号转义。
/// file_actions 传 nullptr 就是默认行为：只留 stdin/stdout/stderr，其余带
/// FD_CLOEXEC 的 fd 在 exec 时自动关掉，不用手工清理。
/// 返回 posix_spawn 的错误码（0 = 进程已起），找不到可执行文件时是 ENOENT。
/// 不 waitpid —— 那又阻塞了；代价是子进程退出后留下一个僵尸。
///
/// ponytail: 这是有意的取舍 —— 调用点是"用户在设置页点了一下打开目录"，
/// 一次点击最多一个僵尸，而且进程退出时会一起回收。真到了需要常驻调用的
/// 地步，**不要**用 `signal(SIGCHLD, SIG_IGN)` —— 那是进程级副作用，
/// 会让之后所有 `waitpid` 静默返回 ECHILD（拿不到退出码还没有报错）。
/// 改成把上一个 pid 记下来、下次调用时 `waitpid(WNOHANG)` 收一下，
/// 或者干脆双 fork。
int spawnDetached(const char* program, const std::vector<const char*>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(program));
    for (const char* arg : args)
        argv.push_back(const_cast<char*>(arg));
    argv.push_back(nullptr); // argv 必须以 nullptr 结尾

    pid_t pid = 0;
    return posix_spawnp(&pid, program, nullptr, nullptr, argv.data(), environ);
}

} // namespace

bool openDirectory(const fs::path& dir) {
    // 目录不存在就别去执行外部命令：给它一个不存在的路径，各家文件管理器
    // 反应不一（有的弹错误框，有的把父目录打开），行为不可控。
    // 用 error_code 重载，路径权限有问题时返回 false 而不是抛异常。
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
        return false;

    // 先把路径落到 std::string 上再取 c_str()：c_str() 是借来的指针，
    // 不能让 argv 指着一个已经析构的临时对象。
    const std::string target = dir.string();

#if defined(__APPLE__)
    return spawnDetached("open", {target.c_str()}) == 0;
#else
    // xdg-open 是 xdg-utils 的正门，桌面发行版基本都装；精简容器里可能没有
    // （posix_spawnp 报 ENOENT），再退到 gio —— 它跟着 glib 走，装了 GTK 就有。
    // 两个都找不到就安全返回 false，不能崩。
    if (spawnDetached("xdg-open", {target.c_str()}) == 0)
        return true;
    return spawnDetached("gio", {"open", target.c_str()}) == 0;
#endif
}

#endif

} // namespace Platform