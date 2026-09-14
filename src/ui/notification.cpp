#include "notification.h"
#include <algorithm>
#include <cstdint>

NotificationSystem& NotificationSystem::instance() {
    static NotificationSystem inst;
    return inst;
}

void NotificationSystem::push(const std::string& text, NotificationType type,
                              float duration) {
    if (!enabled_)
        return;
    Entry e;
    e.text = text;
    e.type = type;
    e.duration = duration;
    queue_.push_back(std::move(e));
    // 队列上限：隐藏的条数不超过 4
    if (queue_.size() > kMaxVisible + 4) {
        queue_.pop_front();
    }
}

void NotificationSystem::clear() {
    queue_.clear();
}

void NotificationSystem::update(float dt) {
    for (auto& e : queue_)
        e.elapsed += dt;
    while (!queue_.empty() && queue_.front().elapsed >= queue_.front().duration) {
        queue_.pop_front();
    }
}

void NotificationSystem::render(sf::RenderTarget& target) {
    if (!enabled_ || !font_ || queue_.empty())
        return;

    auto winSize = target.getSize();
    float w = static_cast<float>(winSize.x);
    float h = static_cast<float>(winSize.y);

    const float width = kWidth;
    const float height = kHeight;
    const float gap = kGap;
    const float margin = kMargin;

    // 水平位置
    float x = 0.f;
    switch (pos_) {
    case NotificationPos::TopLeft:
    case NotificationPos::BottomLeft:
        x = margin;
        break;
    case NotificationPos::TopRight:
    case NotificationPos::BottomRight:
        x = w - width - margin;
        break;
    }

    // 只渲染最后 N 条
    size_t n = std::min(queue_.size(), kMaxVisible);
    float totalH = static_cast<float>(n) * (height + gap) - gap;

    float y = 0.f;
    switch (pos_) {
    case NotificationPos::TopLeft:
    case NotificationPos::TopRight:
        y = margin;
        break;
    case NotificationPos::BottomLeft:
    case NotificationPos::BottomRight:
        y = h - margin - totalH;
        break;
    }

    size_t startIdx = queue_.size() - n;
    for (size_t i = startIdx; i < queue_.size(); ++i) {
        const auto& e = queue_[i];
        float t = e.elapsed / e.duration;

        // 淡入（前 0.15）+ 淡出（后 0.15）
        float alpha = 1.f;
        if (t < 0.15f)
            alpha = t / 0.15f;
        else if (t > 0.85f)
            alpha = (1.f - t) / 0.15f;
        alpha = std::clamp(alpha, 0.f, 1.f);

        auto a8 = static_cast<std::uint8_t>(alpha * 255.f);

        // 类型颜色
        sf::Color bg;
        switch (e.type) {
        case NotificationType::Info:
            bg = {50, 100, 170};
            break;
        case NotificationType::Success:
            bg = {40, 130, 70};
            break;
        case NotificationType::Warning:
            bg = {180, 130, 30};
            break;
        case NotificationType::Error:
            bg = {170, 50, 50};
            break;
        }
        bg.a = a8;

        // 背景
        sf::RectangleShape panel(sf::Vector2f{width, height});
        panel.setPosition({x, y});
        panel.setFillColor(bg);
        panel.setOutlineThickness(2.f);
        panel.setOutlineColor(sf::Color(255, 255, 255, a8));

        target.draw(panel);

        // 文字
        sf::Text text(*font_, sf::String::fromUtf8(e.text.begin(), e.text.end()), 18);
        text.setFillColor(sf::Color(255, 255, 255, a8));
        auto b = text.getLocalBounds();
        text.setOrigin({0.f, b.position.y + b.size.y / 2.f});
        text.setPosition({x + 14.f, y + height / 2.f});
        target.draw(text);

        y += height + gap;
    }
}