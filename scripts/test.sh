#!/usr/bin/env bash
#
# 配置 → 构建 → 跑单元测试。
#
# 用法: scripts/test.sh [ctest 的参数]
#   例如 scripts/test.sh --output-on-failure
#
# 注意：这个脚本原先用的是老式 build/ 目录布局，项目改用 CMakePresets
# 之后就失效了 —— 它去找 build/tests/unit_tests，而 preset 实际产出在
# build/tests/tests/unit_tests，且 cmake --build build 在 preset 布局下
# 根本没有 CMakeCache。现在统一走 preset。
#
set -euo pipefail

# 从脚本自身位置定位仓库根目录，因此在任何目录下都能直接调用
# 先解析软链接再取目录：这些脚本可能被 ~/.local/bin 下的软链调用，
# 不解析的话 dirname 会落到软链所在的目录。
SELF="${BASH_SOURCE[0]}"
if command -v readlink > /dev/null && readlink -f "$SELF" > /dev/null 2>&1; then
    SELF="$(readlink -f "$SELF")"
fi
ROOT="$(cd "$(dirname "$SELF")/.." && pwd)"
cd "$ROOT"

cmake --preset tests > /dev/null
cmake --build --preset tests
exec ctest --preset tests "$@"
