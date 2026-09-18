#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "button.h"
#include "preferences.h"

class PauseMenu {
public:
    enum class Action { None, Resume, SaveAndQuit };

    PauseMenu(const sf::Font& font,
              std::shared_ptr<Preferences> prefs,
              sf::Vector2f windowSize);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);
    void relayout(sf::Vector2f windowSize);
    void syncFocus();              // ⭐ 新增

    Action consumeAction();

private:
    enum class View { Main, Settings };

    void refreshLabels();
    void switchToSettings();
    void switchToMain();
    void applyTheme(int idx);
    void applyAnimation(bool enabled);
    void applyNotification(bool enabled);
    void applyGamepad(bool enabled);
    void refreshSelection();

    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    View view_ = View::Main;
    Action pendingAction_ = Action::None;
    sf::Vector2f windowSize_;

    sf::RectangleShape backdrop_;
    sf::RectangleShape panel_;
    sf::Text title_;

    std::vector<std::unique_ptr<Button>> mainButtons_;

    sf::Text  settingsTitle_;
    sf::Text  labelTheme_;
    sf::Text  labelAnim_;
    sf::Text  labelNotif_;
    sf::Text  labelGamepad_;
    sf::Text  hintText_;
    std::vector<std::unique_ptr<Button>> themeButtons_;
    std::unique_ptr<Button> animOn_, animOff_;
    std::unique_ptr<Button> notifOn_, notifOff_;
    std::unique_ptr<Button> gamepadOn_, gamepadOff_;
    std::unique_ptr<Button> backButton_;
};