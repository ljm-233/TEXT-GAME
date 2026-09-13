#include "button.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include "button_style.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

Button::Button(const std::string& label,
               const sf::Font& font,
               sf::Vector2f position,
               sf::Vector2f size,
               unsigned characterSize)
    : text_(font, toSf(label), scaledFontSize(characterSize)),
      position_(position),
      size_(size) {
    shape_.setPosition(position_);
    text_.setFillColor(getTheme().textPrimary);
    refreshShape();
    refreshColor();
    centerText();
}

void Button::setPosition(sf::Vector2f p) {
    position_ = p;
    shape_.setPosition(position_);
    centerText();
}

void Button::setSize(sf::Vector2f s) {
    size_ = s;
    refreshShape();
    centerText();
}

void Button::setText(const std::string& text) {
    text_.setString(toSf(text));
    centerText();
}

void Button::setSelected(bool s) {
    selected_ = s;
    refreshColor();
}

void Button::refreshShape() {
    const auto& style = getButtonStyle();
    float maxR = std::min(size_.x, size_.y) / 2.f;
    float r = std::clamp(style.cornerRadius, 0.f, maxR);

    if (r < 0.5f) {
        // 直角矩形
        shape_.setPointCount(4);
        shape_.setPoint(0, {0.f, 0.f});
        shape_.setPoint(1, {size_.x, 0.f});
        shape_.setPoint(2, {size_.x, size_.y});
        shape_.setPoint(3, {0.f, size_.y});
    } else {
        const int seg = 5;
        const int total = seg * 4 + 4;
        shape_.setPointCount(total);

        int idx = 0;
        auto arc = [&](sf::Vector2f center, float startAngle) {
            for (int i = 0; i <= seg; ++i) {
                float a = startAngle + (kPi / 2.f) * i / seg;
                float x = center.x + r * std::cos(a);
                float y = center.y + r * std::sin(a);
                shape_.setPoint(idx++, {x, y});
            }
        };
        // 从左上角开始逆时针
        arc({r, r}, kPi);                  // 左上
        arc({size_.x - r, r}, -kPi / 2.f); // 右上
        arc({size_.x - r, size_.y - r}, 0.f);          // 右下
        arc({r, size_.y - r}, kPi / 2.f);              // 左下
    }

    shape_.setOutlineThickness(style.outlineThickness);
}

void Button::refreshColor() {
    const auto& t = getTheme();
    if (pressed_)       shape_.setFillColor(t.buttonPressed);
    else if (hovered_)  shape_.setFillColor(t.buttonHover);
    else if (selected_) shape_.setFillColor(t.buttonSelected);
    else                shape_.setFillColor(t.buttonNormal);

    shape_.setOutlineColor(t.outline);
    text_.setFillColor(t.textPrimary);
}

void Button::centerText() {
    auto b = text_.getLocalBounds();
    text_.setOrigin({b.position.x + b.size.x / 2.f,
                     b.position.y + b.size.y / 2.f});
    text_.setPosition({position_.x + size_.x / 2.f,
                       position_.y + size_.y / 2.f});
}

bool Button::contains(sf::Vector2f point) const {
    return point.x >= position_.x && point.x <= position_.x + size_.x &&
           point.y >= position_.y && point.y <= position_.y + size_.y;
}

void Button::handleEvent(const sf::Event& event) {
    if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        hovered_ = contains({static_cast<float>(mm->position.x),
                             static_cast<float>(mm->position.y)});
    }
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            hovered_ = contains({static_cast<float>(mb->position.x),
                                 static_cast<float>(mb->position.y)});
            if (hovered_) pressed_ = true;
        }
    }
    if (const auto* mb = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            bool inside = contains({static_cast<float>(mb->position.x),
                                    static_cast<float>(mb->position.y)});
            if (pressed_ && inside) clicked_ = true;
            pressed_ = false;
        }
    }
    refreshColor();
}

bool Button::consumeClick() {
    bool c = clicked_;
    clicked_ = false;
    return c;
}

void Button::render(sf::RenderTarget& target) {
    // 主题可能变了，每帧刷新一次颜色和形状（成本很低）
    refreshShape();
    refreshColor();
    target.draw(shape_);
    target.draw(text_);
}