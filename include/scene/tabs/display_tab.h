#pragma once
#include "preferences.h"
#include "runtime_config.h"
#include "window.h"
#include "logging.h"
#include "toggle_row.h"
#include "multi_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class DisplayTab {
public:
    DisplayTab(const sf::Font& font,
               std::shared_ptr<Preferences> prefs,
               std::shared_ptr<RuntimeConfig> runtime,
               std::shared_ptr<Window> window,
               std::shared_ptr<Logger> logger);

    void loadFromPrefs();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target,
                 float contentX, float ctrlX,
                 float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const { return false; }

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences>   prefs_;
    std::shared_ptr<RuntimeConfig> runtime_;
    std::shared_ptr<Window>        window_;
    std::shared_ptr<Logger>        logger_;

    // 状态
    int  selectedResolution_ = 0;
    bool fullscreen_         = false;
    int  windowMode_         = 0;
    bool vsync_              = true;
    int  antiAliasingLevel_  = 8;
    int  logLevel_           = 2;
    int  fpsLimit_           = 60;

    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>>  multiRows_;
    // [0] Resolution  [1] WindowMode  [2] AntiAliasing  [3] LogLevel  [4] FpsLimit

    void applyResolution();
    void applyFullscreen();
    void applyWindowMode();
    void applyVsync();
    void applyAntiAliasing();
    void applyLogLevel();
    void applyFpsLimit();
};