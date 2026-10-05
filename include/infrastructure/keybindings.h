#pragma once
#include <SFML/Window/Keyboard.hpp>

// 全局键位表（和 getTheme() 一样是全局单例）
class KeyBindings {
public:
    enum Action { MoveLeft = 0, MoveRight, Jump, Pause, Restart, Count };

    static KeyBindings& instance();

    sf::Keyboard::Key get(Action a) const;
    void set(Action a, sf::Keyboard::Key key);

    static const char* actionName(Action a);
    static const char* keyToString(sf::Keyboard::Key k);

    /// 某个动作的**固定兜底键**：除可配置键位之外，**始终**也接受的键。
    /// 返回 Unknown 表示这个动作没有兜底键。
    ///
    /// 为什么要有：默认键位是 A / D / 空格（见 kDefaults），而**新玩家进游戏
    /// 第一反应是按方向键** —— 什么都不发生，看起来就像游戏坏了。
    /// 键位是可以改的，所以不能只改默认值；也不能把方向键塞进可配置键位
    /// （一个动作只有一格，会覆盖玩家自己的设置）。固定兜底是第三条路。
    ///
    /// 只给移动与跳跃三个动作兜底：暂停（ESC）与重开（R）不需要，
    /// 而且给它们兜底反而会和别的键冲突。
    static sf::Keyboard::Key fallbackKey(Action a);

    void resetToDefaults();

private:
    KeyBindings();
    sf::Keyboard::Key keys_[Count];
};