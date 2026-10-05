#include "tabs/controls_tab.h"

#include "config/keys.h"
#include "focus_group.h"
#include "infrastructure/gamepad.h"
#include "sound_manager.h"
#include "ui_scale.h"
#include "utils/text_strings.h"
#include "utils/utf8.h"

#include <functional>

namespace {

constexpr float kRowH = 50.f;

/// 动作 → 配置键。
///
/// ⚠️ 以前这里是裸字面量 `"key_left"`，违反项目自己的约定
///    （CLAUDE.md 关键约定 #6：键名必须走 keys.h 常量）。
///    字面量打错字不会报错，只会**静默回退默认键位** ——
///    用户改完键位重启发现白改了，还找不到原因。
const char* prefKeyOf(KeyBindings::Action a) {
    switch (a) {
    case KeyBindings::MoveLeft:
        return ConfigKey::kKeyLeft;
    case KeyBindings::MoveRight:
        return ConfigKey::kKeyRight;
    case KeyBindings::Jump:
        return ConfigKey::kKeyJump;
    case KeyBindings::Pause:
        return ConfigKey::kKeyPause;
    case KeyBindings::Restart:
        return ConfigKey::kKeyRestart;
    default:
        return nullptr;
    }
}

} // namespace

ControlsTab::ControlsTab(const sf::Font& font, std::shared_ptr<Preferences> prefs)
      : font_(font),
        prefs_(std::move(prefs)),
        hintText_(font, sf::String(), scaledFontSize(14)),
        labelVibrationIntensity_(font, sf::String(), scaledFontSize(20)) {
    for (int i = 0; i < KeyBindings::Count; ++i) {
        auto btn = std::make_unique<Button>(
            KeyBindings::keyToString(
                KeyBindings::instance().get(static_cast<KeyBindings::Action>(i))),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 40.f}, 18);
        keyBindingButtons_.push_back(std::move(btn));

        auto label = std::make_unique<sf::Text>(font, sf::String(), scaledFontSize(20));
        label->setFillColor(sf::Color(230, 230, 230));
        keyLabels_.push_back(std::move(label));
    }

    hintText_.setFillColor(sf::Color(180, 180, 200));
    labelVibrationIntensity_.setFillColor(sf::Color(230, 230, 230));

    resetKeysButton_ =
        std::make_unique<Button>(Str::ButtonResetKeys, font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{180.f, 40.f}, 18);

    auto addToggle = [this](const char* key, std::function<void(bool)> cb) {
        auto row = std::make_unique<ToggleRow>(font_, key, std::move(cb));
        auto on = std::make_unique<Button>(Str::On, font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(Str::Off, font_, sf::Vector2f{0.f, 0.f},
                                            sf::Vector2f{86.f, 40.f}, 18);
        row->onButton = std::move(on);
        row->offButton = std::move(off);
        toggles_.push_back(std::move(row));
        return toggles_.back().get();
    };

    rowGamepad_ = addToggle(Str::LabelGamepad, [this](bool v) {
        gamepadEnabled_ = v;
        refreshSelection();
        applyGamepad();
    });
    rowVibration_ = addToggle(Str::LabelGamepadVibration, [this](bool v) {
        gamepadVibrationEnabled_ = v;
        refreshSelection();
        applyGamepadVibration();
    });

    vibrationSlider_ =
        std::make_unique<Slider>(font_, 0.f, 100.f, gamepadVibrationIntensity_ * 100.f,
                                 sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    vibrationSlider_->setDefaultValue(100.f);

    loadFromPrefs();
    refreshLabels();
    refreshSelection();
}

void ControlsTab::loadFromPrefs() {
    gamepadEnabled_ = prefs_->getBool(ConfigKey::kGamepadEnabled, true);
    gamepadVibrationEnabled_ = prefs_->getBool(ConfigKey::kGamepadVibrationEnabled, true);
    gamepadVibrationIntensity_ =
        static_cast<float>(prefs_->getDouble(ConfigKey::kGamepadVibrationIntensity, 1.0));
}

void ControlsTab::applyGamepad() {
    prefs_->setBool(ConfigKey::kGamepadEnabled, gamepadEnabled_);
    FocusGroup::instance().setEnabled(gamepadEnabled_);
}

void ControlsTab::applyGamepadVibration() {
    prefs_->setBool(ConfigKey::kGamepadVibrationEnabled, gamepadVibrationEnabled_);
    Gamepad::instance().setVibrationEnabled(gamepadVibrationEnabled_);
}

void ControlsTab::applyResetKeys() {
    KeyBindings::instance().resetToDefaults();
    // 必须把 5 个键都落盘 —— 只重置内存里的表，重启又会读回旧键位
    for (int i = 0; i < KeyBindings::Count; ++i) {
        const auto act = static_cast<KeyBindings::Action>(i);
        if (const char* k = prefKeyOf(act))
            prefs_->setInt(k, static_cast<int>(KeyBindings::instance().get(act)));
    }
    listeningAction_ = -1;
    refreshLabels();
}

void ControlsTab::refreshBindingText() {
    if (listeningAction_ >= 0)
        return; // 正在监听时不覆盖提示文字
    for (int i = 0; i < static_cast<int>(keyBindingButtons_.size()); ++i) {
        const auto act = static_cast<KeyBindings::Action>(i);
        keyBindingButtons_[i]->setText(
            KeyBindings::keyToString(KeyBindings::instance().get(act)));
    }
}

void ControlsTab::handleEvent(const sf::Event& ev) {
    // 监听中：按下的下一个键就是新绑定
    if (listeningAction_ >= 0) {
        if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Escape) {
                listeningAction_ = -1;
                refreshBindingText();
                return;
            }
            const auto act = static_cast<KeyBindings::Action>(listeningAction_);
            KeyBindings::instance().set(act, kp->code);
            if (const char* k = prefKeyOf(act))
                prefs_->setInt(k, static_cast<int>(kp->code));

            listeningAction_ = -1;
            refreshBindingText();
            return;
        }
        return; // 监听中不转发其它事件
    }

    for (auto& b : keyBindingButtons_)
        b->handleEvent(ev);
    resetKeysButton_->handleEvent(ev);
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    vibrationSlider_->handleEvent(ev);
}

void ControlsTab::update() {
    if (listeningAction_ < 0) {
        for (int i = 0; i < static_cast<int>(keyBindingButtons_.size()); ++i) {
            if (keyBindingButtons_[i]->consumeClick()) {
                listeningAction_ = i;
                keyBindingButtons_[i]->setText(Str::T(Str::KeyPressNew));
                return;
            }
        }
        if (resetKeysButton_->consumeClick()) {
            applyResetKeys();
            SoundManager::instance().playClick();
            return;
        }
    }

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

    if (vibrationSlider_->consumeChanged()) {
        gamepadVibrationIntensity_ = vibrationSlider_->value() / 100.f;
        Gamepad::instance().setVibrationIntensity(gamepadVibrationIntensity_);
        prefs_->setDouble(ConfigKey::kGamepadVibrationIntensity,
                          gamepadVibrationIntensity_);
    }
}

void ControlsTab::refreshLabels() {
    for (int i = 0; i < static_cast<int>(keyLabels_.size()); ++i) {
        const auto act = static_cast<KeyBindings::Action>(i);
        keyLabels_[i]->setString(toSf(Str::T(KeyBindings::actionName(act))));
    }
    hintText_.setString(toSf(Str::T(Str::KeyBindHint)));
    resetKeysButton_->setText(Str::T(Str::ButtonResetKeys));
    labelVibrationIntensity_.setString(toSf(Str::T(Str::LabelVibrationIntensity)));

    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }

    if (listeningAction_ >= 0)
        keyBindingButtons_[listeningAction_]->setText(Str::T(Str::KeyPressNew));
    else
        refreshBindingText();
}

void ControlsTab::refreshSelection() {
    auto setRow = [](ToggleRow* row, bool v) {
        if (!row)
            return;
        row->currentValue = v;
        row->onButton->setSelected(v);
        row->offButton->setSelected(!v);
    };
    setRow(rowGamepad_, gamepadEnabled_);
    setRow(rowVibration_, gamepadVibrationEnabled_);
    vibrationSlider_->setValue(gamepadVibrationIntensity_ * 100.f);
}

void ControlsTab::registerFocus(std::vector<Button*>& out) {
    for (auto& b : keyBindingButtons_)
        out.push_back(b.get());
    out.push_back(resetKeysButton_.get());
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
}

bool ControlsTab::anyEditing() const {
    return vibrationSlider_ && vibrationSlider_->isEditing();
}

float ControlsTab::render(sf::RenderTarget& target, float contentX, float ctrlX,
                          float startY) {
    float y = startY;

    // ---- 键位 ----
    for (int i = 0; i < static_cast<int>(keyBindingButtons_.size()); ++i) {
        keyLabels_[i]->setPosition({contentX, y + 8.f});
        target.draw(*keyLabels_[i]);
        keyBindingButtons_[i]->setPosition({ctrlX, y});
        keyBindingButtons_[i]->render(target);
        y += kRowH;
    }

    hintText_.setPosition({contentX, y + 8.f});
    target.draw(hintText_);
    y += 34.f;

    // ---- 恢复默认按键 ----
    resetKeysButton_->setPosition({ctrlX, y});
    resetKeysButton_->render(target);
    y += kRowH + 6.f;

    // ---- 手柄 ----
    auto drawToggle = [&](ToggleRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);
        row.onButton->setPosition({ctrlX, y});
        row.offButton->setPosition({ctrlX + 96.f, y});
        row.onButton->render(target);
        row.offButton->render(target);
        y += kRowH;
    };

    if (rowGamepad_)
        drawToggle(*rowGamepad_);
    if (rowVibration_)
        drawToggle(*rowVibration_);

    // 振动强度
    labelVibrationIntensity_.setPosition({contentX, y + 4.f});
    target.draw(labelVibrationIntensity_);
    vibrationSlider_->setPosition({ctrlX, y + 4.f});
    vibrationSlider_->render(target);
    y += kRowH;

    return y + 20.f;
}

void ControlsTab::reapply() {
    loadFromPrefs();
    // 键位也要回到默认 —— 只删配置不改内存里的表，界面还是旧键位
    KeyBindings::instance().resetToDefaults();
    applyGamepad();
    applyGamepadVibration();
    refreshLabels();
    refreshSelection();
}
