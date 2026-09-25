#include "tabs/keys_tab.h"
#include "button.h"
#include "text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"

#include <algorithm>
#include <functional>

namespace {
constexpr float kRowH = 50.f;
}

KeysTab::KeysTab(const sf::Font& font,
                 std::shared_ptr<Preferences> prefs)
      : font_(font),
        prefs_(std::move(prefs)),
        hintText_(font, sf::String(), scaledFontSize(14)) {

    for (int i = 0; i < KeyBindings::Count; ++i) {
        auto btn = std::make_unique<Button>(
            KeyBindings::keyToString(KeyBindings::instance().get(
                static_cast<KeyBindings::Action>(i))),
            font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 40.f}, 18);
        keyBindingButtons_.push_back(std::move(btn));

        auto label = std::make_unique<sf::Text>(
            font, sf::String(), scaledFontSize(20));
        label->setFillColor(sf::Color(230, 230, 230));
        keyLabels_.push_back(std::move(label));
    }

    hintText_.setFillColor(sf::Color(180, 180, 200));
}

void KeysTab::refreshButtonText() {
    if (listeningAction_ >= 0) return;   // 正在监听时不覆盖
    for (int i = 0; i < static_cast<int>(keyBindingButtons_.size()); ++i) {
        auto act = static_cast<KeyBindings::Action>(i);
        keyBindingButtons_[i]->setText(
            KeyBindings::keyToString(KeyBindings::instance().get(act)));
    }
}

void KeysTab::handleEvent(const sf::Event& ev) {
    // 监听中：按下的下一个键就是新绑定
    if (listeningAction_ >= 0) {
        if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Escape) {
                listeningAction_ = -1;
                refreshButtonText();
                return;
            }
            auto act = static_cast<KeyBindings::Action>(listeningAction_);
            KeyBindings::instance().set(act, kp->code);

            const char* prefKey = nullptr;
            switch (act) {
                case KeyBindings::MoveLeft:  prefKey = "key_left";    break;
                case KeyBindings::MoveRight: prefKey = "key_right";   break;
                case KeyBindings::Jump:      prefKey = "key_jump";    break;
                case KeyBindings::Pause:     prefKey = "key_pause";   break;
                case KeyBindings::Restart:   prefKey = "key_restart"; break;
                default: break;
            }
            if (prefKey) {
                prefs_->setInt(prefKey, static_cast<int>(kp->code));
            }
            listeningAction_ = -1;
            refreshButtonText();
            return;
        }
        return;   // 监听中不转发其他事件
    }

    for (auto& b : keyBindingButtons_) b->handleEvent(ev);
}

void KeysTab::update() {
    if (listeningAction_ >= 0) return;

    for (int i = 0; i < static_cast<int>(keyBindingButtons_.size()); ++i) {
        if (keyBindingButtons_[i]->consumeClick()) {
            listeningAction_ = i;
            keyBindingButtons_[i]->setText(Str::T(Str::KeyPressNew));
            return;
        }
    }
}

void KeysTab::refreshLabels() {
    for (int i = 0; i < static_cast<int>(keyLabels_.size()); ++i) {
        auto act = static_cast<KeyBindings::Action>(i);
        keyLabels_[i]->setString(toSf(Str::T(KeyBindings::actionName(act))));
    }
    hintText_.setString(toSf(Str::T(Str::KeyBindHint)));

    if (listeningAction_ >= 0) {
        keyBindingButtons_[listeningAction_]->setText(Str::T(Str::KeyPressNew));
    } else {
        refreshButtonText();
    }
}

void KeysTab::registerFocus(std::vector<Button*>& out) {
    for (auto& b : keyBindingButtons_) out.push_back(b.get());
}

float KeysTab::render(sf::RenderTarget& target,
                      float contentX, float ctrlX,
                      float startY) {
    float y = startY;

    for (int i = 0; i < static_cast<int>(keyBindingButtons_.size()); ++i) {
        keyLabels_[i]->setPosition({contentX, y + 8.f});
        target.draw(*keyLabels_[i]);

        keyBindingButtons_[i]->setPosition({ctrlX, y});
        keyBindingButtons_[i]->render(target);

        y += kRowH;
    }

    hintText_.setPosition({contentX, y + 8.f});
    target.draw(hintText_);

    return y + 30.f;
}