#pragma once
#include "button.h"
#include <SFML/Window/Event.hpp>
#include <vector>

// 全局焦点管理器：手柄方向键在按钮间移动，A 键触发
class FocusGroup {
public:
    static FocusGroup& instance();

    // 场景每帧调用，注册当前可聚焦的按钮
    void setItems(const std::vector<Button*>& items);
    void clear();

    // 由 Game 主循环每帧调用
    void update(float dt);

    bool enabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    int  index() const { return index_; }
    void setIndex(int i);

    Button* focused() const;

    // 取出待分发的合成事件（B 键 → ESC，Start 键 → Enter）
    std::vector<sf::Event> takePendingEvents() {
        std::vector<sf::Event> out;
        out.swap(pendingEvents_);
        return out;
    }

private:
    FocusGroup() = default;

    enum class Direction { Up, Down, Left, Right };

    std::vector<Button*> items_;
    int   index_ = 0;
    bool  enabled_ = true;

    bool  lastUp_    = false;
    bool  lastDown_  = false;
    bool  lastLeft_  = false;
    bool  lastRight_ = false;
    bool  lastA_     = false;
    bool  lastB_     = false;
    bool  lastStart_ = false;

    // ⭐ 方向键重复触发状态（每个方向独立）
    struct RepeatState {
        float holdTimer    = 0.f;   // 持续按住时长
        int   triggerCount = 0;     // 已触发的次数（0 = 未按）
    };
    RepeatState upRepeat_, downRepeat_, leftRepeat_, rightRepeat_;

    std::vector<sf::Event> pendingEvents_;

    void moveFocus(Direction dir);
    void moveFocusLinear(int delta);   // 保留线性逻辑作为 fallback

    // ⭐ 单方向的处理（带重复触发）
    void handleDirection(bool now, bool& last,
                         RepeatState& st, Direction dir, float dt);
};