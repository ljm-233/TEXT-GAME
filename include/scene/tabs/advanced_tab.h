#pragma once
#include "button.h"
#include "log/logger.h"
#include "multi_row.h"
#include "paths.h"
#include "preferences.h"
#include "text_input.h"
#include "toggle_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

/// 设置里的「高级」页：日志、调试浮层，以及"打开配置目录 / 导出导入设置"。
///
/// 0.3.8 新建，把原先散在三处的开发向设置收拢到一页：
///   - 日志级别（原来在「显示」—— 跟显示毫无关系）
///   - 日志轮转 / 保留（原来在「其他」—— 跟日志级别隔了两个页）
///   - 显示碰撞盒（原来在「画面」—— 它是调试用的，不是画面效果）
///
/// 这一页的东西**普通玩家基本不需要动**，所以放在最后一页，
/// 并在页内加了说明（见 refreshLabels）。
class AdvancedTab {
public:
    AdvancedTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                std::shared_ptr<Logger> logger, std::shared_ptr<Paths> paths);

    void loadFromPrefs();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target, float contentX, float ctrlX, float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const;

    /// 导入设置成功后要求重启一次。
    ///
    /// 沿用「恢复默认设置」的既有行为：把场景切到 Exit（关窗口退出），
    /// 下次启动读新配置生效 —— 因为有一堆设置在启动阶段就固化了
    /// （分辨率、着色器、字体缩放…），逐个热更新既啰嗦又容易漏。
    bool consumeRestartRequest();

    /// 「恢复本页默认」用：重读配置，并把需要立即生效的东西重新应用一次。
    /// 与 loadFromPrefs() 的区别是它**会**去改全局状态（主题、音量、后处理器…）。
    void reapply();

private:
    void applyLogRotation();
    void doExport();
    void doImport();

    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<Paths> paths_;

    // ===== 状态 =====
    int logLevel_ = 2; // Info 的枚举值由 LogLevel 决定，loadFromPrefs 里覆盖
    int logRotateIdx_ = 0;
    int logKeepIdx_ = 1;
    bool showColliders_ = false;

    bool restartRequested_ = false;
    std::string status_; // 上一次操作的结果，画在页尾

    // ===== 控件 =====
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>> multiRows_;

    ToggleRow* rowColliders_ = nullptr;
    MultiRow* rowLogLevel_ = nullptr;
    MultiRow* rowLogRotate_ = nullptr;
    MultiRow* rowLogKeep_ = nullptr;

    std::unique_ptr<Button> openConfigButton_;
    std::unique_ptr<Button> openLogButton_;
    std::unique_ptr<Button> exportButton_;
    std::unique_ptr<Button> importButton_;

    std::unique_ptr<TextInput> codeInput_;

    sf::Text labelLog_;
    sf::Text labelOpen_;
    sf::Text labelShare_;
    sf::Text labelCode_;
    sf::Text hintText_;
    sf::Text statusText_;
};
