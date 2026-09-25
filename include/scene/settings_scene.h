#pragma once
#include "scene.h"
#include "background.h"
#include "button.h"
#include "slider.h"
#include "text_input.h"
#include "preferences.h"
#include "runtime_config.h"
#include "window.h"
#include "resolution.h"
#include "confirm_dialog.h"
#include "theme.h"
#include "toggle_row.h"
#include "multi_row.h"
#include "tabs/audio_tab.h"
#include "tabs/graphics_tab.h"
#include "tabs/interface_tab.h"
#include "tabs/display_tab.h"
#include "tabs/other_tab.h"
#include "tabs/keys_tab.h"
#include <memory>
#include <vector>

class SettingsScene : public Scene {
public:
    SettingsScene(std::shared_ptr<Background>    background,
                  std::shared_ptr<Preferences>   preferences,
                  std::shared_ptr<RuntimeConfig> runtimeConfig,
                  std::shared_ptr<Window>        window,
                  const sf::Font&                font,
                  std::shared_ptr<Logger>        logger);

    void onEnter() override;
    void onResume() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;
    SceneId nextScene() const override { return nextScene_; }

private:
    enum class Tab { Display = 0, Interface = 1, Graphics = 2, Audio = 3, Keys = 4, Other = 5 };
    static constexpr int kTabCount = 6;

    // ===== 应用状态 =====
    void refreshLabels();
    void refreshSelection();
    void syncFocus();
    void resetGraphicsPost();
    void applyPreset(int idx);

    bool anySliderEditing() const {
        if (audioTab_ && audioTab_->anyEditing()) return true;
        if (graphicsTab_ && graphicsTab_->anyEditing()) return true;
        if (interfaceTab_ && interfaceTab_->anyEditing()) return true;
        if (otherTab_ && otherTab_->anyEditing()) return true;
        if (keysTab_ && keysTab_->anyEditing()) return true;
        return false;
    }
    void applyButtonStyle();
    void applyAnimation();
    void applyNotification();
    // ⭐ applyGamepadVibration 已移入 AudioTab
    void applyShowColliders();
    void applyScreenShake();
    void applyParticles();
    void applyPseudo3D();
    void applyParallax();
    void applyPlayerAnimation();
    void applyLevelIntro();
    void resetAllPreferences();

    // ===== 渲染子函数 =====
    void renderTabs         (Window& window);
    float renderDisplayTab   (Window& window, float contentX, float ctrlX, float y);
    float renderInterfaceTab (Window& window, float contentX, float ctrlX, float y);
    float renderGraphicsTab  (Window& window, float contentX, float ctrlX, float y);
    float renderAudioTab     (Window& window, float contentX, float ctrlX, float y);
    float renderKeysTab      (Window& window, float contentX, float ctrlX, float y);
    float renderOtherTab     (Window& window, float contentX, float ctrlX, float y);

    // ⭐ 设计坐标系 View（UI 整体缩放）
    void updateDesignView();

    // ===== 依赖 =====
    std::shared_ptr<Background>    background_;
    std::shared_ptr<Preferences>   preferences_;
    std::shared_ptr<RuntimeConfig> runtimeConfig_;
    std::shared_ptr<Window>        window_;
    std::shared_ptr<Logger>        logger_;
    const sf::Font&                font_;

    Tab currentTab_ = Tab::Display;
    std::vector<std::unique_ptr<Button>> tabButtons_;

    // ================= Display =================
    // ⭐ 独立 Tab
    std::unique_ptr<DisplayTab> displayTab_;

    // ================= Interface =================
    // ⭐ 独立 Tab
    std::unique_ptr<InterfaceTab> interfaceTab_;

    // ================= Graphics =================
    // ⭐ 独立 Tab
    std::unique_ptr<GraphicsTab> graphicsTab_;

    // ================= Graphics 预设 =================
    std::unique_ptr<MultiRow> presetRow_;

    // ================= Audio =================
    // ⭐ 独立 Tab，自包含
    std::unique_ptr<AudioTab> audioTab_;

    // ================= Keys =================
    // ⭐ 独立 Tab
    std::unique_ptr<KeysTab> keysTab_;

    // ================= Other =================
    // ⭐ 独立 Tab
    std::unique_ptr<OtherTab> otherTab_;

    // 关于 / 重置按钮仍由 SettingsScene 管（底部窗口坐标系）
    std::unique_ptr<Button> aboutButton_;
    std::unique_ptr<Button> resetButton_;
    std::unique_ptr<Button> resetGraphicsButton_;

    std::unique_ptr<Button> backButton_;
    std::unique_ptr<ConfirmDialog> resetConfirm_;
    std::unique_ptr<ConfirmDialog> aboutDialog_;

    // ================= 标题 / 标签 =================
    sf::Text headingDisplay_, headingInterface_, headingGraphics_;
    sf::Text headingAudio_, headingKeys_, headingOther_;

    // ⭐ 已移入 AudioTab

    // ⭐ Audio 状态已移入 AudioTab
    // ⭐ Other 状态已移入 OtherTab

    SceneId nextScene_ = SceneId::None;
    int lastLangVersion_ = -1;

    // ⭐ 设计坐标系（固定 1280×720，由 View 缩放到窗口）
    sf::View designView_;
    static constexpr float kDesignW = 1280.f;
    static constexpr float kDesignH = 720.f;
    // ⭐ uiScale > 1.0 时内容溢出，滚轮可上下平移
    float contentScroll_ = 0.f;
    float contentTotalH_ = 0.f;   // 上一帧内容底部 y
};