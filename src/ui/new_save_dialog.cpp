#include "new_save_dialog.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include "strings.h"

NewSaveDialog::NewSaveDialog(const sf::Font& font,
                             const std::string& defaultName,
                             sf::Vector2f windowSize)
    : title_(font, toSf(Str::NewSaveTitle), scaledFontSize(26)),
      hint_(font, toSf(Str::NewSaveHint), scaledFontSize(20)),
      windowSize_(windowSize) {

    backdrop_.setFillColor(sf::Color(0, 0, 0, 160));
    backdrop_.setSize(windowSize_);

    panel_.setFillColor(getTheme().panelBg);
    panel_.setOutlineThickness(2.f);
    panel_.setOutlineColor(getTheme().outline);

    title_.setFillColor(getTheme().textPrimary);
    hint_.setFillColor(getTheme().textSecondary);

    input_ = std::make_unique<TextInput>(
        font, sf::Vector2f{0.f, 0.f}, sf::Vector2f{380.f, 46.f},
        Str::NewSavePlaceholder, 20, 24);
    input_->setText(defaultName);
    input_->setFocused(true);   // 打开对话框即聚焦

    createButton_ = std::make_unique<Button>(Str::BtnCreate, font,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{140.f, 48.f}, 22);
    cancelButton_ = std::make_unique<Button>(Str::BtnCancel, font,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{140.f, 48.f}, 22);

    relayout(windowSize_);
}

void NewSaveDialog::relayout(sf::Vector2f windowSize) {
    windowSize_ = windowSize;
    backdrop_.setSize(windowSize_);

    const float panelW = 560.f;
    const float panelH = 300.f;
    float px = (windowSize.x - panelW) / 2.f;
    float py = (windowSize.y - panelH) / 2.f;

    panel_.setSize({panelW, panelH});
    panel_.setPosition({px, py});

    // 标题居中
    auto tb = title_.getLocalBounds();
    title_.setOrigin({tb.position.x + tb.size.x / 2.f, tb.position.y});
    title_.setPosition({windowSize.x / 2.f, py + 30.f});

    // 提示文字
    hint_.setPosition({px + 60.f, py + 100.f});

    // 输入框
    input_->setPosition({px + 60.f, py + 140.f});
    input_->setSize({panelW - 120.f, 46.f});

    // 按钮：底部居中并排
    const float gap = 40.f;
    float btnW = 140.f;
    float totalW = btnW * 2 + gap;
    float bx = px + (panelW - totalW) / 2.f;
    float by = py + panelH - 78.f;
    createButton_->setPosition({bx, by});
    cancelButton_->setPosition({bx + btnW + gap, by});
}

void NewSaveDialog::handleEvent(const sf::Event& event) {
    input_->handleEvent(event);
    createButton_->handleEvent(event);
    cancelButton_->handleEvent(event);

    // Enter 键提交
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Enter && input_->isFocused()) {
            result_ = Result::Created;
            return;
        }
    }

    if (createButton_->consumeClick()) {
        result_ = Result::Created;
    }
    if (cancelButton_->consumeClick()) {
        result_ = Result::Cancelled;
    }
}

NewSaveDialog::Result NewSaveDialog::consumeResult() {
    Result r = result_;
    result_ = Result::None;
    return r;
}

const std::string& NewSaveDialog::getName() const {
    return input_->text();
}

void NewSaveDialog::render(sf::RenderTarget& target) {
    panel_.setFillColor(getTheme().panelBg);
    panel_.setOutlineColor(getTheme().outline);
    title_.setFillColor(getTheme().textPrimary);
    hint_.setFillColor(getTheme().textSecondary);

    target.draw(backdrop_);
    target.draw(panel_);
    target.draw(title_);
    target.draw(hint_);
    input_->render(target);
    createButton_->render(target);
    cancelButton_->render(target);
}