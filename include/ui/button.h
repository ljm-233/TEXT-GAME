#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class Button {
public:
    Button(const std::string& label,
           const sf::Font& font,
           sf::Vector2f position,
           sf::Vector2f size,
           unsigned characterSize = 28);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);

    bool consumeClick();

    sf::Vector2f position() const { return position_; }
    sf::Vector2f size() const     { return size_; }
    void setPosition(sf::Vector2f p);
    void setSize(sf::Vector2f s);
    void setText(const std::string& text);
    void setSelected(bool s);

    // ===== 焦点（手柄导航用）=====
    void setFocused(bool f);
    bool isFocused() const { return focused_; }

    // 外部触发点击（手柄 A 键用）
    void triggerClick();

private:
    bool contains(sf::Vector2f point) const;
    void recomputeTextOrigin();   // 只在 setSize / setText / 构造时调
    void updateTextPosition();    // 只在 setPosition / setSize / setText 时调
    void refreshShape();
    void updateColors(float dt);

    sf::ConvexShape shape_;
    sf::Text       text_;
    sf::Vector2f   position_;
    sf::Vector2f   size_;
    bool hovered_  = false;
    bool pressed_  = false;
    bool clicked_  = false;
    bool selected_ = false;
    bool focused_  = false;

    sf::Clock animClock_;
    bool      colorsInitialized_ = false;
    sf::Color currentFill_;
    sf::Color currentOutline_;
    sf::Color currentText_;

    // ⭐ 缓存 refreshShape 的输入，避免每帧重建多边形
    sf::Vector2f lastShapeSize_{-1.f, -1.f};
    bool         lastShapeFocused_ = false;
    float        lastShapeCorner_ = -1.f;
    float        lastShapeOutline_ = -1.f;
};