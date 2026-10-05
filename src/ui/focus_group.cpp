#include "focus_group.h"
#include "infrastructure/gamepad.h"
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
    // 注意：不重置 lastConfirm_ / lastB_ / lastStart_，
    //       否则用户按着 A/Enter 切场景时会立即触发新场景的按钮
    upRepeater_ = {};
    downRepeater_ = {};
    leftRepeater_ = {};
    rightRepeater_ = {};
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

void FocusGroup::moveFocus(Direction dir) {
    if (items_.empty()) return;
    if (index_ < 0 || index_ >= static_cast<int>(items_.size())) return;

    // "往哪个方向走"由 FocusNav 算（纯几何），这里只负责把按钮位置喂进去、
    // 再把结果应用回按钮的聚焦状态
    std::vector<sf::FloatRect> rects;
    rects.reserve(items_.size());
    for (Button* b : items_) {
        const auto p = b->position();
        const auto s = b->size();
        rects.emplace_back(p, s);
    }

    const int bestIdx = FocusNav::nextInDirection(rects, index_, dir);
    if (bestIdx < 0) return;   // 该方向上没有按钮：停在原地

    index_ = bestIdx;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        items_[i]->setFocused(static_cast<int>(i) == index_);
    }
}

void FocusGroup::moveFocusLinear(int delta) {
    const int n = static_cast<int>(items_.size());
    const int next = FocusNav::nextLinear(index_, delta, n);
    if (next < 0) return;

    index_ = next;
    for (int i = 0; i < n; ++i) {
        items_[i]->setFocused(i == index_);
    }
}

void FocusGroup::update(float dt, const NavInput& in) {
    if (!enabled_) return;

    // ===== 方向键：移动焦点（带重复触发）=====
    // 键盘与手柄的输入已由调用方合成进 in；这里只管"按住重复"的时序。
    // 注意：**没有"没接手柄就早退"** —— 那会让纯键盘用户连菜单都进不去。
    if (upRepeater_.tick(in.up, dt))
        moveFocus(Direction::Up);
    if (downRepeater_.tick(in.down, dt))
        moveFocus(Direction::Down);
    if (leftRepeater_.tick(in.left, dt))
        moveFocus(Direction::Left);
    if (rightRepeater_.tick(in.right, dt))
        moveFocus(Direction::Right);

    // ===== 确认键（手柄 A / 键盘 Enter）：触发当前焦点按钮 =====
    if (in.confirm && !lastConfirm_) {
        if (auto* b = focused()) {
            b->triggerClick();
        }
    }
    lastConfirm_ = in.confirm;

    // ===== 下面是手柄专属的按键合成，没接手柄/关了手柄支持就跳过 =====
    auto& gp = Gamepad::instance();
    if (!gamepadEnabled_ || !gp.isConnected()) {
        lastB_ = false;
        lastStart_ = false;
        return;
    }

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