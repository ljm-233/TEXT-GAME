#include "game.h"

Game::Game(std::shared_ptr<Window> window, std::shared_ptr<Logger> logger)
    : window_(window), logger_(logger) {}

void Game::run() {
    logger_->info("游戏启动");

    while (window_->isOpen()) {
        window_->pollEvents();

        window_->clear();
        // 以后在这里画游戏内容（精灵、地图、文字……）
        window_->display();
    }

    logger_->info("游戏结束");
}