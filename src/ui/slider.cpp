#include "slider.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float kInputW = 70.f;
constexpr float kInputH = 26.f;
constexpr float kResetW = 26.f;
constexpr float kResetH = 26.f;
constexpr float kGap    = 8.f;
}

Slider::Slider(const sf::Font& font, float minValue, float maxValue, float initialValue,
               sf::Vector2f position, sf::Vector2f size)
      : position_(position),
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

    // ⭐ 数字输入框
    valueInput_ = std::make_unique<TextInput>(
        font, sf::Vector2f{0.f, 0.f}, sf::Vector2f{kInputW, kInputH},
        "", 16, 8);
    valueInput_->setText(std::to_string(static_cast<int>(value_)));
    valueInput_->setOnSubmit([this](const std::string& s) {
        try {
            float v = std::stof(s);
            setValueFromText(v);
        } catch (...) {
            valueInput_->setText(std::to_string(static_cast<int>(value_)));
        }
        valueInput_->setFocused(false);
    });

    // ⭐ 重置按钮
    resetButton_ = std::make_unique<Button>(
        "R", font, sf::Vector2f{0.f, 0.f},
        sf::Vector2f{kResetW, kResetH}, 16);

    updateLayout();
}

void Slider::setValue(float v) {
    value_ = std::clamp(v, min_, max_);
    updateLayout();
}

void Slider::setValueFromText(float v) {
    v = std::clamp(v, min_, max_);
    if (std::abs(v - value_) > 0.001f) {
        value_ = v;
        changed_ = true;
        updateLayout();
    }
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

    // ⭐ 数字输入框
    float inputX = position_.x + w + 12.f;
    float inputY = position_.y + h / 2.f - kInputH / 2.f;
    valueInput_->setPosition({inputX, inputY});
    valueInput_->setSize({kInputW, kInputH});

    // ⭐ 重置按钮（输入框右侧）
    float resetX = inputX + kInputW + kGap;
    float resetY = position_.y + h / 2.f - kResetH / 2.f;
    resetButton_->setPosition({resetX, resetY});

    // ⭐ 值变了就同步显示（用户正在编辑时不打断）
    if (!valueInput_->isFocused()) {
        std::string newText = std::to_string(static_cast<int>(value_));
        if (valueInput_->text() != newText) {
            valueInput_->setText(newText);
        }
    }
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
    // ⭐ 先交给输入框和重置按钮
    valueInput_->handleEvent(event);
    resetButton_->handleEvent(event);

    // ⭐ 重置按钮被点击
    if (resetButton_->consumeClick()) {
        setValueFromText(defaultValue_);
        valueInput_->setFocused(false);
        return;
    }

    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            float mx = static_cast<float>(mb->position.x);
            float my = static_cast<float>(mb->position.y);

            // 点击落在输入框上：不进入拖拽
            auto ip = valueInput_->position();
            auto is = valueInput_->size();
            if (mx >= ip.x && mx <= ip.x + is.x &&
                my >= ip.y && my <= ip.y + is.y) {
                return;
            }

            // 点击落在重置按钮上：不进入拖拽
            auto rp = resetButton_->position();
            auto rs = resetButton_->size();
            if (mx >= rp.x && mx <= rp.x + rs.x &&
                my >= rp.y && my <= rp.y + rs.y) {
                return;
            }

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
    track_.setFillColor(getTheme().buttonNormal);
    track_.setOutlineColor(getTheme().outline);
    fill_.setFillColor(getTheme().buttonSelected);
    handle_.setFillColor(getTheme().textPrimary);

    target.draw(track_);
    target.draw(fill_);
    target.draw(handle_);
    valueInput_->render(target);
    resetButton_->render(target);
}