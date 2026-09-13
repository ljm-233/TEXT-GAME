#include "game.h"
#include "main_menu_scene.h"
#include "save_select_scene.h"
#include "game_scene.h"
#include "settings_scene.h"
#include "console_scene.h"
#include <SFML/System/Clock.hpp>

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
      fpsText_(fontHolder_->get(), sf::String("FPS: 0"), 20) {
    fpsText_.setFillColor(sf::Color(255, 255, 100));
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
        case SceneId::Game:
            return std::make_unique<GameScene>(
                background_, font, logger_, saveManager_->takePendingSave());
        case SceneId::Settings:
            return std::make_unique<SettingsScene>(
                background_, preferences_, runtimeConfig_,
                window_, font, logger_);
        case SceneId::Console:
            return std::make_unique<ConsoleScene>(
                background_, preferences_, font, logger_);
        default:
            return nullptr;
    }
}

void Game::handleTransition(SceneId next) {
    flushConfigs();
    if (next == SceneId::Exit) {
        window_->close();
        return;
    }
    if (next == SceneId::Back) {
        sceneManager_->pop();
        return;
    }
    sceneManager_->push(next);
}

void Game::saveWindowState() {
    if (!window_->isOpen()) return;
    if (!preferences_->getBool("remember_window_size", true)) return;
    auto size = window_->native().getSize();
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(size.x));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(size.y));
}

void Game::renderFpsOverlay() {
    if (!preferences_->getBool("show_fps", false)) return;

    auto size = window_->native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    fpsText_.setString("FPS: " + std::to_string(static_cast<int>(fpsDisplayed_)));
    auto b = fpsText_.getLocalBounds();

    const float margin = 20.f;
    int pos = preferences_->getInt("fps_position", 1);

    sf::Vector2f p;
    switch (pos) {
        case 0: p = {margin, margin}; break;
        case 1: p = {w - b.size.x - margin, margin}; break;
        case 2: p = {margin, h - b.size.y - margin}; break;
        case 3: p = {w - b.size.x - margin, h - b.size.y - margin}; break;
        default: p = {w - b.size.x - margin, margin};
    }
    fpsText_.setPosition(p);
    window_->native().draw(fpsText_);
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

    sf::Clock clock;
    while (window_->isOpen()) {
        float dt = clock.restart().asSeconds();

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
        window_->pollEvents([&](const sf::Event& e) { scene.handleEvent(e); });
        scene.update(dt);
        scene.render(*window_);
        renderFpsOverlay();
        window_->display();

        SceneId next = scene.nextScene();
        if (next != SceneId::None && next != sceneManager_->currentId()) {
            handleTransition(next);
        }
    }

    saveWindowState();
    flushConfigs();
    logger_->info("游戏结束");
}