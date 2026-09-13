#include "application.h"
#include "logging.h"
#include "paths.h"
#include "config.h"
#include "calculator.h"
#include "window.h"
#include "background.h"
#include "font_holder.h"
#include "save_manager.h"
#include "game.h"

#include <iostream>

using namespace std;

// ---------- Application 单例实现 ----------

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

    // 2. Config
    container_.registerType<Config>([this]() {
        auto paths = container_.resolve<Paths>();
        return make_shared<Config>(*paths);
    });

    // 3. Logger
    container_.registerType<Logger>([this]() {
        auto config = container_.resolve<Config>();
        auto logPath = config->configFile("app.log");
        return make_shared<Logger>(logPath.string());
    });

    // 4. Calculator
    container_.registerType<Calculator>([this]() {
        auto logger = container_.resolve<Logger>();
        return make_shared<Calculator>(logger);
    });

    // 5. Window
    container_.registerType<Window>([this]() {
        auto config = container_.resolve<Config>();
        int w = config->getInt("window_width", 1280);
        int h = config->getInt("window_height", 720);
        std::string title = config->get("window_title", "TEXT-GAME");
        return std::make_shared<Window>(w, h, title);
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

    // 7. FontHolder
    container_.registerType<FontHolder>([this]() {
        auto config = container_.resolve<Config>();
        auto logger = container_.resolve<Logger>();
        auto fontPath = config->assetFile("font.otf");
        return std::make_shared<FontHolder>(fontPath, logger);
    });

    // 8. SaveManager
    container_.registerType<SaveManager>([this]() {
        auto config = container_.resolve<Config>();
        auto logger = container_.resolve<Logger>();
        return std::make_shared<SaveManager>(config, logger);
    });

    // 9. Game
    container_.registerType<Game>([this]() {
        auto window      = container_.resolve<Window>();
        auto logger      = container_.resolve<Logger>();
        auto background  = container_.resolve<Background>();
        auto fontHolder  = container_.resolve<FontHolder>();
        auto saveManager = container_.resolve<SaveManager>();
        return std::make_shared<Game>(
            window, logger, background, fontHolder, saveManager);
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
    auto config = container_.resolve<Config>();

    logger->info("程序启动");
    logger->info("配置目录: " + config->configDir().string());
    logger->info("存档目录: " + config->savesDir().string());
    logger->info("资源目录: " + config->assetsDir().string());

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

            auto window = container_.resolve<Window>();
            auto size = window->native().getSize();
            config->setInt("window_width",  size.x);
            config->setInt("window_height", size.y);
            logger->info("窗口尺寸已保存: " + to_string(size.x) + "x" + to_string(size.y));
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