#include "main_menu_scene.h"

MainMenuScene::MainMenuScene(std::shared_ptr<Background> background,
                             const sf::Font& font,
                             std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      startButton_("启动游戏", font, {0.f, 0.f}, {280.f, 80.f}, 32) {}

void MainMenuScene::handleEvent(const sf::Event& event) {
    startButton_.handleEvent(event);
}

void MainMenuScene::update(float /*dt*/) {
    if (startButton_.consumeClick()) {
        logger_->info("点击: 启动游戏");
        nextScene_ = SceneId::SaveSelect;
    }
}

void MainMenuScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    sf::Vector2f btnSize = startButton_.size();
    startButton_.setPosition({
        (static_cast<float>(size.x) - btnSize.x) / 2.f,
        (static_cast<float>(size.y) - btnSize.y) / 2.f
    });
    startButton_.render(window.native());

    window.display();
}