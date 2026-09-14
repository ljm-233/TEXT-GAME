#pragma once
#include "scene.h"
#include "background.h"
#include "game_world.h"
#include "save_manager.h"
#include "preferences.h"
#include "pause_menu.h"
#include <memory>

class GameScene : public Scene {
public:
    GameScene(std::shared_ptr<Background>  background,
              const sf::Font&              font,
              std::shared_ptr<Logger>      logger,
              SaveInfo                     save,
              std::shared_ptr<SaveManager> saveManager,
              std::shared_ptr<Preferences> preferences);

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    bool loadLevel(int index);
    void renderStateOverlay(sf::RenderTarget& rt, float winW, float winH);

    std::shared_ptr<Background>  background_;
    std::shared_ptr<Logger>      logger_;
    std::shared_ptr<SaveManager> saveManager_;
    std::shared_ptr<Preferences> preferences_;
    SaveInfo                     save_;
    const sf::Font*              font_ = nullptr;

    int levelIndex_ = 1;

    std::unique_ptr<GameWorld> world_;
    std::unique_ptr<PauseMenu> pauseMenu_;
    bool paused_ = false;

    sf::Text  hudText_;
    sf::Text  overlayTitle_;
    sf::Text  overlayHint_;
    sf::RectangleShape overlayBg_;

    SceneId   nextScene_ = SceneId::None;

    static constexpr float kLogicalW = 1280.f;
    static constexpr float kLogicalH = 720.f;
    static constexpr int   kMaxLevels = 9;
};