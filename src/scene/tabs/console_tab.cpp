#include "tabs/console_tab.h"

#include "config/keys.h"
#include "ui_scale.h"
#include "utils/text_strings.h"
#include "utils/utf8.h"

#include <algorithm>

namespace {

constexpr float kRowH = 50.f;

const int kFonts[] = {14, 18, 22, 26};
constexpr int kFontCount = 4;
const char* kFontLabels[] = {"小", "中", "大", "特大"};

const int kHistory[] = {50, 100, 200, 500};
constexpr int kHistoryCount = 4;

const int kLineHeights[] = {20, 26, 32};
constexpr int kLineHeightCount = 3;
const char* kLineHeightLabels[] = {"紧凑", "正常", "宽松"};

const char* kPromptLabels[] = {">", "$", "λ", "❯"};
constexpr int kPromptCount = 4;

int indexOfFont(int f) {
    for (int i = 0; i < kFontCount; ++i)
        if (kFonts[i] == f)
            return i;
    return 1;
}
int indexOfHistory(int n) {
    for (int i = 0; i < kHistoryCount; ++i)
        if (kHistory[i] == n)
            return i;
    return 2;
}
int indexOfLineHeight(int h) {
    for (int i = 0; i < kLineHeightCount; ++i)
        if (kLineHeights[i] == h)
            return i;
    return 1;
}
int indexOfPrompt(int i) {
    return (i < 0 || i >= kPromptCount) ? 0 : i;
}

} // namespace

ConsoleTab::ConsoleTab(const sf::Font& font, std::shared_ptr<Preferences> prefs)
      : font_(font),
        prefs_(std::move(prefs)),
        labelMask_(font, sf::String(), scaledFontSize(20)),
        labelPanelAlpha_(font, sf::String(), scaledFontSize(20)) {
    auto labelColor = sf::Color(230, 230, 230);
    labelMask_.setFillColor(labelColor);
    labelPanelAlpha_.setFillColor(labelColor);

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
    // ⚠️ 返回的是裸指针，但它是**堆上那个对象**的地址，缓存下来是安全的：
    //    vector 扩容搬的是 unique_ptr 本身，指向的 MultiRow 不会跟着动。
    auto addMulti = [this](const char* key, std::function<void(int)> cb) {
        auto row = std::make_unique<MultiRow>(font_, key, std::move(cb));
        multiRows_.push_back(std::move(row));
        return multiRows_.back().get();
    };

    rowFont_ = addMulti(Str::LabelConsoleFont, [this](int i) {
        fontSize_ = kFonts[i];
        refreshSelection();
        applyFont();
    });
    for (int i = 0; i < kFontCount; ++i)
        rowFont_->addButton(std::make_unique<Button>(
            kFontLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));

    rowHistory_ = addMulti(Str::LabelConsoleHistory, [this](int i) {
        historyLines_ = kHistory[i];
        refreshSelection();
        applyHistory();
    });
    for (int i = 0; i < kHistoryCount; ++i)
        rowHistory_->addButton(std::make_unique<Button>(std::to_string(kHistory[i]),
                                                        font_, sf::Vector2f{0.f, 0.f},
                                                        sf::Vector2f{86.f, 40.f}, 18));

    rowLineHeight_ = addMulti(Str::LabelConsoleLineHeight, [this](int i) {
        lineHeight_ = kLineHeights[i];
        refreshSelection();
        applyLineHeight();
    });
    for (int i = 0; i < kLineHeightCount; ++i)
        rowLineHeight_->addButton(std::make_unique<Button>(kLineHeightLabels[i], font_,
                                                           sf::Vector2f{0.f, 0.f},
                                                           sf::Vector2f{86.f, 40.f}, 18));

    rowPrompt_ = addMulti(Str::LabelConsolePrompt, [this](int i) {
        promptIdx_ = i;
        refreshSelection();
        applyPrompt();
    });
    rowPrompt_->stepX = 70.f;
    for (int i = 0; i < kPromptCount; ++i)
        rowPrompt_->addButton(std::make_unique<Button>(kPromptLabels[i], font_,
                                                       sf::Vector2f{0.f, 0.f},
                                                       sf::Vector2f{60.f, 40.f}, 18));

    rowAutoScroll_ = addToggle(Str::LabelConsoleAutoScroll, [this](bool v) {
        autoScroll_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kConsoleAutoScroll, v);
    });
    rowBlink_ = addToggle(Str::LabelConsoleBlink, [this](bool v) {
        blinkCursor_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kConsoleBlinkCursor, v);
    });

    maskSlider_ =
        std::make_unique<Slider>(font_, 0.f, 255.f, static_cast<float>(mask_),
                                 sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    maskSlider_->setDefaultValue(160.f);

    panelAlphaSlider_ =
        std::make_unique<Slider>(font_, 0.f, 255.f, static_cast<float>(panelAlpha_),
                                 sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    panelAlphaSlider_->setDefaultValue(220.f);

    loadFromPrefs();
    refreshLabels();
    refreshSelection();
}

void ConsoleTab::loadFromPrefs() {
    fontSize_ = prefs_->getInt(ConfigKey::kConsoleFontSize, 18);
    historyLines_ = prefs_->getInt(ConfigKey::kConsoleHistoryLines, 200);
    lineHeight_ = prefs_->getInt(ConfigKey::kConsoleLineHeight, 26);
    promptIdx_ = indexOfPrompt(prefs_->getInt(ConfigKey::kConsolePrompt, 0));
    autoScroll_ = prefs_->getBool(ConfigKey::kConsoleAutoScroll, true);
    blinkCursor_ = prefs_->getBool(ConfigKey::kConsoleBlinkCursor, true);
    mask_ = std::clamp(prefs_->getInt(ConfigKey::kConsoleMask, 160), 0, 255);
    panelAlpha_ = std::clamp(prefs_->getInt(ConfigKey::kConsolePanelAlpha, 220), 0, 255);

    maskSlider_->setValue(static_cast<float>(mask_));
    panelAlphaSlider_->setValue(static_cast<float>(panelAlpha_));
}

void ConsoleTab::applyFont() {
    prefs_->setInt(ConfigKey::kConsoleFontSize, fontSize_);
}
void ConsoleTab::applyHistory() {
    prefs_->setInt(ConfigKey::kConsoleHistoryLines, historyLines_);
}
void ConsoleTab::applyLineHeight() {
    prefs_->setInt(ConfigKey::kConsoleLineHeight, lineHeight_);
}
void ConsoleTab::applyPrompt() {
    prefs_->setInt(ConfigKey::kConsolePrompt, promptIdx_);
}

void ConsoleTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_)
        row->refreshLabel();

    labelMask_.setString(toSf(Str::T(Str::LabelConsoleMask)));
    labelPanelAlpha_.setString(toSf(Str::T(Str::LabelConsolePanelAlpha)));
}

void ConsoleTab::refreshSelection() {
    auto setToggle = [](ToggleRow* row, bool v) {
        if (!row)
            return;
        row->currentValue = v;
        row->onButton->setSelected(v);
        row->offButton->setSelected(!v);
    };
    setToggle(rowAutoScroll_, autoScroll_);
    setToggle(rowBlink_, blinkCursor_);

    if (rowFont_)
        rowFont_->setSelected(indexOfFont(fontSize_));
    if (rowHistory_)
        rowHistory_->setSelected(indexOfHistory(historyLines_));
    if (rowLineHeight_)
        rowLineHeight_->setSelected(indexOfLineHeight(lineHeight_));
    if (rowPrompt_)
        rowPrompt_->setSelected(promptIdx_);
}

void ConsoleTab::handleEvent(const sf::Event& ev) {
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            btn->handleEvent(ev);
    maskSlider_->handleEvent(ev);
    panelAlphaSlider_->handleEvent(ev);
}

void ConsoleTab::update() {
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
    for (auto& row : multiRows_) {
        for (size_t i = 0; i < row->buttons.size(); ++i) {
            if (row->buttons[i]->consumeClick()) {
                if (row->currentIndex != static_cast<int>(i) && row->onSelected)
                    row->onSelected(static_cast<int>(i));
                return;
            }
        }
    }
    if (maskSlider_->consumeChanged()) {
        mask_ = static_cast<int>(maskSlider_->value());
        prefs_->setInt(ConfigKey::kConsoleMask, mask_);
    }
    if (panelAlphaSlider_->consumeChanged()) {
        panelAlpha_ = static_cast<int>(panelAlphaSlider_->value());
        prefs_->setInt(ConfigKey::kConsolePanelAlpha, panelAlpha_);
    }
}

void ConsoleTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            out.push_back(btn.get());
}

bool ConsoleTab::anyEditing() const {
    if (maskSlider_ && maskSlider_->isEditing())
        return true;
    if (panelAlphaSlider_ && panelAlphaSlider_->isEditing())
        return true;
    return false;
}

float ConsoleTab::render(sf::RenderTarget& target, float contentX, float ctrlX,
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

    drawSlider(labelMask_, *maskSlider_);
    drawSlider(labelPanelAlpha_, *panelAlphaSlider_);
    if (rowFont_)
        drawMulti(*rowFont_);
    if (rowHistory_)
        drawMulti(*rowHistory_);
    if (rowLineHeight_)
        drawMulti(*rowLineHeight_);
    if (rowAutoScroll_)
        drawToggle(*rowAutoScroll_);
    if (rowBlink_)
        drawToggle(*rowBlink_);
    if (rowPrompt_)
        drawMulti(*rowPrompt_);

    return y;
}

void ConsoleTab::reapply() {
    // 控制台外观在打开控制台时读，没有需要即时改的全局状态
    loadFromPrefs();
    refreshSelection();
}
