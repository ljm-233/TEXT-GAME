#pragma once
#include "preferences.h"
#include "window.h"
#include "background.h"
#include "slider.h"
#include "toggle_row.h"
#include "multi_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class InterfaceTab {
public:
    InterfaceTab(const sf::Font& font,
                 std::shared_ptr<Preferences> prefs,
                 std::shared_ptr<Window> window,
                 std::shared_ptr<Background> background);

    void loadFromPrefs();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target,
                 float contentX, float ctrlX,
                 float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const;

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<Window>      window_;
    std::shared_ptr<Background>  background_;

    // ===== 状态 =====
    bool  showFps_              = false;
    int   fpsPosition_          = 1;
    int   fpsFormat_            = 1;
    float uiScale_              = 1.0f;
    float fontScale_            = 1.0f;
    float renderScale_          = 1.0f;
    int   upscaleMode_          = 1;
    int   themeId_              = 0;
    int   languageIdx_          = 0;
    bool  showClock_            = false;
    int   clockPosition_        = 0;
    int   consoleMask_          = 160;
    int   consolePanelAlpha_    = 220;
    int   consoleFontSize_      = 18;
    int   consoleHistoryLines_  = 200;
    int   consoleLineHeight_    = 26;
    bool  consoleAutoScroll_    = true;
    bool  consoleBlinkCursor_   = true;
    int   consolePrompt_        = 0;

    // ===== 控件 =====
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>>  multiRows_;
    std::unique_ptr<Button> wallpaperButton_;
    std::unique_ptr<Slider> consoleMaskSlider_;
    std::unique_ptr<Slider> consolePanelAlphaSlider_;

    // Slider 标签
    sf::Text labelWallpaper_;
    sf::Text labelConsoleMask_;
    sf::Text labelConsolePanelAlpha_;
    sf::Text hintUiScale_;

    // ===== MultiRow 索引 =====
    // multiRows_[0]  FpsPos
    // multiRows_[1]  FpsFormat
    // multiRows_[2]  UiScale
    // multiRows_[3]  FontScale
    // multiRows_[4]  RenderScale
    // multiRows_[5]  UpscaleMode
    // multiRows_[6]  Theme
    // multiRows_[7]  Language
    // multiRows_[8]  ClockPos
    // multiRows_[9]  ConsoleFont
    // multiRows_[10] ConsoleHistory
    // multiRows_[11] ConsoleLineHeight
    // multiRows_[12] ConsolePrompt

    // ===== 应用 =====
    void applyFpsPosition();
    void applyFpsFormat();
    void applyTheme();
    void applyLanguage();
    void applyWallpaper();
    void applyConsolePrompt();
};