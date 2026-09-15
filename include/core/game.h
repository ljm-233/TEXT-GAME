#pragma once
#include <memory>
#include <vector>
#include <random>
#include "logging.h"
#include "window.h"
#include "background.h"
#include "font_holder.h"
#include "save_manager.h"
#include "preferences.h"
#include "runtime_config.h"
#include "scene.h"
#include "scene_id.h"
#include "scene_manager.h"

class Game {
public:
    Game(std::shared_ptr<Window>        window,
         std::shared_ptr<Logger>        logger,
         std::shared_ptr<Background>    background,
         std::shared_ptr<FontHolder>    fontHolder,
         std::shared_ptr<SaveManager>   saveManager,
         std::shared_ptr<Preferences>   preferences,
         std::shared_ptr<RuntimeConfig> runtimeConfig);

    void run();

private:
    std::unique_ptr<Scene> createScene(SceneId id);
    void switchScene(SceneId next);
    void saveWindowState();
    void renderOverlays();
    void flushConfigs();
    void updateTransition(float dt);
    void buildTransitionGeometry(float winW, float winH);

    std::shared_ptr<Window>        window_;
    std::shared_ptr<Logger>        logger_;
    std::shared_ptr<Background>    background_;
    std::shared_ptr<FontHolder>    fontHolder_;
    std::shared_ptr<SaveManager>   saveManager_;
    std::shared_ptr<Preferences>   preferences_;
    std::shared_ptr<RuntimeConfig> runtimeConfig_;

    std::unique_ptr<SceneManager>  sceneManager_;

    sf::Text fpsText_;
    sf::Text clockText_;

    int      fpsFrameCount_ = 0;
    float    fpsElapsed_    = 0.f;
    float    fpsDisplayed_  = 0.f;
    float    flushTimer_    = 0.f;
    float    clockTimer_    = 0.f;

    bool     autoPaused_    = false;

    // ===== 场景过渡（马赛克溶解） =====
    enum class TransitionPhase { None, FadingOut, FadingIn };

    static constexpr int kCellsX = 40;   // 横向格子数
    static constexpr int kCellsY = 24;   // 纵向格子数

    TransitionPhase transitionPhase_    = TransitionPhase::None;
    float           transitionProgress_ = 0.f;   // 0 ~ 1
    float           transitionSpeed_    = 2.5f;  // 每秒进度变化（越大越快）
    SceneId         pendingScene_       = SceneId::None;

    std::vector<float>  cellDelays_;    // 每格随机延迟 0 ~ 0.7
    sf::VertexArray     transitionVA_;  // 一次 draw 所有格子
};