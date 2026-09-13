#include "settings_scene.h"
#include "ui_scale.h"
#include <algorithm>

namespace {
sf::String toSf(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

// 抗锯齿可选档位
const int kAALevels[] = {0, 4, 8, 16};
constexpr int kAACount = 4;
const char* kAALabels[] = {"关", "4x", "8x", "16x"};

// UI 缩放档位
const float kUiScales[] = {0.8f, 1.0f, 1.2f, 1.5f};
constexpr int kUiScaleCount = 4;
const char* kUiScaleLabels[] = {"0.8x", "1.0x", "1.2x", "1.5x"};

// 控制台字号
const int kConsoleFonts[] = {14, 18, 22, 26};
constexpr int kConsoleFontCount = 4;
const char* kConsoleFontLabels[] = {"小", "中", "大", "特大"};

// 控制台历史行数
const int kConsoleHistory[] = {50, 100, 200, 500};
constexpr int kConsoleHistoryCount = 4;

int indexOfAA(int level) {
    for (int i = 0; i < kAACount; ++i) if (kAALevels[i] == level) return i;
    return 2;  // 默认 8x
}
int indexOfUiScale(float s) {
    for (int i = 0; i < kUiScaleCount; ++i) {
        if (std::abs(kUiScales[i] - s) < 0.01f) return i;
    }
    return 1;
}
int indexOfConsoleFont(int f) {
    for (int i = 0; i < kConsoleFontCount; ++i) {
        if (kConsoleFonts[i] == f) return i;
    }
    return 1;
}
int indexOfConsoleHistory(int n) {
    for (int i = 0; i < kConsoleHistoryCount; ++i) {
        if (kConsoleHistory[i] == n) return i;
    }
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
      labelFps_(font, toSf("帧率显示"), scaledFontSize(22)),
      labelUiScale_(font, toSf("界面缩放"), scaledFontSize(22)),
      labelConsoleMask_(font, toSf("控制台遮罩"), scaledFontSize(22)),
      labelConsoleFont_(font, toSf("控制台字号"), scaledFontSize(22)),
      labelConsoleHistory_(font, toSf("控制台历史"), scaledFontSize(22)),
      labelRememberSize_(font, toSf("记住窗口大小"), scaledFontSize(22)),
      hintUiScale_(font, toSf("* 修改后返回主菜单再进入生效"), scaledFontSize(16)) {

    selectedResolution_ = clampResolutionIndex(
        preferences_->getInt("resolution_index", 0));
    fullscreen_          = preferences_->getBool("fullscreen", false);
    vsync_               = preferences_->getBool("vsync", true);
    antiAliasingLevel_   = preferences_->getInt("anti_aliasing", 8);
    showFps_             = preferences_->getBool("show_fps", false);
    uiScale_             = static_cast<float>(
                               preferences_->getDouble("ui_scale", 1.0));
    consoleFontSize_     = preferences_->getInt("console_font_size", 18);
    consoleHistoryLines_ = preferences_->getInt("console_history_lines", 200);
    rememberSize_        = preferences_->getBool("remember_window_size", true);

    auto headingColor = sf::Color(160, 200, 240);
    headingDisplay_.setFillColor(headingColor);
    headingInterface_.setFillColor(headingColor);
    headingOther_.setFillColor(headingColor);

    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelResolution_, &labelFullscreen_, &labelVsync_,
                    &labelAntiAliasing_, &labelFps_, &labelUiScale_,
                    &labelConsoleMask_, &labelConsoleFont_,
                    &labelConsoleHistory_, &labelRememberSize_}) {
        t->setFillColor(labelColor);
    }
    hintUiScale_.setFillColor(sf::Color(160, 160, 180));

    // ===== Tab 按钮 =====
    const char* tabLabels[] = {"显示", "界面", "其他"};
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_.push_back(std::make_unique<Button>(
            tabLabels[i], font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{180.f, 56.f}, 24));
    }

    // ===== 显示 =====
    for (int i = 0; i < kResolutionCount; ++i) {
        resolutionButtons_.push_back(std::make_unique<Button>(
            kResolutions[i].label, font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 50.f}, 20));
    }

    auto makeToggle = [&](const std::string& onText,
                          const std::string& offText) {
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

    // ===== 界面 =====
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

    // ===== 其他 =====
    {
        auto [on, off] = makeToggle("开", "关");
        rememberOn_ = std::move(on); rememberOff_ = std::move(off);
    }

    backButton_ = std::make_unique<Button>("返回", font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 55.f}, 24);

    refreshSelection();
}

void SettingsScene::refreshSelection() {
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i]->setSelected(i == static_cast<int>(currentTab_));
    }
    for (int i = 0; i < kResolutionCount; ++i) {
        resolutionButtons_[i]->setSelected(i == selectedResolution_);
    }
    fullscreenOn_->setSelected(fullscreen_);
    fullscreenOff_->setSelected(!fullscreen_);
    vsyncOn_->setSelected(vsync_);
    vsyncOff_->setSelected(!vsync_);
    int aaIdx = indexOfAA(antiAliasingLevel_);
    for (int i = 0; i < kAACount; ++i) {
        antiAliasingButtons_[i]->setSelected(i == aaIdx);
    }
    fpsOn_->setSelected(showFps_);
    fpsOff_->setSelected(!showFps_);
    int uiIdx = indexOfUiScale(uiScale_);
    for (int i = 0; i < kUiScaleCount; ++i) {
        uiScaleButtons_[i]->setSelected(i == uiIdx);
    }
    int cfIdx = indexOfConsoleFont(consoleFontSize_);
    for (int i = 0; i < kConsoleFontCount; ++i) {
        consoleFontButtons_[i]->setSelected(i == cfIdx);
    }
    int chIdx = indexOfConsoleHistory(consoleHistoryLines_);
    for (int i = 0; i < kConsoleHistoryCount; ++i) {
        consoleHistoryButtons_[i]->setSelected(i == chIdx);
    }
    rememberOn_->setSelected(rememberSize_);
    rememberOff_->setSelected(!rememberSize_);
}

void SettingsScene::applyResolution() {
    const auto& res = kResolutions[selectedResolution_];
    logger_->info("切换分辨率: " + std::string(res.label));
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setInt("resolution_index", selectedResolution_);
    runtimeConfig_->setInt("last_window_width",  static_cast<int>(res.width));
    runtimeConfig_->setInt("last_window_height", static_cast<int>(res.height));
}

void SettingsScene::applyFullscreen() {
    logger_->info(std::string("切换全屏: ") + (fullscreen_ ? "开" : "关"));
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setBool("fullscreen", fullscreen_);
}

void SettingsScene::applyVsync() {
    logger_->info(std::string("切换 V-Sync: ") + (vsync_ ? "开" : "关"));
    window_->setVsync(vsync_);
    preferences_->setBool("vsync", vsync_);
}

void SettingsScene::applyAntiAliasing() {
    logger_->info("切换抗锯齿: " + std::to_string(antiAliasingLevel_) + "x");
    window_->setAntiAliasing(static_cast<unsigned>(antiAliasingLevel_));
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setInt("anti_aliasing", antiAliasingLevel_);
}

void SettingsScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
    }

    // Tab 按钮永远响应
    for (auto& b : tabButtons_) b->handleEvent(event);

    // 当前 Tab 的控件
    switch (currentTab_) {
        case Tab::Display:
            for (auto& b : resolutionButtons_) b->handleEvent(event);
            fullscreenOn_->handleEvent(event);
            fullscreenOff_->handleEvent(event);
            vsyncOn_->handleEvent(event);
            vsyncOff_->handleEvent(event);
            for (auto& b : antiAliasingButtons_) b->handleEvent(event);
            break;
        case Tab::Interface:
            fpsOn_->handleEvent(event);
            fpsOff_->handleEvent(event);
            for (auto& b : uiScaleButtons_) b->handleEvent(event);
            consoleMaskSlider_->handleEvent(event);
            for (auto& b : consoleFontButtons_) b->handleEvent(event);
            for (auto& b : consoleHistoryButtons_) b->handleEvent(event);
            break;
        case Tab::Other:
            rememberOn_->handleEvent(event);
            rememberOff_->handleEvent(event);
            break;
    }

    backButton_->handleEvent(event);
}

void SettingsScene::update(float /*dt*/) {
    // Tab 切换
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
                        logger_->info("UI 缩放: " +
                            std::to_string(static_cast<int>(uiScale_ * 100)) + "%");
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
            break;
        }
    }

    if (backButton_->consumeClick()) {
        nextScene_ = SceneId::Back;
    }
}

void SettingsScene::renderDisplayTab(Window& window, float x, float y, float /*w*/) {
    float rowH = 58.f;

    headingDisplay_.setPosition({x, y});
    window.native().draw(headingDisplay_);
    y += 44.f;

    // 分辨率
    labelResolution_.setPosition({x, y + 12.f});
    window.native().draw(labelResolution_);

    const float btnW = 280.f, btnH = 50.f, gapX = 16.f, gapY = 12.f;
    float ctrlX = x + 240.f;
    for (int i = 0; i < kResolutionCount; ++i) {
        int row = i / 2, col = i % 2;
        resolutionButtons_[i]->setPosition({
            ctrlX + col * (btnW + gapX),
            y + row * (btnH + gapY)
        });
        resolutionButtons_[i]->render(window.native());
    }
    y += 2 * (btnH + gapY) + 16.f;

    // 全屏
    labelFullscreen_.setPosition({x, y + 10.f});
    window.native().draw(labelFullscreen_);
    fullscreenOn_->setPosition ({ctrlX, y});
    fullscreenOff_->setPosition({ctrlX + 100.f, y});
    fullscreenOn_->render(window.native());
    fullscreenOff_->render(window.native());
    y += rowH;

    // V-Sync
    labelVsync_.setPosition({x, y + 10.f});
    window.native().draw(labelVsync_);
    vsyncOn_->setPosition ({ctrlX, y});
    vsyncOff_->setPosition({ctrlX + 100.f, y});
    vsyncOn_->render(window.native());
    vsyncOff_->render(window.native());
    y += rowH;

    // 抗锯齿
    labelAntiAliasing_.setPosition({x, y + 10.f});
    window.native().draw(labelAntiAliasing_);
    for (int i = 0; i < kAACount; ++i) {
        antiAliasingButtons_[i]->setPosition({ctrlX + i * 100.f, y});
        antiAliasingButtons_[i]->render(window.native());
    }
}

void SettingsScene::renderInterfaceTab(Window& window, float x, float y, float /*w*/) {
    float rowH = 58.f;

    headingInterface_.setPosition({x, y});
    window.native().draw(headingInterface_);
    y += 44.f;

    float ctrlX = x + 240.f;

    // 帧率显示
    labelFps_.setPosition({x, y + 10.f});
    window.native().draw(labelFps_);
    fpsOn_->setPosition ({ctrlX, y});
    fpsOff_->setPosition({ctrlX + 100.f, y});
    fpsOn_->render(window.native());
    fpsOff_->render(window.native());
    y += rowH;

    // 界面缩放
    labelUiScale_.setPosition({x, y + 10.f});
    window.native().draw(labelUiScale_);
    for (int i = 0; i < kUiScaleCount; ++i) {
        uiScaleButtons_[i]->setPosition({ctrlX + i * 100.f, y});
        uiScaleButtons_[i]->render(window.native());
    }
    y += rowH;
    hintUiScale_.setPosition({ctrlX, y - 6.f});
    window.native().draw(hintUiScale_);
    y += 24.f;

    // 控制台遮罩
    labelConsoleMask_.setPosition({x, y + 6.f});
    window.native().draw(labelConsoleMask_);
    consoleMaskSlider_->setPosition({ctrlX, y + 2.f});
    consoleMaskSlider_->render(window.native());
    y += rowH;

    // 控制台字号
    labelConsoleFont_.setPosition({x, y + 10.f});
    window.native().draw(labelConsoleFont_);
    for (int i = 0; i < kConsoleFontCount; ++i) {
        consoleFontButtons_[i]->setPosition({ctrlX + i * 100.f, y});
        consoleFontButtons_[i]->render(window.native());
    }
    y += rowH;

    // 控制台历史
    labelConsoleHistory_.setPosition({x, y + 10.f});
    window.native().draw(labelConsoleHistory_);
    for (int i = 0; i < kConsoleHistoryCount; ++i) {
        consoleHistoryButtons_[i]->setPosition({ctrlX + i * 100.f, y});
        consoleHistoryButtons_[i]->render(window.native());
    }
}

void SettingsScene::renderOtherTab(Window& window, float x, float y, float /*w*/) {
    float rowH = 58.f;

    headingOther_.setPosition({x, y});
    window.native().draw(headingOther_);
    y += 44.f;

    float ctrlX = x + 240.f;

    labelRememberSize_.setPosition({x, y + 10.f});
    window.native().draw(labelRememberSize_);
    rememberOn_->setPosition ({ctrlX, y});
    rememberOff_->setPosition({ctrlX + 100.f, y});
    rememberOn_->render(window.native());
    rememberOff_->render(window.native());
}

void SettingsScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w  = static_cast<float>(size.x);
    float h  = static_cast<float>(size.y);

    // ===== 左侧 Tab 区 =====
    const float tabX = 40.f;
    float tabY = 100.f;
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i]->setPosition({tabX, tabY + i * 70.f});
        tabButtons_[i]->render(window.native());
    }

    // ===== 右侧内容区 =====
    const float contentX = tabX + 200.f;
    float contentY = 100.f;
    float contentW = w - contentX - 40.f;

    switch (currentTab_) {
        case Tab::Display:
            renderDisplayTab(window, contentX, contentY, contentW);
            break;
        case Tab::Interface:
            renderInterfaceTab(window, contentX, contentY, contentW);
            break;
        case Tab::Other:
            renderOtherTab(window, contentX, contentY, contentW);
            break;
    }

    // ===== 返回按钮（右下角） =====
    backButton_->setPosition({w - 200.f, h - 80.f});
    backButton_->render(window.native());
}