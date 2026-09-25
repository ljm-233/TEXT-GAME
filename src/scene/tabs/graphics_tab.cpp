#include "tabs/graphics_tab.h"
#include "text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include "animation.h"
#include "notification.h"
#include "button_style.h"
#include "focus_group.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr float kRowH = 50.f;

// ⭐ 画面预设
struct PostPreset {
    float saturation, contrast, brightness, gamma, vignette;
    float bloomStrength, bloomThreshold;
    float chromatic, grain, scanline, dither;
};

// 顺序必须和 presetRow_ 里按钮的顺序一致
const PostPreset kPresets[] = {
    // 原版
    {100, 100, 100, 100,  0,    0, 70,   0,  0,  0,  0},
    // 复古 CRT
    {105, 110, 105, 105, 40,   30, 60,  35, 15, 30,  0},
    // 电影感
    {110, 115,  95, 100, 50,   60, 55,  15, 20,  0,  0},
    // 像素 8-bit
    {120, 105, 100, 100, 15,   20, 70,  10, 10, 20, 55},
    // 夜晚
    { 90, 110,  85, 100, 60,   50, 50,  20, 20,  0,  0},
};
constexpr int kPresetCount = 5;

int indexOfLives(int lives) {
    const int kLives[] = {1, 3, 5, 10, 100};
    for (int i = 0; i < 5; ++i) if (kLives[i] == lives) return i;
    return 0;
}
int indexOfButtonCorner(float c) {
    const float kCorners[] = {0.f, 6.f, 14.f};
    for (int i = 0; i < 3; ++i)
        if (std::abs(kCorners[i] - c) < 0.5f) return i;
    return 1;
}
int indexOfButtonOutline(float o) {
    const float kOutlines[] = {0.f, 2.f, 4.f};
    for (int i = 0; i < 3; ++i)
        if (std::abs(kOutlines[i] - o) < 0.5f) return i;
    return 1;
}

} // namespace

GraphicsTab::GraphicsTab(const sf::Font& font,
                         std::shared_ptr<Preferences> prefs,
                         std::shared_ptr<Window> window)
      : font_(font),
        prefs_(std::move(prefs)),
        window_(std::move(window)),
        labelPostSaturation_    (font, sf::String(), scaledFontSize(20)),
        labelPostContrast_      (font, sf::String(), scaledFontSize(20)),
        labelPostBrightness_    (font, sf::String(), scaledFontSize(20)),
        labelPostGamma_         (font, sf::String(), scaledFontSize(20)),
        labelPostVignette_      (font, sf::String(), scaledFontSize(20)),
        labelPostBloomStrength_ (font, sf::String(), scaledFontSize(20)),
        labelPostBloomThreshold_(font, sf::String(), scaledFontSize(20)),
        labelPostChromatic_     (font, sf::String(), scaledFontSize(20)),
        labelPostGrain_         (font, sf::String(), scaledFontSize(20)),
        labelPostScanline_      (font, sf::String(), scaledFontSize(20)),
        labelPostDither_        (font, sf::String(), scaledFontSize(20)) {

    loadFromPrefs();

    // ===== 标签颜色 =====
    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelPostSaturation_, &labelPostContrast_,
                    &labelPostBrightness_, &labelPostGamma_,
                    &labelPostVignette_, &labelPostBloomStrength_,
                    &labelPostBloomThreshold_, &labelPostChromatic_,
                    &labelPostGrain_, &labelPostScanline_, &labelPostDither_}) {
        t->setFillColor(labelColor);
    }

    // ===== Toggle 辅助 =====
    auto makeToggle = [&](const std::string& onText,
                          const std::string& offText) {
        auto on  = std::make_unique<Button>(onText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(offText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        return std::make_pair(std::move(on), std::move(off));
    };
    auto addToggle = [&](const char* key, std::function<void(bool)> cb) {
        auto row = std::make_unique<ToggleRow>(font_, key, std::move(cb));
        auto [on, off] = makeToggle(Str::On, Str::Off);
        row->onButton  = std::move(on);
        row->offButton = std::move(off);
        toggles_.push_back(std::move(row));
    };
    auto addMultiRow = [&](const char* key,
                           std::function<void(int)> cb) {
        auto row = std::make_unique<MultiRow>(font_, key, std::move(cb));
        multiRows_.push_back(std::move(row));
        return multiRows_.back().get();
    };

    // ===== InitialLives =====
    {
        const char* kLivesLabels[] = {"1", "3", "5", "10", "100"};
        auto* row = addMultiRow(Str::LabelInitialLives, [this](int i) {
            const int kLives[] = {1, 3, 5, 10, 100};
            initialLives_ = kLives[i];
            refreshSelection();
            prefs_->setInt("initial_lives", initialLives_);
        });
        for (int i = 0; i < 5; ++i) {
            row->addButton(std::make_unique<Button>(
                kLivesLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 18));
        }
    }

    // ===== Toggles =====
    addToggle(Str::LabelAnimation, [this](bool v) {
        animationEnabled_ = v; refreshSelection(); applyAnimation();
    });
    addToggle(Str::LabelPseudo3D, [this](bool v) {
        pseudo3D_ = v; refreshSelection(); applyPseudo3D();
    });
    addToggle(Str::LabelParallax, [this](bool v) {
        parallaxEnabled_ = v; refreshSelection(); applyParallax();
    });
    addToggle(Str::LabelPlayerAnimation, [this](bool v) {
        playerAnimEnabled_ = v; refreshSelection(); applyPlayerAnimation();
    });
    addToggle(Str::LabelLevelIntro, [this](bool v) {
        levelIntroEnabled_ = v; refreshSelection(); applyLevelIntro();
    });
    addToggle(Str::LabelParticles, [this](bool v) {
        particlesEnabled_ = v; refreshSelection(); applyParticles();
    });
    addToggle(Str::LabelScreenShake, [this](bool v) {
        screenShake_ = v; refreshSelection(); applyScreenShake();
    });
    addToggle(Str::LabelNotification, [this](bool v) {
        notificationEnabled_ = v; refreshSelection(); applyNotification();
    });
    addToggle(Str::LabelShowColliders, [this](bool v) {
        showColliders_ = v; refreshSelection(); applyShowColliders();
    });

    // ===== AnimationSpeed =====
    {
        const char* kAnimSpeedLabels[] = {"慢", "正常", "快"};
        auto* row = addMultiRow(Str::LabelAnimationSpeed, [this](int i) {
            animationSpeedIndex_ = i;
            refreshSelection(); applyAnimation();
        });
        for (int i = 0; i < 3; ++i) {
            row->addButton(std::make_unique<Button>(
                kAnimSpeedLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
        }
    }

    // ===== NotificationPos =====
    {
        const char* kPosLabels[] = {"左上", "右上", "左下", "右下"};
        auto* row = addMultiRow(Str::LabelNotificationPos, [this](int i) {
            notificationPosition_ = i;
            refreshSelection(); applyNotification();
        });
        row->stepX = 86.f;
        for (int i = 0; i < 4; ++i) {
            row->addButton(std::make_unique<Button>(
                kPosLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));
        }
    }

    // ===== ButtonCorner =====
    {
        const char* kLabels[] = {"直角", "小圆", "大圆"};
        auto* row = addMultiRow(Str::LabelButtonCorner, [this](int i) {
            const float kCorners[] = {0.f, 6.f, 14.f};
            buttonCorner_ = kCorners[i];
            refreshSelection(); applyButtonStyle();
        });
        for (int i = 0; i < 3; ++i) {
            row->addButton(std::make_unique<Button>(
                kLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
        }
    }

    // ===== ButtonOutline =====
    {
        const char* kLabels[] = {"无", "细", "粗"};
        auto* row = addMultiRow(Str::LabelButtonOutline, [this](int i) {
            const float kOutlines[] = {0.f, 2.f, 4.f};
            buttonOutline_ = kOutlines[i];
            refreshSelection(); applyButtonStyle();
        });
        for (int i = 0; i < 3; ++i) {
            row->addButton(std::make_unique<Button>(
                kLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
        }
    }

    // ===== 预设 =====
    {
        presetRow_ = std::make_unique<MultiRow>(
            font_, Str::LabelPreset, [this](int i) { applyPreset(i); });
        presetRow_->stepX = 112.f;
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetDefault),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetCRT),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetCinematic),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetPixel8),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetNight),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
    }

    // ===== 后处理 Slider =====
    {
        auto& pp = window_->postProcess();
        saturationSlider_ = std::make_unique<Slider>(
            font_, 0.f, 200.f, pp.saturation() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        saturationSlider_->setDefaultValue(100.f);

        contrastSlider_ = std::make_unique<Slider>(
            font_, 50.f, 200.f, pp.contrast() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        contrastSlider_->setDefaultValue(100.f);

        brightnessSlider_ = std::make_unique<Slider>(
            font_, 50.f, 200.f, pp.brightness() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        brightnessSlider_->setDefaultValue(100.f);

        gammaSlider_ = std::make_unique<Slider>(
            font_, 50.f, 250.f, pp.gamma() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        gammaSlider_->setDefaultValue(100.f);

        vignetteSlider_ = std::make_unique<Slider>(
            font_, 0.f, 100.f, pp.vignette() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        vignetteSlider_->setDefaultValue(0.f);

        bloomStrengthSlider_ = std::make_unique<Slider>(
            font_, 0.f, 200.f, pp.bloomStrength() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        bloomStrengthSlider_->setDefaultValue(0.f);

        bloomThresholdSlider_ = std::make_unique<Slider>(
            font_, 0.f, 100.f, pp.bloomThreshold() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        bloomThresholdSlider_->setDefaultValue(70.f);

        chromaticSlider_ = std::make_unique<Slider>(
            font_, 0.f, 100.f, pp.chromatic() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        chromaticSlider_->setDefaultValue(0.f);

        grainSlider_ = std::make_unique<Slider>(
            font_, 0.f, 100.f, pp.grain() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        grainSlider_->setDefaultValue(0.f);

        scanlineSlider_ = std::make_unique<Slider>(
            font_, 0.f, 100.f, pp.scanline() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        scanlineSlider_->setDefaultValue(0.f);

        ditherSlider_ = std::make_unique<Slider>(
            font_, 0.f, 100.f, pp.dither() * 100.f,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        ditherSlider_->setDefaultValue(0.f);
    }

    refreshLabels();
    refreshSelection();
}

void GraphicsTab::loadFromPrefs() {
    animationEnabled_      = prefs_->getBool("animation_enabled", true);
    animationSpeedIndex_   = std::clamp(prefs_->getInt("animation_speed_index", 1), 0, 2);
    pseudo3D_              = prefs_->getBool("pseudo_3d", true);
    parallaxEnabled_       = prefs_->getBool("parallax", true);
    playerAnimEnabled_     = prefs_->getBool("player_animation", true);
    levelIntroEnabled_     = prefs_->getBool("level_intro", true);
    particlesEnabled_      = prefs_->getBool("particles", true);
    screenShake_           = prefs_->getBool("screen_shake", true);
    notificationEnabled_   = prefs_->getBool("notification_enabled", true);
    showColliders_         = prefs_->getBool("show_colliders", false);
    initialLives_          = prefs_->getInt("initial_lives", 1);
    notificationPosition_  = prefs_->getInt("notification_position", 1);
    buttonCorner_          = static_cast<float>(prefs_->getDouble("button_corner", 6.0));
    buttonOutline_         = static_cast<float>(prefs_->getDouble("button_outline", 2.0));
}

void GraphicsTab::applyAnimation() {
    Anim::setEnabled(animationEnabled_);
    static const float kSpeeds[] = {0.5f, 1.0f, 2.0f};
    Anim::setSpeed(kSpeeds[std::clamp(animationSpeedIndex_, 0, 2)]);
    prefs_->setBool("animation_enabled", animationEnabled_);
    prefs_->setInt("animation_speed_index", animationSpeedIndex_);
}
void GraphicsTab::applyPseudo3D()     { prefs_->setBool("pseudo_3d", pseudo3D_); }
void GraphicsTab::applyParallax()     { prefs_->setBool("parallax", parallaxEnabled_); }
void GraphicsTab::applyPlayerAnimation() { prefs_->setBool("player_animation", playerAnimEnabled_); }
void GraphicsTab::applyLevelIntro()   { prefs_->setBool("level_intro", levelIntroEnabled_); }
void GraphicsTab::applyParticles()    { prefs_->setBool("particles", particlesEnabled_); }
void GraphicsTab::applyScreenShake()  { prefs_->setBool("screen_shake", screenShake_); }
void GraphicsTab::applyShowColliders(){ prefs_->setBool("show_colliders", showColliders_); }
void GraphicsTab::applyNotification() {
    NotificationSystem::instance().setEnabled(notificationEnabled_);
    NotificationSystem::instance().setPosition(
        static_cast<NotificationPos>(notificationPosition_));
    prefs_->setBool("notification_enabled", notificationEnabled_);
    prefs_->setInt("notification_position", notificationPosition_);
}
void GraphicsTab::applyButtonStyle() {
    ButtonStyle bs;
    bs.cornerRadius     = buttonCorner_;
    bs.outlineThickness = buttonOutline_;
    setButtonStyle(bs);
    prefs_->setDouble("button_corner",  buttonCorner_);
    prefs_->setDouble("button_outline", buttonOutline_);
}

void GraphicsTab::applyPreset(int idx) {
    if (idx < 0 || idx >= kPresetCount) return;
    const auto& p = kPresets[idx];

    auto& pp = window_->postProcess();
    pp.setSaturation    (p.saturation      / 100.f);
    pp.setContrast      (p.contrast        / 100.f);
    pp.setBrightness    (p.brightness      / 100.f);
    pp.setGamma         (p.gamma           / 100.f);
    pp.setVignette      (p.vignette        / 100.f);
    pp.setBloomStrength (p.bloomStrength   / 100.f);
    pp.setBloomThreshold(p.bloomThreshold  / 100.f);
    pp.setChromatic     (p.chromatic       / 100.f);
    pp.setGrain         (p.grain           / 100.f);
    pp.setScanline      (p.scanline        / 100.f);
    pp.setDither        (p.dither          / 100.f);

    saturationSlider_->setValue(p.saturation);
    contrastSlider_->setValue(p.contrast);
    brightnessSlider_->setValue(p.brightness);
    gammaSlider_->setValue(p.gamma);
    vignetteSlider_->setValue(p.vignette);
    bloomStrengthSlider_->setValue(p.bloomStrength);
    bloomThresholdSlider_->setValue(p.bloomThreshold);
    chromaticSlider_->setValue(p.chromatic);
    grainSlider_->setValue(p.grain);
    scanlineSlider_->setValue(p.scanline);
    ditherSlider_->setValue(p.dither);

    prefs_->setDouble("post_saturation",      p.saturation      / 100.f);
    prefs_->setDouble("post_contrast",        p.contrast        / 100.f);
    prefs_->setDouble("post_brightness",      p.brightness      / 100.f);
    prefs_->setDouble("post_gamma",           p.gamma           / 100.f);
    prefs_->setDouble("post_vignette",        p.vignette        / 100.f);
    prefs_->setDouble("post_bloom_strength",  p.bloomStrength   / 100.f);
    prefs_->setDouble("post_bloom_threshold", p.bloomThreshold  / 100.f);
    prefs_->setDouble("post_chromatic",       p.chromatic       / 100.f);
    prefs_->setDouble("post_grain",           p.grain           / 100.f);
    prefs_->setDouble("post_scanline",        p.scanline        / 100.f);
    prefs_->setDouble("post_dither",          p.dither          / 100.f);
}

void GraphicsTab::resetPost() {
    auto& pp = window_->postProcess();
    pp.setSaturation(1.f);
    pp.setContrast(1.f);
    pp.setBrightness(1.f);
    pp.setGamma(1.f);
    pp.setVignette(0.f);
    pp.setBloomStrength(0.f);
    pp.setBloomThreshold(0.7f);
    pp.setChromatic(0.f);
    pp.setGrain(0.f);
    pp.setScanline(0.f);
    pp.setDither(0.f);

    saturationSlider_->setValue(100.f);
    contrastSlider_->setValue(100.f);
    brightnessSlider_->setValue(100.f);
    gammaSlider_->setValue(100.f);
    vignetteSlider_->setValue(0.f);
    bloomStrengthSlider_->setValue(0.f);
    bloomThresholdSlider_->setValue(70.f);
    chromaticSlider_->setValue(0.f);
    grainSlider_->setValue(0.f);
    scanlineSlider_->setValue(0.f);
    ditherSlider_->setValue(0.f);

    prefs_->setDouble("post_saturation", 1.0);
    prefs_->setDouble("post_contrast", 1.0);
    prefs_->setDouble("post_brightness", 1.0);
    prefs_->setDouble("post_gamma", 1.0);
    prefs_->setDouble("post_vignette", 0.0);
    prefs_->setDouble("post_bloom_strength", 0.0);
    prefs_->setDouble("post_bloom_threshold", 0.7);
    prefs_->setDouble("post_chromatic", 0.0);
    prefs_->setDouble("post_grain", 0.0);
    prefs_->setDouble("post_scanline", 0.0);
    prefs_->setDouble("post_dither", 0.0);
}

void GraphicsTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_) row->refreshLabel();
    if (presetRow_) {
        presetRow_->label.setString(toSf(Str::T(Str::LabelPreset)));
        if (presetRow_->buttons.size() >= 5) {
            presetRow_->buttons[0]->setText(Str::T(Str::PresetDefault));
            presetRow_->buttons[1]->setText(Str::T(Str::PresetCRT));
            presetRow_->buttons[2]->setText(Str::T(Str::PresetCinematic));
            presetRow_->buttons[3]->setText(Str::T(Str::PresetPixel8));
            presetRow_->buttons[4]->setText(Str::T(Str::PresetNight));
        }
    }
    labelPostSaturation_    .setString(toSf(Str::T(Str::LabelPostSaturation)));
    labelPostContrast_      .setString(toSf(Str::T(Str::LabelPostContrast)));
    labelPostBrightness_    .setString(toSf(Str::T(Str::LabelPostBrightness)));
    labelPostGamma_         .setString(toSf(Str::T(Str::LabelPostGamma)));
    labelPostVignette_      .setString(toSf(Str::T(Str::LabelPostVignette)));
    labelPostBloomStrength_ .setString(toSf(Str::T(Str::LabelPostBloomStrength)));
    labelPostBloomThreshold_.setString(toSf(Str::T(Str::LabelPostBloomThreshold)));
    labelPostChromatic_     .setString(toSf(Str::T(Str::LabelPostChromatic)));
    labelPostGrain_         .setString(toSf(Str::T(Str::LabelPostGrain)));
    labelPostScanline_      .setString(toSf(Str::T(Str::LabelPostScanline)));
    labelPostDither_        .setString(toSf(Str::T(Str::LabelPostDither)));
}

void GraphicsTab::refreshSelection() {
    if (toggles_.size() == 9) {
        toggles_[0]->currentValue = animationEnabled_;       toggles_[0]->onButton->setSelected(animationEnabled_);       toggles_[0]->offButton->setSelected(!animationEnabled_);
        toggles_[1]->currentValue = pseudo3D_;               toggles_[1]->onButton->setSelected(pseudo3D_);               toggles_[1]->offButton->setSelected(!pseudo3D_);
        toggles_[2]->currentValue = parallaxEnabled_;        toggles_[2]->onButton->setSelected(parallaxEnabled_);        toggles_[2]->offButton->setSelected(!parallaxEnabled_);
        toggles_[3]->currentValue = playerAnimEnabled_;      toggles_[3]->onButton->setSelected(playerAnimEnabled_);      toggles_[3]->offButton->setSelected(!playerAnimEnabled_);
        toggles_[4]->currentValue = levelIntroEnabled_;      toggles_[4]->onButton->setSelected(levelIntroEnabled_);      toggles_[4]->offButton->setSelected(!levelIntroEnabled_);
        toggles_[5]->currentValue = particlesEnabled_;       toggles_[5]->onButton->setSelected(particlesEnabled_);       toggles_[5]->offButton->setSelected(!particlesEnabled_);
        toggles_[6]->currentValue = screenShake_;            toggles_[6]->onButton->setSelected(screenShake_);            toggles_[6]->offButton->setSelected(!screenShake_);
        toggles_[7]->currentValue = notificationEnabled_;    toggles_[7]->onButton->setSelected(notificationEnabled_);    toggles_[7]->offButton->setSelected(!notificationEnabled_);
        toggles_[8]->currentValue = showColliders_;          toggles_[8]->onButton->setSelected(showColliders_);          toggles_[8]->offButton->setSelected(!showColliders_);
    }
    if (multiRows_.size() == 5) {
        multiRows_[0]->setSelected(indexOfLives(initialLives_));
        multiRows_[1]->setSelected(std::clamp(animationSpeedIndex_, 0, 2));
        multiRows_[2]->setSelected(notificationPosition_);
        multiRows_[3]->setSelected(indexOfButtonCorner(buttonCorner_));
        multiRows_[4]->setSelected(indexOfButtonOutline(buttonOutline_));
    }
}

void GraphicsTab::handleEvent(const sf::Event& ev) {
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons) btn->handleEvent(ev);
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& btn : presetRow_->buttons) btn->handleEvent(ev);
    saturationSlider_->handleEvent(ev);
    contrastSlider_->handleEvent(ev);
    brightnessSlider_->handleEvent(ev);
    gammaSlider_->handleEvent(ev);
    vignetteSlider_->handleEvent(ev);
    bloomStrengthSlider_->handleEvent(ev);
    bloomThresholdSlider_->handleEvent(ev);
    chromaticSlider_->handleEvent(ev);
    grainSlider_->handleEvent(ev);
    scanlineSlider_->handleEvent(ev);
    ditherSlider_->handleEvent(ev);
}

void GraphicsTab::update() {
    // multiRows
    for (auto& row : multiRows_) {
        for (size_t i = 0; i < row->buttons.size(); ++i) {
            if (row->buttons[i]->consumeClick()) {
                if (row->currentIndex != static_cast<int>(i) && row->onSelected)
                    row->onSelected(static_cast<int>(i));
                return;
            }
        }
    }
    // 预设
    for (size_t i = 0; i < presetRow_->buttons.size(); ++i) {
        if (presetRow_->buttons[i]->consumeClick()) {
            applyPreset(static_cast<int>(i));
            return;
        }
    }
    // toggles
    for (auto& row : toggles_) {
        if (row->onButton->consumeClick() && !row->currentValue) {
            if (row->onChanged) row->onChanged(true);
            return;
        }
        if (row->offButton->consumeClick() && row->currentValue) {
            if (row->onChanged) row->onChanged(false);
            return;
        }
    }
    // 后处理 slider
    {
        auto& pp = window_->postProcess();
        if (saturationSlider_->consumeChanged()) {
            pp.setSaturation(saturationSlider_->value() / 100.f);
            prefs_->setDouble("post_saturation", pp.saturation());
        }
        if (contrastSlider_->consumeChanged()) {
            pp.setContrast(contrastSlider_->value() / 100.f);
            prefs_->setDouble("post_contrast", pp.contrast());
        }
        if (brightnessSlider_->consumeChanged()) {
            pp.setBrightness(brightnessSlider_->value() / 100.f);
            prefs_->setDouble("post_brightness", pp.brightness());
        }
        if (gammaSlider_->consumeChanged()) {
            pp.setGamma(gammaSlider_->value() / 100.f);
            prefs_->setDouble("post_gamma", pp.gamma());
        }
        if (vignetteSlider_->consumeChanged()) {
            pp.setVignette(vignetteSlider_->value() / 100.f);
            prefs_->setDouble("post_vignette", pp.vignette());
        }
        if (bloomStrengthSlider_->consumeChanged()) {
            pp.setBloomStrength(bloomStrengthSlider_->value() / 100.f);
            prefs_->setDouble("post_bloom_strength", pp.bloomStrength());
        }
        if (bloomThresholdSlider_->consumeChanged()) {
            pp.setBloomThreshold(bloomThresholdSlider_->value() / 100.f);
            prefs_->setDouble("post_bloom_threshold", pp.bloomThreshold());
        }
        if (chromaticSlider_->consumeChanged()) {
            pp.setChromatic(chromaticSlider_->value() / 100.f);
            prefs_->setDouble("post_chromatic", pp.chromatic());
        }
        if (grainSlider_->consumeChanged()) {
            pp.setGrain(grainSlider_->value() / 100.f);
            prefs_->setDouble("post_grain", pp.grain());
        }
        if (scanlineSlider_->consumeChanged()) {
            pp.setScanline(scanlineSlider_->value() / 100.f);
            prefs_->setDouble("post_scanline", pp.scanline());
        }
        if (ditherSlider_->consumeChanged()) {
            pp.setDither(ditherSlider_->value() / 100.f);
            prefs_->setDouble("post_dither", pp.dither());
        }
    }
}

void GraphicsTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons) out.push_back(btn.get());
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& btn : presetRow_->buttons) out.push_back(btn.get());
}

bool GraphicsTab::anyEditing() const {
    for (auto* s : {saturationSlider_.get(), contrastSlider_.get(),
                    brightnessSlider_.get(), gammaSlider_.get(),
                    vignetteSlider_.get(), bloomStrengthSlider_.get(),
                    bloomThresholdSlider_.get(), chromaticSlider_.get(),
                    grainSlider_.get(), scanlineSlider_.get(),
                    ditherSlider_.get()}) {
        if (s && s->isEditing()) return true;
    }
    return false;
}

float GraphicsTab::render(sf::RenderTarget& target,
                          float contentX, float ctrlX,
                          float startY) {
    float y = startY;

    auto drawToggle = [&](ToggleRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);
        row.onButton->setPosition({ctrlX, y});
        row.offButton->setPosition({ctrlX + 96.f, y});
        row.onButton->render(target);
        row.offButton->render(target);
        y += kRowH;
    };
    auto drawMulti = [&](MultiRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);
        for (size_t i = 0; i < row.buttons.size(); ++i) {
            row.buttons[i]->setPosition(
                {ctrlX + static_cast<float>(i) * row.stepX, y});
            row.buttons[i]->render(target);
        }
        y += kRowH;
    };
    auto drawSlider = [&](sf::Text& label, Slider& slider) {
        label.setPosition({contentX, y + 4.f});
        target.draw(label);
        slider.setPosition({ctrlX, y + 4.f});
        slider.render(target);
        y += kRowH;
    };

    // 初始生命
    drawMulti(*multiRows_[0]);
    // 动画开关
    drawToggle(*toggles_[0]);
    // 动画速度
    drawMulti(*multiRows_[1]);
    // 伪 3D
    drawToggle(*toggles_[1]);
    // 视差背景
    drawToggle(*toggles_[2]);
    // 玩家动画
    drawToggle(*toggles_[3]);
    // 关卡开场
    drawToggle(*toggles_[4]);
    // 粒子
    drawToggle(*toggles_[5]);
    // 屏幕震动
    drawToggle(*toggles_[6]);
    // 通知
    drawToggle(*toggles_[7]);
    // 通知位置
    drawMulti(*multiRows_[2]);
    // 按钮圆角
    drawMulti(*multiRows_[3]);
    // 按钮边框
    drawMulti(*multiRows_[4]);
    // 显示碰撞盒
    drawToggle(*toggles_[8]);

    // 预设
    {
        presetRow_->label.setPosition({contentX, y + 8.f});
        target.draw(presetRow_->label);
        for (size_t i = 0; i < presetRow_->buttons.size(); ++i) {
            presetRow_->buttons[i]->setPosition(
                {ctrlX + static_cast<float>(i) * presetRow_->stepX, y});
            presetRow_->buttons[i]->render(target);
        }
        y += kRowH;
    }

    // 后处理
    drawSlider(labelPostSaturation_,     *saturationSlider_);
    drawSlider(labelPostContrast_,       *contrastSlider_);
    drawSlider(labelPostBrightness_,     *brightnessSlider_);
    drawSlider(labelPostGamma_,          *gammaSlider_);
    drawSlider(labelPostVignette_,       *vignetteSlider_);
    drawSlider(labelPostBloomStrength_,  *bloomStrengthSlider_);
    drawSlider(labelPostBloomThreshold_, *bloomThresholdSlider_);
    drawSlider(labelPostChromatic_,      *chromaticSlider_);
    drawSlider(labelPostGrain_,          *grainSlider_);
    drawSlider(labelPostScanline_,       *scanlineSlider_);
    drawSlider(labelPostDither_,         *ditherSlider_);

    return y;
}