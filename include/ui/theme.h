#pragma once
#include <SFML/Graphics.hpp>

enum class ThemeId { Dark = 0, Blue = 1, Light = 2 };
constexpr int kThemeCount = 3;

struct Theme {
    sf::Color background;
    sf::Color panelBg;
    sf::Color buttonNormal;
    sf::Color buttonHover;
    sf::Color buttonPressed;
    sf::Color buttonSelected;
    sf::Color textPrimary;
    sf::Color textSecondary;
    sf::Color outline;
};

const Theme& getTheme();
ThemeId      getThemeId();
void         setTheme(ThemeId id);
const char*  themeName(ThemeId id);