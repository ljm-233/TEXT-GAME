#!/bin/sh
#
# TEXT-GAME 启动器（自带依赖的 macOS 包用）。
#
# 为什么需要它：包里的 lib/ 里带了 SFML 等依赖，但那些 dylib 的 install name
# 仍然写的是构建机上的绝对路径（/opt/homebrew/opt/sfml/lib/...）。
# DYLD_LIBRARY_PATH 的查找发生在按 install name 解析**之前**，而且是按
# 叶子名匹配的，所以只要 lib/ 里有同名文件，直接与间接依赖都会走包内。
#
# ⚠️ 这里刻意**不用 install_name_tool 改写 install name**。
#    Apple Silicon 上一切可执行代码都必须有有效签名（至少 ad-hoc），
#    而 install_name_tool / strip 这类写文件的工具会让签名失效，
#    内核随后直接 SIGKILL（"Code Signature Invalid"）。
#    原样拷贝 + 启动器设环境变量，Mach-O 一个字节都不动，签名自然还有效。
#
# 包内布局：
#   TEXT-GAME    ← 这个脚本
#   text_game    ← 真正的二进制
#   lib/         ← 依赖 dylib
#   assets/ wallpaper/
#
# 注意 assets 必须和可执行文件同级 —— 游戏的 Paths 靠"可执行文件旁边有没有
# assets/" 判断自己是不是打包模式。

HERE="$(cd "$(dirname "$0")" && pwd)"

export DYLD_LIBRARY_PATH="$HERE/lib:${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"

exec "$HERE/text_game" "$@"
