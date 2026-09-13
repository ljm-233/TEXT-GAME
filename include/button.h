#pragma once
#include <SFML/Graphics.hpp>
#include <string>

// 纯色矩形按钮 + 文字，支持悬停和点击
class Button {
public:
    Button(const std::string& label,
           const sf::Font& font,
           sf::Vector2f position,
           sf::Vector2f size,
           unsigned characterSize = 28);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);

    // 消费式读取：调用后复位，避免重复触发
    bool consumeClick();

    sf::Vector2f position() const { return position_; }
    sf::Vector2f size() const     { return size_; }
    void setPosition(sf::Vector2f p);
    void setSize(sf::Vector2f s);
    void setText(const std::string& text);

private:
    bool contains(sf::Vector2f point) const;
    void centerText();

    sf::RectangleShape shape_;
    sf::Text text_;
    sf::Vector2f position_;
    sf::Vector2f size_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool clicked_ = false;
};