#include "pause_menu.h"
#include "animation.h"
#include "notification.h"
#include "text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include "focus_group.h"

#include <algorithm>
#include <functional>

PauseMenu::PauseMenu(const sf::Font& font,
                     std::shared_ptr<Preferences> prefs,
                     sf::Vector2f windowSize)
    : font_(font),
      prefs_(std::move(prefs)),
      windowSize_(windowSize),
      title_(font, sf::String(), scaledFontSize(32)) {

    backdrop_.setFillColor(sf::Color(0, 0, 0, 180));
    backdrop_.setSize(windowSize_);

    panel_.setFillColor(sf::Color(30, 30, 45));
    panel_.setOutlineThickness(2.f);
    panel_.setOutlineColor(sf::Color(90, 90, 130));

    title_.setFillColor(sf::Color(240, 240, 250));

    mainButtons_.push_back(std::make_unique<Button>(
        Str::T("回到游戏"), font_, sf::Vector2f{0.f, 0.f},
        sf::Vector2f{280.f, 52.f}, 22));
    mainButtons_.push_back(std::make_unique<Button>(
        Str::T(Str::Settings), font_, sf::Vector2f{0.f, 0.f},
        sf::Vector2f{280.f, 52.f}, 22));
    mainButtons_.push_back(std::make_unique<Button>(
        Str::T("保存并退出游戏"), font_, sf::Vector2f{0.f, 0.f},
        sf::Vector2f{280.f, 52.f}, 22));

    refreshLabels();
    relayout(windowSize_);
    syncFocus();
}

void PauseMenu::syncFocus() {
    std::vector<Button*> items;
    for (auto& b : mainButtons_) items.push_back(b.get());
    FocusGroup::instance().setItems(items);
}

void PauseMenu::refreshLabels() {
    title_.setString(toSf(Str::T("已暂停")));

    if (mainButtons_.size() >= 3) {
        mainButtons_[0]->setText(Str::T("回到游戏"));
        mainButtons_[1]->setText(Str::T(Str::Settings));
        mainButtons_[2]->setText(Str::T("保存并退出游戏"));
    }
}

void PauseMenu::relayout(sf::Vector2f windowSize) {
    windowSize_ = windowSize;
    backdrop_.setSize(windowSize_);

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
}

void PauseMenu::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            pendingAction_ = Action::Resume;
            return;
        }
    }

    for (auto& b : mainButtons_) b->handleEvent(event);
}

void PauseMenu::update(float /*dt*/) {
    if (mainButtons_[0]->consumeClick())
        pendingAction_ = Action::Resume;
    if (mainButtons_[1]->consumeClick())
        pendingAction_ = Action::OpenSettings;
    if (mainButtons_[2]->consumeClick())
        pendingAction_ = Action::SaveAndQuit;
}

PauseMenu::Action PauseMenu::consumeAction() {
    Action a = pendingAction_;
    pendingAction_ = Action::None;
    return a;
}

void PauseMenu::render(sf::RenderTarget& target) {
    refreshLabels();

    target.draw(backdrop_);
    target.draw(panel_);
    target.draw(title_);
    for (auto& b : mainButtons_) b->render(target);
}

void PauseMenu::reset() {
    pendingAction_ = Action::None;
    refreshLabels();
    relayout(windowSize_);
    syncFocus();
}