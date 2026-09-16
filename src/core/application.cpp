#include "application.h"
#include "animation.h"
#include "background.h"
#include "bootstrap_config.h"
#include "button_style.h"
#include "font_holder.h"
#include "game.h"
#include "keybindings.h"
#include "logging.h"
#include "notification.h"
#include "paths.h"
#include "platform.h"
#include "preferences.h"
#include "resolution.h"
#include "runtime_config.h"
#include "save_manager.h"
#include "sound_manager.h"
#include "theme.h"
#include "ui_scale.h"
#include "window.h"
#include "lang.h"

#include <algorithm>

using namespace std;

Application& Application::instance() {
    static Application inst;
    return inst;
}

Application::Application() {
    registerDependencies();
}

void Application::registerDependencies() {
    container_.registerType<Paths>([]() { return make_shared<Paths>(); });

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

    // ===== 应用 UI 缩放 / 主题 / 按钮样式 / 动画 / 通知 / 音效 =====
    {
        auto prefs = container_.resolve<Preferences>();

        setUiScale(static_cast<float>(prefs->getDouble("ui_scale", 1.0)));
        setTheme(static_cast<ThemeId>(prefs->getInt("theme", 0)));

        ButtonStyle bs;
        bs.cornerRadius = static_cast<float>(prefs->getDouble("button_corner", 6.0));
        bs.outlineThickness = static_cast<float>(prefs->getDouble("button_outline", 2.0));
        setButtonStyle(bs);

        Anim::setEnabled(prefs->getBool("animation_enabled", true));
        static const float kSpeeds[] = {0.5f, 1.0f, 2.0f};
        int idx = std::clamp(prefs->getInt("animation_speed_index", 1), 0, 2);
        Anim::setSpeed(kSpeeds[idx]);

        NotificationSystem::instance().setEnabled(
            prefs->getBool("notification_enabled", true));
        NotificationSystem::instance().setPosition(static_cast<NotificationPos>(
            std::clamp(prefs->getInt("notification_position", 1), 0, 3)));

        // 多语言
        {
            auto& lang = Lang::instance();
            lang.setLangDir((container_.resolve<Paths>()->assetsDir() / "lang").string());
            lang.scanAvailable();

            std::string code = prefs->get("language", "zh");
            // 校验是否在可用列表里
            const auto& avail = lang.available();
            if (std::find(avail.begin(), avail.end(), code) == avail.end()) {
                code = "zh";
            }
            lang.load(code);
        }

        // 键位
        {
            auto& kb = KeyBindings::instance();
            auto loadKey = [&](KeyBindings::Action a, const char* prefKey,
                               sf::Keyboard::Key defVal) {
                int v = prefs->getInt(prefKey, static_cast<int>(defVal));
                kb.set(a, static_cast<sf::Keyboard::Key>(v));
            };
            loadKey(KeyBindings::MoveLeft,  "key_left",    sf::Keyboard::Key::A);
            loadKey(KeyBindings::MoveRight, "key_right",   sf::Keyboard::Key::D);
            loadKey(KeyBindings::Jump,      "key_jump",    sf::Keyboard::Key::Space);
            loadKey(KeyBindings::Pause,     "key_pause",   sf::Keyboard::Key::Escape);
            loadKey(KeyBindings::Restart,   "key_restart", sf::Keyboard::Key::R);
        }

        // 音效
        SoundManager::instance().init();
        SoundManager::instance().setEnabled(prefs->getBool("sound_enabled", true));
        SoundManager::instance().setMasterVolume(
            static_cast<float>(prefs->getDouble("master_volume", 1.0)));
        SoundManager::instance().setSFXVolume(
            static_cast<float>(prefs->getDouble("sound_volume", 0.6)));
        SoundManager::instance().setMusicVolume(
            static_cast<float>(prefs->getDouble("bgm_volume", 0.4)));
    }

    // ===== Logger =====
    container_.registerType<Logger>([this]() {
        auto cfg = container_.resolve<BootstrapConfig>();
        auto prefs = container_.resolve<Preferences>();
        auto logPath = cfg->configFile("app.log");
        int lvl = prefs->getInt("log_level", static_cast<int>(LogLevel::Info));

        static const size_t sizes[] = {0, 1 * 1024 * 1024, 5 * 1024 * 1024,
                                       10 * 1024 * 1024};
        static const int keeps[] = {1, 3, 5, 10};
        int rotIdx = std::clamp(prefs->getInt("log_rotate", 0), 0, 3);
        int keepIdx = std::clamp(prefs->getInt("log_keep", 1), 0, 3);

        return make_shared<Logger>(logPath.string(), static_cast<LogLevel>(lvl),
                                   sizes[rotIdx], keeps[keepIdx]);
    });

    // ===== Window =====
    container_.registerType<Window>([this]() {
        auto prefs = container_.resolve<Preferences>();
        auto runtime = container_.resolve<RuntimeConfig>();

        unsigned w = 0, h = 0;
        bool rememberSize = prefs->getBool("remember_window_size", true);
        if (rememberSize) {
            int lastW = runtime->getInt("last_window_width", -1);
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

        bool fs = prefs->getBool("fullscreen", false);
        bool vsync = prefs->getBool("vsync", true);
        int aa = prefs->getInt("anti_aliasing", 8);
        int fpsLimit = prefs->getInt("fps_limit", 60);

        auto win =
            std::make_shared<Window>(w, h, "TEXT-GAME", fs, static_cast<unsigned>(aa));
        win->setVsync(vsync);
        win->setFramerateLimit(static_cast<unsigned>(fpsLimit));
        return win;
    });

    // ===== Background =====
    container_.registerType<Background>([this]() {
        auto paths = container_.resolve<Paths>();
        auto prefs = container_.resolve<Preferences>();
        auto window = container_.resolve<Window>();
        auto logger = container_.resolve<Logger>();
        auto size = window->native().getSize();
        std::string initial = prefs->get("current_wallpaper", "");
        return std::make_shared<Background>(paths->wallpaperDir(), initial, size.x,
                                            size.y, logger);
    });

    // ===== FontHolder =====
    container_.registerType<FontHolder>([this]() {
        auto cfg = container_.resolve<BootstrapConfig>();
        auto logger = container_.resolve<Logger>();
        auto fontPath = cfg->assetFile("font.ttf");
        return std::make_shared<FontHolder>(fontPath, logger);
    });

    // ===== SaveManager =====
    container_.registerType<SaveManager>([this]() {
        auto cfg = container_.resolve<RuntimeConfig>();
        auto logger = container_.resolve<Logger>();
        return std::make_shared<SaveManager>(cfg, logger);
    });

    // ===== Game =====
    container_.registerType<Game>([this]() {
        auto window = container_.resolve<Window>();
        auto logger = container_.resolve<Logger>();
        auto background = container_.resolve<Background>();
        auto fontHolder = container_.resolve<FontHolder>();
        auto saveManager = container_.resolve<SaveManager>();
        auto prefs = container_.resolve<Preferences>();
        auto runtime = container_.resolve<RuntimeConfig>();
        return std::make_shared<Game>(window, logger, background, fontHolder, saveManager,
                                      prefs, runtime);
    });
}

void Application::run() {
    auto logger = container_.resolve<Logger>();
    auto paths = container_.resolve<Paths>();

    logger->info("程序启动");
    logger->info("平台: " + std::string(Platform::name));
    logger->info("配置目录: " + paths->configDir().string());
    logger->info("存档目录: " + paths->savesDir().string());
    logger->info("资源目录: " + paths->assetsDir().string());
    logger->info("系统配置目录: " + Platform::userConfigDir().string());
    logger->info("系统缓存目录: " + Platform::userCacheDir().string());

    auto game = container_.resolve<Game>();
    game->run();

    logger->info("程序结束");
}