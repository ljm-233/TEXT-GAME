#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "save_manager.h"
#include <memory>
#include <vector>

class LevelSelectScene : public Scene {
public:
    LevelSelectScene(std::shared_ptr<Background>  background,
                     std::shared_ptr<SaveManager> saveManager,
                     const sf::Font&              font,
                     std::shared_ptr<Logger>      logger);

    void onEnter() override;
    void onResume() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    void refreshSelection();

    std::shared_ptr<Background>  background_;
    std::shared_ptr<SaveManager> saveManager_;
    std::shared_ptr<Logger>      logger_;
    const sf::Font&              font_;

    SaveInfo save_;
    bool     hasSave_      = false;
    int      currentLevel_ = 1;

    std::vector<std::unique_ptr<Button>> levelButtons_;
    std::unique_ptr<Button> backButton_;

    sf::Text titleText_;
    sf::Text saveNameText_;
    sf::Text hintText_;
    sf::Text starText_;         // ⭐ 每关的星级

    SceneId nextScene_ = SceneId::None;

    static constexpr int kMaxLevels = 9;
};