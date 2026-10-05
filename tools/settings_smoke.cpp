// 设置页冒烟测试（**需要 DISPLAY，手动跑**）。
//
//   ./build/release/settings_smoke
//
// 为什么不放进 tests/：每页都要真的渲染一次才能知道按钮在哪（点击是按坐标判定
// 的），而渲染要 sf::Font / sf::RenderTexture —— 都是 GlResource，
// 没有 DISPLAY 时构造就 SIGABRT。tests/ 有"无 DISPLAY 也必须全绿"的硬要求。
//
// 它守的是这次重做最要紧的那条不变量：
//
//   **点某一页的控件，只能改这一页的键。**
//
// 以前各页的控件靠 `toggles_[7]` / `multiRows_[12]` 这种数字下标引用，
// 搬一行就要重排全部下标，排错了不报错 —— 只是"点了这个改了那个"。
// 0.3.8 把下标全换成具名指针，同时用这个工具把结果钉住：
// 逐页、逐个可聚焦控件点过去，比对点击前后配置键的差集。
#include "button.h"
#include "config/keys.h"
#include "config/preferences.h"
#include "config/runtime_config.h"
#include "core/paths.h"
#include "core/resource_manager.h"
#include "log/logger.h"
#include "scene/settings_tab_id.h"
#include "scene/tabs/advanced_tab.h"
#include "scene/tabs/audio_tab.h"
#include "scene/tabs/console_tab.h"
#include "scene/tabs/controls_tab.h"
#include "scene/tabs/display_tab.h"
#include "scene/tabs/game_tab.h"
#include "scene/tabs/graphics_tab.h"
#include "scene/tabs/interface_tab.h"
#include "scene/tabs/wallpaper_tab.h"
#include "ui/background.h"
#include "ui/window.h"
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok)
        ++failures;
}

/// 一页的统一门面：9 个 Tab 类型各不相同，但测试只关心这四个动作
struct Page {
    std::function<float(sf::RenderTarget&, float, float, float)> render;
    std::function<void(const sf::Event&)> handleEvent;
    std::function<void()> update;
    std::function<void(std::vector<Button*>&)> registerFocus;
};

/// 造出某一页（每次调用新建一份，各页之间互不影响）
Page makePage(SettingsTab tab, const sf::Font& font, std::shared_ptr<Preferences> prefs,
              std::shared_ptr<RuntimeConfig> runtime, std::shared_ptr<Window> window,
              std::shared_ptr<Background> background,
              std::shared_ptr<WallpaperLibrary> lib,
              std::shared_ptr<WallpaperLoader> loader, std::shared_ptr<Logger> logger,
              std::shared_ptr<Paths> paths);

/// 把按钮的点击模拟成"在按钮中心按下 + 松开"，然后跑一次本页 update()
void clickButton(Button& b, Page& page) {
    const auto pos = b.position();
    const auto size = b.size();
    const sf::Vector2i at{static_cast<int>(pos.x + size.x * 0.5f),
                          static_cast<int>(pos.y + size.y * 0.5f)};

    sf::Event press(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, at});
    sf::Event release(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, at});
    page.handleEvent(press);
    page.handleEvent(release);
    page.update();
}

std::set<std::string> keySet(const Preferences& p) {
    const auto v = p.keys();
    return {v.begin(), v.end()};
}

struct Sandbox {
    std::filesystem::path root;
    std::unique_ptr<Paths> paths;
    std::unique_ptr<ResourceManager> resources;
    std::shared_ptr<Preferences> prefs;
    std::shared_ptr<RuntimeConfig> runtime;

    explicit Sandbox(const std::string& tag) {
        root = std::filesystem::temp_directory_path() / ("textgame_sett_smoke_" + tag);
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
        paths = std::make_unique<Paths>(root);
        resources = std::make_unique<ResourceManager>(*paths);
        prefs = std::make_shared<Preferences>(*paths, *resources);
        runtime = std::make_shared<RuntimeConfig>(*paths, *resources);
    }
    ~Sandbox() {
        // ⚠️ 顺序要紧：Config 析构会 flush 一次，得赶在删目录之前放掉它，
        //    否则收尾时会刷一堆"无法写入"
        prefs.reset();
        runtime.reset();
        paths.reset();
        resources.reset();
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }

    /// AdvancedTab 要 shared_ptr<Paths>（打开配置/日志目录用）；不转移所有权
    std::shared_ptr<Paths> paths_shared() {
        return std::shared_ptr<Paths>(paths.get(), [](Paths*) {});
    }
};

Page makePage(SettingsTab tab, const sf::Font& font, std::shared_ptr<Preferences> prefs,
              std::shared_ptr<RuntimeConfig> runtime, std::shared_ptr<Window> window,
              std::shared_ptr<Background> background,
              std::shared_ptr<WallpaperLibrary> lib,
              std::shared_ptr<WallpaperLoader> loader, std::shared_ptr<Logger> logger,
              std::shared_ptr<Paths> paths) {
    Page page;
    switch (tab) {
    case SettingsTab::Display: {
        auto t = std::make_shared<DisplayTab>(font, prefs, runtime, window);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Interface: {
        auto t = std::make_shared<InterfaceTab>(font, prefs, window);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Wallpaper: {
        auto t = std::make_shared<WallpaperTab>(font, prefs, background, lib, loader);
        page.render = [t](sf::RenderTarget& tg, float cx, float, float y) {
            return t->render(tg, cx, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Graphics: {
        auto t = std::make_shared<GraphicsTab>(font, prefs, window);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Audio: {
        auto t = std::make_shared<AudioTab>(font, prefs, window);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Controls: {
        auto t = std::make_shared<ControlsTab>(font, prefs);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Game: {
        auto t = std::make_shared<GameTab>(font, prefs);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Console: {
        auto t = std::make_shared<ConsoleTab>(font, prefs);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    case SettingsTab::Advanced: {
        auto t = std::make_shared<AdvancedTab>(font, prefs, logger, paths);
        page.render = [t](sf::RenderTarget& tg, float cx, float ctrl, float y) {
            return t->render(tg, cx, ctrl, y);
        };
        page.handleEvent = [t](const sf::Event& e) { t->handleEvent(e); };
        page.update = [t] { t->update(); };
        page.registerFocus = [t](std::vector<Button*>& out) { t->registerFocus(out); };
        break;
    }
    default:
        break;
    }
    return page;
}

bool belongsTo(SettingsTab tab, const std::string& key) {
    const auto& keys = keysForTab(tab);
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}

} // namespace

int main() {
    // 提供一个 GL 上下文（Font / RenderTexture 都要）
    sf::RenderWindow window(sf::VideoMode({640, 400}), "settings_smoke");
    window.setVisible(false);

    sf::Font font;
    if (!font.openFromFile("assets/font.ttf")) {
        std::cout << "加载字体失败，跳过\n";
        return 2;
    }

    auto logger = std::make_shared<Logger>(LogLevel::Error);
    Sandbox box("main");

    // 各页构造要 Window（DisplayTab 改分辨率、AudioTab 读音量都要它）
    auto win = std::make_shared<Window>(640, 400, "settings_smoke_inner");

    auto lib = std::make_shared<WallpaperLibrary>();
    lib->scan("wallpaper");
    auto loader = std::make_shared<WallpaperLoader>();
    auto background = std::make_shared<Background>(*lib, *loader, "", 640, 400, logger);

    // 每一页：都点一遍，看它改了哪些键
    for (int i = 0; i < kSettingsTabCount; ++i) {
        const auto tab = static_cast<SettingsTab>(i);
        const std::string name = settingsTabLabel(tab);

        std::cout << "\n--- " << name << " ---\n";

        Page page = makePage(tab, font, box.prefs, box.runtime, win, background, lib,
                             loader, logger, box.paths_shared());
        if (!page.render) {
            check(false, "这一页没造出来");
            continue;
        }

        std::vector<Button*> items;
        page.registerFocus(items);

        // 先渲染一次：按钮位置是渲染时落定的，而点击按坐标判定
        sf::RenderTexture rt(sf::Vector2u{1280, 720});
        rt.clear(sf::Color::Black);
        page.render(rt, 240.f, 480.f, 50.f);
        rt.display();
        // ⚠️ registerFocus 是**追加**语义（各页都是 out.push_back）。
        //    第二次必须传一个空 vector —— 传同一个会让列表翻倍，
        //    而"跳过最后 4 个动作按钮"就会跟着错位，
        //    结果是**真的去打开文件管理器**（这个坑踩过一次）。
        std::vector<Button*> laidOut;
        page.registerFocus(laidOut);
        items = laidOut;

        check(!items.empty(),
              name + " 有可聚焦控件（" + std::to_string(items.size()) + " 个）");

        // 「高级」页最后 4 个是动作按钮（打开目录 / 导出 / 导入），点了有副作用
        // （真的去启动文件管理器、写剪贴板），这里跳过它们。
        // 它们不写配置，归属检查对它们本来也没有意义。
        std::size_t clickable = items.size();
        if (tab == SettingsTab::Advanced && clickable >= 4)
            clickable -= 4;

        std::set<std::string> violations;
        for (std::size_t k = 0; k < clickable; ++k) {
            const auto before = keySet(*box.prefs);
            clickButton(*items[k], page);
            const auto after = keySet(*box.prefs);

            for (const auto& key : after) {
                if (before.count(key))
                    continue; // 已存在的键（值可能变了，但归属不变）
                if (!belongsTo(tab, key))
                    violations.insert(key);
            }
        }

        if (violations.empty()) {
            check(true, name + " 的控件只改了本页的键");
        } else {
            std::string list;
            for (const auto& k : violations)
                list += " " + k;
            check(false, name + " 的控件改了不属于本页的键：" + list);
        }
    }

    std::cout << (failures == 0 ? "\n全部通过\n"
                                : "\n失败 " + std::to_string(failures) + " 条\n");
    return failures == 0 ? 0 : 1;
}
