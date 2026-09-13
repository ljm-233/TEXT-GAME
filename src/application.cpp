#include "application.h"
#include "logging.h"
#include "paths.h"
#include "bootstrap_config.h"
#include "runtime_config.h"
#include "preferences.h"
#include "resolution.h"
#include "calculator.h"
#include "window.h"
#include "background.h"
#include "font_holder.h"
#include "save_manager.h"
#include "game.h"

#include <iostream>

using namespace std;

Application& Application::instance() {
    static Application inst;
    return inst;
}

Application::Application() {
    registerDependencies();
}

void Application::registerDependencies() {
    // 1. Paths
    container_.registerType<Paths>([]() {
        return make_shared<Paths>();
    });

    // 2. 三种配置
    container_.registerType<BootstrapConfig>([this]() {
        auto paths = container_.resolve<Paths>();
        return make_shared<BootstrapConfig>(*paths);
    });
    container_.registerType<RuntimeConfig>([this]() {
        auto paths = container_.resolve<Paths>();
        return make_shared<RuntimeConfig>(*paths);
    });
    container_.registerType<Preferences>([this]() {
        auto paths = container_.resolve<Paths>();
        return make_shared<Preferences>(*paths);
    });

    // 3. Logger 从 BootstrapConfig 拿路径
    container_.registerType<Logger>([this]() {
        auto cfg = container_.resolve<BootstrapConfig>();
        auto logPath = cfg->configFile("app.log");
        return make_shared<Logger>(logPath.string());
    });

    // 4. Calculator
    container_.registerType<Calculator>([this]() {
        auto logger = container_.resolve<Logger>();
        return make_shared<Calculator>(logger);
    });

    // 5. Window 从 Preferences 读分辨率
    container_.registerType<Window>([this]() {
        auto prefs = container_.resolve<Preferences>();
        int idx = clampResolutionIndex(prefs->getInt("resolution_index", 0));
        bool fs = prefs->getBool("fullscreen", false);
        auto& res = kResolutions[idx];
        return std::make_shared<Window>(res.width, res.height, "TEXT-GAME", fs);
    });

    // 6. Background
    container_.registerType<Background>([this]() {
        auto paths  = container_.resolve<Paths>();
        auto window = container_.resolve<Window>();
        auto logger = container_.resolve<Logger>();
        auto size   = window->native().getSize();
        return std::make_shared<Background>(
            paths->wallpaperDir(), size.x, size.y, logger);
    });

    // 7. FontHolder 从 BootstrapConfig 拿路径
    container_.registerType<FontHolder>([this]() {
        auto cfg    = container_.resolve<BootstrapConfig>();
        auto logger = container_.resolve<Logger>();
        auto fontPath = cfg->assetFile("font.otf");
        return std::make_shared<FontHolder>(fontPath, logger);
    });

    // 8. SaveManager 从 RuntimeConfig 拿路径
    container_.registerType<SaveManager>([this]() {
        auto cfg    = container_.resolve<RuntimeConfig>();
        auto logger = container_.resolve<Logger>();
        return std::make_shared<SaveManager>(cfg, logger);
    });

    // 9. Game
    container_.registerType<Game>([this]() {
        auto window      = container_.resolve<Window>();
        auto logger      = container_.resolve<Logger>();
        auto background  = container_.resolve<Background>();
        auto fontHolder  = container_.resolve<FontHolder>();
        auto saveManager = container_.resolve<SaveManager>();
        auto prefs       = container_.resolve<Preferences>();
        return std::make_shared<Game>(
            window, logger, background, fontHolder, saveManager, prefs);
    });
}

int Application::showMenu() {
    cout << "\n===== TEXT-GAME =====\n";
    cout << "  1. 计算器\n";
    cout << "  2. 游戏窗口\n";
    cout << "  0. 退出\n";
    cout << "请选择: ";

    int choice = 0;
    cin >> choice;
    if (cin.fail()) {
        cin.clear();
        cin.ignore(1000, '\n');
        return -1;
    }
    return choice;
}

void Application::run() {
    auto logger = container_.resolve<Logger>();
    auto paths  = container_.resolve<Paths>();

    logger->info("程序启动");
    logger->info("配置目录: " + paths->configDir().string());
    logger->info("存档目录: " + paths->savesDir().string());
    logger->info("资源目录: " + paths->assetsDir().string());

    int choice = showMenu();

    switch (choice) {
        case 1: {
            auto calc = container_.resolve<Calculator>();
            calc->run();
            break;
        }
        case 2: {
            auto game = container_.resolve<Game>();
            game->run();
            break;
        }
        case 0:
            logger->info("用户选择退出");
            break;
        default:
            logger->warn("无效选择");
            break;
    }

    logger->info("程序结束");
}