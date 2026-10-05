#pragma once
#include "preferences.h"
#include "runtime_config.h"
#include "window.h"
#include "toggle_row.h"
#include "multi_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class DisplayTab {
public:
    // 不再需要 Logger：日志级别已挪到「高级」页
    DisplayTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
               std::shared_ptr<RuntimeConfig> runtime, std::shared_ptr<Window> window);

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
    bool anyEditing() const { return false; }

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<RuntimeConfig> runtime_;
    std::shared_ptr<Window> window_;
    // Logger 依赖随「日志级别」一起挪去了「高级」页（0.3.8）

    // 状态
    int selectedResolution_ = 0;
    bool fullscreen_ = false;
    bool rememberSize_ = true;
    bool autoPauseOnBlur_ = true;
    int windowMode_ = 0;
    bool vsync_ = true;
    int antiAliasingLevel_ = 8;
    int fpsLimit_ = 60;

    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>> multiRows_;
    // [0] Resolution  [1] WindowMode  [2] AntiAliasing  [3] FpsLimit
    // 日志级别已挪去「高级」页，所以这里没有第 5 行

    void applyResolution();
    void applyFullscreen();
    void applyWindowMode();
    void applyVsync();
    void applyAntiAliasing();
    void applyFpsLimit();
};