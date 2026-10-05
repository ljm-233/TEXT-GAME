#pragma once
#include "multi_row.h"
#include "preferences.h"
#include "text_input.h"
#include "toggle_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

/// 设置里的「游戏」页：跟玩法本身有关的设置（而不是画面/界面）。
///
/// 0.3.8 新建。以前「初始生命」「关卡开场」挂在「画面」页里 —— 它们确实
/// 影响画面，但玩家找它们时想的是"玩法"，不是"渲染"；「玩家名」则被丢在
/// 大杂烩的「其他」页。
class GameTab {
public:
    GameTab(const sf::Font& font, std::shared_ptr<Preferences> prefs);

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
    void applyInitialLives();

    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;

    // ===== 状态 =====
    int initialLives_ = 1;
    bool levelIntroEnabled_ = true;

    // ===== 控件 =====
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>> multiRows_;

    ToggleRow* rowLevelIntro_ = nullptr;
    MultiRow* rowLives_ = nullptr;

    std::unique_ptr<TextInput> playerNameInput_;
    sf::Text labelPlayerName_;
};
