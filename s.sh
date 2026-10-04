#!/usr/bin/env bash
#
# 一键构建并启动游戏。
#
#   ./s.sh            增量构建 Release 后直接进游戏（日常就敲这个）
#   ./s.sh --debug    要接调试器时用
#   ./s.sh --help
#
# 为什么默认 Release：Debug 构建下 SFML 的 sf::Text / sf::Shape 构造极慢，
# 游戏帧率会掉到 20~30fps（见 CLAUDE.md）。日常跑游戏没有理由用 Debug。
#
# 没改代码时这个脚本基本是空转：Ninja 只重编改动过的文件，
# 其余情况一行 "ninja: no work to do" 就过去了。
#
# 注意：本仓库里生成源码快照的那个脚本叫 dump.sh，不是这个。
#
set -euo pipefail

usage() {
    cat <<'EOF'
用法: ./s.sh [选项]

  (无参数)   增量构建 Release 后启动（日常用这个）
  --debug    Debug 构建后启动（要接调试器时用）
  --help     显示这段说明

默认 Release 的原因：Debug 构建下 SFML 的 sf::Text / sf::Shape 构造极慢，
帧率会掉到 20~30fps。日常跑游戏没有理由用 Debug。
EOF
    exit 0
}

PRESET="release"
for arg in "$@"; do
    case "$arg" in
        --debug)   PRESET="debug" ;;
        -h|--help) usage ;;
        *) ;;
    esac
done

# 从脚本自身位置定位仓库根目录，因此在任何目录下都能直接调用。
# PROJECT_ROOT 是编译期写死的，所以启动时的工作目录不影响资源查找。
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

if ! command -v cmake > /dev/null; then
    echo "[run] 找不到 cmake" >&2
    exit 1
fi

BUILD_DIR="build/${PRESET}"
BIN="${BUILD_DIR}/text_game"

# 首次运行、或 build/ 被清掉之后，先配置。
# 判据用 CMakeCache.txt 而不是目录是否存在 —— 空目录也算存在。
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "[run] 未配置过 ${PRESET} preset，先配置..."
    cmake --preset "${PRESET}"
fi

# 增量构建。失败时 set -e 会让脚本在这里停下，
# 编译错误已经原样打在终端上，不需要再包一层。
echo "[run] 构建（${PRESET}）..."
cmake --build --preset "${PRESET}"

if [ ! -x "${BIN}" ]; then
    echo "[run] 构建结束但找不到可执行文件：${BIN}" >&2
    exit 1
fi

if [ "${PRESET}" = "debug" ]; then
    echo "[run] 注意：Debug 构建下 sf::Text/sf::Shape 构造很慢，帧率可能只有 20~30fps"
fi

# exec 而不是调用：让游戏直接接管这个进程，
# 于是 Ctrl+C、退出码、调试器 attach 都按预期工作。
echo "[run] 启动 ${BIN}"
exec "${BIN}"
