#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "preferences.h"
#include "runtime_config.h"
#include "window.h"
#include "resolution.h"
#include <memory>
#include <vector>

class SettingsScene : public Scene {
public:
    SettingsScene(std::shared_ptr<Background>    background,
                  std::shared_ptr<Preferences>   preferences,
                  std::shared_ptr<RuntimeConfig> runtimeConfig,
                  std::shared_ptr<Window>        window,
                  const sf::Font&                font,
                  std::shared_ptr<Logger>        logger);

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    void applyResolution();
    void applyFullscreen();
    void refreshSelection();

    std::shared_ptr<Background>    background_;
    std::shared_ptr<Preferences>   preferences_;
    std::shared_ptr<RuntimeConfig> runtimeConfig_;
    std::shared_ptr<Window>        window_;
    std::shared_ptr<Logger>        logger_;

    std::vector<std::unique_ptr<Button>> resolutionButtons_;
    std::unique_ptr<Button> fullscreenOn_;
    std::unique_ptr<Button> fullscreenOff_;
    std::unique_ptr<Button> backButton_;

    int    selectedResolution_;
    bool   fullscreen_;
    SceneId nextScene_ = SceneId::None;
};