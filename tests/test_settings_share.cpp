#include "doctest.h"
#include "config/keys.h"
#include "config/preferences.h"
#include "core/resource_manager.h"
#include "config/settings_codec.h"
#include "core/paths.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

// 设置分享码的**端到端往返**。
//
// test_settings_codec 只管编解码本身（文本进去、文本出来），
// test_settings_tabs 只管"键归哪一页"。这两条都绿了，仍然可能出这种事：
// 导出把该带的键漏了、或者把**不该带的**键也带上了。
//
// 后者是真的会伤人的：把别人（或另一台机器）的 resolution_index / window_mode
// 导进来，可能开出一个当前显示器放不下、甚至起不来的窗口 ——
// 而设置界面正是用户用来改回去的地方。所以这条测试盯的是白名单边界。

namespace fs = std::filesystem;

namespace {

/// 一个只属于自己的配置沙箱（不碰真实存档）
struct Cfg {
    fs::path root;
    std::unique_ptr<Paths> paths;
    std::unique_ptr<ResourceManager> resources;
    std::shared_ptr<Preferences> prefs;

    explicit Cfg(const std::string& tag) {
        static int counter = 0;
        root = fs::temp_directory_path() /
               ("textgame_share_" + tag + "_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(root, ec);
        paths = std::make_unique<Paths>(root);
        resources = std::make_unique<ResourceManager>(*paths);
        prefs = std::make_shared<Preferences>(*paths, *resources);
    }
    ~Cfg() {
        // 先放掉 Config（析构会 flush），再删目录
        prefs.reset();
        paths.reset();
        resources.reset();
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

/// 走一遍完整链路：导出 → 编码 → 解码 → 应用到另一份配置
void share(const Cfg& from, Cfg& to) {
    const std::string code = SettingsCodec::encode(from.prefs->exportPortable());
    REQUIRE(!code.empty());
    const std::string decoded = SettingsCodec::decode(code);
    REQUIRE(!decoded.empty());
    to.prefs->applyPortable(decoded);
}

} // namespace

TEST_CASE("设置分享 - 观感/音量/玩法类键能完整往返") {
    Cfg a("a");
    Cfg b("b");

    a.prefs->set(ConfigKey::kTheme, "2");
    a.prefs->setDouble(ConfigKey::kAudioMasterVolume, 0.42);
    a.prefs->setInt(ConfigKey::kInitialLives, 5);
    a.prefs->setBool(ConfigKey::kParallax, false);
    a.prefs->setInt(ConfigKey::kKeyJump, 42);
    a.prefs->set(ConfigKey::kLanguage, "ja");

    share(a, b);

    CHECK(b.prefs->get(ConfigKey::kTheme, "?") == "2");
    CHECK(b.prefs->getDouble(ConfigKey::kAudioMasterVolume, -1.0) ==
          doctest::Approx(0.42));
    CHECK(b.prefs->getInt(ConfigKey::kInitialLives, -1) == 5);
    CHECK(b.prefs->getBool(ConfigKey::kParallax, true) == false);
    CHECK(b.prefs->getInt(ConfigKey::kKeyJump, -1) == 42);
    CHECK(b.prefs->get(ConfigKey::kLanguage, "?") == "ja");
}

TEST_CASE("设置分享 - 跟机器绑定的键**不会**被带走") {
    Cfg a("a");
    Cfg b("b");

    // 先把目标配置里的这些键设成"本机自己的值"
    b.prefs->setInt(ConfigKey::kResolutionIndex, 1);
    b.prefs->set(ConfigKey::kWindowMode, "1");
    b.prefs->setBool(ConfigKey::kVsync, true);
    b.prefs->setDouble(ConfigKey::kUiScale, 1.0);

    // 来源机器上是另一套（分辨率 7、独占全屏、关垂直同步、UI 缩放 1.75）
    a.prefs->setInt(ConfigKey::kResolutionIndex, 7);
    a.prefs->set(ConfigKey::kWindowMode, "2");
    a.prefs->setBool(ConfigKey::kVsync, false);
    a.prefs->setDouble(ConfigKey::kUiScale, 1.75);
    // ⚠️ 必须至少有一个**可携带**的键，否则 exportPortable() 就是空串、
    //    编出来的码也解不开 —— 那样这条用例会因为"码是空的"而失败，
    //    而不是因为它想验的东西
    a.prefs->set(ConfigKey::kTheme, "1");

    share(a, b);

    // 一条都不许被改 —— 这正是白名单存在的理由
    CHECK(b.prefs->getInt(ConfigKey::kResolutionIndex, -1) == 1);
    CHECK(b.prefs->get(ConfigKey::kWindowMode, "?") == "1");
    CHECK(b.prefs->getBool(ConfigKey::kVsync, false) == true);
    CHECK(b.prefs->getDouble(ConfigKey::kUiScale, -1.0) == doctest::Approx(1.0));
}

TEST_CASE("设置分享 - 导出的文本里本来就不含机器绑定的键") {
    Cfg a("a");
    a.prefs->setInt(ConfigKey::kResolutionIndex, 3);
    a.prefs->setBool(ConfigKey::kFullscreen, true);
    a.prefs->set(ConfigKey::kCurrentWallpaper, "x.jpg");
    a.prefs->set(ConfigKey::kTheme, "1");

    const std::string text = a.prefs->exportPortable();

    // 连"发出去"这一步都不该包含它们：别人拿到码也无从还原
    CHECK(text.find(ConfigKey::kResolutionIndex) == std::string::npos);
    CHECK(text.find(ConfigKey::kFullscreen) == std::string::npos);
    CHECK(text.find(ConfigKey::kCurrentWallpaper) == std::string::npos);
    CHECK(text.find(ConfigKey::kUiScale) == std::string::npos);
    // 而观感类的要在
    CHECK(text.find(ConfigKey::kTheme) != std::string::npos);
}

TEST_CASE("设置分享 - 粘贴别人的码不会改坏本机的窗口设置") {
    Cfg b("b");
    b.prefs->setInt(ConfigKey::kResolutionIndex, 1);
    // 先把"本机自己的"缩放设上，才验得出外来值**没有**覆盖它
    b.prefs->setDouble(ConfigKey::kUiScale, 1.0);
    b.prefs->setDouble(ConfigKey::kFontScale, 1.0);

    // 手工构造一段"恶意/外来"的文本：混着可携带与不可携带的键
    const std::string evil = "resolution_index=99\n"
                             "window_mode=2\n"
                             "fullscreen=true\n"
                             "vsync=false\n"
                             "ui_scale=3.0\n"
                             "font_scale=3.0\n"
                             "current_wallpaper=不存在的图.jpg\n"
                             "theme=1\n" // 只有这一条应当生效
                             "乱写的一行没有等号\n"
                             "=没有键名\n"
                             "unknown_key=1\n";

    const int applied = b.prefs->applyPortable(evil);

    CHECK(applied == 1); // 只有 theme
    CHECK(b.prefs->getInt(ConfigKey::kResolutionIndex, -1) == 1);
    CHECK(b.prefs->getBool(ConfigKey::kFullscreen, false) == false);
    CHECK(b.prefs->getBool(ConfigKey::kVsync, true) == true);
    CHECK(b.prefs->getDouble(ConfigKey::kUiScale, -1.0) == doctest::Approx(1.0));
    CHECK(b.prefs->getDouble(ConfigKey::kFontScale, -1.0) == doctest::Approx(1.0));
    CHECK(b.prefs->get(ConfigKey::kCurrentWallpaper, "?") == "?");
    CHECK(b.prefs->get(ConfigKey::kTheme, "?") == "1");
}

TEST_CASE("设置分享 - 分享码损坏 / 不是本游戏的码时安全失败") {
    Cfg b("b");
    b.prefs->set(ConfigKey::kTheme, "2");

    // 关卡分享码（TG1:）不能被当成设置码
    CHECK(SettingsCodec::decode("TG1:AAAA").empty());
    // 乱码
    CHECK(SettingsCodec::decode("TS1:!!!not base64!!!").empty());
    // 空
    CHECK(SettingsCodec::decode("").empty());

    // 解不开时调用方什么都不做，现有设置原样保留
    const std::string decoded = SettingsCodec::decode("TS1:!!!not base64!!!");
    CHECK(decoded.empty());
    CHECK(b.prefs->applyPortable(decoded) == 0);
    CHECK(b.prefs->get(ConfigKey::kTheme, "?") == "2");
}

TEST_CASE("设置分享 - 值里带 = 或空格的项不会把后面的行吃掉") {
    Cfg a("a");
    Cfg b("b");

    // 玩家名是最可能含 '=' 与空格的字段
    a.prefs->set(ConfigKey::kPlayerName, "a = b c");
    a.prefs->set(ConfigKey::kTheme, "1");

    share(a, b);

    CHECK(b.prefs->get(ConfigKey::kPlayerName, "?") == "a = b c");
    CHECK(b.prefs->get(ConfigKey::kTheme, "?") == "1");
}
