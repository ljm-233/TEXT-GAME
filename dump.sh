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
# 它扫的是**工作区当前内容**（包含还没提交的新文件），但排除被 .gitignore
# 忽略的构建产物。
#
# ⚠️ 为什么用 git 列文件，而不是 find + 扩展名白名单：
#    以前是 find 全盘遍历再按后缀过滤，于是 build/debug/.cmake/**/*.json
#    和 makepkg 解包出来的 packaging/aur/src/TEXT-GAME-x.y.z/ 整棵源码树
#    都被当成"源码"收了进来 —— 一次把快照从 9 万行灌到 20 万行，一半是
#    构建垃圾，喂给 AI 只会挤掉真正有用的上下文。
#    `git ls-files --cached --others --exclude-standard` 天然跟着 .gitignore 走；
#    `--others` 保证**新建但还没 git add 的文件不会被漏掉**（这一点很关键，
#    只看 --cached 的话新写的文件会整个缺席）。
set -euo pipefail

# 先解析软链接再取目录：脚本可能被 ~/.local/bin 下的软链调用，
# 不解析的话 dirname 会落到软链所在的目录，就扫错地方了。
SELF="${BASH_SOURCE[0]}"
if command -v readlink > /dev/null && readlink -f "$SELF" > /dev/null 2>&1; then
    SELF="$(readlink -f "$SELF")"
fi
cd "$(dirname "$SELF")"

# 收哪些后缀
is_source() {
    case "$1" in
        *.cpp|*.hpp|*.h|*.c|*.cc|*.glsl|*.frag|*.vert|*.cmake|*.md|*.json|*.yml|*.yaml|*.toml)
            return 0 ;;
        */CMakeLists.txt|CMakeLists.txt)
            return 0 ;;
        *)
            return 1 ;;
    esac
}

list_files() {
    if git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
        # -z + NUL 分隔：文件名里有中文或空格也不会被拆开
        git ls-files --cached --others --exclude-standard -z
    else
        # 不在 git 仓库里（比如导出的源码包）时退回遍历，至少把 .git 排掉
        find . -path ./.git -prune -o -type f -print0
    fi
}

{
    while IFS= read -r -d '' f; do
        # 索引里可能留着"已删除但还没提交"的文件，cat 会失败并让 set -e 直接中断
        [ -f "$f" ] || continue
        is_source "$f" || continue
        # 统一成 ./ 开头的显示形式（git ls-files 输出不带前缀，find 带），
        # 保持与改动前一致 —— 快照是被跟踪的文件，格式不该跟着实现churn。
        #
        # ⚠️ 头前面那个 \n 是必需的：有些源文件**末尾没有换行符**，cat 之后
        #    光标还停在那一行，下一个头就会被粘到它末尾 —— 变成
        #    "...#endif===== ./next.cpp ====="，按行切分快照的工具（含 grep '^====='）
        #    会整段漏掉。加个换行保证每个头都单独起一行。
        printf '\n===== ./%s =====\n' "${f#./}"
        cat "$f"
    done < <(list_files)
} > project_dump.txt

echo "[dump] 已写入 project_dump.txt（$(wc -l < project_dump.txt) 行）"
