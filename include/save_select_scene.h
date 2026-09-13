#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "save_manager.h"
#include <memory>
#include <vector>

class SaveSelectScene : public Scene {
public:
    SaveSelectScene(std::shared_ptr<Background> background,
                    std::shared_ptr<SaveManager> saveManager,
                    const sf::Font& font,
                    std::shared_ptr<Logger> logger);

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    std::shared_ptr<Background> background_;
    std::shared_ptr<SaveManager> saveManager_;
    std::shared_ptr<Logger> logger_;
    const sf::Font& font_;

    std::vector<SaveInfo> saves_;
    std::vector<std::unique_ptr<Button>> saveButtons_;
    std::unique_ptr<Button> newButton_;
    std::unique_ptr<Button> backButton_;

    SceneId nextScene_ = SceneId::None;
};