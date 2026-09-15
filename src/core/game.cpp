#include "game.h"
#include "main_menu_scene.h"
#include "save_select_scene.h"
#include "level_select_scene.h"
#include "game_scene.h"
#include "settings_scene.h"
#include "console_scene.h"
#include "notification.h"
#include "sound_manager.h"
#include "gamepad.h"
#include "focus_group.h"
#include <SFML/System/Clock.hpp>
#include <algorithm>
#include <cstdio>
#include <ctime>

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

    transitionOverlay_.setFillColor(sf::Color(0, 0, 0, 0));
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
                saveManager_->takePendingSave(),
                saveManager_,
                preferences_);
        case SceneId::Settings:
            return std::make_unique<SettingsScene>(
                background_, preferences_, runtimeConfig_,
                window_, font, logger_);
        case SceneId::Console:
            return std::make_unique<ConsoleScene>(
                background_, preferences_, saveManager_, font, logger_);
        default:
            return nullptr;
    }
}

void Game::switchScene(SceneId next) {
    if (next == SceneId::None) return;
    pendingScene_ = next;
    transitionPhase_ = TransitionPhase::FadingOut;
}

void Game::updateTransition(float dt) {
    if (transitionPhase_ == TransitionPhase::None) return;

    if (transitionPhase_ == TransitionPhase::FadingOut) {
        transitionAlpha_ += transitionSpeed_ * dt;
        if (transitionAlpha_ >= 1.f) {
            transitionAlpha_ = 1.f;

            flushConfigs();
            if (pendingScene_ == SceneId::Exit) {
                window_->close();
                transitionPhase_ = TransitionPhase::None;
                return;
            } else if (pendingScene_ == SceneId::Back) {
                sceneManager_->pop();
            } else {
                sceneManager_->push(pendingScene_);
            }
            pendingScene_ = SceneId::None;
            transitionPhase_ = TransitionPhase::FadingIn;

            // 场景切换后清空焦点（新场景会重新注册）
            FocusGroup::instance().clear();
        }
    } else if (transitionPhase_ == TransitionPhase::FadingIn) {
        transitionAlpha_ -= transitionSpeed_ * dt;
        if (transitionAlpha_ <= 0.f) {
            transitionAlpha_ = 0.f;
            transitionPhase_ = TransitionPhase::None;
        }
    }
}

void Game::saveWindowState() {
    if (!window_->isOpen()) return;
    if (!preferences_->getBool("remember_window_size", true)) return;
    auto size = window_->native().getSize();
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(size.x));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(size.y));
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

    if (transitionPhase_ != TransitionPhase::None && transitionAlpha_ > 0.f) {
        auto a = static_cast<std::uint8_t>(
            std::clamp(transitionAlpha_, 0.f, 1.f) * 255.f);
        transitionOverlay_.setSize({winW, winH});
        transitionOverlay_.setFillColor(sf::Color(0, 0, 0, a));
        rt.draw(transitionOverlay_);
    }
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

    SoundManager::instance().playBGM();

    sf::Clock clock;
    while (window_->isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        // ============================================================
        // ⭐ 手柄与焦点
        // ============================================================
        Gamepad::instance().update();

        FocusGroup::instance().setEnabled(
            preferences_->getBool("gamepad_enabled", true));
        FocusGroup::instance().update(dt);

        // ============================================================
        // 自动暂停
        // ============================================================
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

        // ============================================================
        // FPS / 定时 flush
        // ============================================================
        fpsFrameCount_++;
        fpsElapsed_ += dt;
        if (fpsElapsed_ >= 0.5f) {
            fpsDisplayed_ = fpsFrameCount_ / fpsElapsed_;
            fpsFrameCount_ = 0;
            fpsElapsed_ = 0.f;
        }

        flushTimer_ += dt;
        if (flushTimer_ >= 5.f) { flushConfigs(); flushTimer_ = 0.f; }

        bool transitioning = (transitionPhase_ != TransitionPhase::None);

        Scene& scene = sceneManager_->current();

        // ============================================================
        // 真实事件（键盘 + 鼠标）
        // ============================================================
        window_->pollEvents([&](const sf::Event& e) {
            if (!transitioning) scene.handleEvent(e);
        });

        // ============================================================
        // 合成事件（来自手柄的 B 键 / Start 键）
        // ============================================================
        auto synth = FocusGroup::instance().takePendingEvents();
        if (!transitioning) {
            for (auto& e : synth) {
                scene.handleEvent(e);
            }
        }

        // ============================================================
        // 更新 + 渲染
        // ============================================================
        if (!transitioning) {
            scene.update(dt);
        }

        scene.render(*window_);

        updateTransition(dt);

        renderOverlays();
        window_->display();

        // ============================================================
        // 场景切换
        // ============================================================
        if (!transitioning) {
            SceneId next = scene.nextScene();
            if (next != SceneId::None && next != sceneManager_->currentId()) {
                switchScene(next);
            }
        }
    }

    SoundManager::instance().stopBGM();
    saveWindowState();
    flushConfigs();
    logger_->info("游戏结束");
}