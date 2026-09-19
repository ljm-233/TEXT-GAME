#!/bin/bash
#
# 打包 AppImage。
#
# 前置：
#   - 已编译 Release 版本（cmake --preset release-package && cmake --build --preset release-package）
#   - 已生成图标（python3 packaging/icons/generate_icons.py）
#   - 已安装 appimagetool
#
# 输出：
#   TEXT-GAME-<version>-x86_64.AppImage
#
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$(pwd)"

VERSION="0.1.0"
BUILD_DIR="${TEXTGAME_BUILD_DIR:-$ROOT/build/release-package}"
APP_NAME="TEXT-GAME"
APP_ID="text-game"
APPDIR="$ROOT/build/AppDir"
OUTPUT="$ROOT/build/${APP_NAME}-${VERSION}-x86_64.AppImage"

echo "=== 检查前置 ==="
if [ ! -x "$BUILD_DIR/text_game" ]; then
    echo "错误: 找不到 $BUILD_DIR/text_game"
    echo "请先运行:"
    echo "  cmake --preset release-package"
    echo "  cmake --build --preset release-package"
    exit 1
fi

if ! command -v appimagetool >/dev/null 2>&1; then
    echo "错误: 未找到 appimagetool"
    echo ""
    echo "下载方式:"
    echo "  wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    echo "  chmod +x appimagetool-x86_64.AppImage"
    echo "  sudo mv appimagetool-x86_64.AppImage /usr/local/bin/appimagetool"
    echo ""
    echo "或者 Arch:"
    echo "  yay -S appimagetool"
    exit 1
fi

if [ ! -f "$ROOT/packaging/icons/text-game.png" ]; then
    echo "错误: 未生成图标"
    echo "请先运行: python3 packaging/icons/generate_icons.py"
    exit 1
fi

echo "=== 清理旧的 AppDir ==="
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor"

echo "=== 复制可执行文件 ==="
cp "$BUILD_DIR/text_game" "$APPDIR/usr/bin/text_game"
chmod +x "$APPDIR/usr/bin/text_game"

echo "=== 复制资源（和可执行文件同目录，便于 Platform::executableDir 找到）==="
cp -r "$ROOT/assets"    "$APPDIR/usr/bin/assets"
cp -r "$ROOT/wallpaper" "$APPDIR/usr/bin/wallpaper"

echo "=== 复制动态库（SFML / 音频）==="
# 复制所有匹配的文件（包括软链接链，例如 .so / .so.3 / .so.3.1 / .so.3.1.0）
copy_lib() {
    local pattern="$1"
    local found=0
    for f in $(find /usr/lib /usr/lib64 -maxdepth 1 -name "$pattern" 2>/dev/null); do
        cp -av "$f" "$APPDIR/usr/lib/"
        found=1
    done
    if [ "$found" -eq 0 ]; then
        echo "  [警告] 未找到: $pattern"
    fi
}

for pat in \
    'libsfml-graphics.so.*' \
    'libsfml-window.so.*' \
    'libsfml-system.so.*' \
    'libsfml-audio.so.*' \
    'libopenal.so.*' \
    'libvorbis.so.*' \
    'libvorbisfile.so.*' \
    'libvorbisenc.so.*' \
    'libogg.so.*' \
    'libFLAC.so.*' \
    'libsndfile.so.*' \
    'libmpg123.so.*' \
    'libopus.so.*' \
    'libopusfile.so.*' \
    'libfreetype.so.*' \
    'libharfbuzz.so.*' \
    'libfontconfig.so.*' \
    'libpng*.so.*' \
    'libjpeg.so.*' \
    'libbrotli*.so.*' \
    'libz.so.*' \
    'libbz2.so.*' \
    'liblzma.so.*' \
    'libexpat.so.*' \
    'libgraphite2.so.*' \
    'libglib-2.0.so.*' \
    'libX11.so.*' \
    'libXext.so.*' \
    'libXcursor.so.*' \
    'libXrandr.so.*' \
    'libXi.so.*' \
    'libXfixes.so.*' \
    'libXrender.so.*' \
    'libxcb.so.*' \
    'libxcb-*.so.*' \
    'libwayland-client.so.*' \
    'libwayland-cursor.so.*' \
    'libwayland-egl.so.*'
do
    copy_lib "$pat"
done

echo "=== 校验依赖 ==="
# 用 ldd 找出可执行文件的所有直接 + 间接依赖，检查 AppDir 里是否都有
MISSING_LIBS=0
ldd "$APPDIR/usr/bin/text_game" 2>/dev/null | while read line; do
    if echo "$line" | grep -q '=>'; then
        lib_path=$(echo "$line" | awk '{print $3}')
        lib_name=$(echo "$line" | awk '{print $1}')
        if [ -n "$lib_path" ] && [ "$lib_path" != "not" ]; then
            # 系统库不算（libc / libm / libpthread 等由系统提供）
            case "$lib_name" in
                linux-vdso*|ld-linux*|libc.so*|libm.so*|libpthread.so*|libdl.so*|librt.so*|libstdc++.so*|libgcc_s.so*)
                    continue
                    ;;
            esac
            # 检查 AppDir 里有没有
            if ! [ -e "$APPDIR/usr/lib/$lib_name" ]; then
                echo "  [缺失] $lib_name  (来自 $lib_path)"
                MISSING_LIBS=1
            fi
        fi
    fi
done

if [ "$MISSING_LIBS" -eq 1 ]; then
    echo ""
    echo "警告: 有依赖未打包。如果 AppImage 在干净系统上跑不起来，"
    echo "      请把这些库名加到上面的 copy_lib 调用列表里。"
    echo ""
fi

echo "=== 复制 .desktop 和图标 ==="
cp "$ROOT/packaging/linux/text-game.desktop" "$APPDIR/"
cp "$ROOT/packaging/linux/text-game.desktop" "$APPDIR/usr/share/applications/"

cp "$ROOT/packaging/icons/text-game.png" "$APPDIR/"
for size in 16 32 48 64 128 256; do
    icon_dir="$APPDIR/usr/share/icons/hicolor/${size}x${size}/apps"
    mkdir -p "$icon_dir"
    cp "$ROOT/packaging/icons/${size}x${size}.png" "$icon_dir/text-game.png"
done

echo "=== 复制 AppRun ==="
cp "$ROOT/packaging/linux/AppRun" "$APPDIR/AppRun"
chmod +x "$APPDIR/AppRun"

echo "=== 打包 AppImage ==="
cd "$ROOT/build"
ARCH=x86_64 appimagetool "$APPDIR" "$OUTPUT"

echo ""
echo "=== 完成 ==="
echo "输出: $OUTPUT"
echo "大小: $(du -h "$OUTPUT" | cut -f1)"
echo ""
echo "运行测试:"
echo "  $OUTPUT"