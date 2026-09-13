#include "application.h"
#include "logging.h"
#include "paths.h"
#include "bootstrap_config.h"
#include "runtime_config.h"
#include "preferences.h"
#include "resolution.h"
#include "window.h"
#include "background.h"
#include "font_holder.h"
#include "save_manager.h"
#include "game.h"
#include "ui_scale.h"
#include "theme.h"

using namespace std;

Application& Application::instance() {
    static Application inst;
    return inst;
}

Application::Application() {
    registerDependencies();
}

void Application::registerDependencies() {

    container_.registerType<Paths>([]() {
        return make_shared<Paths>();
    });

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

    // 应用 UI 缩放 + 主题
    {
        auto prefs = container_.resolve<Preferences>();
        setUiScale(static_cast<float>(prefs->getDouble("ui_scale", 1.0)));
        setTheme(static_cast<ThemeId>(prefs->getInt("theme", 0)));
    }

    container_.registerType<Logger>([this]() {
        auto cfg   = container_.resolve<BootstrapConfig>();
        auto prefs = container_.resolve<Preferences>();
        auto logPath = cfg->configFile("app.log");
        int lvl = prefs->getInt("log_level", static_cast<int>(LogLevel::Info));
        return make_shared<Logger>(logPath.string(), static_cast<LogLevel>(lvl));
    });

    container_.registerType<Window>([this]() {
        auto prefs   = container_.resolve<Preferences>();
        auto runtime = container_.resolve<RuntimeConfig>();

        unsigned w = 0, h = 0;
        bool rememberSize = prefs->getBool("remember_window_size", true);
        if (rememberSize) {
            int lastW = runtime->getInt("last_window_width",  -1);
            int lastH = runtime->getInt("last_window_height", -1);
            if (lastW > 0 && lastH > 0) {
                w = static_cast<unsigned>(lastW);
                h = static_cast<unsigned>(lastH);
            }
        }
        if (w == 0 || h == 0) {
            int idx = clampResolutionIndex(prefs->getInt("resolution_index", 0));
            w = kResolutions[idx].width;
            h = kResolutions[idx].height;
        }

        bool fs    = prefs->getBool("fullscreen", false);
        bool vsync = prefs->getBool("vsync", true);
        int  aa    = prefs->getInt("anti_aliasing", 8);

        auto win = std::make_shared<Window>(
            w, h, "TEXT-GAME", fs, static_cast<unsigned>(aa));
        win->setVsync(vsync);
        return win;
    });

    container_.registerType<Background>([this]() {
        auto paths  = container_.resolve<Paths>();
        auto prefs  = container_.resolve<Preferences>();
        auto window = container_.resolve<Window>();
        auto logger = container_.resolve<Logger>();
        auto size   = window->native().getSize();
        std::string initial = prefs->get("current_wallpaper", "");
        return std::make_shared<Background>(
            paths->wallpaperDir(), initial, size.x, size.y, logger);
    });

    container_.registerType<FontHolder>([this]() {
        auto cfg    = container_.resolve<BootstrapConfig>();
        auto logger = container_.resolve<Logger>();
        auto fontPath = cfg->assetFile("font.ttf");
        return std::make_shared<FontHolder>(fontPath, logger);
    });

    container_.registerType<SaveManager>([this]() {
        auto cfg    = container_.resolve<RuntimeConfig>();
        auto logger = container_.resolve<Logger>();
        return std::make_shared<SaveManager>(cfg, logger);
    });

    container_.registerType<Game>([this]() {
        auto window      = container_.resolve<Window>();
        auto logger      = container_.resolve<Logger>();
        auto background  = container_.resolve<Background>();
        auto fontHolder  = container_.resolve<FontHolder>();
        auto saveManager = container_.resolve<SaveManager>();
        auto prefs       = container_.resolve<Preferences>();
        auto runtime     = container_.resolve<RuntimeConfig>();
        return std::make_shared<Game>(
            window, logger, background, fontHolder,
            saveManager, prefs, runtime);
    });
}

void Application::run() {
    auto logger = container_.resolve<Logger>();
    auto paths  = container_.resolve<Paths>();

    logger->info("程序启动");
    logger->info("配置目录: " + paths->configDir().string());
    logger->info("存档目录: " + paths->savesDir().string());
    logger->info("资源目录: " + paths->assetsDir().string());

    auto game = container_.resolve<Game>();
    game->run();

    logger->info("程序结束");
}