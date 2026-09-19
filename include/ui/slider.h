#pragma once
#include <SFML/Graphics.hpp>
#include "text_input.h"
#include "button.h"
#include <memory>

class Slider {
public:
    Slider(const sf::Font& font, float minValue, float maxValue, float initialValue,
           sf::Vector2f position, sf::Vector2f size);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);

    float value() const { return value_; }
    void setValue(float v);
    bool consumeChanged();
    void setPosition(sf::Vector2f p);

    // ⭐ 设置默认值（重置按钮回到这个值）
    void setDefaultValue(float v) { defaultValue_ = v; }

    // ⭐ 输入框是否处于编辑状态
    bool isEditing() const { return valueInput_ && valueInput_->isFocused(); }

private:
    void updateLayout();
    void setValueFromMouse(float mouseX);
    void setValueFromText(float v);

    sf::RectangleShape track_;
    sf::RectangleShape fill_;
    sf::RectangleShape handle_;

    sf::Vector2f position_;
    sf::Vector2f size_;
    float min_, max_, value_;
    float defaultValue_ = 0.f;
    bool dragging_ = false;
    bool changed_ = false;

    // ⭐ 数字输入框
    std::unique_ptr<TextInput> valueInput_;
    // ⭐ 重置按钮
    std::unique_ptr<Button> resetButton_;
};