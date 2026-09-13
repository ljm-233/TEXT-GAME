#include "settings_scene.h"
#include "ui_scale.h"
#include "utf8.h"
#include <algorithm>
#include <cmath>

namespace {
const int kAALevels[]     = {0, 4, 8, 16};
constexpr int kAACount    = 4;
const char* kAALabels[]   = {"关", "4x", "8x", "16x"};

const LogLevel kLogLevels[] = {
    LogLevel::Trace, LogLevel::Debug, LogLevel::Info,
    LogLevel::Warn,  LogLevel::Error
};
constexpr int kLogCount = 5;
const char* kLogLabels[] = {"Trace", "Debug", "Info", "Warn", "Error"};

const float kUiScales[]    = {0.8f, 1.0f, 1.2f, 1.5f};
constexpr int kUiScaleCount = 4;
const char* kUiScaleLabels[] = {"0.8x", "1.0x", "1.2x", "1.5x"};

const int kConsoleFonts[]  = {14, 18, 22, 26};
constexpr int kConsoleFontCount = 4;
const char* kConsoleFontLabels[] = {"小", "中", "大", "特大"};

const int kConsoleHistory[] = {50, 100, 200, 500};
constexpr int kConsoleHistoryCount = 4;

int indexOfAA(int level) {
    for (int i = 0; i < kAACount; ++i) if (kAALevels[i] == level) return i;
    return 2;
}
int indexOfUiScale(float s) {
    for (int i = 0; i < kUiScaleCount; ++i)
        if (std::abs(kUiScales[i] - s) < 0.01f) return i;
    return 1;
}
int indexOfConsoleFont(int f) {
    for (int i = 0; i < kConsoleFontCount; ++i)
        if (kConsoleFonts[i] == f) return i;
    return 1;
}
int indexOfConsoleHistory(int n) {
    for (int i = 0; i < kConsoleHistoryCount; ++i)
        if (kConsoleHistory[i] == n) return i;
    return 2;
}
int indexOfLogLevel(int l) {
    for (int i = 0; i < kLogCount; ++i)
        if (static_cast<int>(kLogLevels[i]) == l) return i;
    return 2;
}
}

SettingsScene::SettingsScene(std::shared_ptr<Background>    background,
                             std::shared_ptr<Preferences>   preferences,
                             std::shared_ptr<RuntimeConfig> runtimeConfig,
                             std::shared_ptr<Window>        window,
                             const sf::Font&                font,
                             std::shared_ptr<Logger>        logger)
    : background_(std::move(background)),
      preferences_(std::move(preferences)),
      runtimeConfig_(std::move(runtimeConfig)),
      window_(std::move(window)),
      logger_(std::move(logger)),
      font_(font),
      headingDisplay_(font, toSf("显示"), scaledFontSize(26)),
      headingInterface_(font, toSf("界面"), scaledFontSize(26)),
      headingOther_(font, toSf("其他"), scaledFontSize(26)),
      labelResolution_(font, toSf("分辨率"), scaledFontSize(22)),
      labelFullscreen_(font, toSf("全屏"), scaledFontSize(22)),
      labelVsync_(font, toSf("垂直同步"), scaledFontSize(22)),
      labelAntiAliasing_(font, toSf("抗锯齿"), scaledFontSize(22)),
      labelLogLevel_(font, toSf("日志级别"), scaledFontSize(22)),
      labelFps_(font, toSf("帧率显示"), scaledFontSize(22)),
      labelUiScale_(font, toSf("界面缩放"), scaledFontSize(22)),
      labelConsoleMask_(font, toSf("控制台遮罩"), scaledFontSize(22)),
      labelConsoleFont_(font, toSf("控制台字号"), scaledFontSize(22)),
      labelConsoleHistory_(font, toSf("控制台历史"), scaledFontSize(22)),
      labelTheme_(font, toSf("主题"), scaledFontSize(22)),
      labelWallpaper_(font, toSf("壁纸"), scaledFontSize(22)),
      labelRememberSize_(font, toSf("记住窗口大小"), scaledFontSize(22)),
      hintUiScale_(font, toSf("* 修改后返回主菜单再进入生效"), scaledFontSize(16)) {

    selectedResolution_ = clampResolutionIndex(
        preferences_->getInt("resolution_index", 0));
    fullscreen_          = preferences_->getBool("fullscreen", false);
    vsync_               = preferences_->getBool("vsync", true);
    antiAliasingLevel_   = preferences_->getInt("anti_aliasing", 8);
    logLevel_            = preferences_->getInt("log_level",
                            static_cast<int>(LogLevel::Info));
    showFps_             = preferences_->getBool("show_fps", false);
    uiScale_             = static_cast<float>(
                              preferences_->getDouble("ui_scale", 1.0));
    consoleFontSize_     = preferences_->getInt("console_font_size", 18);
    consoleHistoryLines_ = preferences_->getInt("console_history_lines", 200);
    themeId_             = static_cast<ThemeId>(
                              preferences_->getInt("theme", 0));
    rememberSize_        = preferences_->getBool("remember_window_size", true);

    auto headingColor = sf::Color(160, 200, 240);
    headingDisplay_.setFillColor(headingColor);
    headingInterface_.setFillColor(headingColor);
    headingOther_.setFillColor(headingColor);

    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelResolution_, &labelFullscreen_, &labelVsync_,
                    &labelAntiAliasing_, &labelLogLevel_, &labelFps_,
                    &labelUiScale_, &labelConsoleMask_, &labelConsoleFont_,
                    &labelConsoleHistory_, &labelTheme_, &labelWallpaper_,
                    &labelRememberSize_}) {
        t->setFillColor(labelColor);
    }
    hintUiScale_.setFillColor(sf::Color(180, 180, 200));

    // Tab
    const char* tabLabels[] = {"显示", "界面", "其他"};
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_.push_back(std::make_unique<Button>(
            tabLabels[i], font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{180.f, 56.f}, 24));
    }

    // ===== Display =====
    for (int i = 0; i < kResolutionCount; ++i) {
        resolutionButtons_.push_back(std::make_unique<Button>(
            kResolutions[i].label, font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 50.f}, 20));
    }
    auto makeToggle = [&](const std::string& onText, const std::string& offText) {
        auto on  = std::make_unique<Button>(onText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{90.f, 42.f}, 20);
        auto off = std::make_unique<Button>(offText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{90.f, 42.f}, 20);
        return std::make_pair(std::move(on), std::move(off));
    };
    {
        auto [on, off] = makeToggle("开", "关");
        fullscreenOn_ = std::move(on); fullscreenOff_ = std::move(off);
    }
    {
        auto [on, off] = makeToggle("开", "关");
        vsyncOn_ = std::move(on); vsyncOff_ = std::move(off);
    }
    for (int i = 0; i < kAACount; ++i) {
        antiAliasingButtons_.push_back(std::make_unique<Button>(
            kAALabels[i], font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{90.f, 42.f}, 20));
    }
    for (int i = 0; i < kLogCount; ++i) {
        logLevelButtons_.push_back(std::make_unique<Button>(
            kLogLabels[i], font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{100.f, 42.f}, 18));
    }

    // ===== Interface =====
    {
        auto [on, off] = makeToggle("开", "关");
        fpsOn_ = std::move(on); fpsOff_ = std::move(off);
    }
    for (int i = 0; i < kUiScaleCount; ++i) {
        uiScaleButtons_.push_back(std::make_unique<Button>(
            kUiScaleLabels[i], font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{90.f, 42.f}, 20));
    }
    int mask = preferences_->getInt("console_mask", 160);
    mask = std::max(0, std::min(255, mask));
    consoleMaskSlider_ = std::make_unique<Slider>(
        font_, 0.f, 255.f, static_cast<float>(mask),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{260.f, 24.f});

    for (int i = 0; i < kConsoleFontCount; ++i) {
        consoleFontButtons_.push_back(std::make_unique<Button>(
            kConsoleFontLabels[i], font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{90.f, 42.f}, 20));
    }
    for (int i = 0; i < kConsoleHistoryCount; ++i) {
        consoleHistoryButtons_.push_back(std::make_unique<Button>(
            std::to_string(kConsoleHistory[i]), font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{90.f, 42.f}, 20));
    }
    for (int i = 0; i < kThemeCount; ++i) {
        themeButtons_.push_back(std::make_unique<Button>(
            themeName(static_cast<ThemeId>(i)), font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{110.f, 42.f}, 20));
    }
    wallpaperButton_ = std::make_unique<Button>("下一张 →", font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 42.f}, 20);

    // ===== Other =====
    {
        auto [on, off] = makeToggle("开", "关");
        rememberOn_ = std::move(on); rememberOff_ = std::move(off);
    }
    resetButton_ = std::make_unique<Button>("恢复默认设置", font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 50.f}, 22);

    backButton_ = std::make_unique<Button>("返回", font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 55.f}, 24);

    refreshSelection();
}

void SettingsScene::refreshSelection() {
    for (int i = 0; i < kTabCount; ++i)
        tabButtons_[i]->setSelected(i == static_cast<int>(currentTab_));
    for (int i = 0; i < kResolutionCount; ++i)
        resolutionButtons_[i]->setSelected(i == selectedResolution_);
    fullscreenOn_->setSelected(fullscreen_);
    fullscreenOff_->setSelected(!fullscreen_);
    vsyncOn_->setSelected(vsync_);
    vsyncOff_->setSelected(!vsync_);
    int aaIdx = indexOfAA(antiAliasingLevel_);
    for (int i = 0; i < kAACount; ++i)
        antiAliasingButtons_[i]->setSelected(i == aaIdx);
    int lgIdx = indexOfLogLevel(logLevel_);
    for (int i = 0; i < kLogCount; ++i)
        logLevelButtons_[i]->setSelected(i == lgIdx);
    fpsOn_->setSelected(showFps_);
    fpsOff_->setSelected(!showFps_);
    int uiIdx = indexOfUiScale(uiScale_);
    for (int i = 0; i < kUiScaleCount; ++i)
        uiScaleButtons_[i]->setSelected(i == uiIdx);
    int cfIdx = indexOfConsoleFont(consoleFontSize_);
    for (int i = 0; i < kConsoleFontCount; ++i)
        consoleFontButtons_[i]->setSelected(i == cfIdx);
    int chIdx = indexOfConsoleHistory(consoleHistoryLines_);
    for (int i = 0; i < kConsoleHistoryCount; ++i)
        consoleHistoryButtons_[i]->setSelected(i == chIdx);
    for (int i = 0; i < kThemeCount; ++i)
        themeButtons_[i]->setSelected(i == static_cast<int>(themeId_));
    rememberOn_->setSelected(rememberSize_);
    rememberOff_->setSelected(!rememberSize_);
}

void SettingsScene::applyResolution() {
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setInt("resolution_index", selectedResolution_);
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(res.width));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(res.height));
}
void SettingsScene::applyFullscreen() {
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setBool("fullscreen", fullscreen_);
}
void SettingsScene::applyVsync() {
    window_->setVsync(vsync_);
    preferences_->setBool("vsync", vsync_);
}
void SettingsScene::applyAntiAliasing() {
    window_->setAntiAliasing(static_cast<unsigned>(antiAliasingLevel_));
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setInt("anti_aliasing", antiAliasingLevel_);
}
void SettingsScene::applyLogLevel() {
    logger_->setMinLevel(static_cast<LogLevel>(logLevel_));
    preferences_->setInt("log_level", logLevel_);
    logger_->info("日志级别已切换");
}
void SettingsScene::applyTheme() {
    setTheme(themeId_);
    preferences_->setInt("theme", static_cast<int>(themeId_));
    logger_->info("主题已切换");
}
void SettingsScene::applyWallpaper() {
    if (!background_) return;
    if (background_->next()) {
        preferences_->set("current_wallpaper", background_->currentFile());
        logger_->info("壁纸切换: " + background_->currentFile());
    }
}
void SettingsScene::resetAllPreferences() {
    preferences_->resetAll();
    logger_->warn("已恢复默认设置，请重启程序");
}

void SettingsScene::handleEvent(const sf::Event& event) {
    if (resetConfirm_) {
        resetConfirm_->handleEvent(event);
        return;
    }
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
    }
    for (auto& b : tabButtons_) b->handleEvent(event);
    switch (currentTab_) {
        case Tab::Display:
            for (auto& b : resolutionButtons_) b->handleEvent(event);
            fullscreenOn_->handleEvent(event);
            fullscreenOff_->handleEvent(event);
            vsyncOn_->handleEvent(event);
            vsyncOff_->handleEvent(event);
            for (auto& b : antiAliasingButtons_) b->handleEvent(event);
            for (auto& b : logLevelButtons_) b->handleEvent(event);
            break;
        case Tab::Interface:
            fpsOn_->handleEvent(event);
            fpsOff_->handleEvent(event);
            for (auto& b : uiScaleButtons_) b->handleEvent(event);
            consoleMaskSlider_->handleEvent(event);
            for (auto& b : consoleFontButtons_) b->handleEvent(event);
            for (auto& b : consoleHistoryButtons_) b->handleEvent(event);
            for (auto& b : themeButtons_) b->handleEvent(event);
            wallpaperButton_->handleEvent(event);
            break;
        case Tab::Other:
            rememberOn_->handleEvent(event);
            rememberOff_->handleEvent(event);
            resetButton_->handleEvent(event);
            break;
    }
    backButton_->handleEvent(event);
}

void SettingsScene::update(float /*dt*/) {
    if (resetConfirm_) {
        auto r = resetConfirm_->consumeResult();
        if (r == ConfirmDialog::Result::Yes) {
            resetAllPreferences();
            nextScene_ = SceneId::Exit;
        } else if (r == ConfirmDialog::Result::No) {
            resetConfirm_.reset();
        }
        return;
    }

    for (int i = 0; i < kTabCount; ++i) {
        if (tabButtons_[i]->consumeClick()) {
            if (static_cast<int>(currentTab_) != i) {
                currentTab_ = static_cast<Tab>(i);
                refreshSelection();
            }
            return;
        }
    }

    switch (currentTab_) {
        case Tab::Display: {
            for (int i = 0; i < kResolutionCount; ++i) {
                if (resolutionButtons_[i]->consumeClick()) {
                    if (selectedResolution_ != i) {
                        selectedResolution_ = i;
                        refreshSelection();
                        applyResolution();
                    }
                    return;
                }
            }
            if (fullscreenOn_->consumeClick() && !fullscreen_) {
                fullscreen_ = true; refreshSelection(); applyFullscreen(); return;
            }
            if (fullscreenOff_->consumeClick() && fullscreen_) {
                fullscreen_ = false; refreshSelection(); applyFullscreen(); return;
            }
            if (vsyncOn_->consumeClick() && !vsync_) {
                vsync_ = true; refreshSelection(); applyVsync(); return;
            }
            if (vsyncOff_->consumeClick() && vsync_) {
                vsync_ = false; refreshSelection(); applyVsync(); return;
            }
            for (int i = 0; i < kAACount; ++i) {
                if (antiAliasingButtons_[i]->consumeClick()) {
                    if (antiAliasingLevel_ != kAALevels[i]) {
                        antiAliasingLevel_ = kAALevels[i];
                        refreshSelection();
                        applyAntiAliasing();
                    }
                    return;
                }
            }
            for (int i = 0; i < kLogCount; ++i) {
                if (logLevelButtons_[i]->consumeClick()) {
                    int newLevel = static_cast<int>(kLogLevels[i]);
                    if (logLevel_ != newLevel) {
                        logLevel_ = newLevel;
                        refreshSelection();
                        applyLogLevel();
                    }
                    return;
                }
            }
            break;
        }
        case Tab::Interface: {
            if (fpsOn_->consumeClick() && !showFps_) {
                showFps_ = true; refreshSelection();
                preferences_->setBool("show_fps", true);
                return;
            }
            if (fpsOff_->consumeClick() && showFps_) {
                showFps_ = false; refreshSelection();
                preferences_->setBool("show_fps", false);
                return;
            }
            for (int i = 0; i < kUiScaleCount; ++i) {
                if (uiScaleButtons_[i]->consumeClick()) {
                    if (std::abs(uiScale_ - kUiScales[i]) > 0.01f) {
                        uiScale_ = kUiScales[i];
                        refreshSelection();
                        setUiScale(uiScale_);
                        preferences_->setDouble("ui_scale", uiScale_);
                    }
                    return;
                }
            }
            if (consoleMaskSlider_->consumeChanged()) {
                int v = static_cast<int>(consoleMaskSlider_->value());
                preferences_->setInt("console_mask", v);
            }
            for (int i = 0; i < kConsoleFontCount; ++i) {
                if (consoleFontButtons_[i]->consumeClick()) {
                    if (consoleFontSize_ != kConsoleFonts[i]) {
                        consoleFontSize_ = kConsoleFonts[i];
                        refreshSelection();
                        preferences_->setInt("console_font_size", consoleFontSize_);
                    }
                    return;
                }
            }
            for (int i = 0; i < kConsoleHistoryCount; ++i) {
                if (consoleHistoryButtons_[i]->consumeClick()) {
                    if (consoleHistoryLines_ != kConsoleHistory[i]) {
                        consoleHistoryLines_ = kConsoleHistory[i];
                        refreshSelection();
                        preferences_->setInt("console_history_lines",
                                             consoleHistoryLines_);
                    }
                    return;
                }
            }
            for (int i = 0; i < kThemeCount; ++i) {
                if (themeButtons_[i]->consumeClick()) {
                    if (static_cast<int>(themeId_) != i) {
                        themeId_ = static_cast<ThemeId>(i);
                        refreshSelection();
                        applyTheme();
                    }
                    return;
                }
            }
            if (wallpaperButton_->consumeClick()) {
                applyWallpaper();
                return;
            }
            break;
        }
        case Tab::Other: {
            if (rememberOn_->consumeClick() && !rememberSize_) {
                rememberSize_ = true; refreshSelection();
                preferences_->setBool("remember_window_size", true);
                return;
            }
            if (rememberOff_->consumeClick() && rememberSize_) {
                rememberSize_ = false; refreshSelection();
                preferences_->setBool("remember_window_size", false);
                return;
            }
            if (resetButton_->consumeClick()) {
                resetConfirm_ = std::make_unique<ConfirmDialog>(
                    font_, "恢复默认设置？所有自定义将被清除",
                    sf::Vector2f(1280.f, 720.f));
                return;
            }
            break;
        }
    }

    if (backButton_->consumeClick()) {
        nextScene_ = SceneId::Back;
    }
}

void SettingsScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w  = static_cast<float>(size.x);
    float h  = static_cast<float>(size.y);

    // 左侧 Tab
    const float tabX = 40.f;
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i]->setPosition({tabX, 100.f + i * 70.f});
        tabButtons_[i]->render(window.native());
    }

    const float contentX = tabX + 200.f;
    const float ctrlX    = contentX + 240.f;
    const float rowH     = 58.f;
    float y = 100.f;

    switch (currentTab_) {
        case Tab::Display: {
            headingDisplay_.setPosition({contentX, y});
            window.native().draw(headingDisplay_);
            y += 44.f;

            labelResolution_.setPosition({contentX, y + 12.f});
            window.native().draw(labelResolution_);
            const float btnW = 280.f, btnH = 50.f, gapX = 16.f, gapY = 12.f;
            for (int i = 0; i < kResolutionCount; ++i) {
                int row = i / 2, col = i % 2;
                resolutionButtons_[i]->setPosition({
                    ctrlX + col * (btnW + gapX),
                    y + row * (btnH + gapY)});
                resolutionButtons_[i]->render(window.native());
            }
            y += 2 * (btnH + gapY) + 16.f;

            labelFullscreen_.setPosition({contentX, y + 10.f});
            window.native().draw(labelFullscreen_);
            fullscreenOn_->setPosition ({ctrlX, y});
            fullscreenOff_->setPosition({ctrlX + 100.f, y});
            fullscreenOn_->render(window.native());
            fullscreenOff_->render(window.native());
            y += rowH;

            labelVsync_.setPosition({contentX, y + 10.f});
            window.native().draw(labelVsync_);
            vsyncOn_->setPosition ({ctrlX, y});
            vsyncOff_->setPosition({ctrlX + 100.f, y});
            vsyncOn_->render(window.native());
            vsyncOff_->render(window.native());
            y += rowH;

            labelAntiAliasing_.setPosition({contentX, y + 10.f});
            window.native().draw(labelAntiAliasing_);
            for (int i = 0; i < kAACount; ++i) {
                antiAliasingButtons_[i]->setPosition({ctrlX + i * 100.f, y});
                antiAliasingButtons_[i]->render(window.native());
            }
            y += rowH;

            labelLogLevel_.setPosition({contentX, y + 10.f});
            window.native().draw(labelLogLevel_);
            for (int i = 0; i < kLogCount; ++i) {
                logLevelButtons_[i]->setPosition({ctrlX + i * 110.f, y});
                logLevelButtons_[i]->render(window.native());
            }
            break;
        }
        case Tab::Interface: {
            headingInterface_.setPosition({contentX, y});
            window.native().draw(headingInterface_);
            y += 44.f;

            labelFps_.setPosition({contentX, y + 10.f});
            window.native().draw(labelFps_);
            fpsOn_->setPosition ({ctrlX, y});
            fpsOff_->setPosition({ctrlX + 100.f, y});
            fpsOn_->render(window.native());
            fpsOff_->render(window.native());
            y += rowH;

            labelUiScale_.setPosition({contentX, y + 10.f});
            window.native().draw(labelUiScale_);
            for (int i = 0; i < kUiScaleCount; ++i) {
                uiScaleButtons_[i]->setPosition({ctrlX + i * 100.f, y});
                uiScaleButtons_[i]->render(window.native());
            }
            y += rowH;
            hintUiScale_.setPosition({ctrlX, y - 8.f});
            window.native().draw(hintUiScale_);
            y += 22.f;

            labelConsoleMask_.setPosition({contentX, y + 6.f});
            window.native().draw(labelConsoleMask_);
            consoleMaskSlider_->setPosition({ctrlX, y + 2.f});
            consoleMaskSlider_->render(window.native());
            y += rowH;

            labelConsoleFont_.setPosition({contentX, y + 10.f});
            window.native().draw(labelConsoleFont_);
            for (int i = 0; i < kConsoleFontCount; ++i) {
                consoleFontButtons_[i]->setPosition({ctrlX + i * 100.f, y});
                consoleFontButtons_[i]->render(window.native());
            }
            y += rowH;

            labelConsoleHistory_.setPosition({contentX, y + 10.f});
            window.native().draw(labelConsoleHistory_);
            for (int i = 0; i < kConsoleHistoryCount; ++i) {
                consoleHistoryButtons_[i]->setPosition({ctrlX + i * 100.f, y});
                consoleHistoryButtons_[i]->render(window.native());
            }
            y += rowH;

            labelTheme_.setPosition({contentX, y + 10.f});
            window.native().draw(labelTheme_);
            for (int i = 0; i < kThemeCount; ++i) {
                themeButtons_[i]->setPosition({ctrlX + i * 120.f, y});
                themeButtons_[i]->render(window.native());
            }
            y += rowH;

            labelWallpaper_.setPosition({contentX, y + 10.f});
            if (background_) {
                labelWallpaper_.setString(toSf(
                    "壁纸  (" + std::to_string(background_->currentIndex() + 1)
                    + "/" + std::to_string(background_->totalWallpapers()) + ")"));
            }
            window.native().draw(labelWallpaper_);
            wallpaperButton_->setPosition({ctrlX, y});
            wallpaperButton_->render(window.native());
            break;
        }
        case Tab::Other: {
            headingOther_.setPosition({contentX, y});
            window.native().draw(headingOther_);
            y += 44.f;

            labelRememberSize_.setPosition({contentX, y + 10.f});
            window.native().draw(labelRememberSize_);
            rememberOn_->setPosition ({ctrlX, y});
            rememberOff_->setPosition({ctrlX + 100.f, y});
            rememberOn_->render(window.native());
            rememberOff_->render(window.native());
            y += rowH + 30.f;

            resetButton_->setPosition({contentX, y});
            resetButton_->render(window.native());
            break;
        }
    }

    backButton_->setPosition({w - 200.f, h - 80.f});
    backButton_->render(window.native());

    if (resetConfirm_) {
        resetConfirm_->relayout({w, h});
        resetConfirm_->render(window.native());
    }
}