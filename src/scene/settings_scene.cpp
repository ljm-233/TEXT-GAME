#include "settings_scene.h"
#include "strings.h"
#include "utf8.h"
#include "ui_scale.h"
#include "button_style.h"
#include <algorithm>
#include <cmath>

namespace {
// ===== 常量表 =====
const int kAALevels[]     = {0, 4, 8, 16};
constexpr int kAACount    = 4;
const char* kAALabels[]   = {"关", "4x", "8x", "16x"};

const LogLevel kLogLevels[] = {
    LogLevel::Trace, LogLevel::Debug, LogLevel::Info,
    LogLevel::Warn,  LogLevel::Error
};
constexpr int kLogCount = 5;
const char* kLogLabels[] = {"Trace", "Debug", "Info", "Warn", "Error"};

const int kFpsLimits[]  = {0, 30, 60, 120, 144};
constexpr int kFpsLimitCount = 5;
const char* kFpsLimitLabels[] = {"无", "30", "60", "120", "144"};

const char* kFpsPosLabels[] = {"左上", "右上", "左下", "右下"};
constexpr int kFpsPosCount  = 4;

const float kUiScales[]     = {0.8f, 1.0f, 1.2f, 1.5f};
constexpr int kUiScaleCount = 4;
const char* kUiScaleLabels[] = {"0.8x", "1.0x", "1.2x", "1.5x"};

const int kConsoleFonts[]  = {14, 18, 22, 26};
constexpr int kConsoleFontCount = 4;
const char* kConsoleFontLabels[] = {"小", "中", "大", "特大"};

const int kConsoleHistory[] = {50, 100, 200, 500};
constexpr int kConsoleHistoryCount = 4;

const int kConsoleLineHeights[] = {20, 26, 32};
constexpr int kConsoleLineHeightCount = 3;
const char* kConsoleLineHeightLabels[] = {"紧凑", "正常", "宽松"};

const float kButtonCorners[]  = {0.f, 6.f, 14.f};
constexpr int kButtonCornerCount = 3;
const char* kButtonCornerLabels[] = {"直角", "小圆", "大圆"};

const float kButtonOutlines[]  = {0.f, 2.f, 4.f};
constexpr int kButtonOutlineCount = 3;
const char* kButtonOutlineLabels[] = {"无", "细", "粗"};

const size_t kLogRotateSizes[] = {0, 1*1024*1024, 5*1024*1024, 10*1024*1024};
constexpr int kLogRotateCount = 4;
const char* kLogRotateLabels[] = {"无限", "1MB", "5MB", "10MB"};

const int kLogKeeps[] = {1, 3, 5, 10};
constexpr int kLogKeepCount = 4;

// ===== 布局 =====
constexpr float kTabX     = 40.f;
constexpr float kTabY     = 100.f;
constexpr float kContentX = kTabX + 200.f;
constexpr float kCtrlX    = kContentX + 240.f;
constexpr float kRowH     = 50.f;
constexpr float kBtnW     = 280.f;
constexpr float kBtnH     = 46.f;
constexpr float kGapX     = 16.f;
constexpr float kGapY     = 10.f;

// ===== 索引查找 =====
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
int indexOfConsoleLineHeight(int h) {
    for (int i = 0; i < kConsoleLineHeightCount; ++i)
        if (kConsoleLineHeights[i] == h) return i;
    return 1;
}
int indexOfLogLevel(int l) {
    for (int i = 0; i < kLogCount; ++i)
        if (static_cast<int>(kLogLevels[i]) == l) return i;
    return 2;
}
int indexOfFpsLimit(int l) {
    for (int i = 0; i < kFpsLimitCount; ++i)
        if (kFpsLimits[i] == l) return i;
    return 2;
}
int indexOfButtonCorner(float c) {
    for (int i = 0; i < kButtonCornerCount; ++i)
        if (std::abs(kButtonCorners[i] - c) < 0.5f) return i;
    return 0;
}
int indexOfButtonOutline(float o) {
    for (int i = 0; i < kButtonOutlineCount; ++i)
        if (std::abs(kButtonOutlines[i] - o) < 0.5f) return i;
    return 1;
}
int indexOfLogRotate(int idx) {
    if (idx < 0 || idx >= kLogRotateCount) return 0;
    return idx;
}
int indexOfLogKeep(int idx) {
    if (idx < 0 || idx >= kLogKeepCount) return 1;
    return idx;
}
}

// ============================================================
// 构造
// ============================================================

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
      headingDisplay_  (font, toSf(Str::TabDisplay),   scaledFontSize(24)),
      headingInterface_(font, toSf(Str::TabInterface), scaledFontSize(24)),
      headingOther_    (font, toSf(Str::TabOther),     scaledFontSize(24)),
      labelResolution_      (font, toSf(Str::LabelResolution),      scaledFontSize(20)),
      labelFullscreen_      (font, toSf(Str::LabelFullscreen),      scaledFontSize(20)),
      labelVsync_           (font, toSf(Str::LabelVsync),           scaledFontSize(20)),
      labelAntiAliasing_    (font, toSf(Str::LabelAntiAliasing),    scaledFontSize(20)),
      labelLogLevel_        (font, toSf(Str::LabelLogLevel),        scaledFontSize(20)),
      labelFpsLimit_        (font, toSf(Str::LabelFpsLimit),        scaledFontSize(20)),
      labelFps_             (font, toSf(Str::LabelFps),             scaledFontSize(20)),
      labelFpsPos_          (font, toSf(Str::LabelFpsPos),          scaledFontSize(20)),
      labelUiScale_         (font, toSf(Str::LabelUiScale),         scaledFontSize(20)),
      labelConsoleMask_     (font, toSf(Str::LabelConsoleMask),     scaledFontSize(20)),
      labelConsolePanelAlpha_(font, toSf(Str::LabelConsolePanelAlpha), scaledFontSize(20)),
      labelConsoleFont_     (font, toSf(Str::LabelConsoleFont),     scaledFontSize(20)),
      labelConsoleHistory_  (font, toSf(Str::LabelConsoleHistory),  scaledFontSize(20)),
      labelConsoleLineHeight_(font, toSf(Str::LabelConsoleLineHeight), scaledFontSize(20)),
      labelConsoleAutoScroll_(font, toSf(Str::LabelConsoleAutoScroll), scaledFontSize(20)),
      labelConsoleBlink_    (font, toSf(Str::LabelConsoleBlink),    scaledFontSize(20)),
      labelTheme_           (font, toSf(Str::LabelTheme),           scaledFontSize(20)),
      labelWallpaper_       (font, toSf(Str::LabelWallpaper),       scaledFontSize(20)),
      labelClock_           (font, toSf(Str::LabelClock),           scaledFontSize(20)),
      labelClockPos_        (font, toSf(Str::LabelClockPos),        scaledFontSize(20)),
      labelRememberSize_    (font, toSf(Str::LabelRememberSize),    scaledFontSize(20)),
      labelLogRotate_       (font, toSf(Str::LabelLogRotate),       scaledFontSize(20)),
      labelLogKeep_         (font, toSf(Str::LabelLogKeep),         scaledFontSize(20)),
      labelButtonCorner_    (font, toSf(Str::LabelButtonCorner),    scaledFontSize(20)),
      labelButtonOutline_   (font, toSf(Str::LabelButtonOutline),   scaledFontSize(20)),
      hintUiScale_          (font, toSf(Str::HintUiScale),          scaledFontSize(14)) {

    // ===== 读偏好 =====
    selectedResolution_ = clampResolutionIndex(preferences_->getInt("resolution_index", 0));
    fullscreen_          = preferences_->getBool("fullscreen", false);
    vsync_               = preferences_->getBool("vsync", true);
    antiAliasingLevel_   = preferences_->getInt("anti_aliasing", 8);
    logLevel_            = preferences_->getInt("log_level", static_cast<int>(LogLevel::Info));
    fpsLimit_            = preferences_->getInt("fps_limit", 60);
    showFps_             = preferences_->getBool("show_fps", false);
    fpsPosition_         = preferences_->getInt("fps_position", 1);
    uiScale_             = static_cast<float>(preferences_->getDouble("ui_scale", 1.0));
    consoleMask_         = std::clamp(preferences_->getInt("console_mask", 160), 0, 255);
    consolePanelAlpha_   = std::clamp(preferences_->getInt("console_panel_alpha", 220), 0, 255);
    consoleFontSize_     = preferences_->getInt("console_font_size", 18);
    consoleHistoryLines_ = preferences_->getInt("console_history_lines", 200);
    consoleLineHeight_   = preferences_->getInt("console_line_height", 26);
    consoleAutoScroll_   = preferences_->getBool("console_auto_scroll", true);
    consoleBlinkCursor_  = preferences_->getBool("console_blink_cursor", true);
    themeId_             = static_cast<ThemeId>(preferences_->getInt("theme", 0));
    buttonCorner_        = static_cast<float>(preferences_->getDouble("button_corner", 6.0));
    buttonOutline_       = static_cast<float>(preferences_->getDouble("button_outline", 2.0));
    showClock_           = preferences_->getBool("show_clock", false);
    clockPosition_       = preferences_->getInt("clock_position", 0);
    rememberSize_        = preferences_->getBool("remember_window_size", true);
    logRotateIndex_      = indexOfLogRotate(preferences_->getInt("log_rotate", 0));
    logKeepIndex_        = indexOfLogKeep(preferences_->getInt("log_keep", 1));

    // ===== 标签颜色 =====
    auto headingColor = sf::Color(160, 200, 240);
    headingDisplay_.setFillColor(headingColor);
    headingInterface_.setFillColor(headingColor);
    headingOther_.setFillColor(headingColor);

    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelResolution_, &labelFullscreen_, &labelVsync_,
                    &labelAntiAliasing_, &labelLogLevel_, &labelFpsLimit_,
                    &labelFps_, &labelFpsPos_, &labelUiScale_,
                    &labelConsoleMask_, &labelConsolePanelAlpha_,
                    &labelConsoleFont_, &labelConsoleHistory_,
                    &labelConsoleLineHeight_, &labelConsoleAutoScroll_,
                    &labelConsoleBlink_, &labelTheme_, &labelWallpaper_,
                    &labelClock_, &labelClockPos_,
                    &labelRememberSize_, &labelLogRotate_, &labelLogKeep_,
                    &labelButtonCorner_, &labelButtonOutline_}) {
        t->setFillColor(labelColor);
    }
    hintUiScale_.setFillColor(sf::Color(180, 180, 200));

    // ===== Tab =====
    const char* tabLabels[] = {Str::TabDisplay, Str::TabInterface, Str::TabOther};
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_.push_back(std::make_unique<Button>(
            tabLabels[i], font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 52.f}, 22));
    }

    auto makeToggle = [&](const std::string& onText, const std::string& offText) {
        auto on  = std::make_unique<Button>(onText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(offText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        return std::make_pair(std::move(on), std::move(off));
    };

    // ===== Display =====
    for (int i = 0; i < kResolutionCount; ++i) {
        resolutionButtons_.push_back(std::make_unique<Button>(
            kResolutions[i].label, font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{kBtnW, kBtnH}, 18));
    }
    { auto [on, off] = makeToggle(Str::On, Str::Off); fullscreenOn_ = std::move(on); fullscreenOff_ = std::move(off); }
    { auto [on, off] = makeToggle(Str::On, Str::Off); vsyncOn_      = std::move(on); vsyncOff_      = std::move(off); }
    for (int i = 0; i < kAACount; ++i)
        antiAliasingButtons_.push_back(std::make_unique<Button>(
            kAALabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    for (int i = 0; i < kLogCount; ++i)
        logLevelButtons_.push_back(std::make_unique<Button>(
            kLogLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{96.f, 40.f}, 16));
    for (int i = 0; i < kFpsLimitCount; ++i)
        fpsLimitButtons_.push_back(std::make_unique<Button>(
            kFpsLimitLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));

    // ===== Interface =====
    { auto [on, off] = makeToggle(Str::On, Str::Off); fpsOn_ = std::move(on); fpsOff_ = std::move(off); }
    for (int i = 0; i < kFpsPosCount; ++i)
        fpsPosButtons_.push_back(std::make_unique<Button>(
            kFpsPosLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));
    for (int i = 0; i < kUiScaleCount; ++i)
        uiScaleButtons_.push_back(std::make_unique<Button>(
            kUiScaleLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));

    consoleMaskSlider_ = std::make_unique<Slider>(
        font_, 0.f, 255.f, static_cast<float>(consoleMask_),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    consolePanelAlphaSlider_ = std::make_unique<Slider>(
        font_, 0.f, 255.f, static_cast<float>(consolePanelAlpha_),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});

    for (int i = 0; i < kConsoleFontCount; ++i)
        consoleFontButtons_.push_back(std::make_unique<Button>(
            kConsoleFontLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    for (int i = 0; i < kConsoleHistoryCount; ++i)
        consoleHistoryButtons_.push_back(std::make_unique<Button>(
            std::to_string(kConsoleHistory[i]), font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    for (int i = 0; i < kConsoleLineHeightCount; ++i)
        consoleLineHeightButtons_.push_back(std::make_unique<Button>(
            kConsoleLineHeightLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    { auto [on, off] = makeToggle(Str::On, Str::Off); consoleAutoScrollOn_ = std::move(on); consoleAutoScrollOff_ = std::move(off); }
    { auto [on, off] = makeToggle(Str::On, Str::Off); consoleBlinkOn_ = std::move(on); consoleBlinkOff_ = std::move(off); }

    for (int i = 0; i < kThemeCount; ++i)
        themeButtons_.push_back(std::make_unique<Button>(
            themeName(static_cast<ThemeId>(i)), font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));

    wallpaperButton_ = std::make_unique<Button>(Str::NextWallpaper, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{150.f, 40.f}, 18);

    { auto [on, off] = makeToggle(Str::On, Str::Off); clockOn_ = std::move(on); clockOff_ = std::move(off); }
    for (int i = 0; i < kFpsPosCount; ++i)
        clockPosButtons_.push_back(std::make_unique<Button>(
            kFpsPosLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));

    // ===== Other =====
    { auto [on, off] = makeToggle(Str::On, Str::Off); rememberOn_ = std::move(on); rememberOff_ = std::move(off); }
    for (int i = 0; i < kLogRotateCount; ++i)
        logRotateButtons_.push_back(std::make_unique<Button>(
            kLogRotateLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    for (int i = 0; i < kLogKeepCount; ++i)
        logKeepButtons_.push_back(std::make_unique<Button>(
            std::to_string(kLogKeeps[i]), font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    for (int i = 0; i < kButtonCornerCount; ++i)
        buttonCornerButtons_.push_back(std::make_unique<Button>(
            kButtonCornerLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    for (int i = 0; i < kButtonOutlineCount; ++i)
        buttonOutlineButtons_.push_back(std::make_unique<Button>(
            kButtonOutlineLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));

    aboutButton_ = std::make_unique<Button>(Str::ButtonAbout, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 46.f}, 20);
    resetButton_ = std::make_unique<Button>(Str::ResetDefault, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{220.f, 46.f}, 20);

    backButton_ = std::make_unique<Button>(Str::Back, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 52.f}, 22);

    // 应用初始按钮样式
    {
        ButtonStyle bs;
        bs.cornerRadius = buttonCorner_;
        bs.outlineThickness = buttonOutline_;
        setButtonStyle(bs);
    }

    refreshSelection();
}

// ============================================================
// 选中状态刷新
// ============================================================

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

    int flIdx = indexOfFpsLimit(fpsLimit_);
    for (int i = 0; i < kFpsLimitCount; ++i)
        fpsLimitButtons_[i]->setSelected(i == flIdx);

    fpsOn_->setSelected(showFps_);
    fpsOff_->setSelected(!showFps_);
    for (int i = 0; i < kFpsPosCount; ++i)
        fpsPosButtons_[i]->setSelected(i == fpsPosition_);

    int uiIdx = indexOfUiScale(uiScale_);
    for (int i = 0; i < kUiScaleCount; ++i)
        uiScaleButtons_[i]->setSelected(i == uiIdx);

    int cfIdx = indexOfConsoleFont(consoleFontSize_);
    for (int i = 0; i < kConsoleFontCount; ++i)
        consoleFontButtons_[i]->setSelected(i == cfIdx);

    int chIdx = indexOfConsoleHistory(consoleHistoryLines_);
    for (int i = 0; i < kConsoleHistoryCount; ++i)
        consoleHistoryButtons_[i]->setSelected(i == chIdx);

    int clhIdx = indexOfConsoleLineHeight(consoleLineHeight_);
    for (int i = 0; i < kConsoleLineHeightCount; ++i)
        consoleLineHeightButtons_[i]->setSelected(i == clhIdx);

    consoleAutoScrollOn_->setSelected(consoleAutoScroll_);
    consoleAutoScrollOff_->setSelected(!consoleAutoScroll_);
    consoleBlinkOn_->setSelected(consoleBlinkCursor_);
    consoleBlinkOff_->setSelected(!consoleBlinkCursor_);

    for (int i = 0; i < kThemeCount; ++i)
        themeButtons_[i]->setSelected(i == static_cast<int>(themeId_));

    clockOn_->setSelected(showClock_);
    clockOff_->setSelected(!showClock_);
    for (int i = 0; i < kFpsPosCount; ++i)
        clockPosButtons_[i]->setSelected(i == clockPosition_);

    rememberOn_->setSelected(rememberSize_);
    rememberOff_->setSelected(!rememberSize_);

    for (int i = 0; i < kLogRotateCount; ++i)
        logRotateButtons_[i]->setSelected(i == logRotateIndex_);
    for (int i = 0; i < kLogKeepCount; ++i)
        logKeepButtons_[i]->setSelected(i == logKeepIndex_);

    int bcIdx = indexOfButtonCorner(buttonCorner_);
    for (int i = 0; i < kButtonCornerCount; ++i)
        buttonCornerButtons_[i]->setSelected(i == bcIdx);

    int boIdx = indexOfButtonOutline(buttonOutline_);
    for (int i = 0; i < kButtonOutlineCount; ++i)
        buttonOutlineButtons_[i]->setSelected(i == boIdx);
}

// ============================================================
// 应用状态变更
// ============================================================

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
void SettingsScene::applyFpsPosition() {
    preferences_->setInt("fps_position", fpsPosition_);
}
void SettingsScene::applyFpsLimit() {
    window_->setFramerateLimit(static_cast<unsigned>(fpsLimit_));
    preferences_->setInt("fps_limit", fpsLimit_);
}
void SettingsScene::applyButtonStyle() {
    ButtonStyle bs;
    bs.cornerRadius     = buttonCorner_;
    bs.outlineThickness = buttonOutline_;
    setButtonStyle(bs);
    preferences_->setDouble("button_corner",  buttonCorner_);
    preferences_->setDouble("button_outline", buttonOutline_);
}
void SettingsScene::applyLogRotation() {
    logger_->setRotation(kLogRotateSizes[logRotateIndex_], kLogKeeps[logKeepIndex_]);
    preferences_->setInt("log_rotate", logRotateIndex_);
    preferences_->setInt("log_keep",   logKeepIndex_);
    logger_->info("日志轮转配置已更新");
}
void SettingsScene::resetAllPreferences() {
    preferences_->resetAll();
    logger_->warn("已恢复默认设置，请重启程序");
}

// ============================================================
// 事件
// ============================================================

void SettingsScene::handleEvent(const sf::Event& event) {
    if (resetConfirm_) { resetConfirm_->handleEvent(event); return; }
    if (aboutDialog_)  { aboutDialog_->handleEvent(event);  return; }

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
            fullscreenOn_->handleEvent(event);  fullscreenOff_->handleEvent(event);
            vsyncOn_->handleEvent(event);       vsyncOff_->handleEvent(event);
            for (auto& b : antiAliasingButtons_) b->handleEvent(event);
            for (auto& b : logLevelButtons_) b->handleEvent(event);
            for (auto& b : fpsLimitButtons_) b->handleEvent(event);
            break;
        case Tab::Interface:
            fpsOn_->handleEvent(event);  fpsOff_->handleEvent(event);
            for (auto& b : fpsPosButtons_) b->handleEvent(event);
            for (auto& b : uiScaleButtons_) b->handleEvent(event);
            consoleMaskSlider_->handleEvent(event);
            consolePanelAlphaSlider_->handleEvent(event);
            for (auto& b : consoleFontButtons_) b->handleEvent(event);
            for (auto& b : consoleHistoryButtons_) b->handleEvent(event);
            for (auto& b : consoleLineHeightButtons_) b->handleEvent(event);
            consoleAutoScrollOn_->handleEvent(event);
            consoleAutoScrollOff_->handleEvent(event);
            consoleBlinkOn_->handleEvent(event);
            consoleBlinkOff_->handleEvent(event);
            for (auto& b : themeButtons_) b->handleEvent(event);
            wallpaperButton_->handleEvent(event);
            clockOn_->handleEvent(event); clockOff_->handleEvent(event);
            for (auto& b : clockPosButtons_) b->handleEvent(event);
            break;
        case Tab::Other:
            rememberOn_->handleEvent(event); rememberOff_->handleEvent(event);
            for (auto& b : logRotateButtons_) b->handleEvent(event);
            for (auto& b : logKeepButtons_) b->handleEvent(event);
            for (auto& b : buttonCornerButtons_) b->handleEvent(event);
            for (auto& b : buttonOutlineButtons_) b->handleEvent(event);
            aboutButton_->handleEvent(event);
            resetButton_->handleEvent(event);
            break;
    }
    backButton_->handleEvent(event);
}

// ============================================================
// 更新
// ============================================================

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

    if (aboutDialog_) {
        auto r = aboutDialog_->consumeResult();
        if (r == ConfirmDialog::Result::Ok ||
            r == ConfirmDialog::Result::No) {
            aboutDialog_.reset();
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
            for (int i = 0; i < kFpsLimitCount; ++i) {
                if (fpsLimitButtons_[i]->consumeClick()) {
                    if (fpsLimit_ != kFpsLimits[i]) {
                        fpsLimit_ = kFpsLimits[i];
                        refreshSelection();
                        applyFpsLimit();
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
            for (int i = 0; i < kFpsPosCount; ++i) {
                if (fpsPosButtons_[i]->consumeClick()) {
                    if (fpsPosition_ != i) {
                        fpsPosition_ = i;
                        refreshSelection();
                        applyFpsPosition();
                    }
                    return;
                }
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
                consoleMask_ = static_cast<int>(consoleMaskSlider_->value());
                preferences_->setInt("console_mask", consoleMask_);
            }
            if (consolePanelAlphaSlider_->consumeChanged()) {
                consolePanelAlpha_ = static_cast<int>(consolePanelAlphaSlider_->value());
                preferences_->setInt("console_panel_alpha", consolePanelAlpha_);
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
            for (int i = 0; i < kConsoleLineHeightCount; ++i) {
                if (consoleLineHeightButtons_[i]->consumeClick()) {
                    if (consoleLineHeight_ != kConsoleLineHeights[i]) {
                        consoleLineHeight_ = kConsoleLineHeights[i];
                        refreshSelection();
                        preferences_->setInt("console_line_height",
                                             consoleLineHeight_);
                    }
                    return;
                }
            }
            if (consoleAutoScrollOn_->consumeClick() && !consoleAutoScroll_) {
                consoleAutoScroll_ = true; refreshSelection();
                preferences_->setBool("console_auto_scroll", true);
                return;
            }
            if (consoleAutoScrollOff_->consumeClick() && consoleAutoScroll_) {
                consoleAutoScroll_ = false; refreshSelection();
                preferences_->setBool("console_auto_scroll", false);
                return;
            }
            if (consoleBlinkOn_->consumeClick() && !consoleBlinkCursor_) {
                consoleBlinkCursor_ = true; refreshSelection();
                preferences_->setBool("console_blink_cursor", true);
                return;
            }
            if (consoleBlinkOff_->consumeClick() && consoleBlinkCursor_) {
                consoleBlinkCursor_ = false; refreshSelection();
                preferences_->setBool("console_blink_cursor", false);
                return;
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
            if (clockOn_->consumeClick() && !showClock_) {
                showClock_ = true; refreshSelection();
                preferences_->setBool("show_clock", true);
                return;
            }
            if (clockOff_->consumeClick() && showClock_) {
                showClock_ = false; refreshSelection();
                preferences_->setBool("show_clock", false);
                return;
            }
            for (int i = 0; i < kFpsPosCount; ++i) {
                if (clockPosButtons_[i]->consumeClick()) {
                    if (clockPosition_ != i) {
                        clockPosition_ = i;
                        refreshSelection();
                        preferences_->setInt("clock_position", clockPosition_);
                    }
                    return;
                }
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
            for (int i = 0; i < kLogRotateCount; ++i) {
                if (logRotateButtons_[i]->consumeClick()) {
                    if (logRotateIndex_ != i) {
                        logRotateIndex_ = i;
                        refreshSelection();
                        applyLogRotation();
                    }
                    return;
                }
            }
            for (int i = 0; i < kLogKeepCount; ++i) {
                if (logKeepButtons_[i]->consumeClick()) {
                    if (logKeepIndex_ != i) {
                        logKeepIndex_ = i;
                        refreshSelection();
                        applyLogRotation();
                    }
                    return;
                }
            }
            for (int i = 0; i < kButtonCornerCount; ++i) {
                if (buttonCornerButtons_[i]->consumeClick()) {
                    if (std::abs(buttonCorner_ - kButtonCorners[i]) > 0.5f) {
                        buttonCorner_ = kButtonCorners[i];
                        refreshSelection();
                        applyButtonStyle();
                    }
                    return;
                }
            }
            for (int i = 0; i < kButtonOutlineCount; ++i) {
                if (buttonOutlineButtons_[i]->consumeClick()) {
                    if (std::abs(buttonOutline_ - kButtonOutlines[i]) > 0.5f) {
                        buttonOutline_ = kButtonOutlines[i];
                        refreshSelection();
                        applyButtonStyle();
                    }
                    return;
                }
            }
            if (aboutButton_->consumeClick()) {
                std::string msg =
                    std::string(Str::AboutTitle) + "\n\n"
                    + "版本: " + PROJECT_VERSION + "\n"
                    + "构建: " + BUILD_DATE + "\n"
                    + "作者: ljm-233";
                aboutDialog_ = std::make_unique<ConfirmDialog>(
                    font_, msg, sf::Vector2f(1280.f, 720.f),
                    ConfirmDialog::Mode::Info);
                return;
            }
            if (resetButton_->consumeClick()) {
                resetConfirm_ = std::make_unique<ConfirmDialog>(
                    font_, Str::ResetConfirm,
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

// ============================================================
// 渲染子函数
// ============================================================

void SettingsScene::renderTabs(Window& window) {
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i]->setPosition({kTabX, kTabY + i * 66.f});
        tabButtons_[i]->render(window.native());
    }
}

void SettingsScene::renderDisplayTab(Window& window, float contentX,
                                     float ctrlX, float y) {
    headingDisplay_.setPosition({contentX, y});
    window.native().draw(headingDisplay_);
    y += 36.f;

    labelResolution_.setPosition({contentX, y + 10.f});
    window.native().draw(labelResolution_);
    for (int i = 0; i < kResolutionCount; ++i) {
        int row = i / 2, col = i % 2;
        resolutionButtons_[i]->setPosition({
            ctrlX + col * (kBtnW + kGapX),
            y + row * (kBtnH + kGapY)});
        resolutionButtons_[i]->render(window.native());
    }
    y += 2 * (kBtnH + kGapY) + 6.f;

    auto drawToggleRow = [&](sf::Text& label,
                             const std::unique_ptr<Button>& on,
                             const std::unique_ptr<Button>& off) {
        label.setPosition({contentX, y + 8.f});
        window.native().draw(label);
        on->setPosition ({ctrlX, y});
        off->setPosition({ctrlX + 96.f, y});
        on->render(window.native());
        off->render(window.native());
        y += kRowH;
    };

    auto drawMultiRow = [&](sf::Text& label,
                            std::vector<std::unique_ptr<Button>>& btns,
                            float gap = 96.f) {
        label.setPosition({contentX, y + 8.f});
        window.native().draw(label);
        for (size_t i = 0; i < btns.size(); ++i) {
            btns[i]->setPosition({ctrlX + static_cast<float>(i) * gap, y});
            btns[i]->render(window.native());
        }
        y += kRowH;
    };

    drawToggleRow(labelFullscreen_, fullscreenOn_, fullscreenOff_);
    drawToggleRow(labelVsync_,      vsyncOn_,      vsyncOff_);
    drawMultiRow (labelAntiAliasing_, antiAliasingButtons_, 96.f);
    drawMultiRow (labelLogLevel_,     logLevelButtons_,     106.f);
    drawMultiRow (labelFpsLimit_,     fpsLimitButtons_,     86.f);
}

void SettingsScene::renderInterfaceTab(Window& window, float contentX,
                                       float ctrlX, float y) {
    headingInterface_.setPosition({contentX, y});
    window.native().draw(headingInterface_);
    y += 36.f;

    auto drawToggleRow = [&](sf::Text& label,
                             const std::unique_ptr<Button>& on,
                             const std::unique_ptr<Button>& off) {
        label.setPosition({contentX, y + 8.f});
        window.native().draw(label);
        on->setPosition ({ctrlX, y});
        off->setPosition({ctrlX + 96.f, y});
        on->render(window.native());
        off->render(window.native());
        y += kRowH;
    };

    auto drawMultiRow = [&](sf::Text& label,
                            std::vector<std::unique_ptr<Button>>& btns,
                            float gap = 96.f) {
        label.setPosition({contentX, y + 8.f});
        window.native().draw(label);
        for (size_t i = 0; i < btns.size(); ++i) {
            btns[i]->setPosition({ctrlX + static_cast<float>(i) * gap, y});
            btns[i]->render(window.native());
        }
        y += kRowH;
    };

    auto drawSliderRow = [&](sf::Text& label, Slider* s) {
        label.setPosition({contentX, y + 4.f});
        window.native().draw(label);
        s->setPosition({ctrlX, y + 4.f});
        s->render(window.native());
        y += kRowH;
    };

    // FPS 显示
    drawToggleRow(labelFps_,     fpsOn_,       fpsOff_);
    drawMultiRow (labelFpsPos_,  fpsPosButtons_, 86.f);

    // ⭐ UI 缩放：提示独占一行
    labelUiScale_.setPosition({contentX, y + 8.f});
    window.native().draw(labelUiScale_);
    for (size_t i = 0; i < uiScaleButtons_.size(); ++i) {
        uiScaleButtons_[i]->setPosition({
            ctrlX + static_cast<float>(i) * 96.f, y});
        uiScaleButtons_[i]->render(window.native());
    }
    y += 44.f;
    hintUiScale_.setPosition({contentX, y});
    window.native().draw(hintUiScale_);
    y += 20.f;

    // 控制台相关
    drawSliderRow(labelConsoleMask_,       consoleMaskSlider_.get());
    drawSliderRow(labelConsolePanelAlpha_, consolePanelAlphaSlider_.get());
    drawMultiRow (labelConsoleFont_,       consoleFontButtons_, 96.f);
    drawMultiRow (labelConsoleHistory_,    consoleHistoryButtons_, 96.f);
    drawMultiRow (labelConsoleLineHeight_, consoleLineHeightButtons_, 96.f);
    drawToggleRow(labelConsoleAutoScroll_, consoleAutoScrollOn_, consoleAutoScrollOff_);
    drawToggleRow(labelConsoleBlink_,      consoleBlinkOn_,      consoleBlinkOff_);
    drawMultiRow (labelTheme_,             themeButtons_, 110.f);

    // 壁纸
    if (background_) {
        labelWallpaper_.setString(toSf(
            std::string(Str::LabelWallpaper) + "  ("
            + std::to_string(background_->currentIndex() + 1) + "/"
            + std::to_string(background_->totalWallpapers()) + ")"));
    }
    labelWallpaper_.setPosition({contentX, y + 8.f});
    window.native().draw(labelWallpaper_);
    wallpaperButton_->setPosition({ctrlX, y});
    wallpaperButton_->render(window.native());
    y += kRowH;

    // 时钟
    drawToggleRow(labelClock_,    clockOn_, clockOff_);
    drawMultiRow (labelClockPos_, clockPosButtons_, 86.f);
}

void SettingsScene::renderOtherTab(Window& window, float contentX,
                                   float ctrlX, float y) {
    headingOther_.setPosition({contentX, y});
    window.native().draw(headingOther_);
    y += 36.f;

    auto drawToggleRow = [&](sf::Text& label,
                             const std::unique_ptr<Button>& on,
                             const std::unique_ptr<Button>& off) {
        label.setPosition({contentX, y + 8.f});
        window.native().draw(label);
        on->setPosition ({ctrlX, y});
        off->setPosition({ctrlX + 96.f, y});
        on->render(window.native());
        off->render(window.native());
        y += kRowH;
    };

    auto drawMultiRow = [&](sf::Text& label,
                            std::vector<std::unique_ptr<Button>>& btns,
                            float gap = 96.f) {
        label.setPosition({contentX, y + 8.f});
        window.native().draw(label);
        for (size_t i = 0; i < btns.size(); ++i) {
            btns[i]->setPosition({ctrlX + static_cast<float>(i) * gap, y});
            btns[i]->render(window.native());
        }
        y += kRowH;
    };

    drawToggleRow(labelRememberSize_, rememberOn_, rememberOff_);
    drawMultiRow (labelLogRotate_,    logRotateButtons_, 96.f);
    drawMultiRow (labelLogKeep_,      logKeepButtons_,   96.f);
    drawMultiRow (labelButtonCorner_, buttonCornerButtons_, 96.f);
    drawMultiRow (labelButtonOutline_,buttonOutlineButtons_,96.f);

    y += 20.f;
    aboutButton_->setPosition({contentX, y});
    aboutButton_->render(window.native());

    resetButton_->setPosition({contentX + 220.f, y});
    resetButton_->render(window.native());
}

void SettingsScene::renderBackButton(Window& window) {
    auto size = window.native().getSize();
    backButton_->setPosition({static_cast<float>(size.x) - 200.f,
                              static_cast<float>(size.y) - 70.f});
    backButton_->render(window.native());
}

// ============================================================
// 主渲染
// ============================================================

void SettingsScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w  = static_cast<float>(size.x);
    float h  = static_cast<float>(size.y);

    renderTabs(window);

    switch (currentTab_) {
        case Tab::Display:
            renderDisplayTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Interface:
            renderInterfaceTab(window, kContentX, kCtrlX, 50.f);
            break;
        case Tab::Other:
            renderOtherTab(window, kContentX, kCtrlX, 60.f);
            break;
    }

    renderBackButton(window);

    if (resetConfirm_) {
        resetConfirm_->relayout({w, h});
        resetConfirm_->render(window.native());
    }
    if (aboutDialog_) {
        aboutDialog_->relayout({w, h});
        aboutDialog_->render(window.native());
    }
}