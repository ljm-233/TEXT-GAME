#include "doctest.h"
#include "wallpaper/wallpaper_library.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

// wallpaper/ 层的纯逻辑测试：扫盘、匹配、排序。
//
// 这一层故意不依赖 GL，所以能在没有 DISPLAY 的环境里跑全套单测（CLAUDE.md
// 里"测试必须能在没有 DISPLAY 的环境下全绿"那条）。Background 留在 ui 层
// 负责 sf::Texture 上传，这部分没单测。

namespace fs = std::filesystem;

namespace {

// 写一个最小 PNG —— sf::Image 不在这里被用，但 Library 的"扩展名过滤"
// 也只认 .jpg/.jpeg/.png，写 .jpg/.png 才能进入候选列表。
//
// 但其实不需要真 PNG：test 只关心**哪些文件被接受**，不关心文件能不能解码。
// Library 的 scan() 只做"是文件吗、是支持扩展名吗"，不解码 ——
// 解码是 WallpaperLoader 的活儿。
//
// 所以测试里直接写空文件就行 —— Library 不会去打开它。

struct ScratchDir {
    fs::path root;
    explicit ScratchDir(const std::string& tag) {
        static int counter = 0;
        root = fs::temp_directory_path() /
               ("textgame_wplib_" + tag + "_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(root, ec);
        fs::create_directories(root, ec);
        REQUIRE(!ec);
    }
    ~ScratchDir() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    void touch(const std::string& name) const {
        std::ofstream f(root / name);
        // 空文件即可 —— scan 不打开
    }
};

} // namespace

TEST_CASE("WallpaperLibrary - scan 接受 jpg/jpeg/png，大小写不敏感") {
    ScratchDir dir("ext");
    dir.touch("a.JPG");
    dir.touch("b.Jpeg");
    dir.touch("c.PNG");
    dir.touch("d.gif"); // 不接受
    dir.touch("e.txt");
    dir.touch("README.md");

    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.size() == 3);
    // 排序按小写文件名比 → a, b, c
    CHECK(lib.filenames()[0] == "a.JPG");
    CHECK(lib.filenames()[1] == "b.Jpeg");
    CHECK(lib.filenames()[2] == "c.PNG");
}

TEST_CASE("WallpaperLibrary - scan 忽略子目录里的图") {
    // 背景扫描只看"这一层"，不递归。子目录下的图被无视 —— 比起递归扫 + 递归路径
    // 拼装，简单且不会因为误收录深目录里的图而把 Background 的 sf::Texture
    // 渲染路径搞复杂。
    ScratchDir dir("norec");
    dir.touch("root.jpg");
    fs::create_directory(dir.root / "sub");
    std::ofstream(dir.root / "sub" / "deep.png"); // 应被忽略

    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.size() == 1);
    CHECK(lib.filenames()[0] == "root.jpg");
}

TEST_CASE("WallpaperLibrary - scan 目录不存在视为空，不抛异常") {
    WallpaperLibrary lib;
    lib.scan("/nonexistent/path/that/should/not/exist/xyz");
    CHECK(lib.empty());
}

TEST_CASE("WallpaperLibrary - at 越界返回空 info") {
    ScratchDir dir("at");
    dir.touch("a.jpg");
    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.at(0).valid());
    CHECK(!lib.at(-1).valid());
    CHECK(!lib.at(1).valid());
    CHECK(!lib.at(999).valid());
}

TEST_CASE("WallpaperLibrary - resolveIndex 全名匹配") {
    ScratchDir dir("res_full");
    dir.touch("alpha.jpg");
    dir.touch("beta.png");
    dir.touch("gamma.jpeg");
    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.resolveIndex("alpha.jpg") == 0);
    CHECK(lib.resolveIndex("beta.png") == 1);
    CHECK(lib.resolveIndex("gamma.jpeg") == 2);
}

TEST_CASE("WallpaperLibrary - resolveIndex 扩展名换了也能接上") {
    // 0.3.4 把 wallhaven-d88d53.png 换成 .jpg 后，老用户的 current_wallpaper
    // 里存的是 .png —— 没有这层回退，壁纸会静默打回默认，看起来像"设置自己丢了"。
    ScratchDir dir("res_stem");
    dir.touch("wallhaven-d88d53.jpg");
    dir.touch("wallpaper.png");
    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.resolveIndex("wallhaven-d88d53.png") == 0);
    CHECK(lib.resolveIndex("wallhaven-d88d53.jpeg") == 0);
    CHECK(lib.resolveIndex("wallhaven-d88d53.jpg") == 0); // 全名也要中
    CHECK(lib.resolveIndex("wallpaper.png") == 1);
}

TEST_CASE("WallpaperLibrary - resolveIndex 全名优先于主名匹配") {
    // 两个都存在时必须是精确那个，不能被主名匹配抢走
    ScratchDir dir("res_pref");
    dir.touch("x.jpg");
    dir.touch("x.png");
    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.resolveIndex("x.jpg") == 0);
    CHECK(lib.resolveIndex("x.png") == 1);
}

TEST_CASE("WallpaperLibrary - resolveIndex 对不上 / 空请求 / 空库") {
    ScratchDir dir("res_neg");
    dir.touch("a.png");
    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.resolveIndex("nope.png") == -1);
    CHECK(lib.resolveIndex("") == -1);

    WallpaperLibrary empty;
    CHECK(empty.resolveIndex("a.png") == -1);
}

TEST_CASE("WallpaperLibrary - resolveIndex 中文文件名也能接上") {
    // sf::filesystem::path 用 UTF-8 存 stem —— 主名匹配是字节级比较，
    // 中文 UTF-8 字节不会跟英文冲突，所以这一条对中文友好。
    ScratchDir dir("res_cn");
    dir.touch("【哲风壁纸】少女-校园背景.jpg");
    dir.touch("wallpaper.jpg");
    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.resolveIndex("【哲风壁纸】少女-校园背景.jpg") >= 0);
    CHECK(lib.resolveIndex("【哲风壁纸】少女-校园背景.png") >= 0); // 扩展名换了也能接
}

TEST_CASE("WallpaperLibrary - 同一主名 + 不同扩展名多个候选时取第一个") {
    // 库里同时存在 x.jpg 和 x.png，要求 main 名前缀匹配时返回第一个
    // —— 这是退化情况，但不抛异常、不返回 -1 是契约的一部分。
    ScratchDir dir("res_dup");
    dir.touch("x.jpg");
    dir.touch("x.png");
    WallpaperLibrary lib;
    lib.scan(dir.root);
    CHECK(lib.resolveIndex("x.tiff") >= 0); // 任何一个都行
    CHECK(lib.resolveIndex("x.jpg") == 0);  // 全名命中优先
    CHECK(lib.resolveIndex("x.png") == 1);
}
