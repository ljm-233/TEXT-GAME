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

void Game::switchScene(SceneId next) {
    if (next == SceneId::None) return;

    // 切换场景前把配置落盘一次
    flushConfigs();

    if (next == SceneId::Exit) {
        window_->close();
        return;
    }

    if (next == SceneId::Back) {
        if (history_.empty()) return;
        SceneId prev = history_.back();
        history_.pop_back();

        auto newScene = createScene(prev);
        currentScene_ = std::move(newScene);
        currentId_ = prev;
        return;
    }

    if (currentId_ != SceneId::None) {
        history_.push_back(currentId_);
    }

    auto newScene = createScene(next);
    currentScene_ = std::move(newScene);
    currentId_ = next;
}

void Game::saveWindowState() {
    if (!window_->isOpen()) return;
    if (!preferences_->getBool("remember_window_size", true)) {
        logger_->info("已关闭窗口大小记忆，跳过保存");
        return;
    }
    auto size = window_->native().getSize();
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(size.x));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(size.y));
    logger_->info("保存窗口状态: " +
                  std::to_string(size.x) + "x" + std::to_string(size.y));
}

void Game::renderFpsOverlay() {
    if (!preferences_->getBool("show_fps", false)) return;

    auto size = window_->native().getSize();
    fpsText_.setString("FPS: " + std::to_string(static_cast<int>(fpsDisplayed_)));
    auto b = fpsText_.getLocalBounds();
    fpsText_.setPosition({static_cast<float>(size.x) - b.size.x - 20.f, 8.f});
    window_->native().draw(fpsText_);
}

void Game::flushConfigs() {
    preferences_->flush();
    runtimeConfig_->flush();
}

void Game::run() {
    logger_->info("游戏启动");

    currentId_ = SceneId::MainMenu;
    currentScene_ = createScene(currentId_);

    sf::Clock clock;

    while (window_->isOpen()) {
        float dt = clock.restart().asSeconds();

        // FPS 统计
        fpsFrameCount_++;
        fpsElapsed_ += dt;
        if (fpsElapsed_ >= 0.5f) {
            fpsDisplayed_ = fpsFrameCount_ / fpsElapsed_;
            fpsFrameCount_ = 0;
            fpsElapsed_ = 0.f;
        }

        // 每 5 秒 flush 一次配置
        flushTimer_ += dt;
        if (flushTimer_ >= 5.f) {
            flushConfigs();
            flushTimer_ = 0.f;
        }

        window_->pollEvents([&](const sf::Event& e) {
            currentScene_->handleEvent(e);
        });

        currentScene_->update(dt);
        currentScene_->render(*window_);

        renderFpsOverlay();
        window_->display();

        SceneId next = currentScene_->nextScene();
        if (next != SceneId::None && next != currentId_) {
            switchScene(next);
        }
    }

    saveWindowState();
    flushConfigs();   // 退出前最后落盘

    logger_->info("游戏结束");
}