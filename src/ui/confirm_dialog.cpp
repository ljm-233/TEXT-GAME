#include "confirm_dialog.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"

ConfirmDialog::ConfirmDialog(const sf::Font& font,
                             const std::string& message,
                             sf::Vector2f windowSize)
    : message_(font, toSf(message), scaledFontSize(26)),
      windowSize_(windowSize) {

    backdrop_.setFillColor(sf::Color(0, 0, 0, 160));
    backdrop_.setSize(windowSize_);

    panel_.setFillColor(getTheme().panelBg);
    panel_.setOutlineThickness(2.f);
    panel_.setOutlineColor(getTheme().outline);

    message_.setFillColor(getTheme().textPrimary);

    yesButton_ = std::make_unique<Button>("是", font, sf::Vector2f{0.f, 0.f},
                                          sf::Vector2f{120.f, 50.f}, 22);
    noButton_  = std::make_unique<Button>("否", font, sf::Vector2f{0.f, 0.f},
                                          sf::Vector2f{120.f, 50.f}, 22);

    relayout(windowSize_);
}

void ConfirmDialog::relayout(sf::Vector2f windowSize) {
    windowSize_ = windowSize;

    backdrop_.setSize(windowSize_);

    const float panelW = 480.f;
    const float panelH = 220.f;
    float px = (windowSize.x - panelW) / 2.f;
    float py = (windowSize.y - panelH) / 2.f;

    panel_.setSize({panelW, panelH});
    panel_.setPosition({px, py});

    auto b = message_.getLocalBounds();
    message_.setOrigin({b.position.x + b.size.x / 2.f,
                        b.position.y + b.size.y / 2.f});
    message_.setPosition({windowSize.x / 2.f, py + 70.f});

    const float gap = 40.f;
    float btnW = 120.f;
    float totalW = btnW * 2 + gap;
    float bx = px + (panelW - totalW) / 2.f;
    float by = py + panelH - 80.f;

    yesButton_->setPosition({bx, by});
    noButton_->setPosition({bx + btnW + gap, by});
}

void ConfirmDialog::handleEvent(const sf::Event& event) {
    yesButton_->handleEvent(event);
    noButton_->handleEvent(event);

    if (yesButton_->consumeClick()) result_ = Result::Yes;
    if (noButton_->consumeClick())  result_ = Result::No;
}

ConfirmDialog::Result ConfirmDialog::consumeResult() {
    Result r = result_;
    result_ = Result::None;
    return r;
}

void ConfirmDialog::render(sf::RenderTarget& target) {
    // 每帧刷新颜色
    panel_.setFillColor(getTheme().panelBg);
    panel_.setOutlineColor(getTheme().outline);
    message_.setFillColor(getTheme().textPrimary);

    target.draw(backdrop_);
    target.draw(panel_);
    target.draw(message_);
    yesButton_->render(target);
    noButton_->render(target);
}