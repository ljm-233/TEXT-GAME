#include "game.h"
#include "main_menu_scene.h"
#include "save_select_scene.h"
#include "level_select_scene.h"
#include "game_scene.h"
#include "settings_scene.h"
#include "console_scene.h"
#include "editor_scene.h"
#include "notification.h"
#include "sound_manager.h"
#include "gamepad.h"
#include "keybindings.h"
#include "focus_group.h"
#include <SFML/System/Clock.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include "text_strings.h"

Game::Game(std::shared_ptr<Window>        window,
           std::shared_ptr<Logger>        logger,
           std::shared_ptr<Background>    background,
           std::shared_ptr<FontHolder>    fontHolder,
           std::shared_ptr<SaveManager>   saveManager,
           std::shared_ptr<Preferences>   preferences,
           std::shared_ptr<RuntimeConfig> runtimeConfig)
    : window_(std::move(window)),
      logger_(std::move(logger)),
      background_(std::move(background)),
      fontHolder_(std::move(fontHolder)),
      saveManager_(std::move(saveManager)),
      preferences_(std::move(preferences)),
      runtimeConfig_(std::move(runtimeConfig)),
      fpsText_(fontHolder_->get(), sf::String("FPS: 0"), 20),
      clockText_(fontHolder_->get(), sf::String(""), 20) {
    fpsText_.setFillColor(sf::Color(255, 255, 100));
    clockText_.setFillColor(sf::Color(220, 220, 240));

    NotificationSystem::instance().setFont(fontHolder_->get());
    sceneManager_ = std::make_unique<SceneManager>(
        [this](SceneId id) { return createScene(id); });
}

std::unique_ptr<Scene> Game::createScene(SceneId id) {
    const sf::Font& font = fontHolder_->get();
    switch (id) {
        case SceneId::MainMenu:
            return std::make_unique<MainMenuScene>(background_, font, logger_);
        case SceneId::SaveSelect:
            return std::make_unique<SaveSelectScene>(
                background_, saveManager_, font, logger_);
        case SceneId::LevelSelect:
            return std::make_unique<LevelSelectScene>(
                background_, saveManager_, font, logger_);
        case SceneId::Game:
            return std::make_unique<GameScene>(
                background_, font, logger_,
                saveManager_,
                preferences_);
        case SceneId::Settings:
            return std::make_unique<SettingsScene>(
                background_, preferences_, runtimeConfig_,
                window_, font, logger_);
        case SceneId::Console:
            return std::make_unique<ConsoleScene>(
                background_, preferences_, saveManager_, font, logger_);
        case SceneId::Editor:
            return std::make_unique<EditorScene>(
                background_, preferences_, font, logger_);
        default:
            return nullptr;
    }
}

void Game::switchScene(SceneId next) {
    if (next == SceneId::None) return;
    if (next == sceneManager_->currentId()) return;

    flushConfigs();

    if (next == SceneId::Exit) {
        window_->close();
        return;
    }
    if (next == SceneId::Back) {
        sceneManager_->pop();
    } else {
        sceneManager_->push(next);
    }
    FocusGroup::instance().clear();

    // 场景切换后立刻更新标题
    updateWindowTitle(sceneManager_->current());
}

void Game::saveWindowState() {
    if (!window_->isOpen()) return;
    if (!preferences_->getBool("remember_window_size", true)) return;
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

void Game::renderOverlays() {
    auto& rt = window_->native();
    auto winSize = rt.getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    sf::View screenView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
    rt.setView(screenView);

    const float margin = 20.f;

    if (preferences_->getBool("show_fps", false)) {
        int fmt = preferences_->getInt("fps_format", 1);
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
        int pos = preferences_->getInt("fps_position", 1);
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

    if (preferences_->getBool("show_clock", false)) {
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
        int pos = preferences_->getInt("clock_position", 0);
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

    NotificationSystem::instance().render(rt);
}

void Game::flushConfigs() {
    preferences_->flush();
    runtimeConfig_->flush();
}

void Game::run() {
    logger_->info("游戏启动");
    if (!sceneManager_->start(SceneId::MainMenu)) {
        logger_->error("无法创建主菜单场景");
        return;
    }

    // 主菜单初始标题
    updateWindowTitle(sceneManager_->current());

    SoundManager::instance().playBGM();

    sf::Clock clock;
    while (window_->isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        Gamepad::instance().update();

        FocusGroup::instance().setEnabled(
            preferences_->getBool("gamepad_enabled", true));
        FocusGroup::instance().update(dt);

        // 自动暂停
        if (preferences_->getBool("auto_pause_on_blur", true)) {
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
                sceneManager_->current().render(*window_);
                renderOverlays();
                window_->display();
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

        scene.render(*window_);

        renderOverlays();
        window_->display();

        SceneId next = scene.nextScene();
        if (next != SceneId::None && next != sceneManager_->currentId()) {
            switchScene(next);
        }
    }

    SoundManager::instance().stopBGM();
    saveWindowState();
    flushConfigs();
    logger_->info("游戏结束");
}