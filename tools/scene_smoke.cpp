// 场景冒烟测试（**需要 DISPLAY，手动跑**）。
//
//   ./build/release/scene_smoke
//
// 为什么要有它：场景类**写不了单元测试** —— 构造要 `sf::Font` /
// `sf::RenderTexture`，而它们是 `sf::GlResource`，没有 GL 上下文时**构造就
// SIGABRT**（不是返回错误码，拦不住）。而且场景只在进入时才由注册表工厂创建，
// 所以"跑一下游戏"也抓不到构造期崩溃。
//
// 它盯的是这一类问题：
//   - 构造函数的初始化顺序 / 空指针 / 越界
//   - `onEnter()` 里的加载逻辑（尤其"数据为空"和"数据很多"两个极端）
//   - `render()` 的排版算术：除零、负数尺寸、栏数算成 0
//   - **试玩模式的标志泄漏** —— 见下面 GameScene 那段，那个 bug 的后果是
//     "一次试玩之后，正式关卡再也不写存档"，而且完全静默
#include "achievement_scene.h"
#include "background.h"
#include "config/keys.h"
#include "core/paths.h"
#include "core/resource_manager.h"
#include "core/scene_id.h"
#include "editor_scene.h"
#include "game_scene.h"
#include "infrastructure/keybindings.h"
#include "log/logger.h"
#include "main_menu_scene.h"
#include "playtest_request.h"
#include "preferences.h"
#include "runtime_config.h"
#include "save_manager.h"
#include "settings_scene.h"
#include "stats_scene.h"
#include "ui/window.h"
#include "utils/text_strings.h"
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"

#include <SFML/Graphics.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok)
        ++failures;
}

/// 打印**再做**：万一某一步崩了，输出里最后一行就指向崩溃点
void step(const std::string& what) {
    std::cout << "  ....  " << what << std::endl;
}

namespace fs = std::filesystem;

/// 读整个文件（用来比对"这个文件到底有没有被改过"）
std::string readWholeFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

/// 一个只属于自己的沙箱（不碰真实存档）
struct Env {
    fs::path root;
    std::shared_ptr<Paths> paths;       // 沙箱：存档/配置
    std::shared_ptr<Paths> assetPaths_; // 真实：资源别名
    std::shared_ptr<ResourceManager> resources;
    std::shared_ptr<Preferences> prefs;
    std::shared_ptr<RuntimeConfig> runtime;
    std::shared_ptr<Logger> logger;
    std::shared_ptr<SaveManager> saves;

    Env() {
        root = fs::temp_directory_path() / "textgame_scene_smoke";
        std::error_code ec;
        fs::remove_all(root, ec);
        // ⚠️ 两个 Paths 是有意的：
        //    - 沙箱那个给存档/配置用（不碰用户真实数据）
        //    - 资源管理器要**真实的**资源根，否则 `levels/level1.txt` 会被解析到
        //      沙箱里、加载失败 —— 而"加载失败"这条路正好会掩盖掉真正要测的东西
        paths = std::make_shared<Paths>(root);
        assetPaths_ = std::make_shared<Paths>();
        resources = std::make_shared<ResourceManager>(*assetPaths_);
        prefs = std::make_shared<Preferences>(*paths, *resources);
        runtime = std::make_shared<RuntimeConfig>(*paths, *resources);
        // Error 级别 + 不挂 handler = 静默，免得刷屏
        logger = std::make_shared<Logger>(LogLevel::Error);
        saves = std::make_shared<SaveManager>(paths, logger);
    }

    ~Env() {
        // 先放掉会写盘的 Config，再删目录（顺序反了会刷一堆"无法写入"）
        prefs.reset();
        runtime.reset();
        saves.reset();
        paths.reset();
        resources.reset();
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    void clearAllSaves() {
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(paths->savesDir(), ec))
            fs::remove(e.path(), ec);
    }

    SaveInfo newSave(const std::string& name) { return saves->createSave(name); }

    /// 造一个**只认沙箱**的资源根，并把给定关卡写成 `levels/editor.txt`。
    ///
    /// 为什么要另起一个：`EditorScene::onEnter()` 会读 `levels/editor.txt` 并扫整个
    /// levels 目录。用真实资源根的话，"F5 按不按得动"就取决于用户此刻编辑器里
    /// 存着什么（没存过出生点 `P` 就按不动），测试结果会随时间飘。
    std::shared_ptr<ResourceManager>
    sandboxResources(const std::vector<std::string>& levelLines) {
        const fs::path levels = root / "assets" / "levels";
        std::error_code ec;
        fs::create_directories(levels, ec);
        std::ofstream out(levels / "editor.txt");
        for (const auto& l : levelLines)
            out << l << '\n';
        out.close();
        return std::make_shared<ResourceManager>(*paths);
    }

    /// 手写一个 N 关的存档。
    /// 不能靠 `setLevelStar`：它按 `levelStars.size()` 校验关卡序号，而那永远是 9 ——
    /// 想造"关卡数远多于 9"的存档只能直接写文件（格式就是扁平的 key=value）。
    void writeWideSave(const std::string& filename, int levels) {
        std::error_code ec;
        fs::create_directories(paths->savesDir(), ec);
        std::ofstream out(paths->savesDir() / filename);
        std::string stars, times, coins;
        for (int i = 0; i < levels; ++i) {
            if (i)
                stars += ",", times += ",", coins += ",";
            stars += std::to_string(i % 4); // 0~3
            times += std::to_string(10 + i) + ".50";
            coins += std::to_string(i % 7);
        }
        out << "name=" << filename << "\n";
        out << "created_at=2026-01-01 00:00:00\n";
        out << "last_played=2026-01-01 00:00:00\n";
        out << "coins=123\n";
        out << "current_level=1\n";
        out << "level_stars=" << stars << "\n";
        out << "level_best_times=" << times << "\n";
        out << "level_best_coins=" << coins << "\n";
    }
};

} // namespace

int main() {
    // ⚠️ 必须是 DISPLAY（X11），WAYLAND_DISPLAY 不算数：这份 SFML 是 X11 后端，
    //    只有 WAYLAND_DISPLAY 而没有 DISPLAY 时，构造窗口照样 SIGABRT
    //    （实测过：`env -u DISPLAY` 时 exit=134，而不是这里的 exit=2）。
    if (std::getenv("DISPLAY") == nullptr) {
        std::cout << "需要 DISPLAY（X11）—— 场景构造要 GL 上下文。\n";
        return 2;
    }

    sf::RenderWindow stayAlive(sf::VideoMode({320, 240}), "scene_smoke_gl");
    stayAlive.setVisible(false);

    sf::Font font;
    if (!font.openFromFile("assets/font.ttf")) {
        std::cout << "加载字体失败，跳过\n";
        return 2;
    }

    Env env;
    auto win = std::make_shared<Window>(1280, 720, "scene_smoke", false, 0);
    auto lib = std::make_shared<WallpaperLibrary>();
    lib->scan("wallpaper");
    auto loader = std::make_shared<WallpaperLoader>();
    auto background =
        std::make_shared<Background>(*lib, *loader, "", 1280, 720, env.logger);

    // ============================================================
    // 成绩页：五种数据状态各构造 + 渲染一次
    // ============================================================
    std::cout << "\n--- StatsScene ---\n";

    struct StatsCase {
        const char* name;
        int saveCount;
        int levels; // 0 = 空存档（没有任何记录）
        bool clearFirst;
    };
    const StatsCase cases[] = {
        {"一个存档都没有", 0, 0, true},
        {"全新存档（没有任何记录）", 1, 0, true},
        {"有记录的存档（9 关）", 1, 9, true},
        {"关卡数远多于 9（40 关，走多栏排版）", 1, 40, true},
        {"多个存档（走上一个/下一个）", 3, 9, true},
    };

    for (const auto& c : cases) {
        if (c.clearFirst)
            env.clearAllSaves();
        for (int i = 0; i < c.saveCount; ++i) {
            auto s = env.newSave("冒烟" + std::to_string(i));
            if (c.levels > 0) {
                // 用 setter 造有记录的存档（上限 9 关）
                env.saves->setLevelStar(s.filename, 1, 3);
                env.saves->setLevelBestTime(s.filename, 1, 12.5f);
                env.saves->setLevelBestCoins(s.filename, 1, 7);
            }
        }
        if (c.levels > 9)
            env.writeWideSave("save_9999999999999999.conf", c.levels);

        step(std::string("构造 + onEnter + 渲染：") + c.name);
        StatsScene scene(background, env.saves, font, env.logger);
        scene.onEnter();
        scene.update(1.f / 60.f);
        scene.render(*win);
        // 再切一次存档 / 再渲染一帧，覆盖"切换后重排"这条路
        sf::Event key(sf::Event::KeyPressed{sf::Keyboard::Key::Right,
                                            sf::Keyboard::Scan::Right, false, false,
                                            false, false});
        scene.handleEvent(key);
        scene.update(1.f / 60.f);
        scene.render(*win);
        check(true, std::string("StatsScene 没崩：") + c.name);
    }

    // ============================================================
    // 其它场景：构造 + onEnter + 渲染
    // ============================================================
    std::cout << "\n--- 其它场景 ---\n";
    env.clearAllSaves();
    env.newSave("冒烟");

    {
        step("AchievementScene");
        AchievementScene scene(background, font, env.logger);
        scene.onEnter();
        scene.update(1.f / 60.f);
        scene.render(*win);
        check(true, "AchievementScene 没崩");
    }
    {
        step("MainMenuScene");
        MainMenuScene scene(background, font, env.logger);
        scene.onEnter();
        scene.update(1.f / 60.f);
        scene.render(*win);
        check(true, "MainMenuScene 没崩");
    }
    {
        step("SettingsScene");
        SettingsScene scene(background, env.prefs, env.runtime, win, lib, loader,
                            env.paths, font, env.logger);
        scene.onEnter();
        scene.update(1.f / 60.f);
        scene.render(*win);
        check(true, "SettingsScene 没崩");
    }

    // ============================================================
    // GameScene：「试玩」的场景入口 + 标志泄漏
    // ============================================================
    std::cout << "\n--- GameScene 试玩入口 ---\n";

    const std::string playtestBadge = Str::T(Str::PlaytestBadge);

    // 造一个最小合法关卡（必须有玩家出生点 P）
    std::vector<std::string> draft(22, std::string(40, ' '));
    for (int x = 0; x < 40; ++x)
        draft[0][x] = '#', draft[21][x] = '#';
    for (int y = 0; y < 22; ++y)
        draft[y][0] = '#', draft[y][39] = '#';
    draft[2][3] = 'P';

    // 先确认资源别名能解析到真实关卡文件 —— 加载失败的根因往往就在这一步
    {
        const auto lp = env.resources->get("levels", "level1.txt");
        std::cout << "  资源解析: " << lp << "  存在=" << fs::exists(lp) << std::endl;
        check(fs::exists(lp), "ResourceManager 能解析到 level1.txt");
    }

    {
        step("用例 A：正式关卡");
        auto pt = std::make_shared<PlaytestRequest>(); // 没有待处理请求
        env.saves->setPendingSave(env.newSave("正式"));
        GameScene scene(background, font, env.logger, env.saves, env.prefs, env.resources,
                        pt);
        scene.onEnter();
        scene.render(*win);
        check(scene.nextScene() == SceneId::None, "正式关卡 onEnter 成功（不是 Back）");
        check(scene.windowTitleHint().find(playtestBadge) == std::string::npos,
              "正式关卡标题里不该有试玩标记");
    }

    {
        step("用例 B：试玩草稿");
        auto pt = std::make_shared<PlaytestRequest>();
        pt->request(draft, "editor.txt");
        env.saves->setPendingSave(env.newSave("试玩"));
        GameScene scene(background, font, env.logger, env.saves, env.prefs, env.resources,
                        pt);
        scene.onEnter();
        scene.render(*win);
        // 解析失败时 onEnter 会把 nextScene_ 设成 Back
        check(scene.nextScene() == SceneId::None, "试玩 onEnter 成功（草稿能解析）");
        check(scene.windowTitleHint().find(playtestBadge) != std::string::npos,
              "试玩时标题带试玩标记");

        // ⚠️ 最关键的一条：同一个实例再进一次**正式**关卡，标志必须清掉。
        //    清不掉的话，正式关卡也会走"不写存档"那条路 —— 后果是
        //    "试玩过一次之后，这局游戏的存档/星级/成就再也不更新了"，完全静默。
        step("用例 B2：试玩之后紧接着进正式关卡");
        env.saves->setPendingSave(env.newSave("正式2"));
        scene.onEnter();
        scene.render(*win);
        check(scene.nextScene() == SceneId::None, "试玩之后再进正式关卡也成功");
        check(scene.windowTitleHint().find(playtestBadge) == std::string::npos,
              "试玩标记必须被清掉（否则正式关卡不写存档）");
    }

    {
        step("用例 C：坏草稿（空内容）");
        auto pt = std::make_shared<PlaytestRequest>();
        pt->request({}, "empty.txt");
        env.saves->setPendingSave(env.newSave("坏草稿"));
        GameScene scene(background, font, env.logger, env.saves, env.prefs, env.resources,
                        pt);
        scene.onEnter();
        scene.render(*win);
        check(scene.nextScene() == SceneId::Back, "坏草稿应当退回编辑器（Back）");
    }

    {
        // ⭐ 这一条是**这个工具真正的战果**：它第一次跑起来时就崩在这里 ——
        //    `onEnter` 加载失败后 `world_` 是空的，而 `render` 里有十来处
        //    无保护的 `world_->`，于是**每帧 SIGSEGV**，用户永远看不到那句
        //    "关卡加载失败"的通知。
        //    真实可达路径：存档里的 currentLevel 超出现有关卡文件数
        //    （比如关卡文件被删过、或存档来自关卡更多的版本）、关卡文件损坏。
        step("用例 D：关卡加载失败（存档指向不存在的关卡）");
        auto pt = std::make_shared<PlaytestRequest>();
        auto info = env.newSave("坏关卡");
        info.currentLevel = 999; // 远超出实际关卡数
        env.saves->setPendingSave(info);
        GameScene scene(background, font, env.logger, env.saves, env.prefs, env.resources,
                        pt);
        scene.onEnter();
        check(scene.nextScene() == SceneId::Back,
              "加载失败要退回上一页，而不是留在空场景");
        // 关键：加载失败后 update / render 都不能崩
        scene.update(1.f / 60.f);
        scene.render(*win);
        check(true, "加载失败后 update + render 不崩");
    }

    // ============================================================
    // 编辑器：「F5 试玩 → 退出游戏」必须真的回到编辑器
    // ============================================================
    std::cout << "\n--- 编辑器试玩往返 ---\n";
    {
        step("用例 E：F5 之后退出，编辑器不能自己把游戏再推一次");
        auto pt = std::make_shared<PlaytestRequest>();
        auto sandboxRes = env.sandboxResources(draft);
        EditorScene editor(background, env.prefs, font, env.logger, sandboxRes, env.paths,
                           pt);
        editor.onEnter();

        sf::Event f5(sf::Event::KeyPressed{sf::Keyboard::Key::F5, sf::Keyboard::Scan::F5,
                                           false, false, false, false});
        editor.handleEvent(f5);
        check(editor.nextScene() == SceneId::Game, "F5 应当请求进入游戏");
        check(pt->has(), "F5 应当把草稿放进交接通道");

        // 玩家在游戏里点「保存并退出游戏」→ Game::switchScene(Back) → 场景栈 pop
        // → 对露出来的场景调 onResume()（钩子契约见 tests/test_scene_manager.cpp）
        editor.onResume();

        // Game::run 每帧都会问 `current->nextScene()`。这里要是还留着 Game，
        // 下一帧就会立刻再 push 一次 GameScene —— 用户看到的是"点退出 = 重开一局"。
        check(editor.nextScene() == SceneId::None,
              "试玩退出后编辑器不能再请求 Game（否则会当场重开一局）");
    }

    {
        step("用例 F：试玩里点「保存并退出游戏」不能写真实存档");
        env.clearAllSaves();
        // 开场动画会让第一个按键被吃掉（handleEvent 里 introActive_ 优先），
        // 这里关掉它，按键才落到暂停菜单上
        env.prefs->setBool(ConfigKey::kLevelIntro, false);

        auto pt = std::make_shared<PlaytestRequest>();
        GameScene scene(background, font, env.logger, env.saves, env.prefs, env.resources,
                        pt);

        // 先用**正式**关卡进一次，save_ 才指向一个真实存档文件。
        // 少了这一步 save_.filename 是空的，updateProgress 本来就不写 ——
        // 测试会"通过"，但什么也没测到。
        SaveInfo info = env.newSave("试玩退出");
        env.saves->setPendingSave(info);
        scene.onEnter();
        scene.render(*win); // 走一次 render：暂停菜单按窗口尺寸排版

        const fs::path saveFile = env.paths->savesDir() / info.filename;
        const std::string before = readWholeFile(saveFile);
        check(!before.empty(), "存档文件非空（否则下面的比对没有意义）");

        // 再进试玩（同一个场景实例，save_ 仍是上面那个存档）
        pt->request(draft, "editor.txt");
        scene.onEnter();
        scene.render(*win);
        check(scene.windowTitleHint().find(playtestBadge) != std::string::npos,
              "第二次进入确实是试玩");

        // ESC 打开暂停菜单
        sf::Event esc(sf::Event::KeyPressed{
            KeyBindings::instance().get(KeyBindings::Pause), sf::Keyboard::Scan::Escape,
            false, false, false, false});
        scene.handleEvent(esc);
        scene.render(*win); // 再排一次版：按钮坐标依赖窗口尺寸

        // 点第三个按钮。几何来自 PauseMenu：面板 420x340 居中，按钮 280x52，
        // 三个按钮的 y 依次是 py+110 / +180 / +250。
        // ⚠️ 窗口尺寸必须**实测**：这台机器上这个窗口是 1270x1528，写死 1280x720
        //    就会点空 —— 而"点空"会让下面那条存档比对比出**假通过**。
        const auto ws = win->native().getSize();
        const float winW = static_cast<float>(ws.x);
        const float winH = static_cast<float>(ws.y);
        const float px = (winW - 420.f) / 2.f;
        const float py = (winH - 340.f) / 2.f;
        const sf::Vector2i at{static_cast<int>(px + 70.f + 140.f),
                              static_cast<int>(py + 250.f + 26.f)};
        scene.handleEvent(
            sf::Event(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, at}));
        scene.handleEvent(
            sf::Event(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, at}));
        scene.update(1.f / 60.f);

        // 先证明这一下**真的点中了按钮**：点歪了的话 nextScene 还是 None，
        // 下面那条"存档没变"就会变成一条永远通过的假测试
        check(scene.nextScene() == SceneId::Back,
              "点「保存并退出游戏」应当请求退出（这条红了说明点歪了，下一条不算数）");
        check(readWholeFile(saveFile) == before, "试玩退出不能改真实存档");
    }

    std::cout << (failures == 0 ? "\n全部通过\n"
                                : "\n失败 " + std::to_string(failures) + " 条\n");
    return failures == 0 ? 0 : 1;
}
