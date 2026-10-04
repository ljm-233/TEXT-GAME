#pragma once
//
// 日志轮转档位。
//
// 设置界面里存的是"档位下标"而不是字节数，所以下标的合法范围、
// 下标到取值的映射，都是日志层的知识，不该由 UI 层各存一份。
// 旧实现把三张表放在 other_tab.cpp 里，这里收回日志层统一维护。
//
#include <cstddef>

/// 可选档位数量（0..kLogRotationSteps-1）。
constexpr int kLogRotationSteps = 4;

/// 档位 -> 单文件大小上限。0 表示不轮转。
size_t logRotationSizeAt(int index);

/// 档位 -> 保留的历史文件份数。
int logRotationKeepAt(int index);

/// 越界档位夹回默认值。两个默认值沿用旧行为（大小档 0、份数档 1）。
int clampLogRotationIndex(int index);
int clampLogKeepIndex(int index);
