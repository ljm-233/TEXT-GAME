#include "doctest.h"
#include "config/bootstrap.h"
#include "config/preferences.h"
#include "core/container.h"
#include "core/paths.h"

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

    box.resolvePreferences();   // 不应该抛异常

    CHECK_FALSE(fs::exists(box.configFile()));   // 也不会凭空造文件
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
