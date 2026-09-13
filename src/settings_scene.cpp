#include "settings_scene.h"

SettingsScene::SettingsScene(std::shared_ptr<Background>  background,
                             std::shared_ptr<Preferences> preferences,
                             std::shared_ptr<Window>      window,
                             const sf::Font&              font,
                             std::shared_ptr<Logger>      logger)
    : background_(std::move(background)),
      preferences_(std::move(preferences)),
      window_(std::move(window)),
      logger_(std::move(logger)) {

    selectedResolution_ = clampResolutionIndex(
        preferences_->getInt("resolution_index", 0));
    fullscreen_ = preferences_->getBool("fullscreen", false);

    // 分辨率按钮
    for (int i = 0; i < kResolutionCount; ++i) {
        resolutionButtons_.push_back(std::make_unique<Button>(
            kResolutions[i].label, font,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{320.f, 55.f}, 22));
    }

    // 全屏开关
    fullscreenOn_  = std::make_unique<Button>("开", font,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{120.f, 50.f}, 22);
    fullscreenOff_ = std::make_unique<Button>("关", font,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{120.f, 50.f}, 22);

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
}

void SettingsScene::applyResolution() {
    const auto& res = kResolutions[selectedResolution_];
    logger_->info("切换分辨率: " + std::string(res.label));
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setInt("resolution_index", selectedResolution_);
}

void SettingsScene::applyFullscreen() {
    logger_->info(std::string("切换全屏: ") + (fullscreen_ ? "开" : "关"));
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    preferences_->setBool("fullscreen", fullscreen_);
}

void SettingsScene::handleEvent(const sf::Event& event) {
    for (auto& b : resolutionButtons_) b->handleEvent(event);
    fullscreenOn_->handleEvent(event);
    fullscreenOff_->handleEvent(event);
    backButton_->handleEvent(event);
}

void SettingsScene::update(float /*dt*/) {
    // 分辨率
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
    // 全屏
    if (fullscreenOn_->consumeClick() && !fullscreen_) {
        fullscreen_ = true;
        refreshSelection();
        applyFullscreen();
        return;
    }
    if (fullscreenOff_->consumeClick() && fullscreen_) {
        fullscreen_ = false;
        refreshSelection();
        applyFullscreen();
        return;
    }
    // 返回
    if (backButton_->consumeClick()) {
        nextScene_ = SceneId::MainMenu;
    }
}

void SettingsScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);
    float cx = w / 2.f;

    // 分辨率按钮：2 列布局
    const float btnW = 320.f, btnH = 55.f, gapX = 30.f, gapY = 20.f;
    float gridW = btnW * 2 + gapX;
    float startX = cx - gridW / 2.f;
    float startY = h * 0.22f;

    for (int i = 0; i < kResolutionCount; ++i) {
        int row = i / 2;
        int col = i % 2;
        resolutionButtons_[i]->setPosition({
            startX + col * (btnW + gapX),
            startY + row * (btnH + gapY)
        });
        resolutionButtons_[i]->render(window.native());
    }

    // 全屏按钮
    float fsY = startY + 2 * (btnH + gapY) + 20.f;
    fullscreenOn_->setPosition({cx - 140.f, fsY});
    fullscreenOff_->setPosition({cx + 20.f, fsY});
    fullscreenOn_->render(window.native());
    fullscreenOff_->render(window.native());

    // 返回按钮
    backButton_->setPosition({cx - 90.f, h - 90.f});
    backButton_->render(window.native());

    window.display();
}