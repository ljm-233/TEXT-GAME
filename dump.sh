#!/usr/bin/env bash
#
# 把全部源码拼成一个 project_dump.txt —— 用于给 AI 助手喂完整上下文。
#
# 用法: ./dump.sh
# 产出: project_dump.txt（覆盖写入）
#
# 这个脚本原先叫 s.sh，很容易被误认成"start"（启动游戏）。
# 启动游戏的是 ./s.sh，两者名字已区分开。
#
# 注意：它扫的是工作区当前内容，包含未提交的改动；
#       但排除 .git/ 与构建产物（靠下面的 -name 白名单控制）。
#
set -euo pipefail

# 先解析软链接再取目录：脚本可能被 ~/.local/bin 下的软链调用，
# 不解析的话 dirname 会落到软链所在的目录，find 就扫错地方了。
SELF="${BASH_SOURCE[0]}"
if command -v readlink > /dev/null && readlink -f "$SELF" > /dev/null 2>&1; then
    SELF="$(readlink -f "$SELF")"
fi
cd "$(dirname "$SELF")"

{
  for f in $(find . -path ./.git -prune -o -type f \( \
    -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' -o \
    -name '*.cc' -o -name '*.glsl' -o -name '*.frag' -o -name '*.vert' -o \
    -name '*.cmake' -o -name 'CMakeLists.txt' -o -name '*.md' -o \
    -name '*.json' -o -name '*.yml' -o -name '*.yaml' -o -name '*.toml' \
  \) -print); do
    echo "===== $f ====="
    cat "$f"
  done
} > project_dump.txt

echo "[dump] 已写入 project_dump.txt（$(wc -l < project_dump.txt) 行）"
