#pragma once
#include "multi_row.h"
#include "preferences.h"
#include "slider.h"
#include "toggle_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

/// 设置里的「控制台」页：游戏内控制台的外观与行为。
///
/// 0.3.8 从「界面」页里独立出来 —— 那页本来堆了 19 个控件（界面本身的外观
/// 只占一半，剩下全是控制台的），谁想调字号得先在"界面"里翻半天。
///
/// ⚠️ 控件用**具名指针**而不是 `multiRows_[9]` 这种下标存。
///    以前那一页有 13 个 multiRow 靠数字下标引用，动一行就得把所有下标
///    重排一遍，而且排错了不报错、只是"点了这个改了那个"。
class ConsoleTab {
public:
    ConsoleTab(const sf::Font& font, std::shared_ptr<Preferences> prefs);

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
    void applyFont();
    void applyHistory();
    void applyLineHeight();
    void applyPrompt();

    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;

    // ===== 状态 =====
    int fontSize_ = 18;
    int historyLines_ = 200;
    int lineHeight_ = 26;
    int promptIdx_ = 0;
    bool autoScroll_ = true;
    bool blinkCursor_ = true;
    int mask_ = 160;
    int panelAlpha_ = 220;

    // ===== 控件 =====
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>> multiRows_;

    ToggleRow* rowAutoScroll_ = nullptr;
    ToggleRow* rowBlink_ = nullptr;

    MultiRow* rowFont_ = nullptr;
    MultiRow* rowHistory_ = nullptr;
    MultiRow* rowLineHeight_ = nullptr;
    MultiRow* rowPrompt_ = nullptr;

    std::unique_ptr<Slider> maskSlider_;
    std::unique_ptr<Slider> panelAlphaSlider_;

    sf::Text labelMask_;
    sf::Text labelPanelAlpha_;
};
