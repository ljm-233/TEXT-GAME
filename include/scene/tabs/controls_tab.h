#pragma once
#include "button.h"
#include "infrastructure/keybindings.h"
#include "preferences.h"
#include "slider.h"
#include "toggle_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

/// 设置里的「操作」页：键盘键位 + 手柄。
///
/// 0.3.8 由「按键」页扩成 —— 手柄开关、手柄振动开关、振动强度原先挂在
/// 「音频」页里，纯粹因为"振动"听起来像触觉/声音。它们跟音频没有任何关系，
/// 玩家找"手柄设置"时也不会去音频页翻。
class ControlsTab {
public:
    ControlsTab(const sf::Font& font, std::shared_ptr<Preferences> prefs);

    void loadFromPrefs();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target, float contentX, float ctrlX, float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const;

    /// 「恢复本页默认」用：重读配置，并把需要立即生效的东西重新应用一次。
    /// 与 loadFromPrefs() 的区别是它**会**去改全局状态（主题、音量、后处理器…）。
    void reapply();

private:
    void refreshBindingText();
    void applyResetKeys();
    void applyGamepad();
    void applyGamepadVibration();

    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;

    // ===== 键位 =====
    std::vector<std::unique_ptr<Button>> keyBindingButtons_;
    std::vector<std::unique_ptr<sf::Text>> keyLabels_;
    int listeningAction_ = -1; ///< >=0 表示正在等用户按下一个键

    std::unique_ptr<Button> resetKeysButton_;
    sf::Text hintText_;

    // ===== 手柄 =====
    bool gamepadEnabled_ = true;
    bool gamepadVibrationEnabled_ = true;
    float gamepadVibrationIntensity_ = 1.0f;

    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    ToggleRow* rowGamepad_ = nullptr;
    ToggleRow* rowVibration_ = nullptr;
    std::unique_ptr<Slider> vibrationSlider_;
    sf::Text labelVibrationIntensity_;
};
