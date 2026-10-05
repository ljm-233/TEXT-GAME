#include "log/rotation.h"

#include <array>

namespace {

// 档位表。顺序即设置界面里按钮的排列顺序，改动会直接影响老配置的含义。
// 乘法用 size_t 起头：写成 1 * 1024 * 1024 是先按 int 乘、再隐式拓宽成
// size_t（clang-tidy 的 bugprone-implicit-widening-of-multiplication-result）。
constexpr std::array<size_t, kLogRotationSteps> kSizes = {
    0,
    std::size_t{1} * 1024 * 1024,
    std::size_t{5} * 1024 * 1024,
    std::size_t{10} * 1024 * 1024,
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
