#include "button.h"
#include "theme.h"
#include "sound_manager.h"
#include "ui_scale.h"
#include "utf8.h"
#include "button_style.h"
#include "animation.h"
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
}

void Button::setFocused(bool f) {
    focused_ = f;
}

void Button::refreshShape() {
    const auto& style = getButtonStyle();
    float maxR = std::min(size_.x, size_.y) / 2.f;
    float r = std::clamp(style.cornerRadius, 0.f, maxR);

    if (r < 0.5f) {
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
        arc({r, r}, kPi);
        arc({size_.x - r, r}, -kPi / 2.f);
        arc({size_.x - r, size_.y - r}, 0.f);
        arc({r, size_.y - r}, kPi / 2.f);
    }

    // 焦点按钮：加粗描边
    float outline = style.outlineThickness;
    if (focused_) outline = std::max(outline, 4.f);
    shape_.setOutlineThickness(outline);
}

void Button::updateColors(float dt) {
    const auto& t = getTheme();

    sf::Color targetFill;
    if (pressed_)        targetFill = t.buttonPressed;
    else if (hovered_)   targetFill = t.buttonHover;
    else if (focused_)   targetFill = t.buttonSelected;   // 焦点 = 选中色
    else if (selected_)  targetFill = t.buttonSelected;
    else                 targetFill = t.buttonNormal;

    sf::Color targetOutline = focused_ ? sf::Color(255, 240, 120)
                                       : t.outline;
    sf::Color targetText    = t.textPrimary;

    if (!colorsInitialized_) {
        currentFill_    = targetFill;
        currentOutline_ = targetOutline;
        currentText_    = targetText;
        colorsInitialized_ = true;
        return;
    }

    currentFill_    = Anim::approach(currentFill_,    targetFill,    dt);
    currentOutline_ = Anim::approach(currentOutline_, targetOutline, dt);
    currentText_    = Anim::approach(currentText_,    targetText,    dt);
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
            if (pressed_ && inside) {
                clicked_ = true;
                SoundManager::instance().playClick();
            }
            pressed_ = false;
        }
    }
}

bool Button::consumeClick() {
    bool c = clicked_;
    clicked_ = false;
    return c;
}

void Button::render(sf::RenderTarget& target) {
    float dt = animClock_.restart().asSeconds();
    updateColors(dt);

    // ⭐ 只在 size / focus / style 变化时重建形状
    const auto& style = getButtonStyle();
    if (size_ != lastShapeSize_ ||
        focused_ != lastShapeFocused_ ||
        style.cornerRadius != lastShapeCorner_ ||
        style.outlineThickness != lastShapeOutline_) {
        refreshShape();
        lastShapeSize_ = size_;
        lastShapeFocused_ = focused_;
        lastShapeCorner_ = style.cornerRadius;
        lastShapeOutline_ = style.outlineThickness;
    }

    shape_.setFillColor(currentFill_);
    shape_.setOutlineColor(currentOutline_);
    text_.setFillColor(currentText_);

    target.draw(shape_);
    target.draw(text_);
}

void Button::triggerClick() {
    clicked_ = true;
    SoundManager::instance().playClick();
}