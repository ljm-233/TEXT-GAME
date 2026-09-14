#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class Button {
public:
    Button(const std::string& label, const sf::Font& font, sf::Vector2f position,
           sf::Vector2f size, unsigned characterSize = 28);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);

    bool consumeClick();

    sf::Vector2f position() const { return position_; }
    sf::Vector2f size() const { return size_; }
    void setPosition(sf::Vector2f p);
    void setSize(sf::Vector2f s);
    void setText(const std::string& text);
    void setSelected(bool s);

private:
    bool contains(sf::Vector2f point) const;
    void centerText();
    void refreshShape();
    void updateColors(float dt);

    sf::ConvexShape shape_;
    sf::Text text_;
    sf::Vector2f position_;
    sf::Vector2f size_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool clicked_ = false;
    bool selected_ = false;

    // ===== 动画状态 =====
    sf::Clock animClock_;
    bool colorsInitialized_ = false;
    sf::Color currentFill_;
    sf::Color currentOutline_;
    sf::Color currentText_;
};