#include "pause_menu.h"
#include "animation.h"
#include "notification.h"
#include "strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include <algorithm>
#include "focus_group.h"

namespace {
const char* kThemeNames[] = {"深色", "蓝色", "浅色"};
}

PauseMenu::PauseMenu(const sf::Font& font,
                     std::shared_ptr<Preferences> prefs,
                     sf::Vector2f windowSize)
    : font_(font),
      prefs_(std::move(prefs)),
      windowSize_(windowSize),
      title_(font, toSf("已暂停"), scaledFontSize(32)),
      settingsTitle_(font, toSf("设置"), scaledFontSize(28)),
      labelTheme_(font, toSf("主题"), scaledFontSize(20)),
      labelAnim_(font, toSf("动画效果"), scaledFontSize(20)),
      labelNotif_(font, toSf("屏幕通知"), scaledFontSize(20)),
      labelGamepad_(font, toSf("手柄支持"), scaledFontSize(20)),
      hintText_(font, toSf("* 部分设置返回主菜单后完全生效"), scaledFontSize(14)) {
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
        "回到游戏", font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 52.f}, 22));
    mainButtons_.push_back(std::make_unique<Button>("设置", font_, sf::Vector2f{0.f, 0.f},
                                                    sf::Vector2f{280.f, 52.f}, 22));
    mainButtons_.push_back(std::make_unique<Button>(
        "保存并退出游戏", font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{280.f, 52.f}, 22));

    // ===== 设置面板 =====
    for (int i = 0; i < 3; ++i) {
        themeButtons_.push_back(std::make_unique<Button>(
            kThemeNames[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{90.f, 40.f}, 18));
    }
    animOn_ = std::make_unique<Button>("开", font_, sf::Vector2f{0.f, 0.f},
                                       sf::Vector2f{80.f, 40.f}, 18);
    animOff_ = std::make_unique<Button>("关", font_, sf::Vector2f{0.f, 0.f},
                                        sf::Vector2f{80.f, 40.f}, 18);
    notifOn_ = std::make_unique<Button>("开", font_, sf::Vector2f{0.f, 0.f},
                                        sf::Vector2f{80.f, 40.f}, 18);
    notifOff_ = std::make_unique<Button>("关", font_, sf::Vector2f{0.f, 0.f},
                                         sf::Vector2f{80.f, 40.f}, 18);
    gamepadOn_ = std::make_unique<Button>("开", font_, sf::Vector2f{0.f, 0.f},
                                          sf::Vector2f{80.f, 40.f}, 18);
    gamepadOff_ = std::make_unique<Button>("关", font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{80.f, 40.f}, 18);
    backButton_ = std::make_unique<Button>("返回", font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{160.f, 44.f}, 20);

    relayout(windowSize_);
    refreshSelection();
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

        labelAnim_.setPosition({labelX, y + 8.f});
        animOn_->setPosition({ctrlX, y});
        animOff_->setPosition({ctrlX + 90.f, y});
        y += 60.f;

        labelNotif_.setPosition({labelX, y + 8.f});
        notifOn_->setPosition({ctrlX, y});
        notifOff_->setPosition({ctrlX + 90.f, y});
        y += 60.f;

        labelGamepad_.setPosition({labelX, y + 8.f});
        gamepadOn_->setPosition({ctrlX, y});
        gamepadOff_->setPosition({ctrlX + 90.f, y});
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

    bool animOn = prefs_->getBool("animation_enabled", true);
    animOn_->setSelected(animOn);
    animOff_->setSelected(!animOn);

    bool notifOn = prefs_->getBool("notification_enabled", true);
    notifOn_->setSelected(notifOn);
    notifOff_->setSelected(!notifOn);

    bool gamepadOn = prefs_->getBool("gamepad_enabled", true);
    gamepadOn_->setSelected(gamepadOn);
    gamepadOff_->setSelected(!gamepadOn);
}

void PauseMenu::switchToSettings() {
    view_ = View::Settings;
    relayout(windowSize_);
    refreshSelection();
}

void PauseMenu::switchToMain() {
    view_ = View::Main;
    relayout(windowSize_);
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
        NotificationSystem::instance().push("通知已开启", NotificationType::Info);
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
        animOn_->handleEvent(event);
        animOff_->handleEvent(event);
        notifOn_->handleEvent(event);
        notifOff_->handleEvent(event);
        gamepadOn_->handleEvent(event);
        gamepadOff_->handleEvent(event);
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
        if (animOn_->consumeClick()) {
            if (!prefs_->getBool("animation_enabled", true)) {
                applyAnimation(true);
                refreshSelection();
            }
            return;
        }
        if (animOff_->consumeClick()) {
            if (prefs_->getBool("animation_enabled", true)) {
                applyAnimation(false);
                refreshSelection();
            }
            return;
        }
        if (notifOn_->consumeClick()) {
            if (!prefs_->getBool("notification_enabled", true)) {
                applyNotification(true);
                refreshSelection();
            }
            return;
        }
        if (notifOff_->consumeClick()) {
            if (prefs_->getBool("notification_enabled", true)) {
                applyNotification(false);
                refreshSelection();
            }
            return;
        }
        if (gamepadOn_->consumeClick()) {
            if (!prefs_->getBool("gamepad_enabled", true)) {
                applyGamepad(true);
                refreshSelection();
            }
            return;
        }
        if (gamepadOff_->consumeClick()) {
            if (prefs_->getBool("gamepad_enabled", true)) {
                applyGamepad(false);
                refreshSelection();
            }
            return;
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
    target.draw(backdrop_);
    target.draw(panel_);

    if (view_ == View::Main) {
        target.draw(title_);
        for (auto& b : mainButtons_) b->render(target);

        FocusGroup::instance().setItems({
            mainButtons_[0].get(),
            mainButtons_[1].get(),
            mainButtons_[2].get()
        });
    } else {
        target.draw(settingsTitle_);
        target.draw(labelTheme_);
        target.draw(labelAnim_);
        target.draw(labelNotif_);
        target.draw(labelGamepad_);
        target.draw(hintText_);
        for (auto& b : themeButtons_) b->render(target);
        animOn_->render(target);
        animOff_->render(target);
        notifOn_->render(target);
        notifOff_->render(target);
        gamepadOn_->render(target);
        gamepadOff_->render(target);
        backButton_->render(target);

        std::vector<Button*> items;
        for (auto& b : themeButtons_) items.push_back(b.get());
        items.push_back(animOn_.get());
        items.push_back(animOff_.get());
        items.push_back(notifOn_.get());
        items.push_back(notifOff_.get());
        items.push_back(gamepadOn_.get());
        items.push_back(gamepadOff_.get());
        items.push_back(backButton_.get());
        FocusGroup::instance().setItems(items);
    }
}