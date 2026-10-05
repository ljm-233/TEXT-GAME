#include "doctest.h"
#include "background.h"

#include <filesystem>
#include <string>
#include <vector>

// 壁纸名解析。
//
// 这个函数存在的理由：素材换扩展名（wallhaven-d88d53.png -> .jpg）之后，
// 用户存在 current_wallpaper 里的旧名字就找不到文件了。没有这层回退，
// 壁纸会静默打回默认 —— 用户只会看到"设置自己丢了"。
//
// Background 本身有按值的 sf::Texture（GlResource），无界面环境里连构造都
// 做不到，所以这段纯逻辑抽成了自由函数，于是可以在这里直接测。

namespace {

std::vector<std::filesystem::path> files(std::initializer_list<const char*> names) {
    std::vector<std::filesystem::path> v;
    for (const char* n : names)
        v.emplace_back(n);
    return v;
}

} // namespace

TEST_CASE("壁纸名解析 - 全名对上就原样返回") {
    auto list = files({"a.png", "b.jpg", "c.png"});
    CHECK(resolveWallpaperName(list, "b.jpg") == "b.jpg");
    CHECK(resolveWallpaperName(list, "a.png") == "a.png");
}

TEST_CASE("壁纸名解析 - 只有扩展名对不上时按主名接上") {
    // 素材从 .png 换成了 .jpg，配置里还存着旧名字
    auto list = files({"wallhaven-d88d53.jpg", "wallpaper.png"});
    CHECK(resolveWallpaperName(list, "wallhaven-d88d53.png") == "wallhaven-d88d53.jpg");
    CHECK(resolveWallpaperName(list, "wallhaven-d88d53.jpeg") == "wallhaven-d88d53.jpg");
}

TEST_CASE("壁纸名解析 - 全名优先于主名匹配") {
    // 两个都真实存在时，必须是精确那个，不能被主名匹配抢走
    auto list = files({"x.jpg", "x.png"});
    CHECK(resolveWallpaperName(list, "x.jpg") == "x.jpg");
    CHECK(resolveWallpaperName(list, "x.png") == "x.png");
}

TEST_CASE("壁纸名解析 - 对不上或空请求都返回空串") {
    auto list = files({"a.png", "b.jpg"});
    CHECK(resolveWallpaperName(list, "nope.png").empty());
    CHECK(resolveWallpaperName(list, "").empty());
    CHECK(resolveWallpaperName({}, "a.png").empty());
}

TEST_CASE("壁纸名解析 - 中文文件名也能接上") {
    auto list = files({"【哲风壁纸】少女-校园背景.jpg", "wallpaper.jpg"});
    CHECK(resolveWallpaperName(list, "【哲风壁纸】少女-校园背景.jpg") ==
          "【哲风壁纸】少女-校园背景.jpg");
    CHECK(resolveWallpaperName(list, "【哲风壁纸】少女-校园背景.png") ==
          "【哲风壁纸】少女-校园背景.jpg");
}

TEST_CASE("壁纸名解析 - 主名匹配不会跨越目录") {
    // 只比文件名，不能因为目录不同就算匹配上
    auto list = files({"one/a.png", "two/b.png"});
    CHECK(resolveWallpaperName(list, "b.png") == "b.png");
    CHECK(resolveWallpaperName(list, "a.png") == "a.png");
}
