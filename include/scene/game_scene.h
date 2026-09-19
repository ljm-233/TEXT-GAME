#pragma once
#include "scene.h"
#include "background.h"
#include "game_world.h"
#include "save_manager.h"
#include "preferences.h"
#include "pause_menu.h"
#include "parallax.h"
#include "level_intro.h"
#include "button.h"
#include <memory>
#include <vector>

class GameScene : public Scene {
public:
    GameScene(std::shared_ptr<Background>  background,
              const sf::Font&              font,
              std::shared_ptr<Logger>      logger,
              std::shared_ptr<SaveManager> saveManager,
              std::shared_ptr<Preferences> preferences);

    void onEnter() override;
    void onResume() override;

    std::string windowTitleHint() const override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    bool loadLevel(int index);
    bool advanceToNextLevel();
    void syncFocus();
    void subscribeWorldEvents();
    bool handleDebugKey(sf::Keyboard::Key k);
    void renderDebugHud(sf::RenderTarget& rt, Window& window);
    void renderPerfHud(sf::RenderTarget& rt, Window& window,
                       float winW, float winH);
    void refreshHud();
    void refreshOverlayLayout(float winW, float winH);
    void rebuildOverlayButtons();
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
    float hitstopTimer_ = 0.f;   // 受击停顿时长（秒）
    int   finalStars_ = 0;
    int   finalCoins_ = 0;
    int   finalTotalCoins_ = 0;
    float prevBestTime_ = 0.f;    // 本关之前的 PB（0 = 无记录）
    bool  newRecord_ = false;     // 本次是否刷新 PB

    // 屏幕闪光
    float     screenFlashTimer_ = 0.f;
    float     screenFlashDuration_ = 0.3f;
    sf::Color screenFlashColor_ = sf::Color::White;

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
    GameWorld::State lastFocusState_   = GameWorld::State::Playing;   // ⭐ 新增

    std::vector<std::unique_ptr<Button>> overlayButtons_;

    SceneId   nextScene_ = SceneId::None;

    // ⭐ 调试工具
    bool     debugHud_           = false;
    bool     debugInvincible_    = false;
    bool     debugShowColliders_ = false;
    bool     pendingScreenshot_  = false;
    float    debugTimeScale_     = 1.f;
    sf::Text debugText_;
    sf::View lastWorldView_;

    // ⭐ 性能面板
    bool      perfHud_        = false;
    float     perfFrameMs_    = 0.f;
    float     perfUpdateMs_   = 0.f;
    float     perfRenderMs_   = 0.f;
    sf::Clock perfFrameClock_;
    sf::Text  perfText_;

    static constexpr float kLogicalW = 1280.f;
    static constexpr float kLogicalH = 720.f;
    static constexpr int   kMaxLevels = 9;
};