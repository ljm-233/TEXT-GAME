#include "doctest.h"
#include "common/exceptions.h"
#include "core/scene.h"
#include "core/scene_registry.h"

#include <memory>
#include <string>

namespace {

/// 最小场景替身：只为实现 Scene 的纯虚 render。
/// 注册表只关心"能不能造出对象"，不关心场景内容。
class FakeScene : public Scene {
public:
    void render(Window& /*window*/) override {}
};

/// 记录构造次数的替身，用来验证工厂的调用时机。
int g_created = 0;

std::unique_ptr<Scene> makeCounted() {
    ++g_created;
    return std::make_unique<FakeScene>();
}

} // namespace

TEST_CASE("SceneRegistry - 注册后可按 id 构造") {
    g_created = 0;
    SceneRegistry registry;
    registry.add(SceneId::MainMenu, makeCounted);

    CHECK(registry.contains(SceneId::MainMenu));
    CHECK_FALSE(registry.contains(SceneId::Game));
    CHECK(registry.size() == 1);

    auto scene = registry.create(SceneId::MainMenu);
    CHECK(scene != nullptr);
    CHECK(g_created == 1);
}

TEST_CASE("SceneRegistry - 每次 create 都调用工厂（缓存由 SceneManager 负责）") {
    g_created = 0;
    SceneRegistry registry;
    registry.add(SceneId::Game, makeCounted);

    auto first = registry.create(SceneId::Game);
    auto second = registry.create(SceneId::Game);

    CHECK(g_created == 2);
    CHECK(first.get() != second.get());
}

TEST_CASE("SceneRegistry - 重复注册抛 AppError") {
    SceneRegistry registry;
    registry.add(SceneId::Settings, makeCounted);

    CHECK_THROWS_AS(registry.add(SceneId::Settings, makeCounted), AppError);
}

TEST_CASE("SceneRegistry - 空工厂抛 AppError") {
    SceneRegistry registry;

    CHECK_THROWS_AS(registry.add(SceneId::Editor, SceneRegistry::Factory{}),
                    AppError);
}

TEST_CASE("SceneRegistry - 未注册的 id 构造时抛错，而不是返回空") {
    SceneRegistry registry;
    registry.add(SceneId::MainMenu, makeCounted);

    // 这条是刻意选的语义：静默返回 nullptr 会表现成一块黑屏，
    // 报错才能定位到"哪个场景忘了登记"。
    CHECK_THROWS_AS(registry.create(SceneId::Console), AppError);

    try {
        registry.create(SceneId::Console);
        FAIL("应当抛异常");
    } catch (const AppError& e) {
        // 错误消息里必须带场景名，否则还得回来对着枚举数数字
        CHECK(std::string(e.what()).find("Console") != std::string::npos);
    }
}

TEST_CASE("SceneRegistry - ids 按枚举顺序而非注册顺序") {
    SceneRegistry registry;
    // 故意乱序注册
    registry.add(SceneId::Settings, makeCounted);
    registry.add(SceneId::MainMenu, makeCounted);
    registry.add(SceneId::Game, makeCounted);

    const auto ids = registry.ids();

    REQUIRE(ids.size() == 3);
    // 枚举里 MainMenu(3) < Game(5) < Settings(6)
    CHECK(ids[0] == SceneId::MainMenu);
    CHECK(ids[1] == SceneId::Game);
    CHECK(ids[2] == SceneId::Settings);
}

TEST_CASE("SceneRegistry - str 是给人看的调试快照") {
    SceneRegistry registry;
    registry.add(SceneId::MainMenu, makeCounted);
    registry.add(SceneId::Editor, makeCounted);

    const std::string dump = registry.str();

    CHECK(dump.find("2 registered") != std::string::npos);
    CHECK(dump.find("MainMenu") != std::string::npos);
    CHECK(dump.find("Editor") != std::string::npos);
}

TEST_CASE("SceneRegistry - 空注册表的 str 不炸") {
    SceneRegistry registry;

    CHECK(registry.size() == 0);
    CHECK(registry.ids().empty());
    CHECK(registry.str().find("0 registered") != std::string::npos);
}

TEST_CASE("sceneIdName - 每个枚举值都有名字") {
    // 控制值也要有名字：它们在错误消息与日志里都会出现
    CHECK(std::string(sceneIdName(SceneId::None)) == "None");
    CHECK(std::string(sceneIdName(SceneId::Back)) == "Back");
    CHECK(std::string(sceneIdName(SceneId::Exit)) == "Exit");
    CHECK(std::string(sceneIdName(SceneId::MainMenu)) == "MainMenu");
    CHECK(std::string(sceneIdName(SceneId::SaveSelect)) == "SaveSelect");
    CHECK(std::string(sceneIdName(SceneId::Game)) == "Game");
    CHECK(std::string(sceneIdName(SceneId::Settings)) == "Settings");
    CHECK(std::string(sceneIdName(SceneId::Console)) == "Console");
    CHECK(std::string(sceneIdName(SceneId::LevelSelect)) == "LevelSelect");
    CHECK(std::string(sceneIdName(SceneId::Editor)) == "Editor");
    CHECK(std::string(sceneIdName(SceneId::Achievements)) == "Achievements");
}
