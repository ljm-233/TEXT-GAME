#pragma once
#include <SFML/Window/Keyboard.hpp>

// 全局键位表（和 getTheme() 一样是全局单例）
class KeyBindings {
public:
    enum Action {
        MoveLeft = 0,
        MoveRight,
        Jump,
        Pause,
        Restart,
        Count
    };

    static KeyBindings& instance();

    sf::Keyboard::Key get(Action a) const;
    void set(Action a, sf::Keyboard::Key key);

    static const char* actionName(Action a);
    static const char* keyToString(sf::Keyboard::Key k);

    void resetToDefaults();

private:
    KeyBindings();
    sf::Keyboard::Key keys_[Count];
};