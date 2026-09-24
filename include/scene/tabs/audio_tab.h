#pragma once
#include "preferences.h"
#include "window.h"
#include "slider.h"
#include "toggle_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

// ⭐ Audio Tab：自包含的设置 Tab
//    - 自己持有状态变量
//    - 自己读写 Preferences
//    - 自己管理控件、事件、布局
//    SettingsScene 只负责转发
class AudioTab {
public:
    AudioTab(const sf::Font& font,
             std::shared_ptr<Preferences> prefs,
             std::shared_ptr<Window> window);

    // 从 Preferences 读入
    void loadFromPrefs();

    // 事件
    void handleEvent(const sf::Event& ev);
    void update();

    // 渲染：返回渲染结束后的 y 坐标
    float render(sf::RenderTarget& target,
                 float contentX, float ctrlX,
                 float startY);

    // 语言切换时刷新 label
    void refreshLabels();

    // 刷新控件选中状态
    void refreshSelection();

    // 焦点注册（手柄导航）
    void registerFocus(std::vector<Button*>& out);

    // 是否有输入框正在编辑
    bool anyEditing() const;

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<Window>      window_;

    // 状态
    bool  soundEnabled_              = true;
    bool  bgmEnabled_                = true;
    bool  gamepadEnabled_            = true;
    bool  gamepadVibrationEnabled_   = true;
    float masterVolume_              = 1.0f;
    float soundVolume_               = 0.6f;
    float bgmVolume_                 = 0.4f;
    float gamepadVibrationIntensity_ = 1.0f;

    // 控件
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::unique_ptr<Slider> masterVolumeSlider_;
    std::unique_ptr<Slider> soundVolumeSlider_;
    std::unique_ptr<Slider> bgmVolumeSlider_;
    std::unique_ptr<Slider> gamepadVibrationSlider_;

    // Slider 的标签（Slider 本身不带 label）
    sf::Text labelMasterVolume_;
    sf::Text labelSoundVolume_;
    sf::Text labelBGMVolume_;
    sf::Text labelVibrationIntensity_;

    // 应用回调
    void applySound();
    void applyBGM();
    void applyGamepad();
    void applyGamepadVibration();
};