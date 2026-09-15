#pragma once
#include "scene.h"
#include "background.h"
#include "game_world.h"
#include "save_manager.h"
#include "preferences.h"
#include "pause_menu.h"
#include "parallax.h"
#include "level_intro.h"
#include <memory>

class GameScene : public Scene {
public:
    GameScene(std::shared_ptr<Background>  background,
              const sf::Font&              font,
              std::shared_ptr<Logger>      logger,
              std::shared_ptr<SaveManager> saveManager,
              std::shared_ptr<Preferences> preferences);

    void onEnter() override;
    void onResume() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    bool loadLevel(int index);
    void subscribeWorldEvents();
    void refreshHud();
    void refreshOverlayLayout(float winW, float winH);
    void renderStateOverlay(sf::RenderTarget& rt, float winW, float winH);

    int  calcStars() const;
    int  targetTime() const;
    void applyStars();

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

    float levelTime_ = 0.f;
    int   finalStars_ = 0;
    int   finalCoins_ = 0;
    int   finalTotalCoins_ = 0;

    std::unique_ptr<ParallaxBackground> parallax_;
    std::unique_ptr<LevelIntro>         intro_;

    sf::View worldView_;
    float    lastViewWinW_ = 0.f;
    float    lastViewWinH_ = 0.f;

    sf::Text hudText_;
    int      lastHudLives_ = -1;
    int      lastHudCoins_ = -1;
    int      lastHudLevel_ = -1;

    sf::Text  overlayTitle_;
    sf::Text  overlayHint_;
    sf::Text  overlaySubHint_;
    sf::Text  overlayTime_;
    sf::Text  overlayStars_;
    sf::RectangleShape overlayBg_;
    float     lastOverlayWinW_ = 0.f;
    float     lastOverlayWinH_ = 0.f;
    GameWorld::State lastOverlayState_ = GameWorld::State::Playing;

    SceneId   nextScene_ = SceneId::None;

    static constexpr float kLogicalW = 1280.f;
    static constexpr float kLogicalH = 720.f;
    static constexpr int   kMaxLevels = 9;
};