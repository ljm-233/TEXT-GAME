#include "main_menu_scene.h"
#include "strings.h"
#include "animation.h"
#include <algorithm>

MainMenuScene::MainMenuScene(std::shared_ptr<Background> background,
                             const sf::Font& font,
                             std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      startButton_       (Str::StartGame,  font, {0.f, 0.f}, {280.f, 56.f}, 26),
      levelSelectButton_ (Str::LevelSelect, font, {0.f, 0.f}, {280.f, 56.f}, 26),
      calculatorButton_  (Str::Calculator, font, {0.f, 0.f}, {280.f, 56.f}, 26),
      settingsButton_    (Str::Settings,   font, {0.f, 0.f}, {280.f, 56.f}, 26),
      exitButton_        (Str::ExitGame,   font, {0.f, 0.f}, {280.f, 56.f}, 26) {}

void MainMenuScene::handleEvent(const sf::Event& event) {
    startButton_.handleEvent(event);
    levelSelectButton_.handleEvent(event);
    calculatorButton_.handleEvent(event);
    settingsButton_.handleEvent(event);
    exitButton_.handleEvent(event);
}

void MainMenuScene::update(float dt) {
    elapsed_ += dt;

    if (startButton_.consumeClick()) {
        logger_->info("点击: 启动游戏");
        nextScene_ = SceneId::SaveSelect;
    }
    if (levelSelectButton_.consumeClick()) {
        logger_->info("点击: 选关");
        nextScene_ = SceneId::LevelSelect;
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

    const float btnW = 280.f, btnH = 56.f, gap = 14.f;
    float totalH = btnH * 5 + gap * 4;
    float startY = cy - totalH / 2.f;

    Button* btns[] = {&startButton_, &levelSelectButton_, &calculatorButton_,
                      &settingsButton_, &exitButton_};

    for (int i = 0; i < 5; ++i) {
        float targetY = startY + static_cast<float>(i) * (btnH + gap);

        float delay = static_cast<float>(i) * kButtonDelay;
        float t = std::clamp((elapsed_ - delay) / kButtonRise, 0.f, 1.f);
        float yOffset = (1.f - t) * 80.f;

        btns[i]->setPosition({cx - btnW / 2.f, targetY + yOffset});
        btns[i]->render(window.native());
    }
}