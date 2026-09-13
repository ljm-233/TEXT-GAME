#include "game.h"
#include "main_menu_scene.h"

Game::Game(std::shared_ptr<Window>     window,
           std::shared_ptr<Logger>     logger,
           std::shared_ptr<Background> background)
    : window_(std::move(window)),
      logger_(std::move(logger)),
      background_(std::move(background)) {}

void Game::run() {
    logger_->info("游戏启动");

    // 主界面：一直显示，直到用户关窗口
    MainMenuScene menu(background_);

    while (window_->isOpen()) {
        window_->pollEvents([&](const sf::Event& e) {
            menu.handleEvent(e);
        });
        menu.render(*window_);
    }

    logger_->info("窗口关闭，游戏结束");
}