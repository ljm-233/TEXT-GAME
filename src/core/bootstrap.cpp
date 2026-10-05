#include "core/bootstrap.h"

#include "core/achievement.h"
#include "core/game.h"
#include "utils/lang.h"
#include "core/paths.h"
#include "core/resource_manager.h"
#include "core/platform.h"
#include "core/scene_registry.h"
#include "log/logger.h"

#include "config/bootstrap_config.h"
#include "config/keys.h"
#include "config/preferences.h"
#include "config/runtime_config.h"

#include "utils/animation.h"
#include "ui/background.h"
#include "ui/button_style.h"
#include "ui/font_holder.h"
#include "infrastructure/gamepad.h"
#include "infrastructure/keybindings.h"
#include "ui/notification.h"
#include "ui/sound_manager.h"
#include "ui/theme.h"
#include "ui/ui_scale.h"
#include "ui/window.h"

#include "ui/resolution.h"
#include "game/save_manager.h"

#include <algorithm>
#include <memory>

// ============================================================
// 装配：只注册工厂
// ============================================================

namespace {

/// 窗口工厂。参数多且都要读配置，单独提出来避免 registerCore 变成一堵墙。
std::shared_ptr<Window> makeWindow(Container& container) {
    auto prefs = container.require<Preferences>("preferences");
    auto runtime = container.require<RuntimeConfig>("runtime_config");

    unsigned w = 0, h = 0;
    if (prefs->getBool(ConfigKey::kRememberWindowSize, true)) {
        const int lastW = runtime->getInt(ConfigKey::kLastWindowWidth, -1);
        const int lastH = runtime->getInt(ConfigKey::kLastWindowHeight, -1);
        if (lastW > 0 && lastH > 0) {
            w = static_cast<unsigned>(lastW);
            h = static_cast<unsigned>(lastH);
        }
    }
    if (w == 0 || h == 0) {
        const int idx = clampResolutionIndex(prefs->getInt(ConfigKey::kResolutionIndex, 0));
        w = kResolutions[idx].width;
        h = kResolutions[idx].height;
    }

    const bool fullscreen = prefs->getBool(ConfigKey::kFullscreen, false);
    const bool vsync = prefs->getBool(ConfigKey::kVsync, true);
    const int antiAliasing = prefs->getInt(ConfigKey::kAntiAliasing, 8);
    const int fpsLimit = prefs->getInt(ConfigKey::kFpsLimit, 60);

    auto window = std::make_shared<Window>(w, h, "TEXT-GAME", fullscreen,
                                           static_cast<unsigned>(antiAliasing));
    window->setVsync(vsync);
    window->setFramerateLimit(static_cast<unsigned>(fpsLimit));
    window->setRenderScale(
        static_cast<float>(prefs->getDouble(ConfigKey::kRenderScale, 1.0)));

    // 超分 shader（失败则自动回退到双线性）
    auto resources = container.require<ResourceManager>("resources");
    window->loadUpscaler(resources->dir("shaders").string());

    // 恢复后处理参数
    auto& pp = window->postProcess();
    pp.setSaturation(static_cast<float>(prefs->getDouble(ConfigKey::kPostSaturation, 1.0)));
    pp.setContrast(static_cast<float>(prefs->getDouble(ConfigKey::kPostContrast, 1.0)));
    pp.setBrightness(static_cast<float>(prefs->getDouble(ConfigKey::kPostBrightness, 1.0)));
    pp.setGamma(static_cast<float>(prefs->getDouble(ConfigKey::kPostGamma, 1.0)));
    pp.setVignette(static_cast<float>(prefs->getDouble(ConfigKey::kPostVignette, 0.0)));
    pp.setBloomStrength(
        static_cast<float>(prefs->getDouble(ConfigKey::kPostBloomStrength, 0.0)));
    pp.setBloomThreshold(
        static_cast<float>(prefs->getDouble(ConfigKey::kPostBloomThreshold, 0.7)));
    pp.setChromatic(static_cast<float>(prefs->getDouble(ConfigKey::kPostChromatic, 0.0)));
    pp.setGrain(static_cast<float>(prefs->getDouble(ConfigKey::kPostGrain, 0.0)));
    pp.setScanline(static_cast<float>(prefs->getDouble(ConfigKey::kPostScanline, 0.0)));
    pp.setDither(static_cast<float>(prefs->getDouble(ConfigKey::kPostDither, 0.0)));
    window->setUpscaleMode(prefs->getInt(ConfigKey::kUpscaleMode, 1));

    // 启动时根据 window_mode 决定窗口状态
    if (prefs->getInt(ConfigKey::kWindowMode, 0) == 1 && !fullscreen)
        window->requestMaximize();

    return window;
}

}  // namespace

void registerCore(Container& container) {
    // ---------------- 路径 ----------------
    // 三个 Config（BootstrapConfig / Preferences / RuntimeConfig）由
    // config 层的 registerConfig() 负责注册，它们在解析期回头来要这个 Paths。
    container.reg<Paths>("paths", []() { return std::make_shared<Paths>(); });

    // 静态资源寻址。跟着 Paths 走（同一层），所以资源根只有一处定义。
    // 用户可写目录不在这里 —— 那些仍然由 Paths 提供（跟机器绑定、落在 XDG）。
    container.reg<ResourceManager>("resources", [&container]() {
        return std::make_shared<ResourceManager>(*container.require<Paths>("paths"));
    });

    // ---------------- 前端资源 ----------------
    container.reg<Window>("window", [&container]() { return makeWindow(container); });

    container.reg<Background>("background", [&container]() {
        auto resources = container.require<ResourceManager>("resources");
        auto prefs = container.require<Preferences>("preferences");
        auto window = container.require<Window>("window");
        auto logger = container.require<Logger>("logger");

        const auto size = window->native().getSize();
        return std::make_shared<Background>(resources->dir("wallpaper"),
                                            prefs->get(ConfigKey::kCurrentWallpaper, ""),
                                            size.x, size.y, logger);
    });

    container.reg<FontHolder>("font_holder", [&container]() {
        auto resources = container.require<ResourceManager>("resources");
        auto logger = container.require<Logger>("logger");
        return std::make_shared<FontHolder>(resources->get("assets", "font.ttf"),
                                            logger);
    });

    // ---------------- 存档 ----------------
    container.reg<SaveManager>("save_manager", [&container]() {
        // 只给 Paths：存档全在 saves/ 下，跟配置内容无关
        return std::make_shared<SaveManager>(container.require<Paths>("paths"),
                                             container.require<Logger>("logger"));
    });

    // ---------------- 场景注册表 ----------------
    // 这里只建立空表；具体场景由 scene 层的 registerScenes() 填。
    // core 因此不需要认识任何场景类型，只认识"按 id 造场景"这个契约。
    container.reg<SceneRegistry>("scene_registry",
                                 []() { return std::make_shared<SceneRegistry>(); });

    // ---------------- 游戏本体 ----------------
    // 先把依赖逐个取到具名局部变量，再一次性构造。
    // 直接写成 make_shared<Game>(require(...), require(...), ...) 是不行的：
    // C++ 不保证函数实参的求值顺序，那样启动顺序会随编译器变化。
    container.reg<Game>("game", [&container]() {
        auto window = container.require<Window>("window");
        auto logger = container.require<Logger>("logger");
        auto background = container.require<Background>("background");
        auto fontHolder = container.require<FontHolder>("font_holder");
        auto saveManager = container.require<SaveManager>("save_manager");
        auto preferences = container.require<Preferences>("preferences");
        auto runtimeConfig = container.require<RuntimeConfig>("runtime_config");
        auto sceneRegistry = container.require<SceneRegistry>("scene_registry");

        return std::make_shared<Game>(window, logger, background, fontHolder,
                                      saveManager, preferences, runtimeConfig,
                                      sceneRegistry);
    });

    // ---------------- 主循环 ----------------
    // 前端交出主循环的入口：Game 自己实现 MainLoop，所以这里只是换个类型视图，
    // 不会产生第二个 Game 实例。对应 albuswall 里 UI 插件给出的 main_loop。
    container.reg<MainLoop>("main_loop", [&container]() {
        return std::static_pointer_cast<MainLoop>(container.require<Game>("game"));
    });
}

// ============================================================
// 接线：挂生命周期钩子
// ============================================================

namespace {

/// 让偏好生效。这一段全部是副作用，所以只能活在 wire 阶段。
///
/// 顺序有讲究：UI 缩放 -> 主题 -> 控件样式 -> 动画 -> 通知 -> 语言 -> 键位
/// -> 音效 -> 手柄。后面的都假设前面的已经生效。
void applyPreferences(Container& container) {
    auto prefs = container.require<Preferences>("preferences");

    setUiScale(static_cast<float>(prefs->getDouble(ConfigKey::kUiScale, 1.0)));
    setFontScale(static_cast<float>(prefs->getDouble(ConfigKey::kFontScale, 1.0)));
    setTheme(static_cast<ThemeId>(prefs->getInt(ConfigKey::kTheme, 0)));

    ButtonStyle style;
    style.cornerRadius = static_cast<float>(prefs->getDouble(ConfigKey::kButtonCorner, 6.0));
    style.outlineThickness =
        static_cast<float>(prefs->getDouble(ConfigKey::kButtonOutline, 2.0));
    setButtonStyle(style);

    Anim::setEnabled(prefs->getBool(ConfigKey::kAnimationEnabled, true));
    {
        static const float kSpeeds[] = {0.5f, 1.0f, 2.0f};
        const int idx = std::clamp(prefs->getInt(ConfigKey::kAnimationSpeedIndex, 1), 0, 2);
        Anim::setSpeed(kSpeeds[idx]);
    }

    NotificationSystem::instance().setEnabled(
        prefs->getBool(ConfigKey::kNotificationEnabled, true));
    NotificationSystem::instance().setPosition(static_cast<NotificationPos>(
        std::clamp(prefs->getInt(ConfigKey::kNotificationPosition, 1), 0, 3)));

    // 多语言
    {
        auto& lang = Lang::instance();
        lang.setLangDir(
            container.require<ResourceManager>("resources")->dir("lang").string());
        lang.scanAvailable();

        std::string code = prefs->get(ConfigKey::kLanguage, "zh");
        const auto& available = lang.available();
        if (std::find(available.begin(), available.end(), code) == available.end())
            code = "zh";
        lang.load(code);
    }

    // 键位
    {
        auto& keys = KeyBindings::instance();
        auto loadKey = [&](KeyBindings::Action action, const char* prefKey,
                           sf::Keyboard::Key fallback) {
            keys.set(action, static_cast<sf::Keyboard::Key>(
                                 prefs->getInt(prefKey, static_cast<int>(fallback))));
        };
        loadKey(KeyBindings::MoveLeft, ConfigKey::kKeyLeft, sf::Keyboard::Key::A);
        loadKey(KeyBindings::MoveRight, ConfigKey::kKeyRight, sf::Keyboard::Key::D);
        loadKey(KeyBindings::Jump, ConfigKey::kKeyJump, sf::Keyboard::Key::Space);
        loadKey(KeyBindings::Pause, ConfigKey::kKeyPause, sf::Keyboard::Key::Escape);
        loadKey(KeyBindings::Restart, ConfigKey::kKeyRestart, sf::Keyboard::Key::R);
    }

    // 音效
    SoundManager::instance().init();
    SoundManager::instance().setEnabled(prefs->getBool(ConfigKey::kAudioSoundEnabled, true));
    SoundManager::instance().setMasterVolume(
        static_cast<float>(prefs->getDouble(ConfigKey::kAudioMasterVolume, 1.0)));
    SoundManager::instance().setSFXVolume(
        static_cast<float>(prefs->getDouble(ConfigKey::kAudioSoundVolume, 0.6)));
    SoundManager::instance().setMusicVolume(
        static_cast<float>(prefs->getDouble(ConfigKey::kAudioBgmVolume, 0.4)));

    // 手柄振动
    Gamepad::instance().setVibrationEnabled(
        prefs->getBool(ConfigKey::kGamepadVibrationEnabled, true));
    Gamepad::instance().setVibrationIntensity(static_cast<float>(
        prefs->getDouble(ConfigKey::kGamepadVibrationIntensity, 1.0)));
}

/// 需要操作系统资源的子系统。放在偏好之后，因为它们要读偏好。
void initSubsystems(Container& container) {
    auto paths = container.require<Paths>("paths");

    // 手柄振动（evdev / XInput）
    GamepadVibration::instance().init();

    // 成就系统（跨存档全局单例，状态存 config/achievements.conf）
    AchievementManager::instance().init(
        (paths->configDir() / "achievements.conf").string());
}

/// 启动横幅。放在最后，保证前面任何一步失败都还能在日志里看到上下文。
void logStartupInfo(Container& container) {
    auto logger = container.require<Logger>("logger");
    auto paths = container.require<Paths>("paths");

    logger->info("程序启动");
    logger->info("平台: " + std::string(Platform::name));
    logger->info("配置目录: " + paths->configDir().string());
    logger->info("存档目录: " + paths->savesDir().string());
    logger->info("资源目录: " + paths->assetsDir().string());
    // 别名表整张打出来：资源找不到时第一件事就是看它
    logger->info(container.require<ResourceManager>("resources")->str());
    logger->info("系统配置目录: " + Platform::userConfigDir().string());
    logger->info("系统缓存目录: " + Platform::userCacheDir().string());
}

}  // namespace

void wireCore(Application& app) {
    Container& container = app.container();

    // ---------------- on_boot：依赖在前 ----------------
    app.onBoot([&container]() { applyPreferences(container); });
    app.onBoot([&container]() { initSubsystems(container); });
    app.onBoot([&container]() { logStartupInfo(container); });

    // 装配快照。默认 Info 级别下不会输出；把 log_level 调到 Debug 就能看到
    // "到底注册了哪些单例、分别是哪个类型、有没有真的构造出来"。
    //
    // 先问 isEnabled 再拼字符串，是刻意的惰性求值：快照要遍历容器并做类型名
    // demangle，不该在不需要的时候付这个代价。
    // 对应 albuswall __main__.py 里的 _logger.debug("Container: %s", container)。
    app.onBoot([&container, &app]() {
        auto logger = container.tryGet<Logger>("logger");
        if (!logger || !logger->isEnabled(LogLevel::Debug))
            return;

        logger->debug("应用快照:\n" + app.str());
        logger->debug("日志层快照:\n" + logger->str());

        // 配置快照按 key 排序输出，两次启动的快照可以直接 diff。
        // 注意 Preferences 是"运行时可变"的那一份，刚启动时未必等于磁盘内容。
        if (auto prefs = container.tryGet<Preferences>("preferences"))
            logger->debug("偏好快照:\n" + prefs->str());

        // 场景注册表：一眼看清"这个构建里到底有哪些场景可用"。
        // 场景没登记是运行期才炸的错误，这张表是它唯一的事前可见形式。
        if (auto scenes = container.tryGet<SceneRegistry>("scene_registry"))
            logger->debug("场景注册表:\n" + scenes->str());
    });

    // ---------------- on_final：先注册的先拆 ----------------
    // 顺序 = 初始化的逆序。手柄振动是最后拿到的系统资源，所以最先还回去。
    app.onFinal([]() { GamepadVibration::instance().shutdown(); });

    app.onFinal([&container]() {
        auto logger = container.tryGet<Logger>("logger");
        if (logger)
            logger->info("程序结束");
    });
}
