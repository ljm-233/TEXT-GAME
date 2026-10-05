#include "doctest.h"
#include "config/bootstrap.h"
#include "config/preferences.h"
#include "core/container.h"
#include "core/paths.h"
#include "core/resource_manager.h"
#include "core/platform.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

// 首次运行的默认设置播种（config/bootstrap.cpp 里的 seedDefaultIfMissing）。
//
// 为什么要有这块逻辑：打包模式下 Config 读的是 ~/.config/text-game/，
// 包里的文件它根本不会看。想让发行版带上"预设设置"，只能在首次运行时落一次盘。
//
// 这里守两条性质，第二条比第一条重要得多：
//   1. 目标文件不存在 + 包里有默认值  -> 播种
//   2. 目标文件已存在（用户改过）     -> 绝不覆盖
//
// 第 2 条要是坏了，每次升级都会把玩家的设置冲掉，而且很难被注意到。
namespace {

namespace fs = std::filesystem;

/// 一个独立沙箱：Paths 挂在临时目录下，assets/defaults/ 由用例决定放不放。
struct Sandbox {
    fs::path root;
    Container container;

    explicit Sandbox(bool withBundledDefault) {
        static int counter = 0;
        root = fs::temp_directory_path() /
               ("textgame_seed_test_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(root, ec);

        // Paths(root) 会把 config / assets / cache … 都建出来
        auto paths = std::make_shared<Paths>(root);

        if (withBundledDefault) {
            const fs::path dir = paths->assetsDir() / "defaults";
            fs::create_directories(dir, ec);
            std::ofstream out(dir / "preferences.conf");
            out << "theme=2\nparallax=false\n";
        }

        container.reg<Paths>("paths", [paths]() { return paths; });
        // Config 现在从 ResourceManager 取默认配置文件（别名 defaults），
        // 所以沙箱里也得把它注册上
        container.reg<ResourceManager>(
            "resources", [paths]() { return std::make_shared<ResourceManager>(*paths); });
        registerConfig(container);
    }

    ~Sandbox() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    fs::path configFile() const { return root / "config" / "preferences.conf"; }
    fs::path bundledDefault() const {
        return root / "assets" / "defaults" / "preferences.conf";
    }

    std::string readConfig() const {
        std::ifstream in(configFile());
        return std::string(std::istreambuf_iterator<char>(in),
                           std::istreambuf_iterator<char>());
    }

    void writeUserConfig(const std::string& content) const {
        std::ofstream out(configFile());
        out << content;
    }

    /// 走一次真实的解析路径（工厂里会播种）
    void resolvePreferences() const {
        auto p = const_cast<Container&>(container).get<Preferences>("preferences");
        REQUIRE(p != nullptr);
    }

    /// 拿到 Preferences 实例（首次调用会触发工厂 + 播种）。
    /// 注意 Config 在构造时就把文件读进内存了，所以要改配置得先 writeUserConfig()。
    std::shared_ptr<Preferences> prefs() {
        auto p = container.get<Preferences>("preferences");
        REQUIRE(p != nullptr);
        return p;
    }
};

} // namespace

TEST_CASE("配置播种 - 包里带默认值且用户没有配置时，铺一份过去") {
    Sandbox box(true);
    REQUIRE_FALSE(fs::exists(box.configFile()));

    box.resolvePreferences();

    REQUIRE(fs::exists(box.configFile()));
    CHECK(box.readConfig() == "theme=2\nparallax=false\n");
}

TEST_CASE("配置播种 - 用户已经改过就绝不覆盖") {
    // 注意这里其实有**两层**保护：seedDefaultIfMissing 里的 exists 检查，
    // 以及 copy_file 用的是 copy_options::none（它本身就不覆盖已存在的文件）。
    // 变异测试证实过：只摘掉其中任一层，本用例都不会失败 —— 行为没变。
    // 两层都拿掉才会失败。所以本用例守的是"最终语义"，不是某一行的实现。
    Sandbox box(true);
    box.writeUserConfig("theme=1\nplayer_name=我自己\n");

    box.resolvePreferences();

    // 关键断言：内容一个字都没变
    CHECK(box.readConfig() == "theme=1\nplayer_name=我自己\n");
}

TEST_CASE("配置播种 - 包里没带默认值时不报错，走内置默认") {
    Sandbox box(false);
    REQUIRE_FALSE(fs::exists(box.bundledDefault()));

    box.resolvePreferences(); // 不应该抛异常

    CHECK_FALSE(fs::exists(box.configFile())); // 也不会凭空造文件
}

TEST_CASE("配置播种 - 播种出来的值真的能被读出来") {
    Sandbox box(true);

    box.resolvePreferences();

    auto prefs = box.container.get<Preferences>("preferences");
    REQUIRE(prefs != nullptr);
    CHECK(prefs->getInt("theme", -1) == 2);
    CHECK(prefs->getBool("parallax", true) == false);
}

TEST_CASE("配置播种 - 只播一次（第二次解析不会重写）") {
    Sandbox box(true);

    box.resolvePreferences();
    const std::string first = box.readConfig();

    // 模拟用户改了设置
    box.writeUserConfig(first + "ui_scale=1.250000\n");

    box.resolvePreferences();

    CHECK(box.readConfig() == first + "ui_scale=1.250000\n");
}

// ============================================================
// 资源根目录
// ============================================================

TEST_CASE("Paths::resourceRootFor - 平常就是可执行文件所在目录") {
    using std::filesystem::path;

    CHECK(Paths::resourceRootFor("/opt/text-game") == path("/opt/text-game"));
    CHECK(Paths::resourceRootFor("C:/games/text-game") == path("C:/games/text-game"));

    // 只有 Contents/MacOS 才特殊，单独一个 MacOS 不算
    CHECK(Paths::resourceRootFor("/tmp/MacOS") == path("/tmp/MacOS"));
    CHECK(Paths::resourceRootFor("/tmp/Contents/bin") == path("/tmp/Contents/bin"));
}

TEST_CASE("Paths::resourceRootFor - macOS bundle 里指到 Contents/Resources") {
    using std::filesystem::path;

    // .app 里可执行文件在 Contents/MacOS/，资源按 macOS 的规矩在 Contents/Resources/。
    // 塞进 Contents/MacOS/ 会让 codesign 把那些目录当成嵌套代码去验签
    // （"code object is not signed at all"），签名失败、包都做不出来。
    CHECK(Paths::resourceRootFor("/tmp/text_game.app/Contents/MacOS") ==
          path("/tmp/text_game.app/Contents/Resources"));
}

// ============================================================
// 开发模式找源码树
// ============================================================

namespace {

/// 造一棵 "<base>/proj/{CMakeLists.txt,src/,build/release}" 的假源码树
struct FakeTree {
    std::filesystem::path base;

    FakeTree() {
        namespace fs = std::filesystem;
        static int counter = 0;
        base =
            fs::temp_directory_path() / ("textgame_devroot_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(base, ec);
        fs::create_directories(base / "proj" / "src");
        fs::create_directories(base / "proj" / "build" / "release");
        std::ofstream(base / "proj" / "CMakeLists.txt") << "x\n";
    }
    ~FakeTree() {
        std::error_code ec;
        std::filesystem::remove_all(base, ec);
    }
};

} // namespace

TEST_CASE("Paths::findDevRoot - 往上找到同时有 CMakeLists.txt 与 src/ 的那层") {
    FakeTree t;
    CHECK(Paths::findDevRoot(t.base / "proj" / "build" / "release") == t.base / "proj");
    CHECK(Paths::findDevRoot(t.base / "proj" / "build") == t.base / "proj");
    CHECK(Paths::findDevRoot(t.base / "proj" / "src") == t.base / "proj");
}

TEST_CASE("Paths::findDevRoot - 只有 CMakeLists.txt、没有 src/ 不算源码树") {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path base = fs::temp_directory_path() / "textgame_devroot_nosrc";
    fs::remove_all(base, ec);
    fs::create_directories(base / "sub");
    std::ofstream(base / "CMakeLists.txt") << "x\n";

    CHECK(Paths::findDevRoot(base / "sub").empty()); // 只认 src/ + CMakeLists.txt
    fs::remove_all(base, ec);
}

TEST_CASE("Paths::findDevRoot - 找不到就返回空（不能瞎猜一个绝对路径）") {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path base = fs::temp_directory_path() / "textgame_devroot_none";
    fs::remove_all(base, ec);
    fs::create_directories(base / "a" / "b" / "c" / "d" / "e" / "f" / "g");

    CHECK(Paths::findDevRoot(base / "a" / "b" / "c" / "d" / "e" / "f" / "g").empty());
    fs::remove_all(base, ec);
}

TEST_CASE("Paths::findDevRoot - 当前构建目录真能往上找到本项目的源码树") {
    // ⭐ 这条最有用：它验证的是「开发模式下游戏真的找得到 assets/」。
    //    单元测试跑在 build/<preset>/tests/ 下，往上应该落在仓库根。
    //    以前这一步靠编译期宏 PROJECT_ROOT，那会把构建机路径编进二进制
    //    （makepkg 报「软件包含有对 $srcdir 的引用」），所以改成了运行时往上找。
    const std::filesystem::path root = Paths::findDevRoot(Platform::executableDir());

    REQUIRE_FALSE(root.empty());
    CHECK(root == std::filesystem::path(PROJECT_ROOT));
    CHECK(std::filesystem::exists(root / "assets" / "font.ttf"));
}

// ============================================================
// 值里的 ${path:别名} 插值
// ============================================================

TEST_CASE("Config::get - 展开值里的 ${path:别名}") {
    Sandbox box(false);
    // Config 构造时就把文件读走了，所以先写文件再取实例
    box.writeUserConfig("wallpaper_file=${path:wallpaper}/a.jpg\n"
                        "lang_file=${path:lang}/zh.txt\n"
                        "plain=3\n");
    auto prefs = box.prefs();

    CHECK(prefs->get("wallpaper_file") == (box.root / "wallpaper" / "a.jpg").string());
    CHECK(prefs->get("lang_file") == (box.root / "assets" / "lang" / "zh.txt").string());

    // 不含 ${ 的值原样返回（绝大多数键走这条）
    CHECK(prefs->get("plain") == "3");
    CHECK(prefs->getInt("plain") == 3);
}

TEST_CASE("Config::get - 别名写错时保留原文而不是变成空路径") {
    Sandbox box(false);
    box.writeUserConfig("bad=${path:nope}/x\n"
                        "traversal=${path:levels/../../etc/passwd}\n");
    auto prefs = box.prefs();

    // 写错的名字要看得见 —— 静默变成空路径会让"找不到资源"极难排查
    CHECK(prefs->get("bad") == "${path:nope}/x");
    CHECK(prefs->get("traversal") == "${path:levels/../../etc/passwd}");
}

TEST_CASE("Config - 裸路径构造时没有资源表，不做插值") {
    namespace fs = std::filesystem;
    const fs::path f = fs::temp_directory_path() / "textgame_cfg_bare.conf";
    {
        std::ofstream out(f);
        out << "p=${path:wallpaper}/a.jpg\n";
    }

    Config bare(f);
    CHECK(bare.get("p") == "${path:wallpaper}/a.jpg"); // 原样，不崩

    std::error_code ec;
    fs::remove(f, ec);
}
