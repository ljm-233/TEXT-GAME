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

void FocusGroup::moveFocus(int dir) {
    if (items_.empty()) return;
    int n = static_cast<int>(items_.size());
    index_ = (index_ + dir + n) % n;

    for (int i = 0; i < n; ++i) {
        items_[i]->setFocused(i == index_);
    }
}

void FocusGroup::update(float /*dt*/) {
    if (!enabled_) return;

    auto& gp = Gamepad::instance();
    if (!gp.isConnected()) {
        lastUp_ = lastDown_ = lastLeft_ = lastRight_ = false;
        lastA_ = lastB_ = lastStart_ = false;
        return;
    }

    // ===== 方向键：移动焦点（只在刚按下时触发）=====
    bool nowUp    = gp.dpadUp()   || gp.leftY() < -0.5f;
    bool nowDown  = gp.dpadDown() || gp.leftY() >  0.5f;
    bool nowLeft  = gp.dpadLeft() || gp.leftX() < -0.5f;
    bool nowRight = gp.dpadRight()|| gp.leftX() >  0.5f;

    if (nowDown  && !lastDown_)  moveFocus(+1);
    if (nowUp    && !lastUp_)    moveFocus(-1);
    if (nowRight && !lastRight_) moveFocus(+1);
    if (nowLeft  && !lastLeft_)  moveFocus(-1);

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