#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <memory>

enum class NotificationType {
    Info,    // 蓝色
    Success, // 绿色
    Warning, // 黄色
    Error    // 红色
};

enum class NotificationPos { TopLeft, TopRight, BottomLeft, BottomRight };

// 全局单例：任何地方都可以 push，屏幕角落自动显示
class NotificationSystem {
public:
    static NotificationSystem& instance();

    void setFont(const sf::Font& font) {
        font_ = &font;
        text_ = std::make_unique<sf::Text>(font, sf::String(), 18);
    }

    /// 传给 push() 的 duration 用这个值 = "用配置里的默认时长"。
    ///
    /// 用哨兵而不是把默认值改成可变量，是为了区分两种调用：
    ///   - 没写 duration 的（正常通知）→ 跟随设置里的「通知时长」
    ///   - 显式写了 1.2f 的（F3 清空敌人那种调试提示）→ 保持自己的短时长
    static constexpr float kUseDefaultDuration = -1.f;

    void push(const std::string& text, NotificationType type = NotificationType::Info,
              float duration = kUseDefaultDuration);

    /// 设置里的「通知时长」（秒）
    void setDefaultDuration(float seconds) { defaultDuration_ = seconds; }
    float defaultDuration() const { return defaultDuration_; }

    void clear();

    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const { return enabled_; }

    void setPosition(NotificationPos p) { pos_ = p; }
    NotificationPos getPosition() const { return pos_; }

    // ⭐ update + render 合并：渲染时自动推进时间
    //    避免 update 被漏调（曾经导致通知永不消失）
    void render(sf::RenderTarget& target, float dt);

private:
    NotificationSystem() = default;

    struct Entry {
        std::string text;
        NotificationType type;
        float elapsed = 0.f;
        float duration = 3.f;
    };

    std::deque<Entry> queue_;
    const sf::Font* font_ = nullptr;
    bool enabled_ = true;
    /// 调用方没显式给时长时用它（设置里的「通知时长」，默认与旧行为一致）
    float defaultDuration_ = 3.f;
    NotificationPos pos_ = NotificationPos::TopRight;

    // ⭐ 复用渲染对象，避免每帧构造 sf::Text
    mutable std::unique_ptr<sf::Text> text_;
    mutable sf::RectangleShape panel_;

    static constexpr size_t kMaxVisible = 6;
    static constexpr float kWidth = 340.f;
    static constexpr float kHeight = 42.f;
    static constexpr float kGap = 8.f;
    static constexpr float kMargin = 20.f;
};