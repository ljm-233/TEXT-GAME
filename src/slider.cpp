#include "slider.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
const sf::Color kTrackColor {60, 60, 80};
const sf::Color kFillColor  {100, 150, 220};
const sf::Color kHandleColor{230, 230, 240};

sf::String toSf(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}
}

Slider::Slider(const sf::Font& font,
               float minValue, float maxValue, float initialValue,
               sf::Vector2f position, sf::Vector2f size)
    : valueText_(font, sf::String(), 20),
      position_(position), size_(size),
      min_(minValue), max_(maxValue),
      value_(std::clamp(initialValue, minValue, maxValue)) {

    track_.setSize(size_);
    track_.setPosition(position_);
    track_.setFillColor(kTrackColor);
    track_.setOutlineThickness(1.f);
    track_.setOutlineColor(sf::Color(90, 90, 120));

    fill_.setSize({0.f, size_.y});
    fill_.setFillColor(kFillColor);

    handle_.setSize({10.f, size_.y + 4.f});
    handle_.setFillColor(kHandleColor);

    valueText_.setFillColor(sf::Color(230, 230, 240));

    updateLayout();
}

void Slider::setValue(float v) {
    value_ = std::clamp(v, min_, max_);
    updateLayout();
}

void Slider::updateLayout() {
    float ratio = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.f;
    float w = size_.x;
    float h = size_.y;

    fill_.setSize({ratio * w, h});
    fill_.setPosition(position_);

    float handleW = 10.f;
    float handleX = position_.x + ratio * w - handleW / 2.f;
    handle_.setPosition({handleX, position_.y - 2.f});

    std::string v = std::to_string(static_cast<int>(value_));
    valueText_.setString(toSf(v));
    auto b = valueText_.getLocalBounds();
    valueText_.setPosition({
        position_.x + w + 16.f,
        position_.y + h / 2.f - b.size.y / 2.f - b.position.y
    });
}

void Slider::setValueFromMouse(float mouseX) {
    float ratio = (mouseX - position_.x) / size_.x;
    ratio = std::clamp(ratio, 0.f, 1.f);
    float newValue = min_ + ratio * (max_ - min_);
    if (std::abs(newValue - value_) > 0.5f) {
        value_ = newValue;
        changed_ = true;
        updateLayout();
    }
}

void Slider::handleEvent(const sf::Event& event) {
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            float mx = static_cast<float>(mb->position.x);
            float my = static_cast<float>(mb->position.y);
            if (mx >= position_.x && mx <= position_.x + size_.x &&
                my >= position_.y - 8.f && my <= position_.y + size_.y + 8.f) {
                dragging_ = true;
                setValueFromMouse(mx);
            }
        }
    }
    if (const auto* mb = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            dragging_ = false;
        }
    }
    if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        if (dragging_) {
            setValueFromMouse(static_cast<float>(mm->position.x));
        }
    }
}

bool Slider::consumeChanged() {
    bool c = changed_;
    changed_ = false;
    return c;
}

void Slider::render(sf::RenderTarget& target) {
    target.draw(track_);
    target.draw(fill_);
    target.draw(handle_);
    target.draw(valueText_);
}

void Slider::setPosition(sf::Vector2f p) {
    position_ = p;
    track_.setPosition(position_);
    updateLayout();
}