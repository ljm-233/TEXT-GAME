#include "game.h"
#include "main_menu_scene.h"
#include "save_select_scene.h"
#include "game_scene.h"
#include <SFML/System/Clock.hpp>

Game::Game(std::shared_ptr<Window>      window,
           std::shared_ptr<Logger>      logger,
           std::shared_ptr<Background>  background,
           std::shared_ptr<FontHolder>  fontHolder,
           std::shared_ptr<SaveManager> saveManager)
    : window_(std::move(window)),
      logger_(std::move(logger)),
      background_(std::move(background)),
      fontHolder_(std::move(fontHolder)),
      saveManager_(std::move(saveManager)) {}

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

        default:
            return nullptr;
    }
}

void Game::run() {
    logger_->info("游戏启动");

    // 初始场景：主菜单
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
            logger_->info("场景切换: " + std::to_string(static_cast<int>(currentId_))
                          + " -> " + std::to_string(static_cast<int>(next)));
            currentId_ = next;
            currentScene_ = createScene(next);
        }
    }

    logger_->info("游戏结束");
}