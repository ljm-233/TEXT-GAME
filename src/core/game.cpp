#include "game.h"
#include "notification.h"
#include "sound_manager.h"
#include "infrastructure/gamepad.h"
#include "infrastructure/gamepad_config.h"
#include "infrastructure/keybindings.h"
#include "focus_group.h"
#include "config/keys.h"
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include "utils/text_strings.h"

Game::Game(std::shared_ptr<Window>        window,
           std::shared_ptr<Logger>        logger,
           std::shared_ptr<Background>    background,
           std::shared_ptr<FontHolder>    fontHolder,
           std::shared_ptr<SaveManager>   saveManager,
           std::shared_ptr<Preferences>   preferences,
           std::shared_ptr<RuntimeConfig> runtimeConfig,
           std::shared_ptr<SceneRegistry> sceneRegistry)
    : window_(std::move(window)),
      logger_(std::move(logger)),
      background_(std::move(background)),
      fontHolder_(std::move(fontHolder)),
      saveManager_(std::move(saveManager)),
      preferences_(std::move(preferences)),
      runtimeConfig_(std::move(runtimeConfig)),
      sceneRegistry_(std::move(sceneRegistry)),
      fpsText_(fontHolder_->get(), sf::String("FPS: 0"), 20),
      clockText_(fontHolder_->get(), sf::String(""), 20) {
    fpsText_.setFillColor(sf::Color(255, 255, 100));
    clockText_.setFillColor(sf::Color(220, 220, 240));

    NotificationSystem::instance().setFont(fontHolder_->get());

    // 场景从注册表按 id 取，Game 本身不认识任何具体场景类型。
    sceneManager_ = std::make_unique<SceneManager>(
        [this](SceneId id) { return sceneRegistry_->create(id); });
}

void Game::switchScene(SceneId next) {
    if (next == SceneId::None) return;
    if (next == sceneManager_->currentId()) return;

    flushConfigs();

    if (next == SceneId::Exit) {
        window_->close();
        return;
    }

    // ⭐ 先清空焦点，让新场景的 onEnter/onResume 重新注册
    FocusGroup::instance().clear();
    // 键盘导航可能在设置页的文本输入里被挂起过（见 SettingsScene::update）。
    // 换场景一定要恢复 —— 否则"在输入框里按鼠标点返回"会把键盘导航永久关死。
    FocusGroup::instance().setKeyboardNavEnabled(true);

    if (next == SceneId::Back) {
        sceneManager_->pop();
    } else {
        sceneManager_->push(next);
    }

    // 场景切换后立刻更新标题
    updateWindowTitle(sceneManager_->current());
}

void Game::saveWindowState() {
    if (!window_->isOpen()) return;
    if (!preferences_->getBool(ConfigKey::kRememberWindowSize, true)) return;
    auto size = window_->native().getSize();
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(size.x));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(size.y));
}

void Game::updateWindowTitle(const Scene& scene) {
    std::string hint = scene.windowTitleHint();

    if (hint.empty()) {
        switch (sceneManager_->currentId()) {
            case SceneId::MainMenu:    hint = "";                              break;
            case SceneId::SaveSelect:  hint = Str::T(Str::WinTitleSaveSelect);  break;
            case SceneId::LevelSelect: hint = Str::T(Str::WinTitleLevelSelect); break;
            case SceneId::Settings:    hint = Str::T(Str::WinTitleSettings);    break;
            case SceneId::Console:     hint = Str::T(Str::WinTitleConsole);     break;
            case SceneId::Editor:      hint = Str::T(Str::WinTitleEditor);      break;
            case SceneId::Achievements: hint = Str::T(Str::WinTitleAchievements); break;
            default: break;
        }
    }

    std::string title = "TEXT-GAME";
    if (!hint.empty()) title += " - " + hint;

    if (title != lastWindowTitle_) {
        window_->setTitle(title);
        lastWindowTitle_ = title;
        logger_->info("窗口标题: " + title);
    }
}

void Game::renderOverlays(float dt) {
    auto& rt = window_->target();
    auto winSize = window_->native().getSize();   // ⭐ 逻辑尺寸
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    sf::View screenView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
    rt.setView(screenView);

    const float margin = 20.f;

    if (preferences_->getBool(ConfigKey::kShowFps, false)) {
        int fmt = preferences_->getInt(ConfigKey::kFpsFormat, 1);
        std::string s;
        switch (fmt) {
            case 0: s = std::to_string(static_cast<int>(fpsDisplayed_)); break;
            case 1: s = std::to_string(static_cast<int>(fpsDisplayed_)) + " FPS"; break;
            case 2: {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.1f FPS", fpsDisplayed_);
                s = buf;
                break;
            }
            default: s = std::to_string(static_cast<int>(fpsDisplayed_)) + " FPS";
        }
        fpsText_.setString(s);
        auto b = fpsText_.getLocalBounds();
        int pos = preferences_->getInt(ConfigKey::kFpsPosition, 1);
        sf::Vector2f p;
        switch (pos) {
            case 0: p = {margin, margin}; break;
            case 1: p = {winW - b.size.x - margin, margin}; break;
            case 2: p = {margin, winH - b.size.y - margin}; break;
            case 3: p = {winW - b.size.x - margin, winH - b.size.y - margin}; break;
            default: p = {winW - b.size.x - margin, margin};
        }
        fpsText_.setPosition(p);
        rt.draw(fpsText_);
    }

    if (preferences_->getBool(ConfigKey::kShowClock, false)) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
        clockText_.setString(std::string(buf));

        auto b = clockText_.getLocalBounds();
        int pos = preferences_->getInt(ConfigKey::kClockPosition, 0);
        sf::Vector2f p;
        switch (pos) {
            case 0: p = {margin, margin}; break;
            case 1: p = {winW - b.size.x - margin, margin}; break;
            case 2: p = {margin, winH - b.size.y - margin}; break;
            case 3: p = {winW - b.size.x - margin, winH - b.size.y - margin}; break;
            default: p = {margin, margin};
        }
        clockText_.setPosition(p);
        rt.draw(clockText_);
    }

    NotificationSystem::instance().render(rt, dt);
}

void Game::flushConfigs() {
    preferences_->flush();
    runtimeConfig_->flush();
}

int Game::run() {
    logger_->info("游戏启动");
    if (!sceneManager_->start(SceneId::MainMenu)) {
        logger_->error("无法创建主菜单场景");
        return 1;
    }

    // 主菜单初始标题
    updateWindowTitle(sceneManager_->current());

    SoundManager::instance().playBGM();

    sf::Clock clock;
    while (window_->isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        Gamepad::instance().update();

        // ===== 焦点导航的输入翻译：设备状态 → 几个 bool =====
        // FocusGroup 不自己去轮询：sf::Keyboard::isKeyPressed 是全局状态，
        // 测试里伪造不了；把设备翻译留在主循环，导航逻辑才能被测住。
        const bool gamepadOn = preferences_->getBool(ConfigKey::kGamepadEnabled, true);
        FocusGroup::NavInput nav;
        if (FocusGroup::instance().keyboardNavEnabled()) {
            // 移动：方向键 或 WASD；确认：Enter
            // （刻意不用 Space —— 游戏里那是跳跃，暂停菜单开着时容易双触发）
            nav.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
            nav.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
            nav.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
            nav.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
            nav.confirm = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter);
        }
        if (gamepadOn) {
            auto& gp = Gamepad::instance();
            if (gp.isConnected()) {
                constexpr float T = GamepadConfig::kNavStickThreshold;
                nav.up = nav.up || gp.dpadUp() || gp.leftY() < -T;
                nav.down = nav.down || gp.dpadDown() || gp.leftY() > T;
                nav.left = nav.left || gp.dpadLeft() || gp.leftX() < -T;
                nav.right = nav.right || gp.dpadRight() || gp.leftX() > T;
                nav.confirm = nav.confirm || gp.isButtonPressed(0); // A 键
            }
        }

        // "手柄支持"只管手柄那一路，键盘导航不受它影响
        FocusGroup::instance().setGamepadEnabled(gamepadOn);
        FocusGroup::instance().update(dt, nav);

        // 自动暂停
        if (preferences_->getBool(ConfigKey::kAutoPauseOnBlur, true)) {
            bool focused = window_->isFocused();
            if (!focused && !autoPaused_) {
                autoPaused_ = true;
                SoundManager::instance().pauseBGM();
            } else if (focused && autoPaused_) {
                autoPaused_ = false;
                SoundManager::instance().resumeBGM();
            }
            if (autoPaused_) {
                window_->pollEvents();
                window_->beginFrame();
                sceneManager_->current().render(*window_);
                renderOverlays(dt);
                window_->endFrame();
                continue;
            }
        }

        fpsFrameCount_++;
        fpsElapsed_ += dt;
        if (fpsElapsed_ >= 0.5f) {
            fpsDisplayed_ = fpsFrameCount_ / fpsElapsed_;
            fpsFrameCount_ = 0;
            fpsElapsed_ = 0.f;
        }

        flushTimer_ += dt;
        if (flushTimer_ >= 5.f) { flushConfigs(); flushTimer_ = 0.f; }

        Scene& scene = sceneManager_->current();

        window_->pollEvents([&](const sf::Event& e) {
            scene.handleEvent(e);
        });

        auto synth = FocusGroup::instance().takePendingEvents();
        for (auto& e : synth) {
            scene.handleEvent(e);
        }

        scene.update(dt);

        // 每帧检测标题变化
        updateWindowTitle(scene);

        // ⭐ 渲染缩放：beginFrame → 绘制 → endFrame
        window_->beginFrame();

        scene.render(*window_);

        renderOverlays(dt);

        window_->endFrame();

        SceneId next = scene.nextScene();
        if (next != SceneId::None && next != sceneManager_->currentId()) {
            switchScene(next);
        }
    }

    SoundManager::instance().stopBGM();
    saveWindowState();
    flushConfigs();
    logger_->info("游戏结束");
    return 0;
}

void Game::quit() {
    // 主循环的条件是 window_->isOpen()，所以"请求退出"就是关窗。
    // 不另设一个循环标志位，避免和 SFML 自己的事件处理产生两个真相。
    window_->close();
}

std::string Game::str() const {
    return "Game(sceneId=" +
           std::to_string(static_cast<int>(sceneManager_->currentId())) +
           ", windowOpen=" + (window_->isOpen() ? "true" : "false") + ")";
}