#pragma once
#include "button.h"
#include "focus_nav.h"
#include <SFML/Window/Event.hpp>
#include <vector>

// 全局焦点管理器：键盘/手柄方向键在按钮间移动，Enter / 手柄 A 触发
class FocusGroup {
public:
    /// 一帧的导航输入（由调用方把设备状态翻译成这几个 bool）。
    ///
    /// 刻意不在 FocusGroup 里读 sf::Keyboard::isKeyPressed()：那是全局状态，
    /// 测试里伪造不了。设备翻译留在 Game 主循环，"按一下焦点走没走"的逻辑
    /// 则留在可测的 FocusNav。
    struct NavInput {
        bool up = false, down = false, left = false, right = false;
        bool confirm = false; // 手柄 A / 键盘 Enter
    };

    static FocusGroup& instance();

    // 场景每帧调用，注册当前可聚焦的按钮
    void setItems(const std::vector<Button*>& items);
    void clear();

    // 由 Game 主循环每帧调用
    void update(float dt, const NavInput& in);

    bool enabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    /// 只关手柄输入（B→ESC / Start→Enter 的合成）。键盘导航**不受**它影响 ——
    /// 以前这两个开关是同一个，"关掉手柄支持"会连键盘一起关死，菜单再也点不动。
    bool gamepadEnabled() const { return gamepadEnabled_; }
    void setGamepadEnabled(bool e) { gamepadEnabled_ = e; }

    /// 文本输入编辑时挂起键盘导航（否则打字母会同时移动焦点、Enter 误触按钮）。
    /// 调用方在喂 NavInput 的键盘部分前先看这个开关（见 Game::run）。
    bool keyboardNavEnabled() const { return keyboardNavEnabled_; }
    void setKeyboardNavEnabled(bool e) { keyboardNavEnabled_ = e; }

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

    // 导航算法本身在 FocusNav（纯几何，可测），这里只借用它的方向枚举
    using Direction = FocusNav::Direction;

    std::vector<Button*> items_;
    int   index_ = 0;
    bool  enabled_ = true;
    bool gamepadEnabled_ = true;
    bool keyboardNavEnabled_ = true;

    bool lastConfirm_ = false;
    bool lastB_ = false;
    bool lastStart_ = false;

    // ⭐ 方向键重复触发状态（每个方向独立，纯逻辑在 FocusNav::Repeater）
    FocusNav::Repeater upRepeater_, downRepeater_, leftRepeater_, rightRepeater_;

    std::vector<sf::Event> pendingEvents_;

    void moveFocus(Direction dir);
    void moveFocusLinear(int delta);   // 保留线性逻辑作为 fallback
};
