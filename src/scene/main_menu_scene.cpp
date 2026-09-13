#include "main_menu_scene.h"
#include "strings.h"

MainMenuScene::MainMenuScene(std::shared_ptr<Background> background,
                             const sf::Font& font,
                             std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      startButton_     (Str::StartGame,  font, {0.f, 0.f}, {280.f, 70.f}, 30),
      calculatorButton_(Str::Calculator, font, {0.f, 0.f}, {280.f, 70.f}, 30),
      settingsButton_  (Str::Settings,   font, {0.f, 0.f}, {280.f, 70.f}, 30),
      exitButton_      (Str::ExitGame,   font, {0.f, 0.f}, {280.f, 70.f}, 30) {}

void MainMenuScene::handleEvent(const sf::Event& event) {
    startButton_.handleEvent(event);
    calculatorButton_.handleEvent(event);
    settingsButton_.handleEvent(event);
    exitButton_.handleEvent(event);
}

void MainMenuScene::update(float /*dt*/) {
    if (startButton_.consumeClick()) {
        logger_->info("点击: 启动游戏");
        nextScene_ = SceneId::SaveSelect;
    }
    if (calculatorButton_.consumeClick()) {
        logger_->info("点击: 计算器");
        nextScene_ = SceneId::Console;
    }
    if (settingsButton_.consumeClick()) {
        logger_->info("点击: 设置");
        nextScene_ = SceneId::Settings;
    }
    if (exitButton_.consumeClick()) {
        logger_->info("点击: 退出游戏");
        nextScene_ = SceneId::Exit;
    }
}

void MainMenuScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float cx = static_cast<float>(size.x) / 2.f;
    float cy = static_cast<float>(size.y) / 2.f;

    const float btnW = 280.f, btnH = 70.f, gap = 20.f;
    float totalH = btnH * 4 + gap * 3;
    float startY = cy - totalH / 2.f;

    startButton_.setPosition     ({cx - btnW / 2.f, startY});
    calculatorButton_.setPosition({cx - btnW / 2.f, startY + (btnH + gap)});
    settingsButton_.setPosition  ({cx - btnW / 2.f, startY + 2 * (btnH + gap)});
    exitButton_.setPosition      ({cx - btnW / 2.f, startY + 3 * (btnH + gap)});

    startButton_.render(window.native());
    calculatorButton_.render(window.native());
    settingsButton_.render(window.native());
    exitButton_.render(window.native());
}