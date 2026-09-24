// src/ui/gamepad_vibration_linux.cpp
#ifdef __linux__
#include "gamepad_vibration.h"

#include <linux/input.h> // 核心头文件，定义了 ff_effect 等结构
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <vector>
#include <string>
#include <dirent.h>

// ⭐ 用户空间版本的 test_bit（内核空间才有）
static inline int test_bit(int nr, const unsigned long* addr) {
    return 1UL & (addr[nr / (8 * sizeof(unsigned long))]
                  >> (nr % (8 * sizeof(unsigned long))));
}

// 全局句柄
static int g_ff_fd = -1;          // 设备文件描述符
static int g_ff_effect_id = -1;   // 上传的效果 ID
static bool g_ff_available = false;

// 扫描 /dev/input/ 目录，寻找支持 FF_RUMBLE 的设备
static bool scanEvdevDevices() {
    DIR* dir = opendir("/dev/input");
    if (!dir) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;

        std::string path = "/dev/input/" + std::string(entry->d_name);
        int fd = open(path.c_str(), O_RDWR); // 需要写权限
        if (fd < 0) continue;

        // 查询设备支持的力反馈能力
        unsigned long features[(FF_CNT + 8 * sizeof(unsigned long) - 1) /
                               (8 * sizeof(unsigned long))] = {0};
        if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof(features)), features) < 0) {
            close(fd);
            continue;
        }

        // 检查是否支持 FF_RUMBLE
        if (test_bit(FF_RUMBLE, features)) {
            g_ff_fd = fd;
            g_ff_available = true;
            fprintf(stderr, "[Vibration] Found FF device: %s\n", path.c_str());
            break;
        }
        close(fd);
    }
    closedir(dir);
    return g_ff_available;
}

// ---- 接口实现 ----

void GamepadVibration::initPlatform() {
    if (g_ff_available) return;

    if (!scanEvdevDevices()) {
        fprintf(stderr, "[Vibration] No FF_RUMBLE capable device found.\n");
        return;
    }

    // 准备一个基础的 FF_RUMBLE 效果
    struct ff_effect effect;
    memset(&effect, 0, sizeof(effect));
    effect.type = FF_RUMBLE;
    effect.id = -1; // 让内核分配 ID
    effect.u.rumble.strong_magnitude = 0; // 低频马达
    effect.u.rumble.weak_magnitude = 0;   // 高频马达
    effect.replay.length = 0;             // 0 表示由后续指令控制持续时间
    effect.replay.delay = 0;

    // 上传效果到内核
    if (ioctl(g_ff_fd, EVIOCSFF, &effect) < 0) {
        fprintf(stderr, "[Vibration] Failed to upload FF effect: %s\n", strerror(errno));
        close(g_ff_fd);
        g_ff_fd = -1;
        g_ff_available = false;
        return;
    }

    g_ff_effect_id = effect.id;
    fprintf(stderr, "[Vibration] FF effect uploaded, ID: %d\n", g_ff_effect_id);
}

void GamepadVibration::shutdownPlatform() {
    if (g_ff_fd >= 0) {
        // 移除效果
        if (g_ff_effect_id >= 0) {
            ioctl(g_ff_fd, EVIOCRMFF, g_ff_effect_id);
        }
        close(g_ff_fd);
        g_ff_fd = -1;
    }
    g_ff_effect_id = -1;
    g_ff_available = false;
}

void GamepadVibration::setVibration(float low, float high) {
    if (!g_ff_available || g_ff_effect_id < 0) return;

    // 将 0.0f-1.0f 映射到 0-65535
    unsigned short strong = static_cast<unsigned short>(low * 65535.0f);
    unsigned short weak   = static_cast<unsigned short>(high * 65535.0f);

    // 更新效果参数
    struct ff_effect effect;
    memset(&effect, 0, sizeof(effect));
    effect.type = FF_RUMBLE;
    effect.id = g_ff_effect_id;
    effect.u.rumble.strong_magnitude = strong;
    effect.u.rumble.weak_magnitude = weak;
    effect.replay.length = 50; // 效果时长，单位 ms，这里给一个短时长让游戏循环重复设置

    // 重新上传更新后的效果
    if (ioctl(g_ff_fd, EVIOCSFF, &effect) < 0) {
        return;
    }

    // 播放效果
    struct input_event play;
    memset(&play, 0, sizeof(play));
    play.type = EV_FF;
    play.code = g_ff_effect_id;
    play.value = 1; // 1 = 播放

    if (write(g_ff_fd, &play, sizeof(play)) < 0) {
        // 写入失败可能因为效果已过期，可尝试重新上传
    }
}

void GamepadVibration::stop() {
    if (!g_ff_available) return;
    setVibration(0.0f, 0.0f);
}

#endif // __linux__