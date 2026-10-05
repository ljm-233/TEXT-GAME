#pragma once
#include "preferences.h"
#include "window.h"
#include "slider.h"
#include "toggle_row.h"
#include "multi_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class InterfaceTab {
public:
    /// 注意：壁纸已经不在这一页了（0.3.7 起独立成 WallpaperTab），
    /// 所以这里也不再需要 Background。
    ///
    /// 0.3.8：渲染缩放 / 超分辨率挪去「画面」页，控制台那 8 项挪去新的
    /// 「控制台」页；同时从「画面」页接收按钮圆角 / 边框、动画、通知，
    /// 并新增「通知时长」。
    ///
    /// window 参数只是为了不改 SettingsScene 的调用点而保留，本页已不用它。
    InterfaceTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                 std::shared_ptr<Window> window);

    void loadFromPrefs();

    /// 「恢复本页默认」用：重读配置，并把需要立即生效的东西重新应用一次。

    /// 与 loadFromPrefs() 的区别是它**会**去改全局状态（主题、音量、后处理器…）。

    void reapply();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target, float contentX, float ctrlX, float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const;

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;

    // ===== 状态 =====
    bool showFps_ = false;
    int fpsPosition_ = 1;
    int fpsFormat_ = 1;
    float uiScale_ = 1.0f;
    float fontScale_ = 1.0f;
    int themeId_ = 0;
    int languageIdx_ = 0;
    bool showClock_ = false;
    int clockPosition_ = 0;
    float buttonCorner_ = 6.0f;
    float buttonOutline_ = 2.0f;
    bool animationEnabled_ = true;
    int animationSpeedIndex_ = 1;
    bool notificationEnabled_ = true;
    int notificationPosition_ = 0;
    int notificationDuration_ = 3000;

    // ===== 控件 =====
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>> multiRows_;
    std::unique_ptr<Slider> notificationDurationSlider_;

    // Slider 标签
    sf::Text labelNotificationDuration_;
    sf::Text hintUiScale_;

    // ===== 具名控件指针 =====
    // addToggle / addMulti 返回的是**堆上对象**的地址：vector 扩容搬的是
    // unique_ptr，指向的对象本身不会动，所以缓存裸指针是安全的。
    ToggleRow* rowFps_ = nullptr;
    ToggleRow* rowClock_ = nullptr;
    ToggleRow* rowAnimation_ = nullptr;
    ToggleRow* rowNotification_ = nullptr;

    MultiRow* rowFpsPos_ = nullptr;
    MultiRow* rowFpsFormat_ = nullptr;
    MultiRow* rowUiScale_ = nullptr;
    MultiRow* rowFontScale_ = nullptr;
    MultiRow* rowTheme_ = nullptr;
    MultiRow* rowLanguage_ = nullptr;
    MultiRow* rowClockPos_ = nullptr;
    MultiRow* rowButtonCorner_ = nullptr;
    MultiRow* rowButtonOutline_ = nullptr;
    MultiRow* rowAnimationSpeed_ = nullptr;
    MultiRow* rowNotificationPos_ = nullptr;

    // ===== 应用 =====
    void applyFpsPosition();
    void applyFpsFormat();
    void applyTheme();
    void applyLanguage();
    void applyButtonStyle();
    void applyAnimation();
    void applyNotification();
};
