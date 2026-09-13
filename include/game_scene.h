#pragma once
#include "scene.h"
#include "background.h"
#include "save_manager.h"
#include <memory>

// 游戏主场景（占位）：显示存档信息，游戏内容待实现
class GameScene : public Scene {
public:
    GameScene(std::shared_ptr<Background> background,
              const sf::Font& font,
              std::shared_ptr<Logger> logger,
              SaveInfo save);

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    std::shared_ptr<Background> background_;
    std::shared_ptr<Logger> logger_;
    SaveInfo save_;
    sf::Text infoText_;
    SceneId nextScene_ = SceneId::None;
};