#pragma once
#include <memory>
#include <string>
#include <vector>
#include <random>
#include "core/main_loop.h"
#include "log/logger.h"
#include "window.h"
#include "background.h"
#include "font_holder.h"
#include "save_manager.h"
#include "preferences.h"
#include "runtime_config.h"
#include "scene.h"
#include "scene_id.h"
#include "scene_manager.h"
#include "scene_registry.h"

// 游戏前端。
//
// 它同时就是本项目的 MainLoop 实现：核心只知道"有人会跑一个阻塞循环、
// 并且能被请求退出"，至于这个循环体是 SFML 窗口还是别的东西，核心不关心。
// 对应 albuswall 里 UI 插件通过 main_loop 属性交出主循环的做法。
//
// 它不认识任何具体场景：场景由 scene 层的 registerScenes() 登记进
// SceneRegistry，这里只按 id 向注册表要。
class Game : public MainLoop {
public:
    Game(std::shared_ptr<Window>         window,
         std::shared_ptr<Logger>         logger,
         std::shared_ptr<Background>     background,
         std::shared_ptr<FontHolder>     fontHolder,
         std::shared_ptr<SaveManager>    saveManager,
         std::shared_ptr<Preferences>    preferences,
         std::shared_ptr<RuntimeConfig>  runtimeConfig,
         std::shared_ptr<SceneRegistry>  sceneRegistry);

    /// 阻塞跑到窗口关闭，返回退出码。
    int run() override;

    /// 请求退出：关掉窗口，主循环下一轮条件判断就会结束。
    void quit() override;

    std::string str() const override;

private:
    void switchScene(SceneId next);
    void saveWindowState();
    void renderOverlays(float dt);
    void flushConfigs();
    void updateWindowTitle(const Scene& scene);

    std::shared_ptr<Window>         window_;
    std::shared_ptr<Logger>         logger_;
    std::shared_ptr<Background>     background_;
    std::shared_ptr<FontHolder>     fontHolder_;
    std::shared_ptr<SaveManager>    saveManager_;
    std::shared_ptr<Preferences>    preferences_;
    std::shared_ptr<RuntimeConfig>  runtimeConfig_;
    std::shared_ptr<SceneRegistry>  sceneRegistry_;

    std::unique_ptr<SceneManager>   sceneManager_;

    sf::Text fpsText_;
    sf::Text clockText_;

    int      fpsFrameCount_ = 0;
    float    fpsElapsed_    = 0.f;
    float    fpsDisplayed_  = 0.f;
    float    flushTimer_    = 0.f;
    float    clockTimer_    = 0.f;

    bool     autoPaused_    = false;

    std::string lastWindowTitle_;
};