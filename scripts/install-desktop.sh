#!/usr/bin/env bash
#
# 把 TEXT-GAME 装进桌面环境 —— 不用开终端、不用敲路径就能进游戏。
#
#   scripts/install-desktop.sh          安装
#   scripts/install-desktop.sh --remove 卸载
#
# 装三样东西，全在当前用户目录下，不需要 root：
#
#   ~/.local/share/icons/hicolor/<N>x<N>/apps/text-game.png   各尺寸图标
#   ~/.local/share/applications/text-game.desktop             应用菜单项
#   ~/.local/bin/text-game                                    软链接 -> 本仓库的 s.sh
#
# 装完之后：
#   - 应用菜单 / 启动器里搜 "TEXT-GAME" 就能点开
#   - 终端里任意目录敲 text-game 即可（前提是 ~/.local/bin 在 PATH 里）
#
# 仓库里已有的 packaging/linux/text-game.desktop 是给 AppImage 用的
# （Exec=text_game，假定可执行文件已在 PATH）。这里是**开发环境**版本，
# Exec 直接指向仓库里的 s.sh，于是每次启动都会先增量构建。
#
set -euo pipefail

# 先解析软链接再取目录：这些脚本可能被 ~/.local/bin 下的软链调用，
# 不解析的话 dirname 会落到软链所在的目录。
SELF="${BASH_SOURCE[0]}"
if command -v readlink > /dev/null && readlink -f "$SELF" > /dev/null 2>&1; then
    SELF="$(readlink -f "$SELF")"
fi
ROOT="$(cd "$(dirname "$SELF")/.." && pwd)"

DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
ICON_ROOT="$DATA_HOME/icons/hicolor"
APP_DIR="$DATA_HOME/applications"
BIN_DIR="$HOME/.local/bin"

DESKTOP_FILE="$APP_DIR/text-game.desktop"
LINK="$BIN_DIR/text-game"
ICON_SIZES=(16 32 48 64 128 256)

remove() {
    echo "[uninstall] 移除桌面项与图标..."
    rm -f "$DESKTOP_FILE" "$LINK"
    for size in "${ICON_SIZES[@]}"; do
        rm -f "$ICON_ROOT/${size}x${size}/apps/text-game.png"
    done
    refresh_caches
    echo "[uninstall] 完成。"
    exit 0
}

refresh_caches() {
    # 这两个工具不一定装了；缺了也不影响使用，只是缓存更新慢一点
    command -v update-desktop-database > /dev/null &&
        update-desktop-database "$APP_DIR" 2> /dev/null || true
    command -v gtk-update-icon-cache > /dev/null &&
        gtk-update-icon-cache -f -t "$ICON_ROOT" 2> /dev/null || true
}

if [ "${1:-}" = "--remove" ]; then
    remove
fi

# ---------------- 图标 ----------------
missing=0
for size in "${ICON_SIZES[@]}"; do
    src="$ROOT/packaging/icons/${size}x${size}.png"
    if [ ! -f "$src" ]; then
        echo "[install] 缺少图标：$src" >&2
        missing=1
        continue
    fi
    mkdir -p "$ICON_ROOT/${size}x${size}/apps"
    cp "$src" "$ICON_ROOT/${size}x${size}/apps/text-game.png"
done
if [ "$missing" = "1" ]; then
    echo "[install] 有图标缺失，先跑 packaging/icons/generate_icons.py 生成" >&2
    exit 1
fi
echo "[install] 图标 -> $ICON_ROOT/<N>x<N>/apps/text-game.png"

# ---------------- 桌面项 ----------------
# Exec 指向仓库里的 s.sh：先增量构建再启动，所以改完代码直接点图标就是新版。
# StartupWMClass 必须和窗口的 WM_CLASS 对上（实测是 "text_game", "TEXT-GAME"），
# 否则任务栏会把窗口当成另一个程序，图标也就对不上了。
mkdir -p "$APP_DIR"
cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=TEXT-GAME
GenericName=Platformer
Comment=A 2D platformer built from scratch in C++20 + SFML 3
Comment[zh_CN]=用 C++20 + SFML 3 从零手写的 2D 平台跳跃游戏
Exec=$ROOT/s.sh
TryExec=$ROOT/s.sh
Path=$ROOT
Icon=text-game
Terminal=false
Categories=Game;ActionGame;
Keywords=platformer;game;2d;sfml;
StartupNotify=false
StartupWMClass=TEXT-GAME
EOF
echo "[install] 桌面项 -> $DESKTOP_FILE"

# ---------------- PATH 软链接 ----------------
mkdir -p "$BIN_DIR"
ln -sfn "$ROOT/s.sh" "$LINK"
echo "[install] 命令   -> $LINK"

refresh_caches

echo
echo "[install] 完成。试试："
echo "            - 应用菜单里搜 \"TEXT-GAME\""
echo "            - 终端里敲 text-game"
case ":$PATH:" in
    *":$BIN_DIR:"*) ;;
    *)
        echo
        echo "[install] 注意：$BIN_DIR 不在 PATH 里，"
        echo "          敲 text-game 之前需要先把它加进去。"
        ;;
esac
