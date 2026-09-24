#include "tabs/audio_tab.h"
#include "text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include "sound_manager.h"
#include "gamepad.h"
#include "focus_group.h"

#include <algorithm>
#include <functional>

namespace {
constexpr float kRowH = 50.f;
}

AudioTab::AudioTab(const sf::Font& font,
                   std::shared_ptr<Preferences> prefs,
                   std::shared_ptr<Window> window)
      : font_(font),
        prefs_(std::move(prefs)),
        window_(std::move(window)),
        labelMasterVolume_    (font, sf::String(), scaledFontSize(20)),
        labelSoundVolume_     (font, sf::String(), scaledFontSize(20)),
        labelBGMVolume_       (font, sf::String(), scaledFontSize(20)),
        labelVibrationIntensity_(font, sf::String(), scaledFontSize(20)) {

    // 从 prefs 加载状态
    loadFromPrefs();

    // ⭐ 创建 Slider
    masterVolumeSlider_ = std::make_unique<Slider>(
        font_, 0.f, 100.f, masterVolume_ * 100.f,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    masterVolumeSlider_->setDefaultValue(100.f);

    soundVolumeSlider_ = std::make_unique<Slider>(
        font_, 0.f, 100.f, soundVolume_ * 100.f,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    soundVolumeSlider_->setDefaultValue(60.f);

    bgmVolumeSlider_ = std::make_unique<Slider>(
        font_, 0.f, 100.f, bgmVolume_ * 100.f,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    bgmVolumeSlider_->setDefaultValue(40.f);

    gamepadVibrationSlider_ = std::make_unique<Slider>(
        font_, 0.f, 100.f, gamepadVibrationIntensity_ * 100.f,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    gamepadVibrationSlider_->setDefaultValue(100.f);

    // ⭐ 创建 Toggle
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

    addToggle(Str::LabelSound, [this](bool v) {
        soundEnabled_ = v; refreshSelection(); applySound();
    });
    addToggle(Str::LabelBGM, [this](bool v) {
        bgmEnabled_ = v; refreshSelection(); applyBGM();
    });
    addToggle(Str::LabelGamepad, [this](bool v) {
        gamepadEnabled_ = v; refreshSelection(); applyGamepad();
    });
    addToggle(Str::LabelGamepadVibration, [this](bool v) {
        gamepadVibrationEnabled_ = v; refreshSelection(); applyGamepadVibration();
    });

    auto labelColor = sf::Color(230, 230, 230);
    labelMasterVolume_.setFillColor(labelColor);
    labelSoundVolume_.setFillColor(labelColor);
    labelBGMVolume_.setFillColor(labelColor);
    labelVibrationIntensity_.setFillColor(labelColor);

    refreshSelection();
    refreshLabels();
}

void AudioTab::loadFromPrefs() {
    soundEnabled_            = prefs_->getBool("sound_enabled", true);
    bgmEnabled_              = prefs_->getBool("bgm_enabled", true);
    gamepadEnabled_          = prefs_->getBool("gamepad_enabled", true);
    gamepadVibrationEnabled_ = prefs_->getBool("gamepad_vibration_enabled", true);

    masterVolume_ = static_cast<float>(prefs_->getDouble("master_volume", 1.0));
    soundVolume_  = static_cast<float>(prefs_->getDouble("sound_volume", 0.6));
    bgmVolume_    = static_cast<float>(prefs_->getDouble("bgm_volume", 0.4));
    gamepadVibrationIntensity_ = static_cast<float>(
        prefs_->getDouble("gamepad_vibration_intensity", 1.0));
}

void AudioTab::applySound() {
    SoundManager::instance().setEnabled(soundEnabled_);
    SoundManager::instance().setSFXVolume(soundVolume_);
    prefs_->setBool("sound_enabled", soundEnabled_);
    prefs_->setDouble("sound_volume", soundVolume_);
    if (soundEnabled_) SoundManager::instance().playCoin();
}

void AudioTab::applyBGM() {
    SoundManager::instance().setBGMEnabled(bgmEnabled_);
    SoundManager::instance().setMusicVolume(bgmVolume_);
    prefs_->setBool("bgm_enabled", bgmEnabled_);
    prefs_->setDouble("bgm_volume", bgmVolume_);
}

void AudioTab::applyGamepad() {
    prefs_->setBool("gamepad_enabled", gamepadEnabled_);
    FocusGroup::instance().setEnabled(gamepadEnabled_);
}

void AudioTab::applyGamepadVibration() {
    prefs_->setBool("gamepad_vibration_enabled", gamepadVibrationEnabled_);
    Gamepad::instance().setVibrationEnabled(gamepadVibrationEnabled_);
}

void AudioTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    labelMasterVolume_.setString(toSf(Str::T(Str::LabelMasterVolume)));
    labelSoundVolume_.setString(toSf(Str::T(Str::LabelSoundVolume)));
    labelBGMVolume_.setString(toSf(Str::T(Str::LabelBGMVolume)));
    labelVibrationIntensity_.setString(
        toSf(Str::T(Str::LabelVibrationIntensity)));
}

void AudioTab::refreshSelection() {
    if (toggles_.size() != 4) return;
    auto setRow = [](ToggleRow& row, bool v) {
        row.currentValue = v;
        row.onButton->setSelected(v);
        row.offButton->setSelected(!v);
    };
    setRow(*toggles_[0], soundEnabled_);
    setRow(*toggles_[1], bgmEnabled_);
    setRow(*toggles_[2], gamepadEnabled_);
    setRow(*toggles_[3], gamepadVibrationEnabled_);

    masterVolumeSlider_->setValue(masterVolume_ * 100.f);
    soundVolumeSlider_->setValue(soundVolume_ * 100.f);
    bgmVolumeSlider_->setValue(bgmVolume_ * 100.f);
    gamepadVibrationSlider_->setValue(gamepadVibrationIntensity_ * 100.f);
}

void AudioTab::handleEvent(const sf::Event& ev) {
    masterVolumeSlider_->handleEvent(ev);
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    soundVolumeSlider_->handleEvent(ev);
    bgmVolumeSlider_->handleEvent(ev);
    gamepadVibrationSlider_->handleEvent(ev);
}

void AudioTab::update() {
    if (masterVolumeSlider_->consumeChanged()) {
        masterVolume_ = masterVolumeSlider_->value() / 100.f;
        SoundManager::instance().setMasterVolume(masterVolume_);
        prefs_->setDouble("master_volume", masterVolume_);
    }

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

    if (soundVolumeSlider_->consumeChanged()) {
        soundVolume_ = soundVolumeSlider_->value() / 100.f;
        SoundManager::instance().setSFXVolume(soundVolume_);
        prefs_->setDouble("sound_volume", soundVolume_);
    }
    if (bgmVolumeSlider_->consumeChanged()) {
        bgmVolume_ = bgmVolumeSlider_->value() / 100.f;
        SoundManager::instance().setMusicVolume(bgmVolume_);
        prefs_->setDouble("bgm_volume", bgmVolume_);
    }
    if (gamepadVibrationSlider_->consumeChanged()) {
        gamepadVibrationIntensity_ = gamepadVibrationSlider_->value() / 100.f;
        Gamepad::instance().setVibrationIntensity(gamepadVibrationIntensity_);
        prefs_->setDouble("gamepad_vibration_intensity",
                          gamepadVibrationIntensity_);
    }
}

void AudioTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
}

bool AudioTab::anyEditing() const {
    if (masterVolumeSlider_ && masterVolumeSlider_->isEditing()) return true;
    if (soundVolumeSlider_  && soundVolumeSlider_->isEditing())  return true;
    if (bgmVolumeSlider_    && bgmVolumeSlider_->isEditing())    return true;
    if (gamepadVibrationSlider_ && gamepadVibrationSlider_->isEditing()) return true;
    return false;
}

float AudioTab::render(sf::RenderTarget& target,
                       float contentX, float ctrlX,
                       float startY) {
    float y = startY;

    auto drawToggleRow = [&](ToggleRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);
        row.onButton->setPosition({ctrlX, y});
        row.offButton->setPosition({ctrlX + 96.f, y});
        row.onButton->render(target);
        row.offButton->render(target);
        y += kRowH;
    };

    auto drawSlider = [&](sf::Text& label, Slider& slider) {
        label.setPosition({contentX, y + 4.f});
        target.draw(label);
        slider.setPosition({ctrlX, y + 4.f});
        slider.render(target);
        y += kRowH;
    };

    // 总音量
    drawSlider(labelMasterVolume_, *masterVolumeSlider_);
    // 音效开关
    drawToggleRow(*toggles_[0]);
    // 音效音量
    drawSlider(labelSoundVolume_, *soundVolumeSlider_);
    // BGM 开关
    drawToggleRow(*toggles_[1]);
    // BGM 音量
    drawSlider(labelBGMVolume_, *bgmVolumeSlider_);
    // 手柄
    drawToggleRow(*toggles_[2]);
    // 手柄振动
    drawToggleRow(*toggles_[3]);
    // 振动强度
    drawSlider(labelVibrationIntensity_, *gamepadVibrationSlider_);

    return y;
}