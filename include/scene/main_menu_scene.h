#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include <memory>
#include <vector>

class MainMenuScene : public Scene {
public:
    MainMenuScene(std::shared_ptr<Background> background,
                  const sf::Font& font,
                  std::shared_ptr<Logger> logger);

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    std::shared_ptr<Background> background_;
    std::shared_ptr<Logger> logger_;
    Button startButton_;
    Button levelSelectButton_;
    Button calculatorButton_;
    Button settingsButton_;
    Button exitButton_;

    float elapsed_ = 0.f;
    static constexpr float kButtonDelay = 0.10f;
    static constexpr float kButtonRise  = 0.4f;

    SceneId nextScene_ = SceneId::None;
};