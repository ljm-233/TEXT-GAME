// 没有平台实现时的兜底：让项目仍然能编译链接，振动静默失效。
//
// 振动只实现了 Linux（evdev + EVIOCSFF）和 Windows（XInput）。
// macOS 上这两个文件都被各自的 #ifdef 排除掉，编译成空 TU，
// 于是 initPlatform / shutdownPlatform / setVibration / stop 四个符号
// 一个都没有定义 —— 表现是**链接错误**（macOS CI 就是这么挂的）。
//
// 这里的守卫写成"既不是 Linux 也不是 Windows"而不是 #ifdef __APPLE__，
// 这样 FreeBSD 之类没实现过的系统也能直接编译，不用再加一个文件。
#if !defined(__linux__) && !defined(_WIN32)

#include "infrastructure/gamepad_vibration.h"

void GamepadVibration::initPlatform() {
    // 无平台实现：不做任何事。
    // 基类 init() 会把 available_ 置为 true（它的语义是"已初始化"而非
    // "硬件可用"，与 Linux/Windows 两个实现保持一致），
    // 真正的硬件判断在各平台实现自己的静态标志里。
}

void GamepadVibration::shutdownPlatform() {
}

void GamepadVibration::setVibration(float /*low*/, float /*high*/) {
    // 静默忽略
}

void GamepadVibration::stop() {
}

#endif
