#include "settings_scene.h"

namespace {
sf::String toSf(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
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
      headingDisplay_(font, toSf("显示"), 26),
      headingInterface_(font, toSf("界面"), 26),
      labelResolution_(font, toSf("分辨率"), 22),
      labelFullscreen_(font, toSf("全屏"), 22),
      labelVsync_(font, toSf("垂直同步"), 22),
      labelFps_(font, toSf("帧率显示"), 22),
      labelConsoleMask_(font, toSf("控制台遮罩"), 22),
      labelRememberSize_(font, toSf("记住窗口大小"), 22) {

    selectedResolution_ = clampResolutionIndex(
        preferences_->getInt("resolution_index", 0));
    fullscreen_    = preferences_->getBool("fullscreen", false);
    vsync_         = preferences_->getBool("vsync", true);
    showFps_       = preferences_->getBool("show_fps", false);
    rememberSize_  = preferences_->getBool("remember_window_size", true);

    headingDisplay_.setFillColor(sf::Color(160, 200, 240));
    headingInterface_.setFillColor(sf::Color(160, 200, 240));
    for (auto* t : {&labelResolution_, &labelFullscreen_, &labelVsync_,
                    &labelFps_, &labelConsoleMask_, &labelRememberSize_}) {
        t->setFillColor(sf::Color(230, 230, 230));
    }

    // 分辨率按钮
    for (int i = 0; i < kResolutionCount; ++i) {
        resolutionButtons_.push_back(std::make_unique<Button>(
            kResolutions[i].label, font,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 50.f}, 20));
    }

    // ⭐ 关键修复：两个按钮分别传 "开" / "关"
    auto makeToggle = [&](const std::string& onText,
                          const std::string& offText) {
        auto on  = std::make_unique<Button>(onText, font,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{90.f, 42.f}, 20);
        auto off = std::make_unique<Button>(offText, font,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{90.f, 42.f}, 20);
        return std::make_pair(std::move(on), std::move(off));
    };

    {
        auto [on, off] = makeToggle("开", "关");
        fullscreenOn_  = std::move(on);
        fullscreenOff_ = std::move(off);
    }
    {
        auto [on, off] = makeToggle("开", "关");
        vsyncOn_  = std::move(on);
        vsyncOff_ = std::move(off);
    }
    {
        auto [on, off] = makeToggle("开", "关");
        fpsOn_  = std::move(on);
        fpsOff_ = std::move(off);
    }
    {
        auto [on, off] = makeToggle("开", "关");
        rememberOn_  = std::move(on);
        rememberOff_ = std::move(off);
    }

    int mask = preferences_->getInt("console_mask", 160);
    mask = std::max(0, std::min(255, mask));
    consoleMaskSlider_ = std::make_unique<Slider>(
        font, 0.f, 255.f, static_cast<float>(mask),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{260.f, 24.f});

    backButton_ = std::make_unique<Button>("返回", font,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 55.f}, 24);

    refreshSelection();
}

void SettingsScene::refreshSelection() {
    for (int i = 0; i < kResolutionCount; ++i) {
        resolutionButtons_[i]->setSelected(i == selectedResolution_);
    }
    fullscreenOn_->setSelected(fullscreen_);
    fullscreenOff_->setSelected(!fullscreen_);
    vsyncOn_->setSelected(vsync_);
    vsyncOff_->setSelected(!vsync_);
    fpsOn_->setSelected(showFps_);
    fpsOff_->setSelected(!showFps_);
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

void SettingsScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
    }
    for (auto& b : resolutionButtons_) b->handleEvent(event);
    fullscreenOn_->handleEvent(event);
    fullscreenOff_->handleEvent(event);
    vsyncOn_->handleEvent(event);
    vsyncOff_->handleEvent(event);
    fpsOn_->handleEvent(event);
    fpsOff_->handleEvent(event);
    rememberOn_->handleEvent(event);
    rememberOff_->handleEvent(event);
    consoleMaskSlider_->handleEvent(event);
    backButton_->handleEvent(event);
}

void SettingsScene::update(float /*dt*/) {
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

    if (consoleMaskSlider_->consumeChanged()) {
        int v = static_cast<int>(consoleMaskSlider_->value());
        preferences_->setInt("console_mask", v);
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
    float cx = w / 2.f;

    const float labelX = cx - 380.f;
    const float ctrlX  = cx - 120.f;
    const float rowH   = 52.f;

    float y = h * 0.08f;

    // ===== 【显示】=====
    headingDisplay_.setPosition({labelX, y});
    window.native().draw(headingDisplay_);
    y += 40.f;

    // 分辨率
    labelResolution_.setPosition({labelX, y + 12.f});
    window.native().draw(labelResolution_);

    const float btnW = 280.f, btnH = 50.f, gapX = 16.f, gapY = 12.f;
    for (int i = 0; i < kResolutionCount; ++i) {
        int row = i / 2;
        int col = i % 2;
        resolutionButtons_[i]->setPosition({
            ctrlX + col * (btnW + gapX),
            y + row * (btnH + gapY)
        });
        resolutionButtons_[i]->render(window.native());
    }
    y += 2 * (btnH + gapY) + 12.f;

    // 全屏
    labelFullscreen_.setPosition({labelX, y + 10.f});
    window.native().draw(labelFullscreen_);
    fullscreenOn_->setPosition ({ctrlX, y});
    fullscreenOff_->setPosition({ctrlX + 100.f, y});
    fullscreenOn_->render(window.native());
    fullscreenOff_->render(window.native());
    y += rowH;

    // V-Sync
    labelVsync_.setPosition({labelX, y + 10.f});
    window.native().draw(labelVsync_);
    vsyncOn_->setPosition ({ctrlX, y});
    vsyncOff_->setPosition({ctrlX + 100.f, y});
    vsyncOn_->render(window.native());
    vsyncOff_->render(window.native());
    y += rowH;

    // 帧率显示
    labelFps_.setPosition({labelX, y + 10.f});
    window.native().draw(labelFps_);
    fpsOn_->setPosition ({ctrlX, y});
    fpsOff_->setPosition({ctrlX + 100.f, y});
    fpsOn_->render(window.native());
    fpsOff_->render(window.native());
    y += rowH + 14.f;

    // ===== 【界面】=====
    headingInterface_.setPosition({labelX, y});
    window.native().draw(headingInterface_);
    y += 40.f;

    // 控制台遮罩
    labelConsoleMask_.setPosition({labelX, y + 6.f});
    window.native().draw(labelConsoleMask_);
    consoleMaskSlider_->setPosition({ctrlX, y + 2.f});
    consoleMaskSlider_->render(window.native());
    y += rowH;

    // 记住窗口大小
    labelRememberSize_.setPosition({labelX, y + 10.f});
    window.native().draw(labelRememberSize_);
    rememberOn_->setPosition ({ctrlX, y});
    rememberOff_->setPosition({ctrlX + 100.f, y});
    rememberOn_->render(window.native());
    rememberOff_->render(window.native());

    // 返回
    backButton_->setPosition({cx - 90.f, h - 90.f});
    backButton_->render(window.native());
}