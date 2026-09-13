#include "button.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"

Button::Button(const std::string& label,
               const sf::Font& font,
               sf::Vector2f position,
               sf::Vector2f size,
               unsigned characterSize)
    : text_(font, toSf(label), scaledFontSize(characterSize)),
      position_(position),
      size_(size) {
    shape_.setSize(size_);
    shape_.setPosition(position_);
    shape_.setOutlineThickness(2.f);

    text_.setFillColor(getTheme().textPrimary);
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
    shape_.setSize(size_);
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
    target.draw(shape_);
    target.draw(text_);
}