#include "slider.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include <algorithm>
#include <cmath>
#include <string>

Slider::Slider(const sf::Font& font, float minValue, float maxValue, float initialValue,
               sf::Vector2f position, sf::Vector2f size)
      : valueText_(font, sf::String(), scaledFontSize(20)),
        position_(position),
        size_(size),
        min_(minValue),
        max_(maxValue),
        value_(std::clamp(initialValue, minValue, maxValue)) {
    track_.setSize(size_);
    track_.setPosition(position_);
    track_.setFillColor(getTheme().buttonNormal);
    track_.setOutlineThickness(1.f);
    track_.setOutlineColor(getTheme().outline);

    fill_.setSize({0.f, size_.y});
    fill_.setFillColor(getTheme().buttonSelected);

    handle_.setSize({10.f, size_.y + 4.f});
    handle_.setFillColor(getTheme().textPrimary);

    valueText_.setFillColor(getTheme().textPrimary);

    updateLayout();
}

void Slider::setValue(float v) {
    value_ = std::clamp(v, min_, max_);
    updateLayout();
}

void Slider::setPosition(sf::Vector2f p) {
    position_ = p;
    track_.setPosition(position_);
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
    valueText_.setPosition(
        {position_.x + w + 16.f, position_.y + h / 2.f - b.size.y / 2.f - b.position.y});
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
    // 每帧刷新颜色（主题可能变了）
    track_.setFillColor(getTheme().buttonNormal);
    track_.setOutlineColor(getTheme().outline);
    fill_.setFillColor(getTheme().buttonSelected);
    handle_.setFillColor(getTheme().textPrimary);
    valueText_.setFillColor(getTheme().textPrimary);

    target.draw(track_);
    target.draw(fill_);
    target.draw(handle_);
    target.draw(valueText_);
}