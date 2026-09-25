#include "tabs/interface_tab.h"
#include "text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utf8.h"
#include "lang.h"
#include "animation.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr float kRowH = 50.f;

const char* kPosLabels[] = {"左上", "右上", "左下", "右下"};
constexpr int kPosCount  = 4;

const char* kFpsFormatLabels[] = {"纯数字", "60 FPS", "60.0 FPS"};
constexpr int kFpsFormatCount  = 3;

const float kUiScales[] = {0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f};
constexpr int kUiScaleCount = 10;
const char* kUiScaleLabels[] = {"0.5x", "0.6x", "0.7x", "0.8x", "0.9x",
                                "1.0x", "1.1x", "1.25x", "1.5x", "2.0x"};

const float kRenderScales[] = {2.0f, 1.5f, 1.25f, 1.0f, 0.75f, 0.5f, 1.0f/3.0f, 0.25f, 0.10f};
constexpr int kRenderScaleCount = 9;

const int kConsoleFonts[]  = {14, 18, 22, 26};
constexpr int kConsoleFontCount = 4;
const char* kConsoleFontLabels[] = {"小", "中", "大", "特大"};

const int kConsoleHistory[] = {50, 100, 200, 500};
constexpr int kConsoleHistoryCount = 4;

const int kConsoleLineHeights[] = {20, 26, 32};
constexpr int kConsoleLineHeightCount = 3;
const char* kConsoleLineHeightLabels[] = {"紧凑", "正常", "宽松"};

const char* kConsolePromptLabels[] = {">", "$", "λ", "❯"};
constexpr int kConsolePromptCount  = 4;

int indexOfUiScale(float s) {
    for (int i = 0; i < kUiScaleCount; ++i)
        if (std::abs(kUiScales[i] - s) < 0.01f) return i;
    return 5;
}
int indexOfConsoleFont(int f) {
    for (int i = 0; i < kConsoleFontCount; ++i)
        if (kConsoleFonts[i] == f) return i;
    return 1;
}
int indexOfConsoleHistory(int n) {
    for (int i = 0; i < kConsoleHistoryCount; ++i)
        if (kConsoleHistory[i] == n) return i;
    return 2;
}
int indexOfConsoleLineHeight(int h) {
    for (int i = 0; i < kConsoleLineHeightCount; ++i)
        if (kConsoleLineHeights[i] == h) return i;
    return 1;
}
int indexOfPos(int idx) {
    return (idx < 0 || idx >= kPosCount) ? 1 : idx;
}
int indexOfFpsFormat(int idx) {
    return (idx < 0 || idx >= kFpsFormatCount) ? 1 : idx;
}
int indexOfConsolePrompt(int idx) {
    return (idx < 0 || idx >= kConsolePromptCount) ? 0 : idx;
}
int indexOfRenderScale(float s) {
    for (int i = 0; i < kRenderScaleCount; ++i)
        if (std::abs(kRenderScales[i] - s) < 0.01f) return i;
    return 3;
}

} // namespace

InterfaceTab::InterfaceTab(const sf::Font& font,
                           std::shared_ptr<Preferences> prefs,
                           std::shared_ptr<Window> window,
                           std::shared_ptr<Background> background)
      : font_(font),
        prefs_(std::move(prefs)),
        window_(std::move(window)),
        background_(std::move(background)),
        labelWallpaper_      (font, sf::String(), scaledFontSize(20)),
        labelConsoleMask_    (font, sf::String(), scaledFontSize(20)),
        labelConsolePanelAlpha_(font, sf::String(), scaledFontSize(20)),
        hintUiScale_         (font, sf::String(), scaledFontSize(14)) {

    loadFromPrefs();

    auto labelColor = sf::Color(230, 230, 230);
    for (auto* t : {&labelWallpaper_, &labelConsoleMask_,
                    &labelConsolePanelAlpha_}) {
        t->setFillColor(labelColor);
    }
    hintUiScale_.setFillColor(sf::Color(180, 180, 200));

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

    // [0] FpsPos
    {
        auto* row = addMulti(Str::LabelFpsPos, [this](int i) {
            fpsPosition_ = i; refreshSelection(); applyFpsPosition();
        });
        row->stepX = 86.f;
        for (int i = 0; i < kPosCount; ++i)
            row->addButton(std::make_unique<Button>(
                kPosLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));
    }
    // [1] FpsFormat
    {
        auto* row = addMulti(Str::LabelFpsFormat, [this](int i) {
            fpsFormat_ = i; refreshSelection(); applyFpsFormat();
        });
        row->stepX = 114.f;
        for (int i = 0; i < kFpsFormatCount; ++i)
            row->addButton(std::make_unique<Button>(
                kFpsFormatLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{110.f, 40.f}, 16));
    }
    // [2] UiScale
    {
        auto* row = addMulti(Str::LabelUiScale, [this](int i) {
            uiScale_ = kUiScales[i];
            refreshSelection();
            setUiScale(uiScale_);
            prefs_->setDouble("ui_scale", uiScale_);
        });
        for (int i = 0; i < kUiScaleCount; ++i)
            row->addButton(std::make_unique<Button>(
                kUiScaleLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    }
    // [3] FontScale
    {
        auto* row = addMulti(Str::LabelFontScale, [this](int i) {
            fontScale_ = kUiScales[i];
            refreshSelection();
            setFontScale(fontScale_);
            prefs_->setDouble("font_scale", fontScale_);
        });
        for (int i = 0; i < kUiScaleCount; ++i)
            row->addButton(std::make_unique<Button>(
                kUiScaleLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    }
    // [4] RenderScale
    {
        auto* row = addMulti(Str::LabelRenderScale, [this](int i) {
            renderScale_ = kRenderScales[i];
            refreshSelection();
            window_->setRenderScale(renderScale_);
            prefs_->setDouble("render_scale", renderScale_);
        });
        row->stepX = 96.f;
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale200),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale150),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale125),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale100),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale75),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale50),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale33),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale25),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::RenderScale10),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
    }
    // [5] UpscaleMode
    {
        auto* row = addMulti(Str::LabelUpscaleMode, [this](int i) {
            upscaleMode_ = i;
            refreshSelection();
            window_->setUpscaleMode(upscaleMode_);
            prefs_->setInt("upscale_mode", upscaleMode_);
        });
        row->stepX = 100.f;
        row->addButton(std::make_unique<Button>(Str::T(Str::UpscaleOff),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::UpscaleBicubic),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::UpscaleFsr1),
            font_, sf::Vector2f{0.f,0.f}, sf::Vector2f{86.f,40.f}, 18));
    }
    // [6] Theme
    {
        auto* row = addMulti(Str::LabelTheme, [this](int i) {
            themeId_ = i;
            refreshSelection(); applyTheme();
        });
        row->stepX = 110.f;
        for (int i = 0; i < kThemeCount; ++i)
            row->addButton(std::make_unique<Button>(
                themeName(static_cast<ThemeId>(i)), font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{100.f, 40.f}, 18));
    }
    // [7] Language
    {
        auto* row = addMulti(Str::LabelLanguage, [this](int i) {
            languageIdx_ = i; refreshSelection(); applyLanguage();
        });
        row->stepX = 110.f;
        for (const auto& code : Lang::instance().available()) {
            std::string label = code;
            if (code == "zh")         label = "中文";
            else if (code == "zh-TW") label = "繁體中文";
            else if (code == "en")    label = "English";
            else if (code == "ja")    label = "日本語";
            else if (code == "ko")    label = "한국어";
            row->addButton(std::make_unique<Button>(
                label, font_, sf::Vector2f{0.f, 0.f},
                sf::Vector2f{100.f, 40.f}, 18));
        }
    }
    // [8] ClockPos
    {
        auto* row = addMulti(Str::LabelClockPos, [this](int i) {
            clockPosition_ = i; refreshSelection();
            prefs_->setInt("clock_position", clockPosition_);
        });
        row->stepX = 86.f;
        for (int i = 0; i < kPosCount; ++i)
            row->addButton(std::make_unique<Button>(
                kPosLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{76.f, 40.f}, 16));
    }
    // [9] ConsoleFont
    {
        auto* row = addMulti(Str::LabelConsoleFont, [this](int i) {
            consoleFontSize_ = kConsoleFonts[i]; refreshSelection();
            prefs_->setInt("console_font_size", consoleFontSize_);
        });
        for (int i = 0; i < kConsoleFontCount; ++i)
            row->addButton(std::make_unique<Button>(
                kConsoleFontLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    }
    // [10] ConsoleHistory
    {
        auto* row = addMulti(Str::LabelConsoleHistory, [this](int i) {
            consoleHistoryLines_ = kConsoleHistory[i]; refreshSelection();
            prefs_->setInt("console_history_lines", consoleHistoryLines_);
        });
        for (int i = 0; i < kConsoleHistoryCount; ++i)
            row->addButton(std::make_unique<Button>(
                std::to_string(kConsoleHistory[i]), font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    }
    // [11] ConsoleLineHeight
    {
        auto* row = addMulti(Str::LabelConsoleLineHeight, [this](int i) {
            consoleLineHeight_ = kConsoleLineHeights[i]; refreshSelection();
            prefs_->setInt("console_line_height", consoleLineHeight_);
        });
        for (int i = 0; i < kConsoleLineHeightCount; ++i)
            row->addButton(std::make_unique<Button>(
                kConsoleLineHeightLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18));
    }
    // [12] ConsolePrompt
    {
        auto* row = addMulti(Str::LabelConsolePrompt, [this](int i) {
            consolePrompt_ = i; refreshSelection(); applyConsolePrompt();
        });
        row->stepX = 70.f;
        for (int i = 0; i < kConsolePromptCount; ++i)
            row->addButton(std::make_unique<Button>(
                kConsolePromptLabels[i], font_,
                sf::Vector2f{0.f, 0.f}, sf::Vector2f{60.f, 40.f}, 18));
    }

    // Toggles
    addToggle(Str::LabelFps, [this](bool v) {
        showFps_ = v; refreshSelection();
        prefs_->setBool("show_fps", v);
    });
    addToggle(Str::LabelClock, [this](bool v) {
        showClock_ = v; refreshSelection();
        prefs_->setBool("show_clock", v);
    });
    addToggle(Str::LabelConsoleAutoScroll, [this](bool v) {
        consoleAutoScroll_ = v; refreshSelection();
        prefs_->setBool("console_auto_scroll", v);
    });
    addToggle(Str::LabelConsoleBlink, [this](bool v) {
        consoleBlinkCursor_ = v; refreshSelection();
        prefs_->setBool("console_blink_cursor", v);
    });

    // Wallpaper button
    wallpaperButton_ = std::make_unique<Button>(
        Str::NextWallpaper, font_,
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{150.f, 40.f}, 18);

    // Console sliders
    consoleMaskSlider_ = std::make_unique<Slider>(
        font_, 0.f, 255.f, static_cast<float>(consoleMask_),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    consoleMaskSlider_->setDefaultValue(160.f);

    consolePanelAlphaSlider_ = std::make_unique<Slider>(
        font_, 0.f, 255.f, static_cast<float>(consolePanelAlpha_),
        sf::Vector2f{0.f, 0.f}, sf::Vector2f{240.f, 22.f});
    consolePanelAlphaSlider_->setDefaultValue(220.f);

    refreshLabels();
    refreshSelection();
}

void InterfaceTab::loadFromPrefs() {
    showFps_             = prefs_->getBool("show_fps", false);
    fpsPosition_         = prefs_->getInt("fps_position", 1);
    fpsFormat_           = indexOfFpsFormat(prefs_->getInt("fps_format", 1));
    uiScale_             = static_cast<float>(prefs_->getDouble("ui_scale", 1.0));
    fontScale_           = static_cast<float>(prefs_->getDouble("font_scale", 1.0));
    renderScale_         = static_cast<float>(prefs_->getDouble("render_scale", 1.0));
    upscaleMode_         = prefs_->getInt("upscale_mode", 1);
    themeId_             = prefs_->getInt("theme", 0);
    showClock_           = prefs_->getBool("show_clock", false);
    clockPosition_       = prefs_->getInt("clock_position", 0);
    consoleMask_         = std::clamp(prefs_->getInt("console_mask", 160), 0, 255);
    consolePanelAlpha_   = std::clamp(prefs_->getInt("console_panel_alpha", 220), 0, 255);
    consoleFontSize_     = prefs_->getInt("console_font_size", 18);
    consoleHistoryLines_ = prefs_->getInt("console_history_lines", 200);
    consoleLineHeight_   = prefs_->getInt("console_line_height", 26);
    consoleAutoScroll_   = prefs_->getBool("console_auto_scroll", true);
    consoleBlinkCursor_  = prefs_->getBool("console_blink_cursor", true);
    consolePrompt_       = indexOfConsolePrompt(prefs_->getInt("console_prompt", 0));

    {
        std::string langCode = prefs_->get("language", "zh");
        const auto& avail = Lang::instance().available();
        languageIdx_ = 0;
        for (std::size_t i = 0; i < avail.size(); ++i) {
            if (avail[i] == langCode) { languageIdx_ = static_cast<int>(i); break; }
        }
    }
}

void InterfaceTab::applyFpsPosition() {
    prefs_->setInt("fps_position", fpsPosition_);
}
void InterfaceTab::applyFpsFormat() {
    prefs_->setInt("fps_format", fpsFormat_);
}
void InterfaceTab::applyTheme() {
    setTheme(static_cast<ThemeId>(themeId_));
    prefs_->setInt("theme", themeId_);
}
void InterfaceTab::applyLanguage() {
    const auto& avail = Lang::instance().available();
    if (languageIdx_ < 0 || languageIdx_ >= static_cast<int>(avail.size())) return;

    const std::string& code = avail[languageIdx_];
    Lang::instance().load(code);
    prefs_->set("language", code);
}
void InterfaceTab::applyWallpaper() {
    if (!background_) return;
    if (background_->next()) {
        prefs_->set("current_wallpaper", background_->currentFile());
    }
}
void InterfaceTab::applyConsolePrompt() {
    prefs_->setInt("console_prompt", consolePrompt_);
}

void InterfaceTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_) row->refreshLabel();
    if (wallpaperButton_) wallpaperButton_->setText(Str::T(Str::NextWallpaper));

    labelConsoleMask_      .setString(toSf(Str::T(Str::LabelConsoleMask)));
    labelConsolePanelAlpha_.setString(toSf(Str::T(Str::LabelConsolePanelAlpha)));
    hintUiScale_           .setString(toSf(Str::T(Str::HintUiScale)));

    // 主题按钮文字（英文/日文等需要刷新）
    if (multiRows_.size() > 6) {
        auto& themeRow = *multiRows_[6];
        for (int i = 0; i < kThemeCount && i < static_cast<int>(themeRow.buttons.size()); ++i) {
            themeRow.buttons[i]->setText(Str::T(themeName(static_cast<ThemeId>(i))));
        }
    }
    // 壁纸标签显示当前进度
    if (background_) {
        labelWallpaper_.setString(toSf(
            std::string(Str::T(Str::LabelWallpaper)) + "  ("
            + std::to_string(background_->currentIndex() + 1) + "/"
            + std::to_string(background_->totalWallpapers()) + ")"));
    } else {
        labelWallpaper_.setString(toSf(Str::T(Str::LabelWallpaper)));
    }
}

void InterfaceTab::refreshSelection() {
    if (toggles_.size() == 4) {
        auto setRow = [](ToggleRow& row, bool v) {
            row.currentValue = v;
            row.onButton->setSelected(v);
            row.offButton->setSelected(!v);
        };
        setRow(*toggles_[0], showFps_);
        setRow(*toggles_[1], showClock_);
        setRow(*toggles_[2], consoleAutoScroll_);
        setRow(*toggles_[3], consoleBlinkCursor_);
    }
    if (multiRows_.size() == 13) {
        multiRows_[0]->setSelected(indexOfPos(fpsPosition_));
        multiRows_[1]->setSelected(fpsFormat_);
        multiRows_[2]->setSelected(indexOfUiScale(uiScale_));
        multiRows_[3]->setSelected(indexOfUiScale(fontScale_));
        multiRows_[4]->setSelected(indexOfRenderScale(renderScale_));
        multiRows_[5]->setSelected(upscaleMode_);
        multiRows_[6]->setSelected(themeId_);
        multiRows_[7]->setSelected(languageIdx_);
        multiRows_[8]->setSelected(clockPosition_);
        multiRows_[9]->setSelected(indexOfConsoleFont(consoleFontSize_));
        multiRows_[10]->setSelected(indexOfConsoleHistory(consoleHistoryLines_));
        multiRows_[11]->setSelected(indexOfConsoleLineHeight(consoleLineHeight_));
        multiRows_[12]->setSelected(consolePrompt_);
    }
}

void InterfaceTab::handleEvent(const sf::Event& ev) {
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons) btn->handleEvent(ev);
    wallpaperButton_->handleEvent(ev);
    consoleMaskSlider_->handleEvent(ev);
    consolePanelAlphaSlider_->handleEvent(ev);
}

void InterfaceTab::update() {
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
    if (wallpaperButton_->consumeClick()) {
        applyWallpaper();
        refreshLabels();
        return;
    }
    if (consoleMaskSlider_->consumeChanged()) {
        consoleMask_ = static_cast<int>(consoleMaskSlider_->value());
        prefs_->setInt("console_mask", consoleMask_);
    }
    if (consolePanelAlphaSlider_->consumeChanged()) {
        consolePanelAlpha_ = static_cast<int>(consolePanelAlphaSlider_->value());
        prefs_->setInt("console_panel_alpha", consolePanelAlpha_);
    }
}

void InterfaceTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons) out.push_back(btn.get());
    out.push_back(wallpaperButton_.get());
}

bool InterfaceTab::anyEditing() const {
    if (consoleMaskSlider_       && consoleMaskSlider_->isEditing())       return true;
    if (consolePanelAlphaSlider_ && consolePanelAlphaSlider_->isEditing()) return true;
    return false;
}

float InterfaceTab::render(sf::RenderTarget& target,
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
    auto drawSlider = [&](sf::Text& label, Slider& slider) {
        label.setPosition({contentX, y + 4.f});
        target.draw(label);
        slider.setPosition({ctrlX, y + 4.f});
        slider.render(target);
        y += kRowH;
    };

    // FPS
    drawToggle(*toggles_[0]);
    drawMulti(*multiRows_[0]);   // FpsPos
    drawMulti(*multiRows_[1]);   // FpsFormat
    // UI 缩放
    drawMulti(*multiRows_[2]);   // UiScale
    drawMulti(*multiRows_[3]);   // FontScale
    hintUiScale_.setPosition({contentX, y - 26.f});
    target.draw(hintUiScale_);
    // 渲染缩放
    drawMulti(*multiRows_[4]);   // RenderScale
    drawMulti(*multiRows_[5]);   // UpscaleMode
    // 主题 / 语言
    drawMulti(*multiRows_[6]);   // Theme
    drawMulti(*multiRows_[7]);   // Language
    // 壁纸
    {
        labelWallpaper_.setPosition({contentX, y + 8.f});
        target.draw(labelWallpaper_);
        wallpaperButton_->setPosition({ctrlX, y});
        wallpaperButton_->render(target);
        y += kRowH;
    }
    // 时钟
    drawToggle(*toggles_[1]);
    drawMulti(*multiRows_[8]);   // ClockPos
    // 控制台
    drawSlider(labelConsoleMask_,       *consoleMaskSlider_);
    drawSlider(labelConsolePanelAlpha_, *consolePanelAlphaSlider_);
    drawMulti(*multiRows_[9]);   // ConsoleFont
    drawMulti(*multiRows_[10]);  // ConsoleHistory
    drawMulti(*multiRows_[11]);  // ConsoleLineHeight
    drawToggle(*toggles_[2]);    // AutoScroll
    drawToggle(*toggles_[3]);    // Blink
    drawMulti(*multiRows_[12]);  // ConsolePrompt

    return y;
}