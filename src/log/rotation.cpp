#include "log/rotation.h"

#include <array>

namespace {

// 档位表。顺序即设置界面里按钮的排列顺序，改动会直接影响老配置的含义。
constexpr std::array<size_t, kLogRotationSteps> kSizes = {
    0, 1 * 1024 * 1024, 5 * 1024 * 1024, 10 * 1024 * 1024,
};
constexpr std::array<int, kLogRotationSteps> kKeeps = {1, 3, 5, 10};

}  // namespace

size_t logRotationSizeAt(int index) {
    return kSizes[static_cast<size_t>(clampLogRotationIndex(index))];
}

int logRotationKeepAt(int index) {
    return kKeeps[static_cast<size_t>(clampLogKeepIndex(index))];
}

int clampLogRotationIndex(int index) {
    return (index < 0 || index >= kLogRotationSteps) ? 0 : index;
}

int clampLogKeepIndex(int index) {
    return (index < 0 || index >= kLogRotationSteps) ? 1 : index;
}
