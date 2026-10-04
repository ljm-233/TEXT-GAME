#include "tabs/other_tab.h"
#include "utils/text_strings.h"
#include "ui_scale.h"
#include "utils/utf8.h"
#include "log/rotation.h"
#include "config/keys.h"

#include <algorithm>
#include <functional>

namespace {

constexpr float kRowH = 50.f;

// 轮转档位的取值范围与取值本身由日志层维护（log/rotation.h 是唯一权威来源），
// 这里只保留展示用的标签。UI 与日志层各存一份档位表，迟早会漂移。
const char* kLogRotateLabels[] = {"无限", "1MB", "5MB", "10MB"};
static_assert(kLogRotationSteps == 4, "标签表必须与日志层的档位数一致");

} // namespace

OtherTab::OtherTab(const sf::Font& font,
                   std::shared_ptr<Preferences> prefs,
                   std::shared_ptr<Logger> logger)
      : font_(font),
        prefs_(std::move(prefs)),
        logger_(std::move(logger)),
        labelPlayerName_(font, sf::String(), scaledFontSize(20)) {

    loadFromPrefs();

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
    auto addMulti = [&](const char* key, std::function<void(int)> cb) {
        auto row = std::make_unique<MultiRow>(font_, key, std::move(cb));
        multiRows_.push_back(std::move(row));
        return multiRows_.back().get();
    };

    // [0] LogRotate
    {
        auto* row = addMulti(Str::LabelLogRotate, [this](int i) {
            logRotateIndex_ = i;
            refreshSelection(); applyLogRotation();
        });
        for (int i = 0; i < kLogRotationSteps; ++i)
            row->addButton(std::make_unique<Button>(
                kLogRotateLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    }
    // [1] LogKeep
    {
        auto* row = addMulti(Str::LabelLogKeep, [this](int i) {
            logKeepIndex_ = i;
            refreshSelection(); applyLogRotation();
        });
        for (int i = 0; i < kLogRotationSteps; ++i)
            row->addButton(std::make_unique<Button>(
                std::to_string(logRotationKeepAt(i)), font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    }

    // Toggles
    addToggle(Str::LabelRememberSize, [this](bool v) {
        rememberSize_ = v; refreshSelection();
        prefs_->setBool(ConfigKey::kRememberWindowSize, v);
    });
    addToggle(Str::LabelAutoPause, [this](bool v) {
        autoPauseOnBlur_ = v; refreshSelection(); applyAutoPause();
    });

    labelPlayerName_.setFillColor(sf::Color(230, 230, 230));

    // 玩家名输入框
    playerNameInput_ = std::make_unique<TextInput>(
        font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 40.f},
        Str::PlayerNamePlaceholder, 18, 16);
    playerNameInput_->setText(prefs_->get(ConfigKey::kPlayerName, ""));
    playerNameInput_->setOnChanged([this](const std::string& s) {
        prefs_->set(ConfigKey::kPlayerName, s);
    });

    refreshLabels();
    refreshSelection();
}

void OtherTab::loadFromPrefs() {
    rememberSize_    = prefs_->getBool(ConfigKey::kRememberWindowSize, true);
    autoPauseOnBlur_ = prefs_->getBool(ConfigKey::kAutoPauseOnBlur, true);
    logRotateIndex_  = clampLogRotationIndex(prefs_->getInt(ConfigKey::kLogRotate, 0));
    logKeepIndex_    = clampLogKeepIndex(prefs_->getInt(ConfigKey::kLogKeep, 1));
}

void OtherTab::applyAutoPause() {
    prefs_->setBool(ConfigKey::kAutoPauseOnBlur, autoPauseOnBlur_);
}

void OtherTab::applyLogRotation() {
    logger_->setRotation(logRotationSizeAt(logRotateIndex_),
                         logRotationKeepAt(logKeepIndex_));
    prefs_->setInt(ConfigKey::kLogRotate, logRotateIndex_);
    prefs_->setInt(ConfigKey::kLogKeep,   logKeepIndex_);
}

void OtherTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_) row->refreshLabel();
    labelPlayerName_.setString(toSf(Str::T(Str::LabelPlayerName)));
}

void OtherTab::refreshSelection() {
    if (toggles_.size() == 2) {
        auto setRow = [](ToggleRow& row, bool v) {
            row.currentValue = v;
            row.onButton->setSelected(v);
            row.offButton->setSelected(!v);
        };
        setRow(*toggles_[0], rememberSize_);
        setRow(*toggles_[1], autoPauseOnBlur_);
    }
    if (multiRows_.size() == 2) {
        multiRows_[0]->setSelected(logRotateIndex_);
        multiRows_[1]->setSelected(logKeepIndex_);
    }
}

void OtherTab::handleEvent(const sf::Event& ev) {
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons) btn->handleEvent(ev);
    if (playerNameInput_) playerNameInput_->handleEvent(ev);
}

void OtherTab::update() {
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
    for (auto& row : multiRows_) {
        for (size_t i = 0; i < row->buttons.size(); ++i) {
            if (row->buttons[i]->consumeClick()) {
                if (row->currentIndex != static_cast<int>(i) && row->onSelected)
                    row->onSelected(static_cast<int>(i));
                return;
            }
        }
    }
}

void OtherTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons) out.push_back(btn.get());
    // playerNameInput_ 不参与手柄焦点（TextInput 本身不是 Button）
}

bool OtherTab::anyEditing() const {
    return playerNameInput_ && playerNameInput_->isFocused();
}

float OtherTab::render(sf::RenderTarget& target,
                       float contentX, float ctrlX,
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
            row.buttons[i]->setPosition(
                {ctrlX + static_cast<float>(i) * row.stepX, y});
            row.buttons[i]->render(target);
        }
        y += kRowH;
    };

    drawToggle(*toggles_[0]);   // RememberSize
    drawToggle(*toggles_[1]);   // AutoPause
    drawMulti (*multiRows_[0]); // LogRotate
    drawMulti (*multiRows_[1]); // LogKeep

    // 玩家名输入框
    labelPlayerName_.setPosition({contentX, y + 8.f});
    target.draw(labelPlayerName_);
    if (playerNameInput_) {
        playerNameInput_->setPosition({ctrlX, y});
        playerNameInput_->setSize({240.f, 40.f});
        playerNameInput_->render(target);
    }
    y += kRowH;

    // 底部留 40 像素给"关于 / 重置"按钮（这两个在窗口坐标系，不在这里画）
    return y + 40.f;
}