#include "doctest.h"
#include "core/scene.h"
#include "core/scene_manager.h"

#include <memory>
#include <string>
#include <vector>

// SceneManager 的钩子契约。
//
// 之所以单独有一组测试，是因为 ConsoleScene 踩过坑：它把
// "恢复 cin/cout/cerr + 停掉 worker 线程" 的清理只写在了 onExit() 里，
// 而 onExit() 只在程序退出（~SceneManager）时才调 —— 离开控制台走的是
// pop()，那里只调 onPause()。结果就是：进过一次控制台之后，标准流一直被
// 劫持在控制台缓冲区上，终端里再也刷不出日志，而且因为 onEnter 有
// `if (!redirect_)` 判断，再进去看着还是好的，bug 完全静默。
//
// 场景替身不碰 Window（render 是空实现），所以这一组在无 GL 上下文的
// 环境里也能跑。

namespace {

std::vector<std::string> g_hooks;

class RecordingScene : public Scene {
public:
    explicit RecordingScene(std::string name) : name_(std::move(name)) {}

    void render(Window& /*window*/) override {}

    void onEnter()  override { g_hooks.push_back(name_ + ":enter"); }
    void onPause()  override { g_hooks.push_back(name_ + ":pause"); }
    void onResume() override { g_hooks.push_back(name_ + ":resume"); }
    void onExit()   override { g_hooks.push_back(name_ + ":exit"); }

private:
    std::string name_;
};

std::unique_ptr<Scene> makeRecording(SceneId id) {
    return std::make_unique<RecordingScene>(sceneIdName(id));
}

int countExits() {
    int n = 0;
    for (const auto& h : g_hooks)
        if (h.ends_with(":exit")) ++n;
    return n;
}

} // namespace

TEST_CASE("SceneManager - 离开场景走 onPause，不是 onExit") {
    g_hooks.clear();
    SceneManager mgr(makeRecording);

    REQUIRE(mgr.start(SceneId::MainMenu));
    REQUIRE(mgr.push(SceneId::Console));
    g_hooks.clear();

    REQUIRE(mgr.pop());

    // ⭐ 核心断言：pop 时被离开的场景只等得到 onPause。
    //    清理逻辑挂错钩子的 bug 就是被这一行拦下来的。
    CHECK(g_hooks == std::vector<std::string>{"Console:pause", "MainMenu:resume"});
    CHECK(countExits() == 0);
}

TEST_CASE("SceneManager - push / replace 也会给被离开的场景 onPause") {
    g_hooks.clear();
    SceneManager mgr(makeRecording);

    REQUIRE(mgr.start(SceneId::MainMenu));
    g_hooks.clear();

    REQUIRE(mgr.push(SceneId::Settings));
    CHECK(g_hooks == std::vector<std::string>{"MainMenu:pause", "Settings:enter"});

    g_hooks.clear();
    REQUIRE(mgr.replace(SceneId::Game));
    CHECK(g_hooks == std::vector<std::string>{"Settings:pause", "Game:enter"});

    CHECK(countExits() == 0);
}

TEST_CASE("SceneManager - onExit 只在管理器析构时对每个常驻场景调一次") {
    g_hooks.clear();
    {
        SceneManager mgr(makeRecording);
        REQUIRE(mgr.start(SceneId::MainMenu));
        REQUIRE(mgr.push(SceneId::Settings));
        REQUIRE(mgr.replace(SceneId::Game));

        // 一路 push / replace 下来，一个 onExit 都不该有
        CHECK(countExits() == 0);
    }

    // 析构时缓存里的三个场景各一次
    CHECK(countExits() == 3);
}

TEST_CASE("SceneManager - 同一个 id 只构造一次（场景常驻）") {
    g_hooks.clear();
    int created = 0;
    SceneManager mgr([&created](SceneId id) {
        ++created;
        return std::make_unique<RecordingScene>(sceneIdName(id));
    });

    REQUIRE(mgr.start(SceneId::Game));
    REQUIRE(mgr.push(SceneId::Settings));
    REQUIRE(mgr.pop());
    REQUIRE(mgr.push(SceneId::Settings));   // 再进同一个场景

    CHECK(created == 2);   // Game 一次 + Settings 一次，没有第三个
    CHECK(mgr.currentId() == SceneId::Settings);
}

TEST_CASE("SceneManager - 空历史 pop 返回 false，未 start 时 current 是空的") {
    g_hooks.clear();
    SceneManager mgr(makeRecording);

    CHECK(mgr.empty());
    CHECK_FALSE(mgr.pop());

    REQUIRE(mgr.start(SceneId::MainMenu));
    CHECK_FALSE(mgr.empty());
    CHECK_FALSE(mgr.pop());   // 历史为空
    CHECK(mgr.currentId() == SceneId::MainMenu);
}

TEST_CASE("SceneManager - 工厂返回空指针时 start 失败且不切换") {
    g_hooks.clear();
    SceneManager mgr([](SceneId) { return std::unique_ptr<Scene>{}; });

    CHECK_FALSE(mgr.start(SceneId::MainMenu));
    CHECK(mgr.empty());
}
