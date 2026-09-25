#include "settings_scene.h"
#include "text_strings.h"
#include <string>
#include "utf8.h"
#include "ui_scale.h"
#include "button_style.h"
#include "animation.h"
#include "notification.h"
#include "sound_manager.h"
#include "focus_group.h"
#include "keybindings.h"
#include "gamepad.h"
#include "tabs/audio_tab.h"
#include "tabs/graphics_tab.h"
#include "tabs/interface_tab.h"
#include "tabs/display_tab.h"
#include "tabs/other_tab.h"
#include "tabs/keys_tab.h"
#include <algorithm>
#include <cmath>
#include "lang.h"

namespace {

// ===== 布局（设计坐标系 1280×720）=====
constexpr float kTabX     = 40.f;
constexpr float kTabY     = 90.f;
constexpr float kTabGap   = 62.f;
constexpr float kContentX = kTabX + 200.f;
constexpr float kCtrlX    = kContentX + 240.f;
constexpr float kBtnW     = 280.f;
constexpr float kBtnH     = 46.f;
constexpr float kGapX     = 16.f;
constexpr float kGapY     = 10.f;

// ===== 画面预设 =====
struct PostPreset {
    float saturation, contrast, brightness, gamma, vignette;
    float bloomStrength, bloomThreshold;
    float chromatic, grain, scanline, dither;
};

// ⭐ 初始生命值（1/3/5/10/100）↔ 索引
} // namespace

// ============================================================
// 构造
// ============================================================

SettingsScene::SettingsScene(std::shared_ptr<Background>    background,
                             std::shared_ptr<Preferences>   preferences,
                             std::shared_ptr<RuntimeConfig> runtimeConfig,
                             std::shared_ptr<Window>        window,
                             const sf::Font&                font,
                             std::shared_ptr<Logger>        logger)
    : background_(std::move(background)),
      preferences_(std::move(preferences)),
      runtimeConfig_(std::move(runtimeConfig)),
      window_(std::move(window)),
      logger_(std::move(logger)),
      font_(font),
      headingDisplay_  (font, toSf(Str::TabDisplay),   fontSizeInView(24)),
      headingInterface_(font, toSf(Str::TabInterface), fontSizeInView(24)),
      headingGraphics_ (font, toSf(Str::TabGraphics),  fontSizeInView(24)),
      headingAudio_    (font, toSf(Str::TabAudioLog),  fontSizeInView(24)),
      headingKeys_     (font, toSf(Str::TabKeys),      fontSizeInView(24)),
      headingOther_    (font, toSf(Str::TabOther),     fontSizeInView(24)){

    // 标题颜色
    auto headingColor = sf::Color(160, 200, 240);
    headingDisplay_.setFillColor(headingColor);
    headingInterface_.setFillColor(headingColor);
    headingGraphics_.setFillColor(headingColor);
    headingAudio_.setFillColor(headingColor);
    headingKeys_.setFillColor(headingColor);
    headingOther_.setFillColor(headingColor);

    const char* tabLabels[] = {
        Str::TabDisplay, Str::TabInterface, Str::TabGraphics,
        Str::TabAudioLog, Str::TabKeys, Str::TabOther
    };
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_.push_back(std::make_unique<Button>(
            tabLabels[i], font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 50.f}, 22));
    }

    auto makeToggle = [&](const std::string& onText, const std::string& offText) {
        auto on  = std::make_unique<Button>(onText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        auto off = std::make_unique<Button>(offText, font_,
                       sf::Vector2f{0.f, 0.f}, sf::Vector2f{86.f, 40.f}, 18);
        return std::make_pair(std::move(on), std::move(off));
    };

    // ───────────── Display ─────────────
    // ⭐ 独立 Tab
    displayTab_ = std::make_unique<DisplayTab>(
        font_, preferences_, runtimeConfig_, window_, logger_);

    // ───────────── Interface ─────────────
    // ⭐ 独立 Tab
    interfaceTab_ = std::make_unique<InterfaceTab>(
        font_, preferences_, window_, background_);

    // ───────────── Graphics ─────────────
    // ⭐ 独立 Tab
    graphicsTab_ = std::make_unique<GraphicsTab>(font_, preferences_, window_);
    // ───────────── Audio ─────────────
    // ⭐ 独立 Tab
    audioTab_ = std::make_unique<AudioTab>(font_, preferences_, window_);

    // ───────────── Other ─────────────
    // ⭐ 独立 Tab
    otherTab_ = std::make_unique<OtherTab>(font_, preferences_, logger_);

    // ───────────── Keys ─────────────
    // ⭐ 独立 Tab
    keysTab_ = std::make_unique<KeysTab>(font_, preferences_);

    aboutButton_ = std::make_unique<Button>(Str::ButtonAbout, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 46.f}, 20);
    resetButton_ = std::make_unique<Button>(Str::ResetDefault, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{220.f, 46.f}, 20);
    backButton_ = std::make_unique<Button>(Str::Back, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 50.f}, 22);
    resetGraphicsButton_ = std::make_unique<Button>(Str::ResetGraphics, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 50.f}, 22);

    refreshSelection();
    updateDesignView();
}

// ============================================================
// 选中状态
// ============================================================

void SettingsScene::refreshLabels() {
    // 只在语言变化时刷新
    int v = Lang::instance().version();
    if (v == lastLangVersion_) return;
    lastLangVersion_ = v;

    auto setLabel = [](sf::Text& t, const char* key) {
        t.setString(toSf(Str::T(key)));
    };

    // ===== Heading =====
    setLabel(headingDisplay_,   Str::TabDisplay);
    setLabel(headingInterface_, Str::TabInterface);
    setLabel(headingGraphics_,  Str::TabGraphics);
    setLabel(headingAudio_,     Str::TabAudioLog);
    setLabel(headingKeys_,      Str::TabKeys);
    setLabel(headingOther_,     Str::TabOther);

    // ===== Tab 按钮 =====
    if (tabButtons_.size() >= 6) {
        tabButtons_[0]->setText(Str::T(Str::TabDisplay));
        tabButtons_[1]->setText(Str::T(Str::TabInterface));
        tabButtons_[2]->setText(Str::T(Str::TabGraphics));
        tabButtons_[3]->setText(Str::T(Str::TabAudioLog));
        tabButtons_[4]->setText(Str::T(Str::TabKeys));
        tabButtons_[5]->setText(Str::T(Str::TabOther));
    }

    // ===== Toggle / Multi 刷新 =====
    if (displayTab_) displayTab_->refreshLabels();
    if (graphicsTab_) graphicsTab_->refreshLabels();
    if (audioTab_) audioTab_->refreshLabels();
    if (presetRow_) {
        presetRow_->label.setString(toSf(Str::T(Str::LabelPreset)));
        if (presetRow_->buttons.size() >= 5) {
            presetRow_->buttons[0]->setText(Str::T(Str::PresetDefault));
            presetRow_->buttons[1]->setText(Str::T(Str::PresetCRT));
            presetRow_->buttons[2]->setText(Str::T(Str::PresetCinematic));
            presetRow_->buttons[3]->setText(Str::T(Str::PresetPixel8));
            presetRow_->buttons[4]->setText(Str::T(Str::PresetNight));
        }
    }
    if (otherTab_) otherTab_->refreshLabels();
    if (interfaceTab_) interfaceTab_->refreshLabels();
    if (keysTab_) keysTab_->refreshLabels();
    // ===== 其他按钮 =====
    if (aboutButton_)     aboutButton_->setText(Str::T(Str::ButtonAbout));
    if (resetButton_)     resetButton_->setText(Str::T(Str::ResetDefault));
    if (backButton_)      backButton_->setText(Str::T(Str::Back));
}

void SettingsScene::refreshSelection() {
    for (int i = 0; i < kTabCount; ++i)
        tabButtons_[i]->setSelected(i == static_cast<int>(currentTab_));

    auto setToggleRow = [](ToggleRow& row, bool v) {
        row.currentValue = v;
        row.onButton->setSelected(v);
        row.offButton->setSelected(!v);
    };

    // Display
    if (displayTab_) displayTab_->refreshSelection();

    // Interface
    if (interfaceTab_) interfaceTab_->refreshSelection();

    // Graphics
    if (graphicsTab_) graphicsTab_->refreshSelection();

    // Audio
    if (audioTab_) audioTab_->refreshSelection();

    // Other
    if (otherTab_) otherTab_->refreshSelection();
}

void SettingsScene::syncFocus() {
    if (resetConfirm_ || aboutDialog_) {
        FocusGroup::instance().clear();
        return;
    }

    std::vector<Button*> items;
    for (auto& b : tabButtons_) items.push_back(b.get());

    switch (currentTab_) {
        case Tab::Display:
            if (displayTab_) displayTab_->registerFocus(items);
            break;
        case Tab::Interface:
            if (interfaceTab_) interfaceTab_->registerFocus(items);
            break;
        case Tab::Graphics:
            if (graphicsTab_) graphicsTab_->registerFocus(items);
            break;
        case Tab::Audio:
            audioTab_->registerFocus(items);
            break;
        case Tab::Keys:
            if (keysTab_) keysTab_->registerFocus(items);
            break;
        case Tab::Other:
            if (otherTab_) otherTab_->registerFocus(items);
            break;
    }
    // ⭐ 返回按钮在窗口坐标系固定右下角，不参与设计坐标系几何导航
    //    用 ESC 或手柄 B 键返回
    FocusGroup::instance().setItems(items);
}

// ============================================================
// 应用状态
// ============================================================

void SettingsScene::resetAllPreferences() { preferences_->resetAll(); }

// ============================================================
// 事件
// ============================================================

void SettingsScene::onEnter() {
    nextScene_ = SceneId::None;
    syncFocus();
}

void SettingsScene::onResume() {
    nextScene_ = SceneId::None;
    syncFocus();
}

void SettingsScene::handleEvent(const sf::Event& event) {
    // ⭐ 鼠标事件坐标转换：屏幕像素 → 设计坐标
    updateDesignView();

    sf::Event ev = event;
    auto& native = window_->native();

    auto convert = [&](sf::Vector2i pixel) -> sf::Vector2i {
        auto p = native.mapPixelToCoords(pixel, designView_);
        return {static_cast<int>(p.x), static_cast<int>(p.y)};
    };

    if (auto* mm = ev.getIf<sf::Event::MouseMoved>()) {
        mm->position = convert(mm->position);
    } else if (auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
        mb->position = convert(mb->position);
    } else if (auto* mr = ev.getIf<sf::Event::MouseButtonReleased>()) {
        mr->position = convert(mr->position);
    } else if (auto* ws = ev.getIf<sf::Event::MouseWheelScrolled>()) {
        ws->position = convert(ws->position);

        // ⭐ 内容高 > View 高时允许滚动
        float uiS = getUiScale();
        float viewH = kDesignH / uiS;
        constexpr float kBottomReserve = 60.f;
        float maxScroll = std::max(0.f, contentTotalH_ - viewH + kBottomReserve);
        if (maxScroll > 1.f) {
            contentScroll_ -= ws->delta * 40.f;
            contentScroll_ = std::clamp(contentScroll_, 0.f, maxScroll);
        }
        return;   // 不往下传
    }

    // ⭐ 底部一行按钮在窗口坐标系（不缩放），用原始 event
    backButton_->handleEvent(event);
    if (currentTab_ == Tab::Other) {
        aboutButton_->handleEvent(event);
        resetButton_->handleEvent(event);
    }
    if (currentTab_ == Tab::Graphics) {
        resetGraphicsButton_->handleEvent(event);
    }

    if (resetConfirm_) { resetConfirm_->handleEvent(ev); return; }
    if (aboutDialog_)  { aboutDialog_->handleEvent(ev);  return; }

    bool inputFocused = anySliderEditing();
    if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape && !inputFocused) {
            nextScene_ = SceneId::Back;
            return;
        }
    }
    for (auto& b : tabButtons_) b->handleEvent(ev);

    switch (currentTab_) {
        case Tab::Display:
            if (displayTab_) displayTab_->handleEvent(ev);
            break;
        case Tab::Interface:
            if (interfaceTab_) interfaceTab_->handleEvent(ev);
            break;
        case Tab::Graphics:
            if (graphicsTab_) graphicsTab_->handleEvent(ev);
            break;
        case Tab::Audio:
            if (audioTab_) audioTab_->handleEvent(ev);
            break;
        case Tab::Keys:
            if (keysTab_) keysTab_->handleEvent(ev);
            break;
        case Tab::Other:
            if (otherTab_) otherTab_->handleEvent(ev);
            break;
    }
}

// ============================================================
// 更新
// ============================================================

void SettingsScene::update(float /*dt*/) {
    // ⭐ 画面 Tab 重置按钮
    if (currentTab_ == Tab::Graphics
        && resetGraphicsButton_->consumeClick()) {
        if (graphicsTab_) graphicsTab_->resetPost();
        return;
    }

    if (resetConfirm_) {
        auto r = resetConfirm_->consumeResult();
        if (r == ConfirmDialog::Result::Yes) {
            resetAllPreferences();
            nextScene_ = SceneId::Exit;
        } else if (r == ConfirmDialog::Result::No) {
            resetConfirm_.reset();
            syncFocus();
        }
        return;
    }
    if (aboutDialog_) {
        auto r = aboutDialog_->consumeResult();
        if (r == ConfirmDialog::Result::Ok || r == ConfirmDialog::Result::No) {
            aboutDialog_.reset();
            syncFocus();
        }
        return;
    }

    for (int i = 0; i < kTabCount; ++i) {
        if (tabButtons_[i]->consumeClick()) {
            if (static_cast<int>(currentTab_) != i) {
                currentTab_ = static_cast<Tab>(i);
                contentScroll_ = 0.f;      // ⭐ 切 Tab 重置滚动
                refreshSelection();
                syncFocus();
            }
            return;
        }
    }

    switch (currentTab_) {
        case Tab::Display:
            if (displayTab_) displayTab_->update();
            break;
        case Tab::Interface:
            if (interfaceTab_) interfaceTab_->update();
            break;
        case Tab::Graphics:
            if (graphicsTab_) graphicsTab_->update();
            break;
        case Tab::Audio:
            if (audioTab_) audioTab_->update();
            break;
        case Tab::Keys:
            if (keysTab_) keysTab_->update();
            break;
        case Tab::Other: {
            if (otherTab_) otherTab_->update();

            if (aboutButton_->consumeClick()) {
                std::string msg =
                    std::string(Str::T(Str::AboutTitle)) + "\n\n"
                    + Str::T(Str::AboutVersion) + PROJECT_VERSION + "\n"
                    + Str::T(Str::AboutBuild)   + BUILD_DATE + "\n"
                    + Str::T(Str::AboutAuthor)  + "ljm-233";
                aboutDialog_ = std::make_unique<ConfirmDialog>(
                    font_, msg, sf::Vector2f(kDesignW, kDesignH),
                    ConfirmDialog::Mode::Info);
                syncFocus();
                return;
            }
            if (resetButton_->consumeClick()) {
                resetConfirm_ = std::make_unique<ConfirmDialog>(
                    font_, Str::T(Str::ResetConfirm),
                    sf::Vector2f(kDesignW, kDesignH));
                syncFocus();
                return;
            }
            break;
        }
    }

    if (backButton_->consumeClick()) nextScene_ = SceneId::Back;
}

// ============================================================
// 渲染
// ============================================================

void SettingsScene::renderTabs(Window& window) {
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i]->setPosition({kTabX, kTabY + i * kTabGap});
        tabButtons_[i]->render(window.target());
    }
}

namespace {
struct RowDrawer {
    sf::RenderTarget& target;
    float contentX;
    float ctrlX;
    float y;
    float rowH = 50.f;
    float availableWidth = 800.f;

    void toggle(ToggleRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);
        row.onButton->setPosition ({ctrlX, y});
        row.offButton->setPosition({ctrlX + 96.f, y});
        row.onButton->render(target);
        row.offButton->render(target);
        y += rowH;
    }

    void toggle(sf::Text& label,
                const std::unique_ptr<Button>& on,
                const std::unique_ptr<Button>& off) {
        label.setPosition({contentX, y + 8.f});
        target.draw(label);
        on->setPosition ({ctrlX, y});
        off->setPosition({ctrlX + 96.f, y});
        on->render(target);
        off->render(target);
        y += rowH;
    }

    void multi(sf::Text& label,
               std::vector<std::unique_ptr<Button>>& btns,
               float gap = 96.f) {
        label.setPosition({contentX, y + 8.f});
        target.draw(label);
        for (size_t i = 0; i < btns.size(); ++i) {
            btns[i]->setPosition({ctrlX + static_cast<float>(i) * gap, y});
            btns[i]->render(target);
        }
        y += rowH;
    }

    // MultiRow 版本（单行 + 自动折行 + 网格）
    void multi(MultiRow& row) {
        row.label.setPosition({contentX, y + 8.f});
        target.draw(row.label);

        if (row.buttons.empty()) { y += rowH; return; }

        if (row.columns <= 0) {
            const float btnW = row.buttons[0]->size().x;
            const float totalW = row.stepX * (row.buttons.size() - 1) + btnW;

            if (totalW <= availableWidth) {
                for (size_t i = 0; i < row.buttons.size(); ++i) {
                    row.buttons[i]->setPosition(
                        {ctrlX + static_cast<float>(i) * row.stepX, y});
                    row.buttons[i]->render(target);
                }
                y += rowH;
            } else {
                float usable = availableWidth - btnW;
                if (usable < 0.f) usable = 0.f;
                int perRow = 1 + static_cast<int>(usable / row.stepX);
                if (perRow < 1) perRow = 1;
                if (perRow > static_cast<int>(row.buttons.size()))
                    perRow = static_cast<int>(row.buttons.size());

                for (size_t i = 0; i < row.buttons.size(); ++i) {
                    int r = static_cast<int>(i) / perRow;
                    int c = static_cast<int>(i) % perRow;
                    row.buttons[i]->setPosition(
                        {ctrlX + static_cast<float>(c) * row.stepX,
                         y + static_cast<float>(r) * rowH});
                    row.buttons[i]->render(target);
                }
                int rows = (static_cast<int>(row.buttons.size()) + perRow - 1)
                           / perRow;
                y += static_cast<float>(rows) * rowH + 6.f;
            }
        } else {
            const int cols = row.columns;
            float stepY = rowH;
            for (size_t i = 0; i < row.buttons.size(); ++i) {
                int r = static_cast<int>(i) / cols;
                int c = static_cast<int>(i) % cols;
                row.buttons[i]->setPosition(
                    {ctrlX + static_cast<float>(c) * row.stepX,
                     y + static_cast<float>(r) * stepY});
                row.buttons[i]->render(target);
            }
            int rows = (static_cast<int>(row.buttons.size()) + cols - 1) / cols;
            y += static_cast<float>(rows) * stepY + 6.f;
        }
    }

    void slider(sf::Text& label, Slider* s) {
        label.setPosition({contentX, y + 4.f});
        target.draw(label);
        s->setPosition({ctrlX, y + 4.f});
        s->render(target);
        y += rowH;
    }
};
} // namespace

float SettingsScene::renderDisplayTab(Window& window, float contentX,
                                     float ctrlX, float y) {
    headingDisplay_.setPosition({contentX, y});
    window.target().draw(headingDisplay_);
    y += 36.f;

    if (displayTab_) {
        return displayTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderInterfaceTab(Window& window, float contentX,
                                       float ctrlX, float y) {
    headingInterface_.setPosition({contentX, y});
    window.target().draw(headingInterface_);
    y += 36.f;

    if (interfaceTab_) {
        return interfaceTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderGraphicsTab(Window& window, float contentX,
                                      float ctrlX, float y) {
    headingGraphics_.setPosition({contentX, y});
    window.target().draw(headingGraphics_);
    y += 36.f;

    if (graphicsTab_) {
        return graphicsTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderAudioTab(Window& window, float contentX,
                                   float ctrlX, float y) {
    headingAudio_.setPosition({contentX, y});
    window.target().draw(headingAudio_);
    y += 36.f;

    if (audioTab_) {
        return audioTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderKeysTab(Window& window, float contentX,
                                  float ctrlX, float y) {
    headingKeys_.setPosition({contentX, y});
    window.target().draw(headingKeys_);
    y += 36.f;

    if (keysTab_) {
        return keysTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y + 30.f;
}

float SettingsScene::renderOtherTab(Window& window, float contentX,
                                   float ctrlX, float y) {
    headingOther_.setPosition({contentX, y});
    window.target().draw(headingOther_);
    y += 36.f;

    if (otherTab_) {
        return otherTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y + 40.f;
}

// ============================================================
// 设计坐标系 View
// ============================================================

void SettingsScene::updateDesignView() {
    auto size = window_->native().getSize();
    float winW = static_cast<float>(size.x);
    float winH = static_cast<float>(size.y);
    if (winW <= 0.f || winH <= 0.f) return;

    // ⭐ uiScale 决定设计区域的"多少"映射到窗口
    //   uiScale = 1.0 → 1280×720 设计区域占满窗口
    //   uiScale = 0.5 → 2560×1440 设计区域缩到窗口（UI 缩小一半）
    //   uiScale = 2.0 → 640×360 设计区域放大到窗口（UI 放大两倍）
    float uiS = getUiScale();
    if (uiS < 0.01f) uiS = 1.0f;

    float viewW = kDesignW / uiS;
    float viewH = kDesignH / uiS;

    designView_.setSize({viewW, viewH});

    // ⭐ 应用滚动偏移
    // 底部多留 60 设计像素，让内容能滚到返回按钮上方
    constexpr float kBottomReserve = 60.f;
    float maxScroll = std::max(0.f, contentTotalH_ - viewH + kBottomReserve);
    contentScroll_ = std::clamp(contentScroll_, 0.f, maxScroll);
    designView_.setCenter({viewW * 0.5f, viewH * 0.5f + contentScroll_});

    // Viewport 保持设计宽高比
    float winAspect    = winW / winH;
    float designAspect = viewW / viewH;

    sf::FloatRect vp;
    if (winAspect > designAspect) {
        float vpW = designAspect / winAspect;
        vp = sf::FloatRect(sf::Vector2f{(1.f - vpW) * 0.5f, 0.f},
                           sf::Vector2f{vpW, 1.f});
    } else {
        float vpH = winAspect / designAspect;
        vp = sf::FloatRect(sf::Vector2f{0.f, (1.f - vpH) * 0.5f},
                           sf::Vector2f{1.f, vpH});
    }
    designView_.setViewport(vp);
}

void SettingsScene::render(Window& window) {
    refreshLabels();
    updateDesignView();

    auto& native = window.target();

    // 阶段 1：窗口坐标系画背景
    native.setView(native.getDefaultView());
    native.clear(sf::Color::Black);
    if (background_) background_->render(native);

    // 阶段 2a：Tab 栏用不滚动的 View（固定左侧）
    {
        float uiS = getUiScale();
        if (uiS < 0.01f) uiS = 1.0f;
        sf::View tabView = designView_;
        tabView.setCenter({kDesignW / uiS * 0.5f, kDesignH / uiS * 0.5f});
        native.setView(tabView);
        renderTabs(window);
    }

    // 阶段 2b：内容用带滚动的 View
    native.setView(designView_);

    float contentBottom = 0.f;
    switch (currentTab_) {
        case Tab::Display:
            contentBottom = renderDisplayTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Interface:
            contentBottom = renderInterfaceTab(window, kContentX, kCtrlX, 50.f);
            break;
        case Tab::Graphics:
            contentBottom = renderGraphicsTab(window, kContentX, kCtrlX, 50.f);
            break;
        case Tab::Audio:
            contentBottom = renderAudioTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Keys:
            contentBottom = renderKeysTab(window, kContentX, kCtrlX, 60.f);
            break;
        case Tab::Other:
            contentBottom = renderOtherTab(window, kContentX, kCtrlX, 60.f);
            break;
    }
    contentTotalH_ = contentBottom;

    if (resetConfirm_) {
        resetConfirm_->relayout({kDesignW, kDesignH});
        resetConfirm_->render(native);
    }
    if (aboutDialog_) {
        aboutDialog_->relayout({kDesignW, kDesignH});
        aboutDialog_->render(native);
    }

    // ⭐ 滚动条指示器（在 designView 下画）
    {
        float uiS = getUiScale();
        if (uiS > 1.01f) {
            float viewH = kDesignH / uiS;
            constexpr float kBottomReserve = 60.f;
            float maxScroll = std::max(0.f, contentTotalH_ - viewH + kBottomReserve);
            if (maxScroll > 1.f) {
                float viewW = kDesignW / uiS;
                float ratio = contentScroll_ / maxScroll;
                float barH = std::max(40.f, viewH * 0.25f);
                float barY = 10.f + ratio * (viewH - barH - 20.f);

                sf::RectangleShape bar({6.f, barH});
                bar.setFillColor(sf::Color(255, 255, 255, 120));
                bar.setPosition({viewW - 14.f, barY});
                native.draw(bar);
            }
        }
    }

    // ⭐ 阶段 3：窗口坐标系底部一行（不随 UI 缩放）
    native.setView(native.getDefaultView());
    {
        auto size = window.native().getSize();
        float winW = static_cast<float>(size.x);
        float winH = static_cast<float>(size.y);

        const float gap    = 20.f;
        const float margin = 20.f;

        float bw = backButton_->size().x;
        float bh = backButton_->size().y;
        float y  = winH - bh - margin;

        float backX = winW - bw - margin;

        // ⭐ 先画关于/重置，再画返回（保证返回在最上层）
        if (currentTab_ == Tab::Other) {
            float rw = resetButton_->size().x;
            float aw = aboutButton_->size().x;
            float resetX = backX - gap - rw;
            float aboutX = resetX - gap - aw;

            aboutButton_->setPosition({aboutX, y});
            aboutButton_->render(native);

            resetButton_->setPosition({resetX, y});
            resetButton_->render(native);
        }
        if (currentTab_ == Tab::Graphics) {
            float gw = resetGraphicsButton_->size().x;
            float gx = backX - gap - gw;
            resetGraphicsButton_->setPosition({gx, y});
            resetGraphicsButton_->render(native);
        }

        // 返回按钮最后画，确保在最上层
        backButton_->setPosition({backX, y});
        backButton_->render(native);
    }
}