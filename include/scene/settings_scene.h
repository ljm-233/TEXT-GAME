#pragma once
#include "background.h"
#include "button.h"
#include "confirm_dialog.h"
#include "core/paths.h"
#include "multi_row.h"
#include "preferences.h"
#include "runtime_config.h"
#include "scene.h"
#include "scene/settings_tab_id.h"
#include "slider.h"
#include "tabs/advanced_tab.h"
#include "tabs/audio_tab.h"
#include "tabs/console_tab.h"
#include "tabs/controls_tab.h"
#include "tabs/display_tab.h"
#include "tabs/game_tab.h"
#include "tabs/graphics_tab.h"
#include "tabs/interface_tab.h"
#include "tabs/wallpaper_tab.h"
#include "text_input.h"
#include "theme.h"
#include "toggle_row.h"
#include "ui/resolution.h"
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"
#include "window.h"

#include <memory>
#include <vector>

/// 设置场景：9 个自包含的 Tab + 一条底栏。
///
/// 0.3.8 重排了分类。Tab 的**身份与归属表**抽到了 scene/settings_tab_id.h ——
/// 那边同时被「恢复本页默认」和归属测试使用，见那个文件的注释。
class SettingsScene : public Scene {
public:
    SettingsScene(std::shared_ptr<Background> background,
                  std::shared_ptr<Preferences> preferences,
                  std::shared_ptr<RuntimeConfig> runtimeConfig,
                  std::shared_ptr<Window> window,
                  std::shared_ptr<WallpaperLibrary> wallpaperLibrary,
                  std::shared_ptr<WallpaperLoader> wallpaperLoader,
                  std::shared_ptr<Paths> paths, const sf::Font& font,
                  std::shared_ptr<Logger> logger);

    void onEnter() override;
    void onResume() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;
    SceneId nextScene() const override { return nextScene_; }

private:
    static constexpr int kTabCount = static_cast<int>(kSettingsTabCount);

    // ===== 应用状态 =====
    void refreshLabels();
    void refreshSelection();
    void syncFocus();
    void resetAllPreferences();

    /// 「恢复本页默认」：把本页的键从配置里删掉，再让本页重读一遍并把
    /// 需要立即生效的东西重新应用（主题、音量、后处理…）。
    ///
    /// 比"改完就退出游戏"友好，也比逐键写一份默认值可靠 ——
    /// 默认值只存在于读取处（见 Config::resetKeys）。
    void resetCurrentTab();

    bool anySliderEditing() const;
    void applyButtonStyle();

    // ===== 渲染子函数 =====
    void renderTabs(Window& window);
    float renderDisplayTab(Window& window, float contentX, float ctrlX, float y);
    float renderInterfaceTab(Window& window, float contentX, float ctrlX, float y);
    float renderWallpaperTab(Window& window, float contentX, float ctrlX, float y);
    float renderGraphicsTab(Window& window, float contentX, float ctrlX, float y);
    float renderAudioTab(Window& window, float contentX, float ctrlX, float y);
    float renderControlsTab(Window& window, float contentX, float ctrlX, float y);
    float renderGameTab(Window& window, float contentX, float ctrlX, float y);
    float renderConsoleTab(Window& window, float contentX, float ctrlX, float y);
    float renderAdvancedTab(Window& window, float contentX, float ctrlX, float y);

    // ⭐ 设计坐标系 View（UI 整体缩放）
    void updateDesignView();

    // ===== 依赖 =====
    std::shared_ptr<Background> background_;
    std::shared_ptr<Preferences> preferences_;
    std::shared_ptr<RuntimeConfig> runtimeConfig_;
    std::shared_ptr<Window> window_;
    // 壁纸页要用：library 提供候选列表，loader 提供缩略图
    std::shared_ptr<WallpaperLibrary> wallpaperLibrary_;
    std::shared_ptr<WallpaperLoader> wallpaperLoader_;
    // 「高级」页要用：打开配置/日志目录
    std::shared_ptr<Paths> paths_;
    std::shared_ptr<Logger> logger_;
    const sf::Font& font_;

    SettingsTab currentTab_ = SettingsTab::Display;
    std::vector<std::unique_ptr<Button>> tabButtons_;

    // ===== 9 个 Tab（都是自包含类）=====
    std::unique_ptr<DisplayTab> displayTab_;
    std::unique_ptr<InterfaceTab> interfaceTab_;
    std::unique_ptr<WallpaperTab> wallpaperTab_;
    std::unique_ptr<GraphicsTab> graphicsTab_;
    std::unique_ptr<AudioTab> audioTab_;
    std::unique_ptr<ControlsTab> controlsTab_;
    std::unique_ptr<GameTab> gameTab_;
    std::unique_ptr<ConsoleTab> consoleTab_;
    std::unique_ptr<AdvancedTab> advancedTab_;

    // ===== 底栏按钮（窗口坐标系，固定右下角）=====
    std::unique_ptr<Button> backButton_;
    std::unique_ptr<Button> resetTabButton_; ///< 恢复本页默认（每一页都有）
    std::unique_ptr<Button> resetAllButton_; ///< 恢复全部默认（只在「高级」页）
    std::unique_ptr<Button> aboutButton_;    ///< 关于（只在「高级」页）

    std::unique_ptr<ConfirmDialog> resetTabConfirm_;
    std::unique_ptr<ConfirmDialog> resetAllConfirm_;
    std::unique_ptr<ConfirmDialog> aboutDialog_;

    // ===== 标题 / 标签 =====
    sf::Text headingDisplay_, headingInterface_, headingWallpaper_, headingGraphics_;
    sf::Text headingAudio_, headingControls_, headingGame_, headingConsole_,
        headingAdvanced_;

    /// 内容溢出时的滚动提示。
    /// 之前只有一个滚动条指示器（一根白条），没有任何文字 ——
    /// uiScale 调大之后内容被裁掉，用户不知道还能滚。
    sf::Text scrollHint_;

    SceneId nextScene_ = SceneId::None;
    int lastLangVersion_ = -1;

    // ⭐ 设计坐标系（固定 1280×720，由 View 缩放到窗口）
    sf::View designView_;
    static constexpr float kDesignW = 1280.f;
    static constexpr float kDesignH = 720.f;
    // ⭐ uiScale > 1.0 时内容溢出，滚轮可上下平移
    float contentScroll_ = 0.f;
    float contentTotalH_ = 0.f; // 上一帧内容底部 y
};
