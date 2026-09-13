#pragma once
#include "scene.h"
#include "background.h"
#include "game_world.h"
#include "save_manager.h"
#include <memory>

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
    std::shared_ptr<Logger>     logger_;
    SaveInfo                    save_;

    std::unique_ptr<GameWorld> world_;

    sf::Text  hudText_;
    SceneId   nextScene_ = SceneId::None;

    // 固定逻辑分辨率
    static constexpr float kLogicalW = 1280.f;
    static constexpr float kLogicalH = 720.f;
};