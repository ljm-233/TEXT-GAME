#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "button.h"
#include "preferences.h"

class PauseMenu {
public:
    enum class Action { None, Resume, SaveAndQuit, OpenSettings };

    PauseMenu(const sf::Font& font,
              std::shared_ptr<Preferences> prefs,
              sf::Vector2f windowSize);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);
    void relayout(sf::Vector2f windowSize);
    void syncFocus();
    void reset();

    Action consumeAction();

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;  // 保留（当前未使用，供将来扩展）
    Action pendingAction_ = Action::None;
    sf::Vector2f windowSize_;

    sf::RectangleShape backdrop_;
    sf::RectangleShape panel_;
    sf::Text title_;

    std::vector<std::unique_ptr<Button>> mainButtons_;

    void refreshLabels();
};