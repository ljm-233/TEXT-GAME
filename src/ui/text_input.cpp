#include "text_input.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include <algorithm>

namespace {

// ===== UTF-8 ↔ UTF-32 =====

std::u32string toU32(const std::string& s) {
    std::u32string out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        char32_t cp = 0;
        size_t len = 1;
        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            len = 4;
        } else {
            ++i;
            continue;
        }
        if (i + len > s.size())
            break;
        for (size_t k = 1; k < len; ++k) {
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        }
        out.push_back(cp);
        i += len;
    }
    return out;
}

std::string fromU32(const std::u32string& s) {
    std::string out;
    for (char32_t cp : s) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

// 返回 u32 前缀 [0, n) 对应的字节长度
size_t byteLenOfPrefix(const std::u32string& u32, size_t n) {
    size_t bytes = 0;
    for (size_t i = 0; i < n && i < u32.size(); ++i) {
        char32_t cp = u32[i];
        if (cp < 0x80)
            bytes += 1;
        else if (cp < 0x800)
            bytes += 2;
        else if (cp < 0x10000)
            bytes += 3;
        else
            bytes += 4;
    }
    return bytes;
}

} // namespace

// ============================================================

TextInput::TextInput(const sf::Font& font, sf::Vector2f position, sf::Vector2f size,
                     const std::string& placeholder, unsigned characterSize,
                     size_t maxLength)
      : font_(font),
        characterSize_(characterSize),
        textDraw_(font, sf::String(), scaledFontSize(characterSize)),
        placeholderDraw_(font, toSf(placeholder), scaledFontSize(characterSize)),
        position_(position),
        size_(size),
        placeholder_(placeholder),
        maxLength_(maxLength) {
    shape_.setPosition(position_);
    shape_.setOutlineThickness(2.f);
    shape_.setSize(size_);

    textDraw_.setFillColor(getTheme().textPrimary);
    placeholderDraw_.setFillColor(getTheme().textSecondary);
    cursor_.setFillColor(getTheme().textPrimary);

    refreshShape();
    refreshTexts();
    refreshCursor();
}

void TextInput::setText(const std::string& text) {
    auto u32 = toU32(text);
    if (u32.size() > maxLength_)
        u32.resize(maxLength_);
    content_ = fromU32(u32);
    if (cursorPos_ > u32.size())
        cursorPos_ = u32.size();
    refreshTexts();
    refreshCursor();
    notifyChanged();
}

void TextInput::setPosition(sf::Vector2f p) {
    position_ = p;
    shape_.setPosition(position_);
    refreshTexts();
    refreshCursor();
}

void TextInput::setSize(sf::Vector2f s) {
    size_ = s;
    shape_.setSize(size_);
    refreshTexts();
    refreshCursor();
}

void TextInput::setPlaceholder(const std::string& p) {
    placeholder_ = p;
    placeholderDraw_.setString(toSf(placeholder_));
    refreshTexts();
}

void TextInput::setFocused(bool f) {
    focused_ = f;
    blinkClock_.restart();
    refreshCursor();
}

void TextInput::notifyChanged() {
    if (onChanged_)
        onChanged_(content_);
}

void TextInput::refreshShape() {
    shape_.setSize(size_);
    shape_.setFillColor(hovered_ && !focused_ ? getTheme().buttonHover
                                              : getTheme().buttonNormal);
    shape_.setOutlineColor(focused_ ? getTheme().buttonSelected : getTheme().outline);
}

void TextInput::refreshTexts() {
    const float padX = 12.f;
    float textY = position_.y +
                  (size_.y - static_cast<float>(scaledFontSize(characterSize_))) / 2.f -
                  4.f;

    textDraw_.setString(sf::String::fromUtf8(content_.begin(), content_.end()));
    placeholderDraw_.setString(toSf(placeholder_));

    textDraw_.setPosition({position_.x + padX, textY});
    placeholderDraw_.setPosition({position_.x + padX, textY});
}

void TextInput::refreshCursor() {
    if (!focused_) {
        cursor_.setSize({0.f, 0.f});
        return;
    }

    float h = static_cast<float>(scaledFontSize(characterSize_)) + 6.f;
    cursor_.setSize({2.f, h});

    const float padX = 12.f;
    float cursorX = position_.x + padX;

    if (cursorPos_ > 0) {
        auto u32 = toU32(content_);
        size_t bytePos = byteLenOfPrefix(u32, cursorPos_);
        // 取前缀子串的宽度
        std::string prefix = content_.substr(0, bytePos);
        sf::Text probe(font_, sf::String::fromUtf8(prefix.begin(), prefix.end()),
                       scaledFontSize(characterSize_));
        auto b = probe.getLocalBounds();
        cursorX = position_.x + padX + b.size.x;
    }

    float cursorY = position_.y + (size_.y - h) / 2.f;
    cursor_.setPosition({cursorX, cursorY});
}

bool TextInput::contains(sf::Vector2f point) const {
    return point.x >= position_.x && point.x <= position_.x + size_.x &&
           point.y >= position_.y && point.y <= position_.y + size_.y;
}

void TextInput::handleEvent(const sf::Event& event) {
    if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        hovered_ = contains(
            {static_cast<float>(mm->position.x), static_cast<float>(mm->position.y)});
        refreshShape();
    }

    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            bool inside = contains(
                {static_cast<float>(mb->position.x), static_cast<float>(mb->position.y)});
            setFocused(inside);
            refreshShape();
        }
    }

    if (!focused_)
        return;

    // 文字输入
    if (const auto* te = event.getIf<sf::Event::TextEntered>()) {
        char32_t cp = te->unicode;
        if (cp < 32 || cp == 127)
            return;
        auto u32 = toU32(content_);
        if (u32.size() >= maxLength_)
            return;
        u32.insert(u32.begin() + static_cast<std::ptrdiff_t>(cursorPos_), cp);
        content_ = fromU32(u32);
        ++cursorPos_;
        refreshTexts();
        refreshCursor();
        notifyChanged();
        blinkClock_.restart();
    }

    // 功能键
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        auto u32 = toU32(content_);
        switch (kp->code) {
        case sf::Keyboard::Key::Backspace:
            if (cursorPos_ > 0) {
                u32.erase(u32.begin() + static_cast<std::ptrdiff_t>(cursorPos_ - 1));
                content_ = fromU32(u32);
                --cursorPos_;
                refreshTexts();
                refreshCursor();
                notifyChanged();
            }
            blinkClock_.restart();
            break;

        case sf::Keyboard::Key::Delete:
            if (cursorPos_ < u32.size()) {
                u32.erase(u32.begin() + static_cast<std::ptrdiff_t>(cursorPos_));
                content_ = fromU32(u32);
                refreshTexts();
                refreshCursor();
                notifyChanged();
            }
            blinkClock_.restart();
            break;

        case sf::Keyboard::Key::Left:
            if (cursorPos_ > 0) {
                --cursorPos_;
                refreshCursor();
            }
            blinkClock_.restart();
            break;

        case sf::Keyboard::Key::Right:
            if (cursorPos_ < u32.size()) {
                ++cursorPos_;
                refreshCursor();
            }
            blinkClock_.restart();
            break;

        case sf::Keyboard::Key::Home:
            cursorPos_ = 0;
            refreshCursor();
            blinkClock_.restart();
            break;

        case sf::Keyboard::Key::End:
            cursorPos_ = u32.size();
            refreshCursor();
            blinkClock_.restart();
            break;

        case sf::Keyboard::Key::Enter:
            if (onSubmit_)
                onSubmit_(content_);
            break;

        case sf::Keyboard::Key::Escape:
            setFocused(false);
            refreshShape();
            if (onCancel_)
                onCancel_();
            break;

        default:
            break;
        }
    }
}

void TextInput::render(sf::RenderTarget& target) {
    refreshShape();
    textDraw_.setFillColor(getTheme().textPrimary);
    placeholderDraw_.setFillColor(getTheme().textSecondary);
    cursor_.setFillColor(getTheme().textPrimary);

    target.draw(shape_);

    if (content_.empty()) {
        target.draw(placeholderDraw_);
    } else {
        target.draw(textDraw_);
    }

    if (focused_) {
        auto ms = blinkClock_.getElapsedTime().asMilliseconds();
        if ((ms / 500) % 2 == 0) {
            target.draw(cursor_);
        }
    }
}