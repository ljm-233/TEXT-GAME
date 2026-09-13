#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class TextInput {
public:
    TextInput(const sf::Font& font,
              sf::Vector2f position,
              sf::Vector2f size,
              const std::string& placeholder = "",
              unsigned characterSize = 20,
              size_t maxLength = 32);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);

    // ===== 状态 =====
    const std::string& text() const { return content_; }
    void setText(const std::string& text);
    void clear() { setText(""); }

    void setPosition(sf::Vector2f p);
    void setSize(sf::Vector2f s);
    void setPlaceholder(const std::string& p);
    void setFocused(bool f);
    bool isFocused() const { return focused_; }

    sf::Vector2f position() const { return position_; }
    sf::Vector2f size()     const { return size_; }

    // ===== 回调 =====
    void setOnSubmit (std::function<void(const std::string&)> cb) { onSubmit_  = std::move(cb); }
    void setOnCancel (std::function<void()> cb)                    { onCancel_  = std::move(cb); }
    void setOnChanged(std::function<void(const std::string&)> cb) { onChanged_ = std::move(cb); }

private:
    void refreshShape();
    void refreshTexts();
    void refreshCursor();
    void notifyChanged();

    bool contains(sf::Vector2f point) const;

    const sf::Font& font_;
    unsigned     characterSize_;

    sf::RectangleShape shape_;
    sf::Text           textDraw_;        // 显示 text_
    sf::Text           placeholderDraw_; // 显示 placeholder_
    sf::RectangleShape cursor_;

    sf::Vector2f position_;
    sf::Vector2f size_;

    // ===== 数据 =====
    std::string content_;                // UTF-8 内容
    std::string placeholder_;            // 占位符文字
    size_t      cursorPos_ = 0;          // 按字符索引，不是字节
    size_t      maxLength_;

    bool focused_ = false;
    bool hovered_ = false;

    sf::Clock blinkClock_;

    std::function<void(const std::string&)> onSubmit_;
    std::function<void()>                   onCancel_;
    std::function<void(const std::string&)> onChanged_;
};