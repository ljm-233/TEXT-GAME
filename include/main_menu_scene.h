#pragma once
#include "scene.h"
#include "background.h"
#include <memory>

class MainMenuScene : public Scene {
public:
    explicit MainMenuScene(std::shared_ptr<Background> background);

    void handleEvent(const sf::Event& event) override;
    void render(Window& window) override;

    bool isFinished() const override { return finished_; }

private:
    std::shared_ptr<Background> background_;
    bool finished_ = false;
};