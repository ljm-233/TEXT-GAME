#include "focus_group.h"
#include "gamepad.h"
#include <algorithm>

FocusGroup& FocusGroup::instance() {
    static FocusGroup inst;
    return inst;
}

void FocusGroup::setItems(const std::vector<Button*>& items) {
    // 列表相同就不重置焦点
    if (items.size() == items_.size()) {
        bool same = true;
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i] != items_[i]) { same = false; break; }
        }
        if (same) return;
    }

    // 清除旧焦点
    if (index_ >= 0 && index_ < static_cast<int>(items_.size())) {
        items_[index_]->setFocused(false);
    }

    items_ = items;

    if (index_ >= static_cast<int>(items_.size())) index_ = 0;
    if (index_ < 0) index_ = 0;

    if (index_ < static_cast<int>(items_.size())) {
        items_[index_]->setFocused(true);
    }
}

void FocusGroup::clear() {
    for (auto* b : items_) b->setFocused(false);
    items_.clear();
    index_ = 0;

    // ⭐ 重置重复触发状态，避免场景切换时残留
    // 注意：不重置 lastA_ / lastB_ / lastStart_，
    //       否则用户按着 A 切场景时会立即触发新场景的按钮
    upRepeat_    = {};
    downRepeat_  = {};
    leftRepeat_  = {};
    rightRepeat_ = {};
}

void FocusGroup::setIndex(int i) {
    if (items_.empty()) return;
    if (index_ >= 0 && index_ < static_cast<int>(items_.size())) {
        items_[index_]->setFocused(false);
    }
    index_ = std::clamp(i, 0, static_cast<int>(items_.size()) - 1);
    if (index_ >= 0 && index_ < static_cast<int>(items_.size())) {
        items_[index_]->setFocused(true);
    }
}

Button* FocusGroup::focused() const {
    if (items_.empty()) return nullptr;
    if (index_ < 0 || index_ >= static_cast<int>(items_.size())) return nullptr;
    return items_[index_];
}

void FocusGroup::handleDirection(bool now, bool& last,
                                 RepeatState& st, Direction dir, float dt) {
    // ⭐ 手柄方向键重复触发手感参数
    constexpr float kInitialDelay = 0.35f;
    constexpr float kRepeatEvery  = 0.10f;

    if (!now) {
        st.holdTimer    = 0.f;
        st.triggerCount = 0;
        return;
    }

    if (!last) {
        moveFocus(dir);
        st.holdTimer    = 0.f;
        st.triggerCount = 1;
        return;
    }

    st.holdTimer += dt;

    float threshold = (st.triggerCount == 1) ? kInitialDelay : kRepeatEvery;
    if (st.holdTimer >= threshold) {
        moveFocus(dir);
        st.holdTimer = 0.f;
        ++st.triggerCount;
    }
}

void FocusGroup::moveFocus(Direction dir) {
    if (items_.empty()) return;
    if (index_ < 0 || index_ >= static_cast<int>(items_.size())) return;

    // ⭐ 几何导航：读每个按钮的实际屏幕位置，找"该方向上最近"的按钮
    Button* cur = items_[index_];
    auto cp = cur->position();
    auto cs = cur->size();
    sf::Vector2f ccenter = { cp.x + cs.x * 0.5f, cp.y + cs.y * 0.5f };

    float dx = (dir == Direction::Right) ? 1.f
             : (dir == Direction::Left)  ? -1.f : 0.f;
    float dy = (dir == Direction::Down)  ? 1.f
             : (dir == Direction::Up)    ? -1.f : 0.f;

    int bestIdx = -1;
    float bestScore = 1e30f;

    for (size_t i = 0; i < items_.size(); ++i) {
        if (static_cast<int>(i) == index_) continue;
        Button* b = items_[i];
        auto bp = b->position();
        auto bs = b->size();
        // 跳过未定位的按钮（位置为 0 且尺寸为 0）
        if (bs.x <= 0.f || bs.y <= 0.f) continue;

        sf::Vector2f bc = { bp.x + bs.x * 0.5f, bp.y + bs.y * 0.5f };
        sf::Vector2f delta = { bc.x - ccenter.x, bc.y - ccenter.y };

        // 主方向投影：必须为正，否则该按钮不在 dir 方向上
        float proj = delta.x * dx + delta.y * dy;
        if (proj < 2.f) continue;

        // 垂直偏移（叉积绝对值）
        float perp = std::abs(delta.x * dy - delta.y * dx);

        // 评分：主方向距离 + 垂直偏移 * 3（垂直偏移惩罚更重，让同行/同列优先）
        float score = proj + perp * 3.f;

        if (score < bestScore) {
            bestScore = score;
            bestIdx = static_cast<int>(i);
        }
    }

    if (bestIdx >= 0) {
        index_ = bestIdx;
        for (size_t i = 0; i < items_.size(); ++i) {
            items_[i]->setFocused(static_cast<int>(i) == index_);
        }
    }
    // 该方向上没有按钮：停在原地
}

void FocusGroup::moveFocusLinear(int delta) {
    if (items_.empty()) return;
    int n = static_cast<int>(items_.size());
    index_ = (index_ + delta + n) % n;

    for (int i = 0; i < n; ++i) {
        items_[i]->setFocused(i == index_);
    }
}

void FocusGroup::update(float dt) {
    if (!enabled_) return;

    auto& gp = Gamepad::instance();
    if (!gp.isConnected()) {
        lastUp_ = lastDown_ = lastLeft_ = lastRight_ = false;
        lastA_ = lastB_ = lastStart_ = false;
        return;
    }

    // ===== 方向键：移动焦点（带重复触发）=====
    bool nowUp    = gp.dpadUp()   || gp.leftY() < -0.5f;
    bool nowDown  = gp.dpadDown() || gp.leftY() >  0.5f;
    bool nowLeft  = gp.dpadLeft() || gp.leftX() < -0.5f;
    bool nowRight = gp.dpadRight()|| gp.leftX() >  0.5f;

    handleDirection(nowUp,    lastUp_,    upRepeat_,    Direction::Up,    dt);
    handleDirection(nowDown,  lastDown_,  downRepeat_,  Direction::Down,  dt);
    handleDirection(nowLeft,  lastLeft_,  leftRepeat_,  Direction::Left,  dt);
    handleDirection(nowRight, lastRight_, rightRepeat_, Direction::Right, dt);

    lastUp_    = nowUp;
    lastDown_  = nowDown;
    lastLeft_  = nowLeft;
    lastRight_ = nowRight;

    // ===== A 键：触发当前焦点按钮 =====
    bool nowA = gp.isButtonPressed(0);
    if (nowA && !lastA_) {
        if (auto* b = focused()) {
            b->triggerClick();
        }
    }
    lastA_ = nowA;

    // ===== B 键：合成 ESC =====
    bool nowB = gp.isButtonPressed(1);
    if (nowB && !lastB_) {
        pendingEvents_.push_back(sf::Event{sf::Event::KeyPressed{
            sf::Keyboard::Key::Escape,
            sf::Keyboard::Scan::Escape,
            false, false, false, false}});
    }
    lastB_ = nowB;

    // ===== Start 键：合成 Enter =====
    bool nowStart = gp.isButtonPressed(7);
    if (nowStart && !lastStart_) {
        pendingEvents_.push_back(sf::Event{sf::Event::KeyPressed{
            sf::Keyboard::Key::Enter,
            sf::Keyboard::Scan::Enter,
            false, false, false, false}});
    }
    lastStart_ = nowStart;
}