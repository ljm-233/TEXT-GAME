#include "tabs/display_tab.h"
#include "utils/text_strings.h"
#include "theme.h"
#include "ui_scale.h"
#include "utils/utf8.h"
#include "ui/resolution.h"
#include "config/keys.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr float kRowH = 50.f;

const int kaaLevels[] = {0, 4, 8, 16};
constexpr int kaaCount = 4;
const char* kAALabels[] = {"关", "4x", "8x", "16x"};

const int kfpsLimits[] = {0, 30, 60, 120, 144};
constexpr int kfpsLimitCount = 5;
const char* kFpsLimitLabels[] = {"无", "30", "60", "120", "144"};

int indexOfAA(int level) {
    for (int i = 0; i < kaaCount; ++i)
        if (kaaLevels[i] == level)
            return i;
    return 2;
}
int indexOfFpsLimit(int l) {
    for (int i = 0; i < kfpsLimitCount; ++i)
        if (kfpsLimits[i] == l)
            return i;
    return 2;
}

} // namespace

DisplayTab::DisplayTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                       std::shared_ptr<RuntimeConfig> runtime,
                       std::shared_ptr<Window> window)
      : font_(font),
        prefs_(std::move(prefs)),
        runtime_(std::move(runtime)),
        window_(std::move(window)) {
    loadFromPrefs();

    auto makeToggle = [&](const std::string& onText, const std::string& offText) {
        auto on = std::make_unique<Button>(onText, font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(offText, font_, sf::Vector2f{0.f, 0.f},
                                            sf::Vector2f{86.f, 40.f}, 18);
        return std::make_pair(std::move(on), std::move(off));
    };
    auto addToggle = [&](const char* key, std::function<void(bool)> cb) {
        auto row = std::make_unique<ToggleRow>(font_, key, std::move(cb));
        auto [on, off] = makeToggle(Str::On, Str::Off);
        row->onButton = std::move(on);
        row->offButton = std::move(off);
        toggles_.push_back(std::move(row));
    };
    auto addMulti = [&](const char* key, std::function<void(int)> cb) {
        auto row = std::make_unique<MultiRow>(font_, key, std::move(cb));
        multiRows_.push_back(std::move(row));
        return multiRows_.back().get();
    };

    // [0] Resolution (2x2 grid)
    {
        auto* row = addMulti(Str::LabelResolution, [this](int i) {
            selectedResolution_ = i;
            refreshSelection();
            applyResolution();
        });
        row->columns = 2;
        row->stepX = 280.f + 16.f;
        row->stepY = 46.f;
        for (int i = 0; i < kResolutionCount; ++i) {
            row->addButton(std::make_unique<Button>(kResolutions[i].label, font_,
                                                    sf::Vector2f{0.f, 0.f},
                                                    sf::Vector2f{280.f, 42.f}, 16));
        }
    }
    // [1] WindowMode
    {
        auto* row = addMulti(Str::LabelWindowMode, [this](int i) {
            windowMode_ = i;
            prefs_->setInt(ConfigKey::kWindowMode, i);
            applyWindowMode();
        });
        row->stepX = 110.f;
        row->addButton(std::make_unique<Button>(Str::T(Str::WinModeWindowed), font_,
                                                sf::Vector2f{0.f, 0.f},
                                                sf::Vector2f{100.f, 40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::WinModeMaximized), font_,
                                                sf::Vector2f{0.f, 0.f},
                                                sf::Vector2f{100.f, 40.f}, 18));
        row->addButton(std::make_unique<Button>(Str::T(Str::WinModeFullscreen), font_,
                                                sf::Vector2f{0.f, 0.f},
                                                sf::Vector2f{100.f, 40.f}, 18));
    }
    // [2] AntiAliasing
    {
        auto* row = addMulti(Str::LabelAntiAliasing, [this](int i) {
            antiAliasingLevel_ = kaaLevels[i];
            refreshSelection();
            applyAntiAliasing();
        });
        for (int i = 0; i < kaaCount; ++i)
            row->addButton(std::make_unique<Button>(kAALabels[i], font_,
                                                    sf::Vector2f{0.f, 0.f},
                                                    sf::Vector2f{86.f, 40.f}, 18));
    }
    // [3] FpsLimit
    {
        auto* row = addMulti(Str::LabelFpsLimit, [this](int i) {
            fpsLimit_ = kfpsLimits[i];
            refreshSelection();
            applyFpsLimit();
        });
        row->stepX = 86.f;
        for (int i = 0; i < kfpsLimitCount; ++i)
            row->addButton(std::make_unique<Button>(kFpsLimitLabels[i], font_,
                                                    sf::Vector2f{0.f, 0.f},
                                                    sf::Vector2f{76.f, 40.f}, 16));
    }

    // Toggles
    addToggle(Str::LabelFullscreen, [this](bool v) {
        fullscreen_ = v;
        refreshSelection();
        applyFullscreen();
    });
    addToggle(Str::LabelVsync, [this](bool v) {
        vsync_ = v;
        refreshSelection();
        applyVsync();
    });
    // 窗口行为跟"显示"是一类，0.3.8 从「其他」页挪过来
    addToggle(Str::LabelRememberSize, [this](bool v) {
        rememberSize_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kRememberWindowSize, v);
    });
    addToggle(Str::LabelAutoPause, [this](bool v) {
        autoPauseOnBlur_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kAutoPauseOnBlur, v);
    });

    refreshSelection();
}

void DisplayTab::loadFromPrefs() {
    selectedResolution_ =
        clampResolutionIndex(prefs_->getInt(ConfigKey::kResolutionIndex, 0));
    fullscreen_ = prefs_->getBool(ConfigKey::kFullscreen, false);
    windowMode_ = prefs_->getInt(ConfigKey::kWindowMode, 0);
    vsync_ = prefs_->getBool(ConfigKey::kVsync, true);
    antiAliasingLevel_ = prefs_->getInt(ConfigKey::kAntiAliasing, 8);
    rememberSize_ = prefs_->getBool(ConfigKey::kRememberWindowSize, true);
    autoPauseOnBlur_ = prefs_->getBool(ConfigKey::kAutoPauseOnBlur, true);
    fpsLimit_ = prefs_->getInt(ConfigKey::kFpsLimit, 60);
}

void DisplayTab::applyResolution() {
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    prefs_->setInt(ConfigKey::kResolutionIndex, selectedResolution_);
    runtime_->setInt("last_window_width", static_cast<int>(res.width));
    runtime_->setInt("last_window_height", static_cast<int>(res.height));
}

void DisplayTab::applyFullscreen() {
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    prefs_->setBool(ConfigKey::kFullscreen, fullscreen_);
}

void DisplayTab::applyWindowMode() {
    if (windowMode_ == 1 && !fullscreen_) {
        window_->requestMaximize();
    }
}

void DisplayTab::applyVsync() {
    window_->setVsync(vsync_);
    prefs_->setBool(ConfigKey::kVsync, vsync_);
}

void DisplayTab::applyAntiAliasing() {
    window_->setAntiAliasing(static_cast<unsigned>(antiAliasingLevel_));
    const auto& res = kResolutions[selectedResolution_];
    window_->recreate(res.width, res.height, fullscreen_);
    prefs_->setInt(ConfigKey::kAntiAliasing, antiAliasingLevel_);
}

void DisplayTab::applyFpsLimit() {
    window_->setFramerateLimit(static_cast<unsigned>(fpsLimit_));
    prefs_->setInt(ConfigKey::kFpsLimit, fpsLimit_);
}

void DisplayTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_)
        row->refreshLabel();

    // WindowMode 按钮文字
    if (multiRows_.size() > 1) {
        auto& wm = *multiRows_[1];
        if (wm.buttons.size() >= 3) {
            wm.buttons[0]->setText(Str::T(Str::WinModeWindowed));
            wm.buttons[1]->setText(Str::T(Str::WinModeMaximized));
            wm.buttons[2]->setText(Str::T(Str::WinModeFullscreen));
        }
    }
}

void DisplayTab::refreshSelection() {
    if (toggles_.size() == 4) {
        auto setRow = [](ToggleRow& row, bool v) {
            row.currentValue = v;
            row.onButton->setSelected(v);
            row.offButton->setSelected(!v);
        };
        setRow(*toggles_[0], fullscreen_);
        setRow(*toggles_[1], vsync_);
        setRow(*toggles_[2], rememberSize_);
        setRow(*toggles_[3], autoPauseOnBlur_);
    }
    // 日志级别已挪去「高级」页，这里少了一行，下标整体前移一位
    if (multiRows_.size() == 4) {
        multiRows_[0]->setSelected(selectedResolution_);
        multiRows_[1]->setSelected(windowMode_);
        multiRows_[2]->setSelected(indexOfAA(antiAliasingLevel_));
        multiRows_[3]->setSelected(indexOfFpsLimit(fpsLimit_));
    }
}

void DisplayTab::handleEvent(const sf::Event& ev) {
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            btn->handleEvent(ev);
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
}

void DisplayTab::update() {
    for (auto& row : multiRows_) {
        for (size_t i = 0; i < row->buttons.size(); ++i) {
            if (row->buttons[i]->consumeClick()) {
                if (row->currentIndex != static_cast<int>(i) && row->onSelected)
                    row->onSelected(static_cast<int>(i));
                return;
            }
        }
    }
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
}

void DisplayTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            out.push_back(btn.get());
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
}

float DisplayTab::render(sf::RenderTarget& target, float contentX, float ctrlX,
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

        if (row.columns > 0) {
            int cols = row.columns;
            for (size_t i = 0; i < row.buttons.size(); ++i) {
                int r = static_cast<int>(i) / cols;
                int c = static_cast<int>(i) % cols;
                row.buttons[i]->setPosition({ctrlX + static_cast<float>(c) * row.stepX,
                                             y + static_cast<float>(r) * row.stepY});
                row.buttons[i]->render(target);
            }
            int rows = (static_cast<int>(row.buttons.size()) + cols - 1) / cols;
            y += static_cast<float>(rows) * row.stepY + 6.f;
        } else {
            for (size_t i = 0; i < row.buttons.size(); ++i) {
                row.buttons[i]->setPosition(
                    {ctrlX + static_cast<float>(i) * row.stepX, y});
                row.buttons[i]->render(target);
            }
            y += kRowH;
        }
    };

    drawMulti(*multiRows_[0]); // Resolution
    drawMulti(*multiRows_[1]); // WindowMode
    drawToggle(*toggles_[0]);  // Fullscreen
    drawToggle(*toggles_[1]);  // VSync
    drawMulti(*multiRows_[2]); // AntiAliasing
    drawMulti(*multiRows_[3]); // FpsLimit
    drawToggle(*toggles_[2]);  // RememberSize
    drawToggle(*toggles_[3]);  // AutoPause

    return y;
}

void DisplayTab::reapply() {
    loadFromPrefs();
    applyResolution();
    applyFullscreen();
    applyWindowMode();
    applyVsync();
    applyAntiAliasing();
    applyFpsLimit();
    refreshSelection();
}
