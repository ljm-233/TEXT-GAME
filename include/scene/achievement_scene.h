#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include <memory>
#include <vector>

class AchievementScene : public Scene {
public:
    AchievementScene(std::shared_ptr<Background> background,
                     const sf::Font& font,
                     std::shared_ptr<Logger> logger);

    void onEnter() override;
    void onResume() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    void refreshLabels();
    void syncFocus();

    std::shared_ptr<Background> background_;
    std::shared_ptr<Logger> logger_;
    const sf::Font& font_;

    std::unique_ptr<Button> backButton_;
    sf::Text titleText_;
    sf::Text progressText_;

    std::vector<std::unique_ptr<sf::Text>> nameTexts_;
    std::vector<std::unique_ptr<sf::Text>> descTexts_;

    SceneId nextScene_ = SceneId::None;
    int     lastLangVersion_ = -1;
    int     lastUnlockedCount_ = -1;
};