#include "tabs/interface_tab.h"
#include "utils/text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utils/utf8.h"
#include "utils/lang.h"
#include "utils/animation.h"
#include "notification.h"
#include "button_style.h"
#include "config/keys.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr float kRowH = 50.f;

const char* kPosLabels[] = {"左上", "右上", "左下", "右下"};
constexpr int kPosCount = 4;

const char* kFpsFormatLabels[] = {"纯数字", "60 FPS", "60.0 FPS"};
constexpr int kFpsFormatCount = 3;

const float kUiScales[] = {0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f};
constexpr int kUiScaleCount = 10;
const char* kUiScaleLabels[] = {"0.5x", "0.6x", "0.7x",  "0.8x", "0.9x",
                                "1.0x", "1.1x", "1.25x", "1.5x", "2.0x"};

// 按钮圆角 / 边框：档位与标签从「画面」页原样搬来（graphics_tab.cpp），
// 不要另发明一套取值。
const float kCorners[] = {0.f, 6.f, 14.f};
constexpr int kCornerCount = 3;
const char* kCornerLabels[] = {"直角", "小圆", "大圆"};

const float kOutlines[] = {0.f, 2.f, 4.f};
constexpr int kOutlineCount = 3;
const char* kOutlineLabels[] = {"无", "细", "粗"};

const char* kAnimSpeedLabels[] = {"慢", "正常", "快"};
constexpr int kAnimSpeedCount = 3;

int indexOfUiScale(float s) {
    for (int i = 0; i < kUiScaleCount; ++i)
        if (std::abs(kUiScales[i] - s) < 0.01f)
            return i;
    return 5;
}
int indexOfButtonCorner(float c) {
    for (int i = 0; i < kCornerCount; ++i)
        if (std::abs(kCorners[i] - c) < 0.5f)
            return i;
    return 1;
}
int indexOfButtonOutline(float o) {
    for (int i = 0; i < kOutlineCount; ++i)
        if (std::abs(kOutlines[i] - o) < 0.5f)
            return i;
    return 1;
}
int indexOfPos(int idx) {
    return (idx < 0 || idx >= kPosCount) ? 1 : idx;
}
int indexOfFpsFormat(int idx) {
    return (idx < 0 || idx >= kFpsFormatCount) ? 1 : idx;
}

} // namespace

InterfaceTab::InterfaceTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                           std::shared_ptr<Window> /*window*/)
      : font_(font),
        prefs_(std::move(prefs)),
        labelNotificationDuration_(font, sf::String(), scaledFontSize(20)),
        hintUiScale_(font, sf::String(), scaledFontSize(14)) {
    loadFromPrefs();

    auto labelColor = sf::Color(230, 230, 230);
    labelNotificationDuration_.setFillColor(labelColor);
    hintUiScale_.setFillColor(sf::Color(180, 180, 200));

    auto makeToggle = [&](const std::string& onText, const std::string& offText) {
        auto on = std::make_unique<Button>(onText, font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(offText, font_, sf::Vector2f{0.f, 0.f},
                                            sf::Vector2f{86.f, 40.f}, 18);
        return std::make_pair(std::move(on), std::move(off));
    };
    auto addToggle = [&](const char* key, std::function<void(bool)> cb) -> ToggleRow* {
        auto row = std::make_unique<ToggleRow>(font_, key, std::move(cb));
        auto [on, off] = makeToggle(Str::On, Str::Off);
        row->onButton = std::move(on);
        row->offButton = std::move(off);
        toggles_.push_back(std::move(row));
        return toggles_.back().get();
    };
    auto addMulti = [&](const char* key, std::function<void(int)> cb) {
        auto row = std::make_unique<MultiRow>(font_, key, std::move(cb));
        multiRows_.push_back(std::move(row));
        return multiRows_.back().get();
    };

    // FpsPos
    rowFpsPos_ = addMulti(Str::LabelFpsPos, [this](int i) {
        fpsPosition_ = i;
        refreshSelection();
        applyFpsPosition();
    });
    rowFpsPos_->stepX = 86.f;
    for (int i = 0; i < kPosCount; ++i)
        rowFpsPos_->addButton(std::make_unique<Button>(
            kPosLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));

    // FpsFormat
    rowFpsFormat_ = addMulti(Str::LabelFpsFormat, [this](int i) {
        fpsFormat_ = i;
        refreshSelection();
        applyFpsFormat();
    });
    rowFpsFormat_->stepX = 114.f;
    for (int i = 0; i < kFpsFormatCount; ++i)
        rowFpsFormat_->addButton(std::make_unique<Button>(kFpsFormatLabels[i], font_,
                                                          sf::Vector2f{0.f, 0.f},
                                                          sf::Vector2f{110.f, 40.f}, 16));

    // UiScale
    rowUiScale_ = addMulti(Str::LabelUiScale, [this](int i) {
        uiScale_ = kUiScales[i];
        refreshSelection();
        setUiScale(uiScale_);
        prefs_->setDouble(ConfigKey::kUiScale, uiScale_);
    });
    for (int i = 0; i < kUiScaleCount; ++i)
        rowUiScale_->addButton(std::make_unique<Button>(kUiScaleLabels[i], font_,
                                                        sf::Vector2f{0.f, 0.f},
                                                        sf::Vector2f{86.f, 40.f}, 18));

    // FontScale
    rowFontScale_ = addMulti(Str::LabelFontScale, [this](int i) {
        fontScale_ = kUiScales[i];
        refreshSelection();
        setFontScale(fontScale_);
        prefs_->setDouble(ConfigKey::kFontScale, fontScale_);
    });
    for (int i = 0; i < kUiScaleCount; ++i)
        rowFontScale_->addButton(std::make_unique<Button>(kUiScaleLabels[i], font_,
                                                          sf::Vector2f{0.f, 0.f},
                                                          sf::Vector2f{86.f, 40.f}, 18));

    // Theme
    rowTheme_ = addMulti(Str::LabelTheme, [this](int i) {
        themeId_ = i;
        refreshSelection();
        applyTheme();
    });
    rowTheme_->stepX = 110.f;
    for (int i = 0; i < kThemeCount; ++i)
        rowTheme_->addButton(std::make_unique<Button>(themeName(static_cast<ThemeId>(i)),
                                                      font_, sf::Vector2f{0.f, 0.f},
                                                      sf::Vector2f{100.f, 40.f}, 18));

    // Language
    rowLanguage_ = addMulti(Str::LabelLanguage, [this](int i) {
        languageIdx_ = i;
        refreshSelection();
        applyLanguage();
    });
    rowLanguage_->stepX = 110.f;
    for (const auto& code : Lang::instance().available()) {
        std::string label = code;
        if (code == "zh")
            label = "中文";
        else if (code == "zh-TW")
            label = "繁體中文";
        else if (code == "en")
            label = "English";
        else if (code == "ja")
            label = "日本語";
        else if (code == "ko")
            label = "한국어";
        rowLanguage_->addButton(std::make_unique<Button>(
            label, font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
    }

    // ClockPos
    rowClockPos_ = addMulti(Str::LabelClockPos, [this](int i) {
        clockPosition_ = i;
        refreshSelection();
        prefs_->setInt(ConfigKey::kClockPosition, clockPosition_);
    });
    rowClockPos_->stepX = 86.f;
    for (int i = 0; i < kPosCount; ++i)
        rowClockPos_->addButton(std::make_unique<Button>(
            kPosLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));

    // ButtonCorner
    rowButtonCorner_ = addMulti(Str::LabelButtonCorner, [this](int i) {
        buttonCorner_ = kCorners[i];
        refreshSelection();
        applyButtonStyle();
    });
    for (int i = 0; i < kCornerCount; ++i)
        rowButtonCorner_->addButton(
            std::make_unique<Button>(kCornerLabels[i], font_, sf::Vector2f{0.f, 0.f},
                                     sf::Vector2f{86.f, 40.f}, 18));

    // ButtonOutline
    rowButtonOutline_ = addMulti(Str::LabelButtonOutline, [this](int i) {
        buttonOutline_ = kOutlines[i];
        refreshSelection();
        applyButtonStyle();
    });
    for (int i = 0; i < kOutlineCount; ++i)
        rowButtonOutline_->addButton(
            std::make_unique<Button>(kOutlineLabels[i], font_, sf::Vector2f{0.f, 0.f},
                                     sf::Vector2f{86.f, 40.f}, 18));

    // AnimationSpeed
    rowAnimationSpeed_ = addMulti(Str::LabelAnimationSpeed, [this](int i) {
        animationSpeedIndex_ = i;
        refreshSelection();
        applyAnimation();
    });
    for (int i = 0; i < kAnimSpeedCount; ++i)
        rowAnimationSpeed_->addButton(
            std::make_unique<Button>(kAnimSpeedLabels[i], font_, sf::Vector2f{0.f, 0.f},
                                     sf::Vector2f{86.f, 40.f}, 18));

    // NotificationPos
    rowNotificationPos_ = addMulti(Str::LabelNotificationPos, [this](int i) {
        notificationPosition_ = i;
        refreshSelection();
        applyNotification();
    });
    rowNotificationPos_->stepX = 86.f;
    for (int i = 0; i < kPosCount; ++i)
        rowNotificationPos_->addButton(std::make_unique<Button>(
            kPosLabels[i], font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));

    // Toggles
    rowFps_ = addToggle(Str::LabelFps, [this](bool v) {
        showFps_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kShowFps, v);
    });
    rowClock_ = addToggle(Str::LabelClock, [this](bool v) {
        showClock_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kShowClock, v);
    });
    rowAnimation_ = addToggle(Str::LabelAnimation, [this](bool v) {
        animationEnabled_ = v;
        refreshSelection();
        applyAnimation();
    });
    rowNotification_ = addToggle(Str::LabelNotification, [this](bool v) {
        notificationEnabled_ = v;
        refreshSelection();
        applyNotification();
    });

    // 通知时长 Slider（配置里是毫秒）
    notificationDurationSlider_ = std::make_unique<Slider>(
        font_, 1000.f, 8000.f, static_cast<float>(notificationDuration_),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    notificationDurationSlider_->setDefaultValue(3000.f);

    refreshLabels();
    refreshSelection();
}

void InterfaceTab::loadFromPrefs() {
    showFps_ = prefs_->getBool(ConfigKey::kShowFps, false);
    fpsPosition_ = prefs_->getInt(ConfigKey::kFpsPosition, 1);
    fpsFormat_ = indexOfFpsFormat(prefs_->getInt(ConfigKey::kFpsFormat, 1));
    uiScale_ = static_cast<float>(prefs_->getDouble(ConfigKey::kUiScale, 1.0));
    fontScale_ = static_cast<float>(prefs_->getDouble(ConfigKey::kFontScale, 1.0));
    themeId_ = prefs_->getInt(ConfigKey::kTheme, 0);
    showClock_ = prefs_->getBool(ConfigKey::kShowClock, false);
    clockPosition_ = prefs_->getInt(ConfigKey::kClockPosition, 0);
    buttonCorner_ = static_cast<float>(prefs_->getDouble(ConfigKey::kButtonCorner, 6.0));
    buttonOutline_ =
        static_cast<float>(prefs_->getDouble(ConfigKey::kButtonOutline, 2.0));
    animationEnabled_ = prefs_->getBool(ConfigKey::kAnimationEnabled, true);
    animationSpeedIndex_ = std::clamp(prefs_->getInt(ConfigKey::kAnimationSpeedIndex, 1),
                                      0, kAnimSpeedCount - 1);
    notificationEnabled_ = prefs_->getBool(ConfigKey::kNotificationEnabled, true);
    notificationPosition_ =
        std::clamp(prefs_->getInt(ConfigKey::kNotificationPosition, 0), 0, kPosCount - 1);
    notificationDuration_ =
        std::clamp(prefs_->getInt(ConfigKey::kNotificationDuration, 3000), 1000, 8000);

    {
        std::string langCode = prefs_->get(ConfigKey::kLanguage, "zh");
        const auto& avail = Lang::instance().available();
        languageIdx_ = 0;
        for (std::size_t i = 0; i < avail.size(); ++i) {
            if (avail[i] == langCode) {
                languageIdx_ = static_cast<int>(i);
                break;
            }
        }
    }
}

void InterfaceTab::applyFpsPosition() {
    prefs_->setInt(ConfigKey::kFpsPosition, fpsPosition_);
}
void InterfaceTab::applyFpsFormat() {
    prefs_->setInt(ConfigKey::kFpsFormat, fpsFormat_);
}
void InterfaceTab::applyTheme() {
    setTheme(static_cast<ThemeId>(themeId_));
    prefs_->setInt(ConfigKey::kTheme, themeId_);
}
void InterfaceTab::applyLanguage() {
    const auto& avail = Lang::instance().available();
    if (languageIdx_ < 0 || languageIdx_ >= static_cast<int>(avail.size()))
        return;

    const std::string& code = avail[languageIdx_];
    Lang::instance().load(code);
    prefs_->set(ConfigKey::kLanguage, code);
}
void InterfaceTab::applyButtonStyle() {
    ButtonStyle bs;
    bs.cornerRadius = buttonCorner_;
    bs.outlineThickness = buttonOutline_;
    setButtonStyle(bs);
    prefs_->setDouble(ConfigKey::kButtonCorner, buttonCorner_);
    prefs_->setDouble(ConfigKey::kButtonOutline, buttonOutline_);
}
void InterfaceTab::applyAnimation() {
    Anim::setEnabled(animationEnabled_);
    static const float kSpeeds[] = {0.5f, 1.0f, 2.0f};
    Anim::setSpeed(kSpeeds[std::clamp(animationSpeedIndex_, 0, kAnimSpeedCount - 1)]);
    prefs_->setBool(ConfigKey::kAnimationEnabled, animationEnabled_);
    prefs_->setInt(ConfigKey::kAnimationSpeedIndex, animationSpeedIndex_);
}
void InterfaceTab::applyNotification() {
    NotificationSystem::instance().setEnabled(notificationEnabled_);
    NotificationSystem::instance().setPosition(
        static_cast<NotificationPos>(notificationPosition_));
    // 立即生效（bootstrap 只在启动时读一次配置）
    NotificationSystem::instance().setDefaultDuration(
        static_cast<float>(notificationDuration_) / 1000.f);
    prefs_->setBool(ConfigKey::kNotificationEnabled, notificationEnabled_);
    prefs_->setInt(ConfigKey::kNotificationPosition, notificationPosition_);
    prefs_->setInt(ConfigKey::kNotificationDuration, notificationDuration_);
}

void InterfaceTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_)
        row->refreshLabel();

    labelNotificationDuration_.setString(toSf(Str::T(Str::LabelNotificationDuration)));
    hintUiScale_.setString(toSf(Str::T(Str::HintUiScale)));

    // 主题按钮文字（英文/日文等需要刷新）
    if (rowTheme_) {
        for (int i = 0;
             i < kThemeCount && i < static_cast<int>(rowTheme_->buttons.size()); ++i) {
            rowTheme_->buttons[i]->setText(Str::T(themeName(static_cast<ThemeId>(i))));
        }
    }
}

void InterfaceTab::refreshSelection() {
    auto setToggle = [](ToggleRow* row, bool v) {
        if (!row)
            return;
        row->currentValue = v;
        row->onButton->setSelected(v);
        row->offButton->setSelected(!v);
    };
    setToggle(rowFps_, showFps_);
    setToggle(rowClock_, showClock_);
    setToggle(rowAnimation_, animationEnabled_);
    setToggle(rowNotification_, notificationEnabled_);

    if (rowFpsPos_)
        rowFpsPos_->setSelected(indexOfPos(fpsPosition_));
    if (rowFpsFormat_)
        rowFpsFormat_->setSelected(fpsFormat_);
    if (rowUiScale_)
        rowUiScale_->setSelected(indexOfUiScale(uiScale_));
    if (rowFontScale_)
        rowFontScale_->setSelected(indexOfUiScale(fontScale_));
    if (rowTheme_)
        rowTheme_->setSelected(themeId_);
    if (rowLanguage_)
        rowLanguage_->setSelected(languageIdx_);
    if (rowClockPos_)
        rowClockPos_->setSelected(clockPosition_);
    if (rowButtonCorner_)
        rowButtonCorner_->setSelected(indexOfButtonCorner(buttonCorner_));
    if (rowButtonOutline_)
        rowButtonOutline_->setSelected(indexOfButtonOutline(buttonOutline_));
    if (rowAnimationSpeed_)
        rowAnimationSpeed_->setSelected(
            std::clamp(animationSpeedIndex_, 0, kAnimSpeedCount - 1));
    if (rowNotificationPos_)
        rowNotificationPos_->setSelected(notificationPosition_);
}

void InterfaceTab::handleEvent(const sf::Event& ev) {
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            btn->handleEvent(ev);
    notificationDurationSlider_->handleEvent(ev);
}

void InterfaceTab::update() {
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
    if (notificationDurationSlider_->consumeChanged()) {
        notificationDuration_ = static_cast<int>(notificationDurationSlider_->value());
        applyNotification();
    }
}

void InterfaceTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            out.push_back(btn.get());
}

bool InterfaceTab::anyEditing() const {
    if (notificationDurationSlider_ && notificationDurationSlider_->isEditing())
        return true;
    return false;
}

float InterfaceTab::render(sf::RenderTarget& target, float contentX, float ctrlX,
                           float startY) {
    float y = startY;

    auto drawToggle = [&](ToggleRow* row) {
        if (!row)
            return;
        row->label.setPosition({contentX, y + 8.f});
        target.draw(row->label);
        row->onButton->setPosition({ctrlX, y});
        row->offButton->setPosition({ctrlX + 96.f, y});
        row->onButton->render(target);
        row->offButton->render(target);
        y += kRowH;
    };
    auto drawMulti = [&](MultiRow* row) {
        if (!row)
            return;
        row->label.setPosition({contentX, y + 8.f});
        target.draw(row->label);
        for (size_t i = 0; i < row->buttons.size(); ++i) {
            row->buttons[i]->setPosition({ctrlX + static_cast<float>(i) * row->stepX, y});
            row->buttons[i]->render(target);
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

    // FPS
    drawToggle(rowFps_);
    drawMulti(rowFpsPos_);
    drawMulti(rowFpsFormat_);
    // UI 缩放
    drawMulti(rowUiScale_);
    drawMulti(rowFontScale_);
    hintUiScale_.setPosition({contentX, y - 26.f});
    target.draw(hintUiScale_);
    // 主题 / 语言
    drawMulti(rowTheme_);
    drawMulti(rowLanguage_);
    // 时钟
    drawToggle(rowClock_);
    drawMulti(rowClockPos_);
    // 按钮样式
    drawMulti(rowButtonCorner_);
    drawMulti(rowButtonOutline_);
    // 动画
    drawToggle(rowAnimation_);
    drawMulti(rowAnimationSpeed_);
    // 通知
    drawToggle(rowNotification_);
    drawMulti(rowNotificationPos_);
    drawSlider(labelNotificationDuration_, *notificationDurationSlider_);

    return y;
}

void InterfaceTab::reapply() {
    loadFromPrefs();
    // 这几个改的是全局单例（主题表 / 语言 / 按钮样式 / 动画 / 通知），
    // 光重读配置不改它们，界面上是看不出变化的
    setUiScale(uiScale_);
    setFontScale(fontScale_);
    applyTheme();
    applyLanguage();
    applyButtonStyle();
    applyAnimation();
    applyNotification();
    refreshSelection();
}
