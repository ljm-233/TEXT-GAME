#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>

enum class NotificationType {
    Info,     // 蓝色
    Success,  // 绿色
    Warning,  // 黄色
    Error     // 红色
};

enum class NotificationPos {
    TopLeft, TopRight, BottomLeft, BottomRight
};

// 全局单例：任何地方都可以 push，屏幕角落自动显示
class NotificationSystem {
public:
    static NotificationSystem& instance();

    void setFont(const sf::Font& font) { font_ = &font; }

    void push(const std::string& text,
              NotificationType type = NotificationType::Info,
              float duration = 3.f);

    void clear();

    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const  { return enabled_; }

    void setPosition(NotificationPos p) { pos_ = p; }
    NotificationPos getPosition() const { return pos_; }

    void update(float dt);
    void render(sf::RenderTarget& target);

private:
    NotificationSystem() = default;

    struct Entry {
        std::string      text;
        NotificationType type;
        float            elapsed  = 0.f;
        float            duration = 3.f;
    };

    std::deque<Entry> queue_;
    const sf::Font*   font_ = nullptr;
    bool              enabled_ = true;
    NotificationPos   pos_ = NotificationPos::TopRight;

    static constexpr size_t kMaxVisible = 6;
    static constexpr float  kWidth  = 340.f;
    static constexpr float  kHeight = 42.f;
    static constexpr float  kGap    = 8.f;
    static constexpr float  kMargin = 20.f;
};