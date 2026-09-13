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
      runtimeConfig_(std::move(runtimeConfig)) {}

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
                background_, font, logger_);

        default:
            return nullptr;
    }
}

void Game::saveWindowState() {
    if (!window_->isOpen()) return;
    auto size = window_->native().getSize();
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(size.x));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(size.y));
    logger_->info("保存窗口状态: " +
                  std::to_string(size.x) + "x" + std::to_string(size.y));
}

void Game::run() {
    logger_->info("游戏启动");

    currentId_ = SceneId::MainMenu;
    currentScene_ = createScene(currentId_);

    sf::Clock clock;

    while (window_->isOpen()) {
        float dt = clock.restart().asSeconds();

        window_->pollEvents([&](const sf::Event& e) {
            currentScene_->handleEvent(e);
        });

        currentScene_->update(dt);
        currentScene_->render(*window_);

        SceneId next = currentScene_->nextScene();
        if (next == SceneId::Exit) {
            window_->close();
            break;
        }
        if (next != SceneId::None && next != currentId_) {
            currentId_ = next;
            currentScene_ = createScene(next);
        }
    }

    // 退出前保存窗口大小，下次启动时恢复
    saveWindowState();

    logger_->info("游戏结束");
}