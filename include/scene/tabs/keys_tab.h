#pragma once
#include "button.h"
#include "preferences.h"
#include "keybindings.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class KeysTab {
public:
    KeysTab(const sf::Font& font,
            std::shared_ptr<Preferences> prefs);

    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target,
                 float contentX, float ctrlX,
                 float startY);

    void refreshLabels();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const { return false; }

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;

    std::vector<std::unique_ptr<Button>> keyBindingButtons_;
    std::vector<std::unique_ptr<sf::Text>> keyLabels_;   // 每动作一个 label
    sf::Text hintText_;
    int listeningAction_ = -1;

    void refreshButtonText();
};