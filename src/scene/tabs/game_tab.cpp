#include "tabs/game_tab.h"

#include "config/keys.h"
#include "ui_scale.h"
#include "utils/text_strings.h"
#include "utils/utf8.h"

#include <functional>

namespace {

constexpr float kRowH = 50.f;

const int kLives[] = {1, 3, 5, 10, 100};
constexpr int kLivesCount = 5;

int indexOfLives(int lives) {
    for (int i = 0; i < kLivesCount; ++i)
        if (kLives[i] == lives)
            return i;
    return 0;
}

} // namespace

GameTab::GameTab(const sf::Font& font, std::shared_ptr<Preferences> prefs)
      : font_(font),
        prefs_(std::move(prefs)),
        labelPlayerName_(font, sf::String(), scaledFontSize(20)) {
    labelPlayerName_.setFillColor(sf::Color(230, 230, 230));

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
    auto addMulti = [this](const char* key, std::function<void(int)> cb) {
        auto row = std::make_unique<MultiRow>(font_, key, std::move(cb));
        multiRows_.push_back(std::move(row));
        return multiRows_.back().get();
    };

    // 初始生命：给整局游戏定个难度基调，所以放在「游戏」页而不是画面页
    rowLives_ = addMulti(Str::LabelInitialLives, [this](int i) {
        initialLives_ = kLives[i];
        refreshSelection();
        applyInitialLives();
    });
    for (int i = 0; i < kLivesCount; ++i)
        rowLives_->addButton(std::make_unique<Button>(std::to_string(kLives[i]), font_,
                                                      sf::Vector2f{0.f, 0.f},
                                                      sf::Vector2f{86.f, 40.f}, 18));

    rowLevelIntro_ = addToggle(Str::LabelLevelIntro, [this](bool v) {
        levelIntroEnabled_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kLevelIntro, v);
    });

    playerNameInput_ = std::make_unique<TextInput>(font_, sf::Vector2f{0.f, 0.f},
                                                   sf::Vector2f{240.f, 40.f},
                                                   Str::PlayerNamePlaceholder, 18, 16);
    playerNameInput_->setText(prefs_->get(ConfigKey::kPlayerName, ""));
    playerNameInput_->setOnChanged(
        [this](const std::string& s) { prefs_->set(ConfigKey::kPlayerName, s); });

    loadFromPrefs();
    refreshLabels();
    refreshSelection();
}

void GameTab::loadFromPrefs() {
    initialLives_ = prefs_->getInt(ConfigKey::kInitialLives, 1);
    levelIntroEnabled_ = prefs_->getBool(ConfigKey::kLevelIntro, true);
}

void GameTab::applyInitialLives() {
    prefs_->setInt(ConfigKey::kInitialLives, initialLives_);
}

void GameTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_)
        row->refreshLabel();
    labelPlayerName_.setString(toSf(Str::T(Str::LabelPlayerName)));
}

void GameTab::refreshSelection() {
    if (rowLevelIntro_) {
        rowLevelIntro_->currentValue = levelIntroEnabled_;
        rowLevelIntro_->onButton->setSelected(levelIntroEnabled_);
        rowLevelIntro_->offButton->setSelected(!levelIntroEnabled_);
    }
    if (rowLives_)
        rowLives_->setSelected(indexOfLives(initialLives_));
}

void GameTab::handleEvent(const sf::Event& ev) {
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            btn->handleEvent(ev);
    if (playerNameInput_)
        playerNameInput_->handleEvent(ev);
}

void GameTab::update() {
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
}

void GameTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            out.push_back(btn.get());
    // 玩家名输入框不是 Button，不参与几何导航（Tab 键 / 鼠标点进去）
}

bool GameTab::anyEditing() const {
    return playerNameInput_ && playerNameInput_->isFocused();
}

float GameTab::render(sf::RenderTarget& target, float contentX, float ctrlX,
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

    if (rowLives_)
        drawMulti(*rowLives_);
    if (rowLevelIntro_)
        drawToggle(*rowLevelIntro_);

    // 玩家名
    labelPlayerName_.setPosition({contentX, y + 8.f});
    target.draw(labelPlayerName_);
    if (playerNameInput_) {
        playerNameInput_->setPosition({ctrlX, y});
        playerNameInput_->setSize({240.f, 40.f});
        playerNameInput_->render(target);
    }
    y += kRowH;

    return y;
}

void GameTab::reapply() {
    // 玩法项在进入关卡时才读，没有需要即时改的全局状态
    loadFromPrefs();
    refreshSelection();
}
