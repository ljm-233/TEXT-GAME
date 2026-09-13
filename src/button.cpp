#include "button.h"

namespace {
const sf::Color kNormal  {70, 70, 80};
const sf::Color kHover   {105, 105, 125};
const sf::Color kPressed {45, 45, 55};

// 把 UTF-8 编码的 std::string 转成 sf::String
sf::String toSf(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}
}

Button::Button(const std::string& label,
               const sf::Font& font,
               sf::Vector2f position,
               sf::Vector2f size,
               unsigned characterSize)
    : text_(font, toSf(label), characterSize),
      position_(position),
      size_(size) {
    shape_.setSize(size_);
    shape_.setPosition(position_);
    shape_.setFillColor(kNormal);
    shape_.setOutlineThickness(2.f);
    shape_.setOutlineColor(sf::Color::White);

    text_.setFillColor(sf::Color::White);
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

    if (pressed_)      shape_.setFillColor(kPressed);
    else if (hovered_) shape_.setFillColor(kHover);
    else               shape_.setFillColor(kNormal);
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