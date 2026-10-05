#!/bin/sh
#
# TEXT-GAME 启动器（自带依赖的 Linux 包用）。
#
# 为什么需要它：包里的 lib/ 里带了 SFML 等依赖，但**光设可执行文件的 RPATH
# 不够** —— RPATH 只管可执行文件的直接依赖，间接依赖（SFML 依赖的 freetype /
# harfbuzz / X11 …）是用【那个库自己的 RUNPATH】去解析的，包里那些库没设，
# 于是会静默回退到系统路径。开发机上一切正常，到了没装这些库的机器上就起不来。
#
# LD_LIBRARY_PATH 是全局的，直接 + 间接依赖都会优先从包内解析。
# 这也是 AppImage 的做法（见 packaging/linux/AppRun）。
#
# 包内布局：
#   TEXT-GAME    ← 这个脚本
#   text_game    ← 真正的二进制
#   lib/         ← 依赖库
#   assets/ wallpaper/
#
# 注意 assets 必须和可执行文件同级 —— 游戏的 Paths 靠"可执行文件旁边有没有
# assets/" 判断自己是不是打包模式。

HERE="$(dirname "$(readlink -f "$0")")"

export LD_LIBRARY_PATH="$HERE/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "$HERE/text_game" "$@"
