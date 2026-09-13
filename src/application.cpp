#include "application.h"
#include "logging.h"
#include "paths.h"
#include "config.h"
#include "calculator.h"
#include "window.h"
#include "game.h"

#include <iostream>

using namespace std;

// ---------- Application 单例实现 ----------

Application& Application::instance() {
    static Application inst;   // C++11 保证线程安全的懒加载
    return inst;
}

Application::Application() {
    registerDependencies();
}

void Application::registerDependencies() {
    // 1. Paths 无依赖，先注册
    container_.registerType<Paths>([]() {
        return make_shared<Paths>();
    });

    // 2. Config 依赖 Paths
    container_.registerType<Config>([this]() {
        auto paths = container_.resolve<Paths>();
        return make_shared<Config>(*paths);
    });

    // 3. Logger 从 Config 拿路径
    container_.registerType<Logger>([this]() {
        auto config = container_.resolve<Config>();
        auto logPath = config->configFile("app.log");
        return make_shared<Logger>(logPath.string());
    });

    // 4. Calculator 依赖 Logger
    container_.registerType<Calculator>([this]() {
        auto logger = container_.resolve<Logger>();
        return make_shared<Calculator>(logger);
    });

    // 5. Window 依赖 Config（懒加载：只有被 resolve 时才真正创建）
    container_.registerType<Window>([this]() {
        auto config = container_.resolve<Config>();
        int w = config->getInt("window_width", 1280);
        int h = config->getInt("window_height", 720);
        string title = config->get("window_title", "TEXT-GAME");
        return make_shared<Window>(w, h, title);
    });

    // 6. Game 依赖 Window 和 Logger
    container_.registerType<Game>([this]() {
        auto window = container_.resolve<Window>();
        auto logger = container_.resolve<Logger>();
        return make_shared<Game>(window, logger);
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
        return -1;   // 无效输入
    }
    return choice;
}

void Application::run() {
    auto logger = container_.resolve<Logger>();
    auto config = container_.resolve<Config>();

    logger->info("程序启动");
    logger->info("配置目录: " + config->configDir().string());
    logger->info("存档目录: " + config->savesDir().string());

    int choice = showMenu();

    switch (choice) {
        case 1: {
            // 只创建计算器，不创建窗口
            auto calc = container_.resolve<Calculator>();
            calc->run();
            break;
        }
        case 2: {
            // 创建窗口 + 游戏
            auto game = container_.resolve<Game>();
            game->run();

            // 游戏结束后保存窗口尺寸到配置
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