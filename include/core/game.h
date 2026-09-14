#pragma once
#include <memory>
#include <vector>
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

    // ===== 场景过渡 =====
    enum class TransitionPhase { None, FadingOut, FadingIn };
    TransitionPhase transitionPhase_  = TransitionPhase::None;
    float           transitionAlpha_  = 0.f;
    float           transitionSpeed_  = 4.f;   // 每秒多少 alpha（0~1）
    SceneId         pendingScene_     = SceneId::None;
    sf::RectangleShape transitionOverlay_;
};