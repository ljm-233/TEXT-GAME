#include "tabs/graphics_tab.h"
#include "utils/text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utils/utf8.h"
#include "focus_group.h"
#include "config/keys.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr float kRowH = 50.f;

// ⭐ 渲染缩放 9 档（顺序必须和 rowRenderScale_ 里按钮的顺序一致）
const float kRenderScales[] = {2.0f, 1.5f,        1.25f, 1.0f, 0.75f,
                               0.5f, 1.0f / 3.0f, 0.25f, 0.10f};
constexpr int kRenderScaleCount = 9;

// ⭐ 画面预设
struct PostPreset {
    float saturation, contrast, brightness, gamma, vignette;
    float bloomStrength, bloomThreshold;
    float chromatic, grain, scanline, dither;
};

// 顺序必须和 presetRow_ 里按钮的顺序一致
const PostPreset kPresets[] = {
    // 原版
    {100, 100, 100, 100, 0, 0, 70, 0, 0, 0, 0},
    // 复古 CRT
    {105, 110, 105, 105, 40, 30, 60, 35, 15, 30, 0},
    // 电影感
    {110, 115, 95, 100, 50, 60, 55, 15, 20, 0, 0},
    // 像素 8-bit
    {120, 105, 100, 100, 15, 20, 70, 10, 10, 20, 55},
    // 夜晚
    {90, 110, 85, 100, 60, 50, 50, 20, 20, 0, 0},
};
constexpr int kPresetCount = 5;

// 容差 0.01 找档位；找不到回退到 1.0 那一档（下标 3）
int indexOfRenderScale(float s) {
    for (int i = 0; i < kRenderScaleCount; ++i)
        if (std::abs(kRenderScales[i] - s) < 0.01f)
            return i;
    return 3;
}

} // namespace

GraphicsTab::GraphicsTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                         std::shared_ptr<Window> window)
      : font_(font),
        prefs_(std::move(prefs)),
        window_(std::move(window)),
        labelPostSaturation_(font, sf::String(), scaledFontSize(20)),
        labelPostContrast_(font, sf::String(), scaledFontSize(20)),
        labelPostBrightness_(font, sf::String(), scaledFontSize(20)),
        labelPostGamma_(font, sf::String(), scaledFontSize(20)),
        labelPostVignette_(font, sf::String(), scaledFontSize(20)),
        labelPostBloomStrength_(font, sf::String(), scaledFontSize(20)),
        labelPostBloomThreshold_(font, sf::String(), scaledFontSize(20)),
        labelPostChromatic_(font, sf::String(), scaledFontSize(20)),
        labelPostGrain_(font, sf::String(), scaledFontSize(20)),
        labelPostScanline_(font, sf::String(), scaledFontSize(20)),
        labelPostDither_(font, sf::String(), scaledFontSize(20)),
        labelShakeIntensity_(font, sf::String(), scaledFontSize(20)) {
    loadFromPrefs();

    // ===== 标签颜色 =====
    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelPostSaturation_, &labelPostContrast_, &labelPostBrightness_,
                    &labelPostGamma_, &labelPostVignette_, &labelPostBloomStrength_,
                    &labelPostBloomThreshold_, &labelPostChromatic_, &labelPostGrain_,
                    &labelPostScanline_, &labelPostDither_, &labelShakeIntensity_}) {
        t->setFillColor(labelColor);
    }

    // ===== Toggle 辅助 =====
    auto makeToggle = [&](const std::string& onText, const std::string& offText) {
        auto on = std::make_unique<Button>(onText, font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(offText, font_, sf::Vector2f{0.f, 0.f},
                                            sf::Vector2f{86.f, 40.f}, 18);
        return std::make_pair(std::move(on), std::move(off));
    };
    // 返回行对象在堆上的地址；行由 toggles_ 里的 unique_ptr 持有，
    // vector 扩容搬的是 unique_ptr，指向的对象不动，缓存裸指针安全。
    auto addToggle = [&](const char* key, std::function<void(bool)> cb) -> ToggleRow* {
        auto row = std::make_unique<ToggleRow>(font_, key, std::move(cb));
        auto [on, off] = makeToggle(Str::On, Str::Off);
        row->onButton = std::move(on);
        row->offButton = std::move(off);
        ToggleRow* ptr = row.get();
        toggles_.push_back(std::move(row));
        return ptr;
    };
    auto addMultiRow = [&](const char* key, std::function<void(int)> cb) {
        auto row = std::make_unique<MultiRow>(font_, key, std::move(cb));
        multiRows_.push_back(std::move(row));
        return multiRows_.back().get();
    };

    // ===== RenderScale =====
    {
        rowRenderScale_ = addMultiRow(Str::LabelRenderScale, [this](int i) {
            renderScale_ = kRenderScales[i];
            refreshSelection();
            window_->setRenderScale(renderScale_);
            prefs_->setDouble(ConfigKey::kRenderScale, renderScale_);
        });
        rowRenderScale_->stepX = 96.f;
        const char* kLabels[] = {
            Str::RenderScale200, Str::RenderScale150, Str::RenderScale125,
            Str::RenderScale100, Str::RenderScale75,  Str::RenderScale50,
            Str::RenderScale33,  Str::RenderScale25,  Str::RenderScale10};
        for (const char* label : kLabels) {
            rowRenderScale_->addButton(
                std::make_unique<Button>(Str::T(label), font_, sf::Vector2f{0.f, 0.f},
                                         sf::Vector2f{86.f, 40.f}, 18));
        }
    }

    // ===== UpscaleMode =====
    {
        rowUpscaleMode_ = addMultiRow(Str::LabelUpscaleMode, [this](int i) {
            upscaleMode_ = i;
            refreshSelection();
            window_->setUpscaleMode(upscaleMode_);
            prefs_->setInt(ConfigKey::kUpscaleMode, upscaleMode_);
        });
        rowUpscaleMode_->stepX = 100.f;
        const char* kLabels[] = {Str::UpscaleOff, Str::UpscaleBicubic, Str::UpscaleFsr1};
        for (const char* label : kLabels) {
            rowUpscaleMode_->addButton(
                std::make_unique<Button>(Str::T(label), font_, sf::Vector2f{0.f, 0.f},
                                         sf::Vector2f{86.f, 40.f}, 18));
        }
    }

    // ===== Toggles =====
    rowPseudo3D_ = addToggle(Str::LabelPseudo3D, [this](bool v) {
        pseudo3D_ = v;
        refreshSelection();
        applyPseudo3D();
    });
    rowParallax_ = addToggle(Str::LabelParallax, [this](bool v) {
        parallaxEnabled_ = v;
        refreshSelection();
        applyParallax();
    });
    rowPlayerAnim_ = addToggle(Str::LabelPlayerAnimation, [this](bool v) {
        playerAnimEnabled_ = v;
        refreshSelection();
        applyPlayerAnimation();
    });
    rowParticles_ = addToggle(Str::LabelParticles, [this](bool v) {
        particlesEnabled_ = v;
        refreshSelection();
        applyParticles();
    });
    rowScreenShake_ = addToggle(Str::LabelScreenShake, [this](bool v) {
        screenShake_ = v;
        refreshSelection();
        applyScreenShake();
    });

    // ===== ParticleDensity =====
    {
        rowParticleDensity_ = addMultiRow(Str::LabelParticleDensity, [this](int i) {
            particleDensity_ = i;
            refreshSelection();
            prefs_->setInt(ConfigKey::kParticleDensity, particleDensity_);
        });
        rowParticleDensity_->stepX = 86.f;
        const char* kLabels[] = {"关", "少", "标准", "多"};
        for (const char* label : kLabels) {
            rowParticleDensity_->addButton(std::make_unique<Button>(
                label, font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 18));
        }
    }

    // ===== ShakeIntensity =====
    shakeIntensitySlider_ =
        std::make_unique<Slider>(font_, 0.f, 200.f, shakeIntensity_ * 100.f,
                                 sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    shakeIntensitySlider_->setDefaultValue(100.f);

    // ===== 预设 =====
    {
        presetRow_ = std::make_unique<MultiRow>(font_, Str::LabelPreset,
                                                [this](int i) { applyPreset(i); });
        presetRow_->stepX = 112.f;
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetDefault), font_,
                                                       sf::Vector2f{0.f, 0.f},
                                                       sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetCRT), font_,
                                                       sf::Vector2f{0.f, 0.f},
                                                       sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetCinematic),
                                                       font_, sf::Vector2f{0.f, 0.f},
                                                       sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetPixel8), font_,
                                                       sf::Vector2f{0.f, 0.f},
                                                       sf::Vector2f{100.f, 40.f}, 18));
        presetRow_->addButton(std::make_unique<Button>(Str::T(Str::PresetNight), font_,
                                                       sf::Vector2f{0.f, 0.f},
                                                       sf::Vector2f{100.f, 40.f}, 18));
    }

    // ===== 后处理 Slider =====
    {
        auto& pp = window_->postProcess();
        saturationSlider_ =
            std::make_unique<Slider>(font_, 0.f, 200.f, pp.saturation() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        saturationSlider_->setDefaultValue(100.f);

        contrastSlider_ =
            std::make_unique<Slider>(font_, 50.f, 200.f, pp.contrast() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        contrastSlider_->setDefaultValue(100.f);

        brightnessSlider_ =
            std::make_unique<Slider>(font_, 50.f, 200.f, pp.brightness() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        brightnessSlider_->setDefaultValue(100.f);

        gammaSlider_ =
            std::make_unique<Slider>(font_, 50.f, 250.f, pp.gamma() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        gammaSlider_->setDefaultValue(100.f);

        vignetteSlider_ =
            std::make_unique<Slider>(font_, 0.f, 100.f, pp.vignette() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        vignetteSlider_->setDefaultValue(0.f);

        bloomStrengthSlider_ =
            std::make_unique<Slider>(font_, 0.f, 200.f, pp.bloomStrength() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        bloomStrengthSlider_->setDefaultValue(0.f);

        bloomThresholdSlider_ =
            std::make_unique<Slider>(font_, 0.f, 100.f, pp.bloomThreshold() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        bloomThresholdSlider_->setDefaultValue(70.f);

        chromaticSlider_ =
            std::make_unique<Slider>(font_, 0.f, 100.f, pp.chromatic() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        chromaticSlider_->setDefaultValue(0.f);

        grainSlider_ =
            std::make_unique<Slider>(font_, 0.f, 100.f, pp.grain() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        grainSlider_->setDefaultValue(0.f);

        scanlineSlider_ =
            std::make_unique<Slider>(font_, 0.f, 100.f, pp.scanline() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        scanlineSlider_->setDefaultValue(0.f);

        ditherSlider_ =
            std::make_unique<Slider>(font_, 0.f, 100.f, pp.dither() * 100.f,
                                     sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
        ditherSlider_->setDefaultValue(0.f);
    }

    refreshLabels();
    refreshSelection();
}

void GraphicsTab::loadFromPrefs() {
    pseudo3D_ = prefs_->getBool(ConfigKey::kPseudo3d, true);
    parallaxEnabled_ = prefs_->getBool(ConfigKey::kParallax, true);
    playerAnimEnabled_ = prefs_->getBool(ConfigKey::kPlayerAnimation, true);
    particlesEnabled_ = prefs_->getBool(ConfigKey::kParticles, true);
    screenShake_ = prefs_->getBool(ConfigKey::kScreenShake, true);
    renderScale_ = static_cast<float>(prefs_->getDouble(ConfigKey::kRenderScale, 1.0));
    upscaleMode_ = prefs_->getInt(ConfigKey::kUpscaleMode, 1);
    particleDensity_ = std::clamp(prefs_->getInt(ConfigKey::kParticleDensity, 2), 0, 3);
    shakeIntensity_ = std::clamp(
        static_cast<float>(prefs_->getDouble(ConfigKey::kShakeIntensity, 1.0)), 0.f, 2.f);
}

void GraphicsTab::applyPseudo3D() {
    prefs_->setBool(ConfigKey::kPseudo3d, pseudo3D_);
}
void GraphicsTab::applyParallax() {
    prefs_->setBool(ConfigKey::kParallax, parallaxEnabled_);
}
void GraphicsTab::applyPlayerAnimation() {
    prefs_->setBool(ConfigKey::kPlayerAnimation, playerAnimEnabled_);
}
void GraphicsTab::applyParticles() {
    prefs_->setBool(ConfigKey::kParticles, particlesEnabled_);
}
void GraphicsTab::applyScreenShake() {
    prefs_->setBool(ConfigKey::kScreenShake, screenShake_);
}

void GraphicsTab::applyPreset(int idx) {
    if (idx < 0 || idx >= kPresetCount)
        return;
    const auto& p = kPresets[idx];

    auto& pp = window_->postProcess();
    pp.setSaturation(p.saturation / 100.f);
    pp.setContrast(p.contrast / 100.f);
    pp.setBrightness(p.brightness / 100.f);
    pp.setGamma(p.gamma / 100.f);
    pp.setVignette(p.vignette / 100.f);
    pp.setBloomStrength(p.bloomStrength / 100.f);
    pp.setBloomThreshold(p.bloomThreshold / 100.f);
    pp.setChromatic(p.chromatic / 100.f);
    pp.setGrain(p.grain / 100.f);
    pp.setScanline(p.scanline / 100.f);
    pp.setDither(p.dither / 100.f);

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

    prefs_->setDouble(ConfigKey::kPostSaturation, p.saturation / 100.f);
    prefs_->setDouble(ConfigKey::kPostContrast, p.contrast / 100.f);
    prefs_->setDouble(ConfigKey::kPostBrightness, p.brightness / 100.f);
    prefs_->setDouble(ConfigKey::kPostGamma, p.gamma / 100.f);
    prefs_->setDouble(ConfigKey::kPostVignette, p.vignette / 100.f);
    prefs_->setDouble(ConfigKey::kPostBloomStrength, p.bloomStrength / 100.f);
    prefs_->setDouble(ConfigKey::kPostBloomThreshold, p.bloomThreshold / 100.f);
    prefs_->setDouble(ConfigKey::kPostChromatic, p.chromatic / 100.f);
    prefs_->setDouble(ConfigKey::kPostGrain, p.grain / 100.f);
    prefs_->setDouble(ConfigKey::kPostScanline, p.scanline / 100.f);
    prefs_->setDouble(ConfigKey::kPostDither, p.dither / 100.f);
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

    prefs_->setDouble(ConfigKey::kPostSaturation, 1.0);
    prefs_->setDouble(ConfigKey::kPostContrast, 1.0);
    prefs_->setDouble(ConfigKey::kPostBrightness, 1.0);
    prefs_->setDouble(ConfigKey::kPostGamma, 1.0);
    prefs_->setDouble(ConfigKey::kPostVignette, 0.0);
    prefs_->setDouble(ConfigKey::kPostBloomStrength, 0.0);
    prefs_->setDouble(ConfigKey::kPostBloomThreshold, 0.7);
    prefs_->setDouble(ConfigKey::kPostChromatic, 0.0);
    prefs_->setDouble(ConfigKey::kPostGrain, 0.0);
    prefs_->setDouble(ConfigKey::kPostScanline, 0.0);
    prefs_->setDouble(ConfigKey::kPostDither, 0.0);
}

void GraphicsTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_)
        row->refreshLabel();
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
    labelPostSaturation_.setString(toSf(Str::T(Str::LabelPostSaturation)));
    labelPostContrast_.setString(toSf(Str::T(Str::LabelPostContrast)));
    labelPostBrightness_.setString(toSf(Str::T(Str::LabelPostBrightness)));
    labelPostGamma_.setString(toSf(Str::T(Str::LabelPostGamma)));
    labelPostVignette_.setString(toSf(Str::T(Str::LabelPostVignette)));
    labelPostBloomStrength_.setString(toSf(Str::T(Str::LabelPostBloomStrength)));
    labelPostBloomThreshold_.setString(toSf(Str::T(Str::LabelPostBloomThreshold)));
    labelPostChromatic_.setString(toSf(Str::T(Str::LabelPostChromatic)));
    labelPostGrain_.setString(toSf(Str::T(Str::LabelPostGrain)));
    labelPostScanline_.setString(toSf(Str::T(Str::LabelPostScanline)));
    labelPostDither_.setString(toSf(Str::T(Str::LabelPostDither)));
    labelShakeIntensity_.setString(toSf(Str::T(Str::LabelShakeIntensity)));
}

void GraphicsTab::refreshSelection() {
    if (rowPseudo3D_) {
        rowPseudo3D_->currentValue = pseudo3D_;
        rowPseudo3D_->onButton->setSelected(pseudo3D_);
        rowPseudo3D_->offButton->setSelected(!pseudo3D_);
    }
    if (rowParallax_) {
        rowParallax_->currentValue = parallaxEnabled_;
        rowParallax_->onButton->setSelected(parallaxEnabled_);
        rowParallax_->offButton->setSelected(!parallaxEnabled_);
    }
    if (rowPlayerAnim_) {
        rowPlayerAnim_->currentValue = playerAnimEnabled_;
        rowPlayerAnim_->onButton->setSelected(playerAnimEnabled_);
        rowPlayerAnim_->offButton->setSelected(!playerAnimEnabled_);
    }
    if (rowParticles_) {
        rowParticles_->currentValue = particlesEnabled_;
        rowParticles_->onButton->setSelected(particlesEnabled_);
        rowParticles_->offButton->setSelected(!particlesEnabled_);
    }
    if (rowScreenShake_) {
        rowScreenShake_->currentValue = screenShake_;
        rowScreenShake_->onButton->setSelected(screenShake_);
        rowScreenShake_->offButton->setSelected(!screenShake_);
    }
    if (rowRenderScale_)
        rowRenderScale_->setSelected(indexOfRenderScale(renderScale_));
    if (rowUpscaleMode_)
        rowUpscaleMode_->setSelected(upscaleMode_);
    if (rowParticleDensity_)
        rowParticleDensity_->setSelected(particleDensity_);
}

void GraphicsTab::handleEvent(const sf::Event& ev) {
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            btn->handleEvent(ev);
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& btn : presetRow_->buttons)
        btn->handleEvent(ev);
    if (shakeIntensitySlider_)
        shakeIntensitySlider_->handleEvent(ev);
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
            if (row->onChanged)
                row->onChanged(true);
            return;
        }
        if (row->offButton->consumeClick() && row->currentValue) {
            if (row->onChanged)
                row->onChanged(false);
            return;
        }
    }
    // 震动强度
    if (shakeIntensitySlider_ && shakeIntensitySlider_->consumeChanged()) {
        shakeIntensity_ = shakeIntensitySlider_->value() / 100.f;
        prefs_->setDouble(ConfigKey::kShakeIntensity, shakeIntensity_);
    }
    // 后处理 slider
    {
        auto& pp = window_->postProcess();
        if (saturationSlider_->consumeChanged()) {
            pp.setSaturation(saturationSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostSaturation, pp.saturation());
        }
        if (contrastSlider_->consumeChanged()) {
            pp.setContrast(contrastSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostContrast, pp.contrast());
        }
        if (brightnessSlider_->consumeChanged()) {
            pp.setBrightness(brightnessSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostBrightness, pp.brightness());
        }
        if (gammaSlider_->consumeChanged()) {
            pp.setGamma(gammaSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostGamma, pp.gamma());
        }
        if (vignetteSlider_->consumeChanged()) {
            pp.setVignette(vignetteSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostVignette, pp.vignette());
        }
        if (bloomStrengthSlider_->consumeChanged()) {
            pp.setBloomStrength(bloomStrengthSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostBloomStrength, pp.bloomStrength());
        }
        if (bloomThresholdSlider_->consumeChanged()) {
            pp.setBloomThreshold(bloomThresholdSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostBloomThreshold, pp.bloomThreshold());
        }
        if (chromaticSlider_->consumeChanged()) {
            pp.setChromatic(chromaticSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostChromatic, pp.chromatic());
        }
        if (grainSlider_->consumeChanged()) {
            pp.setGrain(grainSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostGrain, pp.grain());
        }
        if (scanlineSlider_->consumeChanged()) {
            pp.setScanline(scanlineSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostScanline, pp.scanline());
        }
        if (ditherSlider_->consumeChanged()) {
            pp.setDither(ditherSlider_->value() / 100.f);
            prefs_->setDouble(ConfigKey::kPostDither, pp.dither());
        }
    }
}

void GraphicsTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            out.push_back(btn.get());
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& btn : presetRow_->buttons)
        out.push_back(btn.get());
}

bool GraphicsTab::anyEditing() const {
    for (auto* s :
         {saturationSlider_.get(), contrastSlider_.get(), brightnessSlider_.get(),
          gammaSlider_.get(), vignetteSlider_.get(), bloomStrengthSlider_.get(),
          bloomThresholdSlider_.get(), chromaticSlider_.get(), grainSlider_.get(),
          scanlineSlider_.get(), ditherSlider_.get(), shakeIntensitySlider_.get()}) {
        if (s && s->isEditing())
            return true;
    }
    return false;
}

float GraphicsTab::render(sf::RenderTarget& target, float contentX, float ctrlX,
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
            row.buttons[i]->setPosition({ctrlX + static_cast<float>(i) * row.stepX, y});
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

    // 渲染缩放
    if (rowRenderScale_)
        drawMulti(*rowRenderScale_);
    // 超分辨率
    if (rowUpscaleMode_)
        drawMulti(*rowUpscaleMode_);
    // 伪 3D
    if (rowPseudo3D_)
        drawToggle(*rowPseudo3D_);
    // 视差背景
    if (rowParallax_)
        drawToggle(*rowParallax_);
    // 玩家动画
    if (rowPlayerAnim_)
        drawToggle(*rowPlayerAnim_);
    // 粒子
    if (rowParticles_)
        drawToggle(*rowParticles_);
    // 粒子密度
    if (rowParticleDensity_)
        drawMulti(*rowParticleDensity_);
    // 屏幕震动
    if (rowScreenShake_)
        drawToggle(*rowScreenShake_);
    // 震动强度
    if (shakeIntensitySlider_)
        drawSlider(labelShakeIntensity_, *shakeIntensitySlider_);

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
    drawSlider(labelPostSaturation_, *saturationSlider_);
    drawSlider(labelPostContrast_, *contrastSlider_);
    drawSlider(labelPostBrightness_, *brightnessSlider_);
    drawSlider(labelPostGamma_, *gammaSlider_);
    drawSlider(labelPostVignette_, *vignetteSlider_);
    drawSlider(labelPostBloomStrength_, *bloomStrengthSlider_);
    drawSlider(labelPostBloomThreshold_, *bloomThresholdSlider_);
    drawSlider(labelPostChromatic_, *chromaticSlider_);
    drawSlider(labelPostGrain_, *grainSlider_);
    drawSlider(labelPostScanline_, *scanlineSlider_);
    drawSlider(labelPostDither_, *ditherSlider_);

    return y;
}

void GraphicsTab::reapply() {
    loadFromPrefs();
    // 渲染缩放与超分是**推给 window_ 才生效**的（不是每帧读配置），
    // 重置后必须重新推一次，否则键删了、画面还是旧参数
    if (window_) {
        window_->setRenderScale(renderScale_);
        window_->setUpscaleMode(upscaleMode_);
    }
    // 震动强度也是滑块驱动的，重置后要把滑块拨回默认位置
    if (shakeIntensitySlider_)
        shakeIntensitySlider_->setValue(shakeIntensity_ * 100.f);
    // 后处理的 uniform 是即时生效的，重置后必须重新灌一遍 ——
    // 否则滑块回到默认位置了，画面还是旧的效果
    resetPost();
    applyPseudo3D();
    applyParallax();
    applyPlayerAnimation();
    applyParticles();
    applyScreenShake();
    refreshSelection();
}
