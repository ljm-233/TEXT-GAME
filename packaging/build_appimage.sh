#!/bin/bash
#
# 打包 AppImage。
#
# 前置：
#   - 已编译并配置好 Release（cmake --preset release-package）
#   - 已生成图标（python3 packaging/icons/generate_icons.py）
#   - 已安装 appimagetool
#
# 输出：
#   TEXT-GAME-<version>-x86_64.AppImage
#
# 依赖库不再手写清单，而是复用 CMake 的 install 规则：
# `BUNDLE_RUNTIME_DEPS=ON` 时 install(CODE) 会用 GET_RUNTIME_DEPENDENCIES
# 把 SFML 及其**间接**依赖的完整闭包算出来放进 lib/。
# 所以 AppImage 现在和 tar.gz / deb / rpm 用的是同一套闭包 ——
# 以前这里维护着一份四十来个库名的模式列表，漏一个就是"干净机器上起不来"，
# 而且和 CMake 那份各改各的。
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$(pwd)"

# 版本号从 CMakeLists.txt 读，避免和 project(... VERSION ...) 各写一份、慢慢漂移
VERSION="$(grep -oP 'project\(text_game VERSION \K[0-9]+\.[0-9]+\.[0-9]+' \
           "$ROOT/CMakeLists.txt" | head -1)"
if [ -z "$VERSION" ]; then
    echo "错误: 无法从 CMakeLists.txt 解析出版本号"
    exit 1
fi

BUILD_DIR="${TEXTGAME_BUILD_DIR:-$ROOT/build/release-package}"
STAGE="$ROOT/build/appstage"
APP_NAME="TEXT-GAME"
APPDIR="$ROOT/build/AppDir"
OUTPUT="$ROOT/build/${APP_NAME}-${VERSION}-x86_64.AppImage"

echo "=== 检查前置 ==="
if [ ! -f "$BUILD_DIR/cmake_install.cmake" ]; then
    echo "错误: $BUILD_DIR 还不是一个配置好的构建目录"
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

echo "=== 用 CMake 的 install 规则生成自带依赖的目录树 ==="
# 这一步就是 tar.gz / deb / rpm 走的同一条路：
# install(CODE) 里的 GET_RUNTIME_DEPENDENCIES + FOLLOW_SYMLINK_CHAIN
rm -rf "$STAGE"
cmake --install "$BUILD_DIR" --prefix "$STAGE"

LIBS=$(find "$STAGE/lib" -maxdepth 1 -name '*.so*' 2>/dev/null | wc -l)
echo "  依赖库: $LIBS 个"
if [ "$LIBS" -lt 5 ]; then
    echo "错误: lib/ 几乎是空的 —— 构建时是不是没开 BUNDLE_RUNTIME_DEPS？"
    exit 1
fi

echo "=== 组装 AppDir ==="
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor"

cp "$STAGE/text_game" "$APPDIR/usr/bin/text_game"
chmod +x "$APPDIR/usr/bin/text_game"

# 资源必须和可执行文件同目录 —— Paths 靠"可执行文件旁边有没有 assets/"
# 判断自己是不是打包模式
cp -r "$STAGE/assets"    "$APPDIR/usr/bin/assets"
cp -r "$STAGE/wallpaper" "$APPDIR/usr/bin/wallpaper"

# -a 保留软链（依赖里大量 .so -> .so.3 -> .so.3.1.0 的链）
cp -a "$STAGE/lib/." "$APPDIR/usr/lib/"

echo "=== 校验依赖 ==="
# 用 AppRun 同款的环境变量跑 ldd：有 not found 就说明闭包不完整
if LD_LIBRARY_PATH="$APPDIR/usr/lib" ldd "$APPDIR/usr/bin/text_game" | grep -q "not found"; then
    echo "错误: 有未解析的依赖："
    LD_LIBRARY_PATH="$APPDIR/usr/lib" ldd "$APPDIR/usr/bin/text_game" | grep "not found"
    exit 1
fi
echo "  无 not found ✓"

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
