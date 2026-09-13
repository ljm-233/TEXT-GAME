#pragma once
#include <SFML/Graphics.hpp>

class Slider {
public:
    Slider(const sf::Font& font,
           float minValue, float maxValue, float initialValue,
           sf::Vector2f position, sf::Vector2f size);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);

    float value() const { return value_; }
    void setValue(float v);
    bool consumeChanged();
    void setPosition(sf::Vector2f p);

private:
    void updateLayout();
    void setValueFromMouse(float mouseX);

    sf::RectangleShape track_;
    sf::RectangleShape fill_;
    sf::RectangleShape handle_;
    sf::Text           valueText_;

    sf::Vector2f position_;
    sf::Vector2f size_;
    float min_, max_, value_;
    bool dragging_ = false;
    bool changed_  = false;
};