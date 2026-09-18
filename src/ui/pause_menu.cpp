#include "pause_menu.h"
#include "animation.h"
#include "notification.h"
#include "text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include <algorithm>
#include <functional>
#include "focus_group.h"

namespace {
const char* kThemeKeys[] = {"深色", "蓝色", "浅色"};
}

PauseMenu::PauseMenu(const sf::Font& font,
                     std::shared_ptr<Preferences> prefs,
                     sf::Vector2f windowSize)
    : font_(font),
      prefs_(std::move(prefs)),
      windowSize_(windowSize),
      title_(font, sf::String(), scaledFontSize(32)),
      settingsTitle_(font, sf::String(), scaledFontSize(28)),
      labelTheme_(font, sf::String(), scaledFontSize(20)),
      labelAnim_(font, sf::String(), scaledFontSize(20)),
      labelNotif_(font, sf::String(), scaledFontSize(20)),
      labelGamepad_(font, sf::String(), scaledFontSize(20)),
      hintText_(font, sf::String(), scaledFontSize(14)) {
    backdrop_.setFillColor(sf::Color(0, 0, 0, 180));
    backdrop_.setSize(windowSize_);

    panel_.setFillColor(sf::Color(30, 30, 45));
    panel_.setOutlineThickness(2.f);
    panel_.setOutlineColor(sf::Color(90, 90, 130));

    title_.setFillColor(sf::Color(240, 240, 250));
    settingsTitle_.setFillColor(sf::Color(240, 240, 250));
    labelTheme_.setFillColor(sf::Color(220, 220, 230));
    labelAnim_.setFillColor(sf::Color(220, 220, 230));
    labelNotif_.setFillColor(sf::Color(220, 220, 230));
    hintText_.setFillColor(sf::Color(150, 150, 170));

    // ===== 主菜单按钮 =====
    mainButtons_.push_back(std::make_unique<Button>(
        Str::T("回到游戏"), font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 52.f}, 22));
    mainButtons_.push_back(std::make_unique<Button>(
        Str::T(Str::Settings), font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 52.f}, 22));
    mainButtons_.push_back(std::make_unique<Button>(
        Str::T("保存并退出游戏"), font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 52.f}, 22));

    // ===== 设置面板 =====
    for (int i = 0; i < 3; ++i) {
        themeButtons_.push_back(std::make_unique<Button>(
            Str::T(kThemeKeys[i]), font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{90.f, 40.f}, 18));
    }
    auto makeToggleRow = [&](const char* labelKey,
                             std::function<void(bool)> onChanged) {
        auto row = std::make_unique<ToggleRow>(font_, labelKey,
                                               std::move(onChanged));
        auto on  = std::make_unique<Button>(Str::T(Str::On),  font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{80.f, 40.f}, 18);
        auto off = std::make_unique<Button>(Str::T(Str::Off), font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{80.f, 40.f}, 18);
        row->onButton  = std::move(on);
        row->offButton = std::move(off);
        return row;
    };
    settingsToggles_.push_back(makeToggleRow(Str::LabelAnimation, [this](bool v) {
        applyAnimation(v); refreshSelection();
    }));
    settingsToggles_.push_back(makeToggleRow(Str::LabelNotification, [this](bool v) {
        applyNotification(v); refreshSelection();
    }));
    settingsToggles_.push_back(makeToggleRow(Str::LabelGamepad, [this](bool v) {
        applyGamepad(v); refreshSelection();
    }));
    backButton_ = std::make_unique<Button>(Str::T(Str::Back), font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{160.f, 44.f}, 20);

    refreshLabels();
    relayout(windowSize_);
    refreshSelection();
}

void PauseMenu::syncFocus() {
    if (view_ == View::Main) {
        std::vector<Button*> items;
        for (auto& b : mainButtons_) items.push_back(b.get());
        FocusGroup::instance().setItems(items);
    } else {
        std::vector<Button*> items;
        for (auto& b : themeButtons_) items.push_back(b.get());
        for (auto& row : settingsToggles_) {
            items.push_back(row->onButton.get());
            items.push_back(row->offButton.get());
        }
        items.push_back(backButton_.get());
        FocusGroup::instance().setItems(items);
    }
}


void PauseMenu::refreshLabels() {
    title_.setString(toSf(Str::T("已暂停")));
    settingsTitle_.setString(toSf(Str::T(Str::Settings)));
    labelTheme_.setString(toSf(Str::T(Str::LabelTheme)));
    labelAnim_.setString(toSf(Str::T(Str::LabelAnimation)));
    labelNotif_.setString(toSf(Str::T(Str::LabelNotification)));
    labelGamepad_.setString(toSf(Str::T(Str::LabelGamepad)));
    hintText_.setString(toSf(Str::T("* 部分设置返回主菜单后完全生效")));

    if (mainButtons_.size() >= 3) {
        mainButtons_[0]->setText(Str::T("回到游戏"));
        mainButtons_[1]->setText(Str::T(Str::Settings));
        mainButtons_[2]->setText(Str::T("保存并退出游戏"));
    }
    for (int i = 0; i < 3 && i < static_cast<int>(themeButtons_.size()); ++i) {
        themeButtons_[i]->setText(Str::T(kThemeKeys[i]));
    }
    for (auto& row : settingsToggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    if (backButton_)backButton_->setText(Str::T(Str::Back));
}

void PauseMenu::relayout(sf::Vector2f windowSize) {
    windowSize_ = windowSize;
    backdrop_.setSize(windowSize_);

    if (view_ == View::Main) {
        const float panelW = 420.f;
        const float panelH = 340.f;
        float px = (windowSize.x - panelW) / 2.f;
        float py = (windowSize.y - panelH) / 2.f;

        panel_.setSize({panelW, panelH});
        panel_.setPosition({px, py});

        auto tb = title_.getLocalBounds();
        title_.setOrigin({tb.position.x + tb.size.x / 2.f, tb.position.y});
        title_.setPosition({windowSize.x / 2.f, py + 30.f});

        float y = py + 110.f;
        for (auto& b : mainButtons_) {
            b->setPosition({px + (panelW - 280.f) / 2.f, y});
            y += 70.f;
        }
    } else {
        const float panelW = 520.f;
        const float panelH = 400.f;
        float px = (windowSize.x - panelW) / 2.f;
        float py = (windowSize.y - panelH) / 2.f;

        panel_.setSize({panelW, panelH});
        panel_.setPosition({px, py});

        auto tb = settingsTitle_.getLocalBounds();
        settingsTitle_.setOrigin({tb.position.x + tb.size.x / 2.f, tb.position.y});
        settingsTitle_.setPosition({windowSize.x / 2.f, py + 30.f});

        float labelX = px + 50.f;
        float ctrlX = px + 200.f;

        float y = py + 110.f;
        labelTheme_.setPosition({labelX, y + 8.f});
        for (size_t i = 0; i < themeButtons_.size(); ++i) {
            themeButtons_[i]->setPosition({ctrlX + static_cast<float>(i) * 100.f, y});
        }
        y += 60.f;

        settingsToggles_[0]->label.setPosition({labelX, y + 8.f});
        settingsToggles_[0]->onButton->setPosition({ctrlX, y});
        settingsToggles_[0]->offButton->setPosition({ctrlX + 90.f, y});
        y += 60.f;

        settingsToggles_[1]->label.setPosition({labelX, y + 8.f});
        settingsToggles_[1]->onButton->setPosition({ctrlX, y});
        settingsToggles_[1]->offButton->setPosition({ctrlX + 90.f, y});
        y += 60.f;

        settingsToggles_[2]->label.setPosition({labelX, y + 8.f});
        settingsToggles_[2]->onButton->setPosition({ctrlX, y});
        settingsToggles_[2]->offButton->setPosition({ctrlX + 90.f, y});
        y += 70.f;

        hintText_.setPosition({labelX, y});
        y += 30.f;

        backButton_->setPosition({windowSize.x / 2.f - 80.f, py + panelH - 60.f});
    }
}

void PauseMenu::refreshSelection() {
    int themeIdx = std::clamp(prefs_->getInt("theme", 0), 0, 2);
    for (int i = 0; i < 3; ++i)
        themeButtons_[i]->setSelected(i == themeIdx);

    if (settingsToggles_.size() == 3) {
        auto setRow = [](ToggleRow& row, bool v) {
            row.currentValue = v;
            row.onButton->setSelected(v);
            row.offButton->setSelected(!v);
        };
        setRow(*settingsToggles_[0], prefs_->getBool("animation_enabled", true));
        setRow(*settingsToggles_[1], prefs_->getBool("notification_enabled", true));
        setRow(*settingsToggles_[2], prefs_->getBool("gamepad_enabled", true));
    }
}

void PauseMenu::switchToSettings() {
    view_ = View::Settings;
    refreshLabels();
    relayout(windowSize_);
    refreshSelection();
    syncFocus();
}

void PauseMenu::switchToMain() {
    view_ = View::Main;
    refreshLabels();
    relayout(windowSize_);
    syncFocus();
}

void PauseMenu::applyTheme(int idx) {
    setTheme(static_cast<ThemeId>(idx));
    prefs_->setInt("theme", idx);
}

void PauseMenu::applyAnimation(bool enabled) {
    Anim::setEnabled(enabled);
    prefs_->setBool("animation_enabled", enabled);
}

void PauseMenu::applyNotification(bool enabled) {
    NotificationSystem::instance().setEnabled(enabled);
    prefs_->setBool("notification_enabled", enabled);
    if (enabled)
        NotificationSystem::instance().push(Str::T("通知已开启"), NotificationType::Info);
}

void PauseMenu::applyGamepad(bool enabled) {
    prefs_->setBool("gamepad_enabled", enabled);
    FocusGroup::instance().setEnabled(enabled);
}

void PauseMenu::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            if (view_ == View::Settings) {
                switchToMain();
            } else {
                pendingAction_ = Action::Resume;
            }
            return;
        }
    }

    if (view_ == View::Main) {
        for (auto& b : mainButtons_)
            b->handleEvent(event);
    } else {
        for (auto& b : themeButtons_)
            b->handleEvent(event);
        for (auto& row : settingsToggles_) {
            row->onButton->handleEvent(event);
            row->offButton->handleEvent(event);
        }
        backButton_->handleEvent(event);
    }
}

void PauseMenu::update(float /*dt*/) {
    if (view_ == View::Main) {
        if (mainButtons_[0]->consumeClick())
            pendingAction_ = Action::Resume;
        if (mainButtons_[1]->consumeClick()) {
            switchToSettings();
            return;
        }
        if (mainButtons_[2]->consumeClick())
            pendingAction_ = Action::SaveAndQuit;
    } else {
        for (int i = 0; i < 3; ++i) {
            if (themeButtons_[i]->consumeClick()) {
                if (prefs_->getInt("theme", 0) != i) {
                    applyTheme(i);
                    refreshSelection();
                }
                return;
            }
        }
        for (auto& row : settingsToggles_) {
            if (row->onButton->consumeClick() && !row->currentValue) {
                if (row->onChanged) row->onChanged(true);
                return;
            }
            if (row->offButton->consumeClick() && row->currentValue) {
                if (row->onChanged) row->onChanged(false);
                return;
            }
        }
        if (backButton_->consumeClick()) {
            switchToMain();
        }
    }
}

PauseMenu::Action PauseMenu::consumeAction() {
    Action a = pendingAction_;
    pendingAction_ = Action::None;
    return a;
}

void PauseMenu::render(sf::RenderTarget& target) {
    // ⭐ 每帧刷新字符串（廉价且保证语言切换生效）
    refreshLabels();

    target.draw(backdrop_);
    target.draw(panel_);

    if (view_ == View::Main) {
        target.draw(title_);
        for (auto& b : mainButtons_) b->render(target);
    } else {
        target.draw(settingsTitle_);
        target.draw(labelTheme_);
        target.draw(hintText_);
        for (auto& b : themeButtons_) b->render(target);
        for (auto& row : settingsToggles_) {
            target.draw(row->label);
            row->onButton->render(target);
            row->offButton->render(target);
        }
        backButton_->render(target);
    }
}