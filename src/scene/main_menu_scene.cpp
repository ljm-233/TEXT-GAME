#include "main_menu_scene.h"
#include "text_strings.h"
#include "focus_group.h"
#include <algorithm>

MainMenuScene::MainMenuScene(std::shared_ptr<Background> background,
                             const sf::Font& font,
                             std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      startButton_       (Str::T(Str::StartGame),  font, {0.f, 0.f}, {280.f, 52.f}, 24),
      levelSelectButton_ (Str::T(Str::LevelSelect), font, {0.f, 0.f}, {280.f, 52.f}, 24),
      editorButton_      (Str::T(Str::LevelEditor), font, {0.f, 0.f}, {280.f, 52.f}, 24),
      calculatorButton_  (Str::T(Str::Calculator), font, {0.f, 0.f}, {280.f, 52.f}, 24),
      settingsButton_    (Str::T(Str::Settings),   font, {0.f, 0.f}, {280.f, 52.f}, 24),
      exitButton_        (Str::T(Str::ExitGame),   font, {0.f, 0.f}, {280.f, 52.f}, 24) {}

void MainMenuScene::refreshLabels() {
    startButton_.setText(Str::T(Str::StartGame));
    levelSelectButton_.setText(Str::T(Str::LevelSelect));
    editorButton_.setText(Str::T(Str::LevelEditor));
    calculatorButton_.setText(Str::T(Str::Calculator));
    settingsButton_.setText(Str::T(Str::Settings));
    exitButton_.setText(Str::T(Str::ExitGame));
}

void MainMenuScene::onEnter() {
    nextScene_ = SceneId::None;
    elapsed_ = 0.f;
    refreshLabels();
}

void MainMenuScene::onResume() {
    nextScene_ = SceneId::None;
    refreshLabels();
}

void MainMenuScene::handleEvent(const sf::Event& event) {
    startButton_.handleEvent(event);
    levelSelectButton_.handleEvent(event);
    editorButton_.handleEvent(event);
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
    if (editorButton_.consumeClick()) {
        logger_->info("点击: 关卡编辑器");
        nextScene_ = SceneId::Editor;
    }
    if (calculatorButton_.consumeClick()) {
        logger_->info("点击: 控制台");
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

    const float btnW = 280.f, btnH = 52.f, gap = 12.f;
    constexpr int kCount = 6;
    float totalH = btnH * kCount + gap * (kCount - 1);
    float startY = cy - totalH / 2.f;

    Button* btns[kCount] = {
        &startButton_, &levelSelectButton_, &editorButton_,
        &calculatorButton_, &settingsButton_, &exitButton_
    };

    for (int i = 0; i < kCount; ++i) {
        float targetY = startY + static_cast<float>(i) * (btnH + gap);

        float delay = static_cast<float>(i) * kButtonDelay;
        float t = std::clamp((elapsed_ - delay) / kButtonRise, 0.f, 1.f);
        float yOffset = (1.f - t) * 80.f;

        btns[i]->setPosition({cx - btnW / 2.f, targetY + yOffset});
        btns[i]->render(window.native());
    }

    FocusGroup::instance().setItems({
        &startButton_, &levelSelectButton_, &editorButton_,
        &calculatorButton_, &settingsButton_, &exitButton_
    });
}