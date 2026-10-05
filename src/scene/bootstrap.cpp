#include "scene/bootstrap.h"

#include "achievement_scene.h"
#include "console_scene.h"
#include "editor_scene.h"
#include "game_scene.h"
#include "level_select_scene.h"
#include "main_menu_scene.h"
#include "save_select_scene.h"
#include "settings_scene.h"
#include "stats_scene.h"

#include "core/scene_registry.h"
#include "log/logger.h"

#include "config/preferences.h"
#include "config/runtime_config.h"
#include "game/save_manager.h"
#include "ui/background.h"
#include "ui/font_holder.h"
#include "ui/window.h"
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"

#include <memory>

namespace {

/// 场景依赖的统一入口。
///
/// 存在的意义是把容器键名收在一处：8 个场景各写一遍字符串字面量，
/// 打错一个字就是运行期 ContainerError —— 比配置键好一点（会炸），
/// 但仍然是本可以在编译期发现的问题。
///
/// 所有访问器都是**调用时才解析**：工厂被调用（首次进入该场景）之前，
/// 不会碰容器里的任何单例。
struct Deps {
    Container& container;

    std::shared_ptr<Background> background() {
        return container.require<Background>("background");
    }
    std::shared_ptr<Logger> logger() { return container.require<Logger>("logger"); }
    std::shared_ptr<SaveManager> saveManager() {
        return container.require<SaveManager>("save_manager");
    }
    std::shared_ptr<Preferences> preferences() {
        return container.require<Preferences>("preferences");
    }
    std::shared_ptr<RuntimeConfig> runtimeConfig() {
        return container.require<RuntimeConfig>("runtime_config");
    }
    std::shared_ptr<Window> window() { return container.require<Window>("window"); }
    std::shared_ptr<ResourceManager> resources() {
        return container.require<ResourceManager>("resources");
    }
    std::shared_ptr<Paths> paths() { return container.require<Paths>("paths"); }
    // 壁纸页要：library 提供候选列表，loader 提供缩略图
    std::shared_ptr<WallpaperLibrary> wallpaperLibrary() {
        return container.require<WallpaperLibrary>("wallpaper_library");
    }
    std::shared_ptr<WallpaperLoader> wallpaperLoader() {
        return container.require<WallpaperLoader>("wallpaper_loader");
    }
    // 「试玩」的交接通道：编辑器写、关卡场景读
    std::shared_ptr<PlaytestRequest> playtest() {
        return container.require<PlaytestRequest>("playtest_request");
    }

    /// 字体由 FontHolder 这个容器单例持有，生命周期到进程结束，
    /// 所以这里返回引用是安全的（临时 shared_ptr 析构不影响对象本身）。
    const sf::Font& font() { return container.require<FontHolder>("font_holder")->get(); }
};

} // namespace

void registerScenes(Container& container) {
    auto registry = container.require<SceneRegistry>("scene_registry");

    // 「试玩」的交接通道。注册在这里而不是 registerCore：它是纯场景层的东西
    // （两个场景之间递一份草稿），core 层不该认识它。
    container.reg<PlaytestRequest>("playtest_request",
                                   []() { return std::make_shared<PlaytestRequest>(); });

    // ---------------- 主菜单 ----------------
    registry->add(SceneId::MainMenu, [&container]() {
        Deps deps{container};
        return std::make_unique<MainMenuScene>(deps.background(), deps.font(),
                                               deps.logger());
    });

    // ---------------- 存档选择 ----------------
    registry->add(SceneId::SaveSelect, [&container]() {
        Deps deps{container};
        return std::make_unique<SaveSelectScene>(deps.background(), deps.saveManager(),
                                                 deps.font(), deps.logger());
    });

    // ---------------- 关卡选择 ----------------
    registry->add(SceneId::LevelSelect, [&container]() {
        Deps deps{container};
        return std::make_unique<LevelSelectScene>(deps.background(), deps.saveManager(),
                                                  deps.font(), deps.logger());
    });

    // ---------------- 游戏本体 ----------------
    registry->add(SceneId::Game, [&container]() {
        Deps deps{container};
        return std::make_unique<GameScene>(deps.background(), deps.font(), deps.logger(),
                                           deps.saveManager(), deps.preferences(),
                                           deps.resources(), deps.playtest());
    });

    // ---------------- 设置 ----------------
    registry->add(SceneId::Settings, [&container]() {
        Deps deps{container};
        return std::make_unique<SettingsScene>(
            deps.background(), deps.preferences(), deps.runtimeConfig(), deps.window(),
            deps.wallpaperLibrary(), deps.wallpaperLoader(), deps.paths(), deps.font(),
            deps.logger());
    });

    // ---------------- 控制台 ----------------
    registry->add(SceneId::Console, [&container]() {
        Deps deps{container};
        return std::make_unique<ConsoleScene>(deps.background(), deps.preferences(),
                                              deps.saveManager(), deps.font(),
                                              deps.logger());
    });

    // ---------------- 关卡编辑器 ----------------
    registry->add(SceneId::Editor, [&container]() {
        Deps deps{container};
        return std::make_unique<EditorScene>(deps.background(), deps.preferences(),
                                             deps.font(), deps.logger(), deps.resources(),
                                             deps.paths(), deps.playtest());
    });

    // ---------------- 成就 ----------------
    registry->add(SceneId::Achievements, [&container]() {
        Deps deps{container};
        return std::make_unique<AchievementScene>(deps.background(), deps.font(),
                                                  deps.logger());
    });

    // ---------------- 成绩 / 统计 ----------------
    registry->add(SceneId::Stats, [&container]() {
        Deps deps{container};
        return std::make_unique<StatsScene>(deps.background(), deps.saveManager(),
                                            deps.font(), deps.logger());
    });
}
