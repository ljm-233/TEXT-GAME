#include "doctest.h"
#include "core/paths.h"
#include "core/resource_manager.h"

#include <filesystem>
#include <fstream>
#include <string>

// 静态资源寻址。
//
// 以前调用方各自手拼 —— assetFile("levels/editor.txt")、assetsDir() / "lang" ——
// 资源根散落在十几个地方，而且没有任何穿越防护。这个类把它们收敛成一张别名表。

namespace {

namespace fs = std::filesystem;

/// 平台无关地比较路径字符串。
///
/// ⚠️ 拿 `.string()` 和写死的 "/a/b" 比是**平台相关**的：Windows 上
///    `path("C:\\tmp") / "x"` 用反斜杠拼出来是 `C:\tmp\x`，跟 POSIX 字面量对不上
///    （CI 的 Windows job 就是这么红的）。
///    `make_preferred()` 把分隔符统一成当前平台的首选形式再比。
///    另外 `fs::path == fs::path` 是**逐段**比较的，不受内部分隔符影响 ——
///    两边都是路径时优先用 path 比。
bool samePath(const std::string& a, const std::string& b) {
    return std::filesystem::path(a).make_preferred() ==
           std::filesystem::path(b).make_preferred();
}

/// 手工装一个别名表（不依赖 Paths，测试更直接）
ResourceManager makeManager() {
    ResourceManager rm;
    rm.add("assets", "/pkg/assets");
    rm.add("levels", "/pkg/assets/levels");
    rm.add("lang", "/pkg/assets/lang");
    rm.add("wallpaper", "/pkg/wallpaper");
    return rm;
}

/// 临时根目录，用来验证标准别名表
struct Sandbox {
    fs::path root;
    Sandbox() {
        static int counter = 0;
        root = fs::temp_directory_path() / ("textgame_res_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(root, ec);
        fs::create_directories(root / "assets" / "levels");
    }
    ~Sandbox() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

} // namespace

TEST_CASE("ResourceManager - 标准别名表指向正确的位置") {
    Sandbox box;
    Paths paths(box.root);
    ResourceManager rm(paths);

    CHECK(rm.dir("assets") == box.root / "assets");
    CHECK(rm.dir("levels") == box.root / "assets" / "levels");
    CHECK(rm.dir("shaders") == box.root / "assets" / "shaders");
    CHECK(rm.dir("lang") == box.root / "assets" / "lang");
    CHECK(rm.dir("defaults") == box.root / "assets" / "defaults");
    CHECK(rm.dir("wallpaper") == box.root / "wallpaper");

    // 别名按字典序，方便直接 diff 两次启动的快照
    CHECK(rm.aliases() == std::vector<std::string>{"assets", "defaults", "lang", "levels",
                                                   "shaders", "wallpaper"});
    CHECK(rm.str().find("6 个别名") != std::string::npos);
}

TEST_CASE("ResourceManager - get 正常拼子路径") {
    auto rm = makeManager();

    CHECK(rm.get("levels") == fs::path("/pkg/assets/levels"));
    CHECK(rm.get("levels", "level1.txt") == fs::path("/pkg/assets/levels/level1.txt"));
    CHECK(rm.get("levels", "sub/deep.txt") ==
          fs::path("/pkg/assets/levels/sub/deep.txt"));
    CHECK(rm.get("assets", "font.ttf") == fs::path("/pkg/assets/font.ttf"));

    // 多余的 "./" 会被归一化掉
    CHECK(rm.get("levels", "./level1.txt") == fs::path("/pkg/assets/levels/level1.txt"));
}

TEST_CASE("ResourceManager - 未知别名返回空而不是瞎猜") {
    auto rm = makeManager();

    CHECK_FALSE(rm.has("nope"));
    CHECK(rm.dir("nope").empty());
    CHECK(rm.get("nope", "x").empty());
    CHECK(rm.get("nope").empty());
}

// ============================================================
// 路径穿越防护
// ============================================================

TEST_CASE("ResourceManager - 挡掉往上爬的 ../") {
    auto rm = makeManager();

    CHECK(rm.get("levels", "../../etc/passwd").empty());
    CHECK(rm.get("levels", "..").empty());
    CHECK(rm.get("levels", "../").empty());
    CHECK(rm.get("levels", "../lang/zh.txt").empty()); // 爬到兄弟目录也不行
    CHECK(rm.get("assets", "../wallpaper/x.jpg").empty());
}

TEST_CASE("ResourceManager - 挡掉当前平台意义上的绝对路径") {
    auto rm = makeManager();

    // POSIX 绝对路径：任何平台上 has_root_directory 都成立，一律拒
    CHECK(rm.get("levels", "/etc/passwd").empty());

    // ⚠️ Windows 的 "C:\evil" 只在 Windows 上算绝对路径。
    //    is_absolute() / has_root_name() 本身就是**平台相关**的（跟
    //    is_absolute("/etc/passwd") 在 Windows 上不成立是一回事），
    //    所以这里跟着平台走，不硬套另一边的语义：
    //    在 Linux 上它就是个普通文件名，拼在别名根里出不去。
    const fs::path win("C:\\evil");
    if (win.is_absolute() || win.has_root_name())
        CHECK(rm.get("levels", "C:\\evil").empty());
    else
        CHECK(rm.get("levels", "C:\\evil") == fs::path("/pkg/assets/levels/C:\\evil"));
}

TEST_CASE("ResourceManager - 不能靠同名前缀蒙混过关") {
    auto rm = makeManager();
    rm.add("assets2", "/pkg/assets2"); // 故意造一个前缀相同的兄弟

    // "/pkg/assets/levels/../levels2/x" 归一化后是 "/pkg/assets/levels2/x"，
    // 它**不在** levels 的子树里 —— 逐段比较才拦得住，纯字符串前缀比较会放过
    CHECK(rm.get("levels", "../levels2/x").empty());
    CHECK(rm.get("assets", "ett").empty() == false); // assets 自己的子路径正常
    // 这一处用 .string() 比是因为路径是拼出来的（Windows 上会用反斜杠）
    CHECK(samePath(rm.get("assets2", "x").string(), "/pkg/assets2/x"));
}

TEST_CASE("ResourceManager - 没爬出子树的 .. 是允许的") {
    auto rm = makeManager();

    // "levels/../levels/level1.txt" 归一化之后还在 levels 里，没必要拒
    CHECK(rm.get("levels", "../levels/level1.txt") ==
          fs::path("/pkg/assets/levels/level1.txt"));
    CHECK(rm.get("levels", "sub/../level2.txt") ==
          fs::path("/pkg/assets/levels/level2.txt"));
}

TEST_CASE("ResourceManager - 空别名表的 get 一律返回空") {
    ResourceManager empty;
    CHECK(empty.aliases().empty());
    CHECK(empty.get("levels", "../x").empty());
    CHECK(empty.get("", "").empty());
}

// ============================================================
// ${path:别名} 展开
// ============================================================

TEST_CASE("ResourceManager - expand 展开 ${path:别名}") {
    auto rm = makeManager();

    CHECK(rm.expand("${path:levels}") == "/pkg/assets/levels");
    CHECK(rm.expand("${path:levels}/level1.txt") == "/pkg/assets/levels/level1.txt");
    CHECK(rm.expand("${path:wallpaper}/a.jpg") == "/pkg/wallpaper/a.jpg");
    CHECK(rm.expand("x=${path:lang}/zh.txt") == "x=/pkg/assets/lang/zh.txt");
    CHECK(rm.expand("${path:lang} 和 ${path:levels}") ==
          "/pkg/assets/lang 和 /pkg/assets/levels");

    // 子路径整个写进花括号里时，拼出来的是**当前平台的原生分隔符**；
    // 写成 "${path:别名}/子路径"（斜杠在花括号外面）时那一段是配置里的字面文本，
    // 不会被改写 —— 需要跨平台一致的路径就用前一种写法。
    CHECK(rm.expand("${path:levels/level1.txt}") ==
          rm.get("levels", "level1.txt").string());
    CHECK(samePath(rm.expand("${path:levels}/level1.txt"),
                   rm.get("levels", "level1.txt").string()));
}

TEST_CASE("ResourceManager - expand 对不需要展开的值零成本原样返回") {
    auto rm = makeManager();

    // 热路径：绝大多数配置值里没有 ${，只该是一次 find
    CHECK(rm.expand("") == "");
    CHECK(rm.expand("zh") == "zh");
    CHECK(rm.expand("1.5") == "1.5");
    CHECK(rm.expand("{}") == "{}");
}

TEST_CASE("ResourceManager - expand 解析不出来时保留原文") {
    auto rm = makeManager();

    // 写错的别名要看得见，不能静默变成空路径
    CHECK(rm.expand("${path:nope}") == "${path:nope}");
    CHECK(rm.expand("${path:nope}/x") == "${path:nope}/x");
    // 只认 path: 这一个命名空间
    CHECK(rm.expand("${other:levels}") == "${other:levels}");
    CHECK(rm.expand("${}") == "${}");
    // 语法不完整
    CHECK(rm.expand("${path:levels") == "${path:levels");
    // 穿越同样挡住，且保留原文
    CHECK(rm.expand("${path:levels/../../etc/passwd}") ==
          "${path:levels/../../etc/passwd}");
}
