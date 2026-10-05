#include "settings_scene.h"
#include "utils/text_strings.h"
#include <string>
#include "utils/utf8.h"
#include "ui_scale.h"
#include "button_style.h"
#include "utils/animation.h"
#include "notification.h"
#include "sound_manager.h"
#include "focus_group.h"
#include "infrastructure/keybindings.h"
#include "infrastructure/gamepad.h"
#include "tabs/audio_tab.h"
#include "tabs/graphics_tab.h"
#include "tabs/interface_tab.h"
#include "tabs/display_tab.h"
#include <algorithm>
#include <cmath>
#include "utils/lang.h"

namespace {

// ===== SFML 版本兼容 =====
//
// `Event::getIf` 的非 const 重载是 SFML **3.1** 才加的；3.0.x 只有 const 版本，
// 返回 `const T*`。而 brew 和 vcpkg 目前都还停在 3.0.2 —— 直接用 3.1 的写法
// 会让 macOS / Windows 上的构建直接失败（Arch 的 3.1 能过，所以本地发现不了）。
//
// 这里抹掉 const 是安全的：下面改的是**我们自己拷贝出来的** ev，对象本身非 const，
// 也没碰别人的数据。3.1 上走的本来就是非 const 重载，这个转换是空操作。
template <typename T> T* eventIfMutable(sf::Event& e) {
    return const_cast<T*>(e.getIf<T>());
}

// ===== 布局（设计坐标系 1280×720）=====
constexpr float kTabX = 40.f;
constexpr float kTabY = 90.f;
constexpr float kTabGap = 62.f;
constexpr float kContentX = kTabX + 200.f;
constexpr float kCtrlX = kContentX + 240.f;
constexpr float kBtnW = 280.f;
constexpr float kBtnH = 46.f;
constexpr float kGapX = 16.f;
constexpr float kGapY = 10.f;

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

SettingsScene::SettingsScene(std::shared_ptr<Background> background,
                             std::shared_ptr<Preferences> preferences,
                             std::shared_ptr<RuntimeConfig> runtimeConfig,
                             std::shared_ptr<Window> window,
                             std::shared_ptr<WallpaperLibrary> wallpaperLibrary,
                             std::shared_ptr<WallpaperLoader> wallpaperLoader,
                             std::shared_ptr<Paths> paths, const sf::Font& font,
                             std::shared_ptr<Logger> logger)
      : background_(std::move(background)),
        preferences_(std::move(preferences)),
        runtimeConfig_(std::move(runtimeConfig)),
        window_(std::move(window)),
        wallpaperLibrary_(std::move(wallpaperLibrary)),
        wallpaperLoader_(std::move(wallpaperLoader)),
        paths_(std::move(paths)),
        logger_(std::move(logger)),
        font_(font),
        headingDisplay_(font, toSf(Str::TabDisplay), fontSizeInView(24)),
        headingInterface_(font, toSf(Str::TabInterface), fontSizeInView(24)),
        headingWallpaper_(font, toSf(Str::LabelWallpaper), fontSizeInView(24)),
        headingGraphics_(font, toSf(Str::TabGraphics), fontSizeInView(24)),
        headingAudio_(font, toSf(Str::TabAudioLog), fontSizeInView(24)),
        headingControls_(font, toSf(Str::TabControls), fontSizeInView(24)),
        headingGame_(font, toSf(Str::TabGame), fontSizeInView(24)),
        headingConsole_(font, toSf(Str::TabConsole), fontSizeInView(24)),
        headingAdvanced_(font, toSf(Str::TabAdvanced), fontSizeInView(24)),
        scrollHint_(font, sf::String(), fontSizeInView(16)) {
    // 标题颜色
    auto headingColor = sf::Color(160, 200, 240);
    headingDisplay_.setFillColor(headingColor);
    headingInterface_.setFillColor(headingColor);
    headingWallpaper_.setFillColor(headingColor);
    headingGraphics_.setFillColor(headingColor);
    headingAudio_.setFillColor(headingColor);
    headingControls_.setFillColor(headingColor);
    headingGame_.setFillColor(headingColor);
    headingConsole_.setFillColor(headingColor);
    headingAdvanced_.setFillColor(headingColor);
    scrollHint_.setFillColor(sf::Color(200, 200, 140));

    // 文案从 settings_tab_id.h 取 —— 那边是 Tab 身份的唯一来源，
    // "枚举顺序 / 标签 / 归属表"三者一致由它保证（还有归属测试兜着）。
    // 以前这里手抄一份数组，加了 Tab 忘补一行就会顶着别人的名字显示，且完全静默。
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_.push_back(std::make_unique<Button>(
            settingsTabLabel(static_cast<SettingsTab>(i)), font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{180.f, 50.f}, 22));
    }

    // ───────────── Display ─────────────
    // ⭐ 独立 Tab
    displayTab_ =
        std::make_unique<DisplayTab>(font_, preferences_, runtimeConfig_, window_);

    // ───────────── Interface ─────────────
    // ⭐ 独立 Tab
    interfaceTab_ = std::make_unique<InterfaceTab>(font_, preferences_, window_);

    // ───────────── Wallpaper ─────────────
    // ⭐ 独立 Tab（0.3.7 从 InterfaceTab 里独立出来）
    // 要 library（有哪些壁纸）与 loader（缩略图）才能把候选列全
    wallpaperTab_ = std::make_unique<WallpaperTab>(font_, preferences_, background_,
                                                   wallpaperLibrary_, wallpaperLoader_);

    // ───────────── Graphics ─────────────
    // ⭐ 独立 Tab
    graphicsTab_ = std::make_unique<GraphicsTab>(font_, preferences_, window_);
    // ───────────── Audio ─────────────
    // ⭐ 独立 Tab
    audioTab_ = std::make_unique<AudioTab>(font_, preferences_, window_);

    // ───────────── Controls（键位 + 手柄）─────────────
    // 0.3.8：手柄三项原在「音频」页（因为它们"像"触觉），现在归位
    controlsTab_ = std::make_unique<ControlsTab>(font_, preferences_);

    // ───────────── Game（玩法）─────────────
    // 0.3.8 新建：初始生命/关卡开场原在「画面」，玩家名原在「其他」
    gameTab_ = std::make_unique<GameTab>(font_, preferences_);

    // ───────────── Console（控制台外观）─────────────
    // 0.3.8 从「界面」独立出来：那页本来 19 个控件，一半是控制台的
    consoleTab_ = std::make_unique<ConsoleTab>(font_, preferences_);

    // ───────────── Advanced（日志 / 调试 / 文件 / 设置分享）─────────────
    advancedTab_ = std::make_unique<AdvancedTab>(font_, preferences_, logger_, paths_);

    aboutButton_ = std::make_unique<Button>(
        Str::ButtonAbout, font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 46.f}, 20);
    resetAllButton_ = std::make_unique<Button>(
        Str::ResetDefault, font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{210.f, 46.f}, 20);
    backButton_ = std::make_unique<Button>(Str::Back, font_, sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{160.f, 50.f}, 22);
    // 「恢复本页默认」每一页都有 —— 以前只有「画面」页有单独的重置按钮，
    // 别的页调坏一项只能全局重置（连累其它页）
    resetTabButton_ =
        std::make_unique<Button>(Str::ButtonResetTab, font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{190.f, 50.f}, 20);

    refreshSelection();
    updateDesignView();
}

// ============================================================
// 选中状态
// ============================================================

void SettingsScene::refreshLabels() {
    // 只在语言变化时刷新
    int v = Lang::instance().version();
    if (v == lastLangVersion_)
        return;
    lastLangVersion_ = v;

    auto setLabel = [](sf::Text& t, const char* key) { t.setString(toSf(Str::T(key))); };

    // ===== Heading =====
    sf::Text* headings[] = {&headingDisplay_,  &headingInterface_, &headingWallpaper_,
                            &headingGraphics_, &headingAudio_,     &headingControls_,
                            &headingGame_,     &headingConsole_,   &headingAdvanced_};
    static_assert(sizeof(headings) / sizeof(headings[0]) ==
                      static_cast<std::size_t>(kTabCount),
                  "标题数组必须与 Tab 数量一致");
    for (int i = 0; i < kTabCount; ++i)
        setLabel(*headings[i], settingsTabLabel(static_cast<SettingsTab>(i)));

    // ===== Tab 按钮 =====
    for (int i = 0; i < kTabCount && i < static_cast<int>(tabButtons_.size()); ++i)
        tabButtons_[i]->setText(Str::T(settingsTabLabel(static_cast<SettingsTab>(i))));

    // ===== Toggle / Multi 刷新 =====
    if (displayTab_)
        displayTab_->refreshLabels();
    if (graphicsTab_)
        graphicsTab_->refreshLabels();
    if (audioTab_)
        audioTab_->refreshLabels();
    if (interfaceTab_)
        interfaceTab_->refreshLabels();
    if (wallpaperTab_)
        wallpaperTab_->refreshLabels();
    if (controlsTab_)
        controlsTab_->refreshLabels();
    if (gameTab_)
        gameTab_->refreshLabels();
    if (consoleTab_)
        consoleTab_->refreshLabels();
    if (advancedTab_)
        advancedTab_->refreshLabels();
    // ===== 其他按钮 =====
    if (aboutButton_)
        aboutButton_->setText(Str::T(Str::ButtonAbout));
    if (resetAllButton_)
        resetAllButton_->setText(Str::T(Str::ResetDefault));
    if (resetTabButton_)
        resetTabButton_->setText(Str::T(Str::ButtonResetTab));
    if (backButton_)
        backButton_->setText(Str::T(Str::Back));
    scrollHint_.setString(toSf(Str::T(Str::HintScroll)));
}

void SettingsScene::refreshSelection() {
    for (int i = 0; i < kTabCount; ++i)
        tabButtons_[i]->setSelected(i == static_cast<int>(currentTab_));

    // Display
    if (displayTab_)
        displayTab_->refreshSelection();

    // Interface
    if (interfaceTab_)
        interfaceTab_->refreshSelection();

    if (wallpaperTab_)
        wallpaperTab_->refreshSelection();

    // Graphics
    if (graphicsTab_)
        graphicsTab_->refreshSelection();

    // Audio
    if (audioTab_)
        audioTab_->refreshSelection();

    if (controlsTab_)
        controlsTab_->refreshSelection();
    if (gameTab_)
        gameTab_->refreshSelection();
    if (consoleTab_)
        consoleTab_->refreshSelection();
    if (advancedTab_)
        advancedTab_->refreshSelection();
}

void SettingsScene::syncFocus() {
    if (resetTabConfirm_ || resetAllConfirm_ || aboutDialog_) {
        FocusGroup::instance().clear();
        return;
    }

    std::vector<Button*> items;
    for (auto& b : tabButtons_)
        items.push_back(b.get());

    switch (currentTab_) {
    case SettingsTab::Display:
        if (displayTab_)
            displayTab_->registerFocus(items);
        break;
    case SettingsTab::Interface:
        if (interfaceTab_)
            interfaceTab_->registerFocus(items);
        break;
    case SettingsTab::Wallpaper:
        if (wallpaperTab_)
            wallpaperTab_->registerFocus(items);
        break;
    case SettingsTab::Graphics:
        if (graphicsTab_)
            graphicsTab_->registerFocus(items);
        break;
    case SettingsTab::Audio:
        audioTab_->registerFocus(items);
        break;
    case SettingsTab::Controls:
        if (controlsTab_)
            controlsTab_->registerFocus(items);
        break;
    case SettingsTab::Game:
        if (gameTab_)
            gameTab_->registerFocus(items);
        break;
    case SettingsTab::Console:
        if (consoleTab_)
            consoleTab_->registerFocus(items);
        break;
    case SettingsTab::Advanced:
        if (advancedTab_)
            advancedTab_->registerFocus(items);
        break;
    }
    // 「恢复本页默认」在底栏（窗口坐标系），和返回按钮一样不参与几何导航，
    // 用鼠标点或手柄的合成事件触发
    if (resetTabButton_)
        items.push_back(resetTabButton_.get());
    // ⭐ 返回按钮在窗口坐标系固定右下角，不参与设计坐标系几何导航
    //    用 ESC 或手柄 B 键返回
    FocusGroup::instance().setItems(items);
}

// ============================================================
// 应用状态
// ============================================================

void SettingsScene::resetAllPreferences() {
    preferences_->resetAll();
}

bool SettingsScene::anySliderEditing() const {
    // 有任何一页在编辑输入框就吞掉 ESC —— 否则用户按 ESC 想取消输入，
    // 结果整个设置页退出了
    if (displayTab_ && displayTab_->anyEditing())
        return true;
    if (interfaceTab_ && interfaceTab_->anyEditing())
        return true;
    if (wallpaperTab_ && wallpaperTab_->anyEditing())
        return true;
    if (graphicsTab_ && graphicsTab_->anyEditing())
        return true;
    if (audioTab_ && audioTab_->anyEditing())
        return true;
    if (controlsTab_ && controlsTab_->anyEditing())
        return true;
    if (gameTab_ && gameTab_->anyEditing())
        return true;
    if (consoleTab_ && consoleTab_->anyEditing())
        return true;
    if (advancedTab_ && advancedTab_->anyEditing())
        return true;
    return false;
}

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

    if (auto* mm = eventIfMutable<sf::Event::MouseMoved>(ev)) {
        mm->position = convert(mm->position);
    } else if (auto* mb = eventIfMutable<sf::Event::MouseButtonPressed>(ev)) {
        mb->position = convert(mb->position);
    } else if (auto* mr = eventIfMutable<sf::Event::MouseButtonReleased>(ev)) {
        mr->position = convert(mr->position);
    } else if (auto* ws = eventIfMutable<sf::Event::MouseWheelScrolled>(ev)) {
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
        return; // 不往下传
    }

    // ⭐ 底部一行按钮在窗口坐标系（不缩放），用原始 event
    backButton_->handleEvent(event);
    resetTabButton_->handleEvent(event);
    if (currentTab_ == SettingsTab::Advanced) {
        aboutButton_->handleEvent(event);
        resetAllButton_->handleEvent(event);
    }

    if (resetTabConfirm_) {
        resetTabConfirm_->handleEvent(ev);
        return;
    }
    if (resetAllConfirm_) {
        resetAllConfirm_->handleEvent(ev);
        return;
    }
    if (aboutDialog_) {
        aboutDialog_->handleEvent(ev);
        return;
    }

    bool inputFocused = anySliderEditing();
    if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape && !inputFocused) {
            nextScene_ = SceneId::Back;
            return;
        }
    }
    for (auto& b : tabButtons_)
        b->handleEvent(ev);

    switch (currentTab_) {
    case SettingsTab::Display:
        if (displayTab_)
            displayTab_->handleEvent(ev);
        break;
    case SettingsTab::Interface:
        if (interfaceTab_)
            interfaceTab_->handleEvent(ev);
        break;
    case SettingsTab::Wallpaper:
        if (wallpaperTab_)
            wallpaperTab_->handleEvent(ev);
        break;
    case SettingsTab::Graphics:
        if (graphicsTab_)
            graphicsTab_->handleEvent(ev);
        break;
    case SettingsTab::Audio:
        if (audioTab_)
            audioTab_->handleEvent(ev);
        break;
    case SettingsTab::Controls:
        if (controlsTab_)
            controlsTab_->handleEvent(ev);
        break;
    case SettingsTab::Game:
        if (gameTab_)
            gameTab_->handleEvent(ev);
        break;
    case SettingsTab::Console:
        if (consoleTab_)
            consoleTab_->handleEvent(ev);
        break;
    case SettingsTab::Advanced:
        if (advancedTab_)
            advancedTab_->handleEvent(ev);
        break;
    }
}

// ============================================================
// 更新
// ============================================================

void SettingsScene::update(float /*dt*/) {
    // ⭐ 文本输入/滑块编辑期间挂起键盘导航：否则在"玩家名"里打字母会同时
    // 移动焦点，按 Enter 还会误触按钮。每帧同步一次，输入框失焦后自然恢复。
    FocusGroup::instance().setKeyboardNavEnabled(!anySliderEditing());

    // 「恢复本页默认」的确认框。重置只碰本页的键，所以确认之后不用重启 ——
    // 重置完就地重新读一遍并把需要即时生效的东西应用上（见 resetCurrentTab）
    if (resetTabConfirm_) {
        const auto r = resetTabConfirm_->consumeResult();
        resetTabConfirm_.reset();
        if (r == ConfirmDialog::Result::Yes)
            resetCurrentTab();
        syncFocus();
        return;
    }

    if (resetAllConfirm_) {
        const auto r = resetAllConfirm_->consumeResult();
        if (r == ConfirmDialog::Result::Yes) {
            resetAllPreferences();
            // 全局重置仍然走"退出后下次启动生效"：涉及分辨率、字体缩放、
            // 着色器等一堆启动期固化的东西，逐个热更新既啰嗦又容易漏
            nextScene_ = SceneId::Exit;
        } else if (r == ConfirmDialog::Result::No) {
            resetAllConfirm_.reset();
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
                currentTab_ = static_cast<SettingsTab>(i);
                contentScroll_ = 0.f; // ⭐ 切 Tab 重置滚动
                refreshSelection();
                syncFocus();
            }
            return;
        }
    }

    switch (currentTab_) {
    case SettingsTab::Display:
        if (displayTab_)
            displayTab_->update();
        break;
    case SettingsTab::Interface:
        if (interfaceTab_)
            interfaceTab_->update();
        break;
    case SettingsTab::Wallpaper:
        if (wallpaperTab_)
            wallpaperTab_->update();
        break;
    case SettingsTab::Graphics:
        if (graphicsTab_)
            graphicsTab_->update();
        break;
    case SettingsTab::Audio:
        if (audioTab_)
            audioTab_->update();
        break;
    case SettingsTab::Controls:
        if (controlsTab_)
            controlsTab_->update();
        break;
    case SettingsTab::Game:
        if (gameTab_)
            gameTab_->update();
        break;
    case SettingsTab::Console:
        if (consoleTab_)
            consoleTab_->update();
        break;
    case SettingsTab::Advanced: {
        if (advancedTab_) {
            advancedTab_->update();
            // 导入设置之后有一堆东西（分辨率、字体缩放、着色器…）在启动期就固化了，
            // 沿用「恢复默认设置」的既有做法：退出，下次启动生效
            if (advancedTab_->consumeRestartRequest()) {
                nextScene_ = SceneId::Exit;
                return;
            }
        }

        if (aboutButton_->consumeClick()) {
            std::string msg = std::string(Str::T(Str::AboutTitle)) + "\n\n" +
                              Str::T(Str::AboutVersion) + PROJECT_VERSION + "\n" +
                              Str::T(Str::AboutBuild) + BUILD_DATE + "\n" +
                              Str::T(Str::AboutAuthor) + "ljm-233";
            aboutDialog_ = std::make_unique<ConfirmDialog>(
                font_, msg, sf::Vector2f(kDesignW, kDesignH), ConfirmDialog::Mode::Info);
            syncFocus();
            return;
        }
        if (resetAllButton_->consumeClick()) {
            resetAllConfirm_ = std::make_unique<ConfirmDialog>(
                font_, Str::T(Str::ResetConfirm), sf::Vector2f(kDesignW, kDesignH));
            syncFocus();
            return;
        }
        break;
    }
    }

    // 「恢复本页默认」：每一页都能用
    if (resetTabButton_->consumeClick()) {
        resetTabConfirm_ = std::make_unique<ConfirmDialog>(
            font_, Str::T(Str::ResetTabConfirm), sf::Vector2f(kDesignW, kDesignH));
        syncFocus();
        return;
    }

    if (backButton_->consumeClick())
        nextScene_ = SceneId::Back;
}

void SettingsScene::resetCurrentTab() {
    // 把本页的键从配置里**删掉**（不是写回一份默认值）——
    // 默认值只存在于各处读取点，删掉之后自然回落，永远只有一处定义
    std::vector<std::string> keys;
    for (const char* k : keysForTab(currentTab_))
        keys.emplace_back(k);
    preferences_->resetKeys(keys);

    // 再让本页重读一遍，并把需要即时生效的东西应用上
    switch (currentTab_) {
    case SettingsTab::Display:
        if (displayTab_)
            displayTab_->reapply();
        break;
    case SettingsTab::Interface:
        if (interfaceTab_)
            interfaceTab_->reapply();
        break;
    case SettingsTab::Wallpaper:
        if (wallpaperTab_)
            wallpaperTab_->reapply();
        break;
    case SettingsTab::Graphics:
        if (graphicsTab_)
            graphicsTab_->reapply();
        break;
    case SettingsTab::Audio:
        if (audioTab_)
            audioTab_->reapply();
        break;
    case SettingsTab::Controls:
        if (controlsTab_)
            controlsTab_->reapply();
        break;
    case SettingsTab::Game:
        if (gameTab_)
            gameTab_->reapply();
        break;
    case SettingsTab::Console:
        if (consoleTab_)
            consoleTab_->reapply();
        break;
    case SettingsTab::Advanced:
        if (advancedTab_)
            advancedTab_->reapply();
        break;
    default:
        break;
    }
    refreshSelection();
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

float SettingsScene::renderDisplayTab(Window& window, float contentX, float ctrlX,
                                      float y) {
    headingDisplay_.setPosition({contentX, y});
    window.target().draw(headingDisplay_);
    y += 36.f;

    if (displayTab_) {
        return displayTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderInterfaceTab(Window& window, float contentX, float ctrlX,
                                        float y) {
    headingInterface_.setPosition({contentX, y});
    window.target().draw(headingInterface_);
    y += 36.f;

    if (interfaceTab_) {
        return interfaceTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderWallpaperTab(Window& window, float contentX, float /*ctrlX*/,
                                        float y) {
    headingWallpaper_.setPosition({contentX, y});
    window.target().draw(headingWallpaper_);
    y += 36.f;

    if (wallpaperTab_) {
        return wallpaperTab_->render(window.target(), contentX, y);
    }
    return y;
}

float SettingsScene::renderGraphicsTab(Window& window, float contentX, float ctrlX,
                                       float y) {
    headingGraphics_.setPosition({contentX, y});
    window.target().draw(headingGraphics_);
    y += 36.f;

    if (graphicsTab_) {
        return graphicsTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderAudioTab(Window& window, float contentX, float ctrlX,
                                    float y) {
    headingAudio_.setPosition({contentX, y});
    window.target().draw(headingAudio_);
    y += 36.f;

    if (audioTab_) {
        return audioTab_->render(window.target(), contentX, ctrlX, y);
    }
    return y;
}

float SettingsScene::renderControlsTab(Window& window, float contentX, float ctrlX,
                                       float y) {
    headingControls_.setPosition({contentX, y});
    window.target().draw(headingControls_);
    y += 36.f;

    if (controlsTab_)
        return controlsTab_->render(window.target(), contentX, ctrlX, y);
    return y + 30.f;
}

float SettingsScene::renderGameTab(Window& window, float contentX, float ctrlX, float y) {
    headingGame_.setPosition({contentX, y});
    window.target().draw(headingGame_);
    y += 36.f;

    if (gameTab_)
        return gameTab_->render(window.target(), contentX, ctrlX, y);
    return y + 30.f;
}

float SettingsScene::renderConsoleTab(Window& window, float contentX, float ctrlX,
                                      float y) {
    headingConsole_.setPosition({contentX, y});
    window.target().draw(headingConsole_);
    y += 36.f;

    if (consoleTab_)
        return consoleTab_->render(window.target(), contentX, ctrlX, y);
    return y + 30.f;
}

float SettingsScene::renderAdvancedTab(Window& window, float contentX, float ctrlX,
                                       float y) {
    headingAdvanced_.setPosition({contentX, y});
    window.target().draw(headingAdvanced_);
    y += 36.f;

    if (advancedTab_)
        return advancedTab_->render(window.target(), contentX, ctrlX, y);
    return y + 40.f;
}

// ============================================================
// 设计坐标系 View
// ============================================================

void SettingsScene::updateDesignView() {
    auto size = window_->native().getSize();
    float winW = static_cast<float>(size.x);
    float winH = static_cast<float>(size.y);
    if (winW <= 0.f || winH <= 0.f)
        return;

    // ⭐ uiScale 决定设计区域的"多少"映射到窗口
    //   uiScale = 1.0 → 1280×720 设计区域占满窗口
    //   uiScale = 0.5 → 2560×1440 设计区域缩到窗口（UI 缩小一半）
    //   uiScale = 2.0 → 640×360 设计区域放大到窗口（UI 放大两倍）
    float uiS = getUiScale();
    if (uiS < 0.01f)
        uiS = 1.0f;

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
    float winAspect = winW / winH;
    float designAspect = viewW / viewH;

    sf::FloatRect vp;
    if (winAspect > designAspect) {
        float vpW = designAspect / winAspect;
        vp = sf::FloatRect(sf::Vector2f{(1.f - vpW) * 0.5f, 0.f}, sf::Vector2f{vpW, 1.f});
    } else {
        float vpH = winAspect / designAspect;
        vp = sf::FloatRect(sf::Vector2f{0.f, (1.f - vpH) * 0.5f}, sf::Vector2f{1.f, vpH});
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
    if (background_)
        background_->render(native);

    // 阶段 2a：Tab 栏用不滚动的 View（固定左侧）
    {
        float uiS = getUiScale();
        if (uiS < 0.01f)
            uiS = 1.0f;
        sf::View tabView = designView_;
        tabView.setCenter({kDesignW / uiS * 0.5f, kDesignH / uiS * 0.5f});
        native.setView(tabView);
        renderTabs(window);
    }

    // 阶段 2b：内容用带滚动的 View
    native.setView(designView_);

    float contentBottom = 0.f;
    switch (currentTab_) {
    case SettingsTab::Display:
        contentBottom = renderDisplayTab(window, kContentX, kCtrlX, 60.f);
        break;
    case SettingsTab::Interface:
        contentBottom = renderInterfaceTab(window, kContentX, kCtrlX, 50.f);
        break;
    case SettingsTab::Wallpaper:
        contentBottom = renderWallpaperTab(window, kContentX, kCtrlX, 50.f);
        break;
    case SettingsTab::Graphics:
        contentBottom = renderGraphicsTab(window, kContentX, kCtrlX, 50.f);
        break;
    case SettingsTab::Audio:
        contentBottom = renderAudioTab(window, kContentX, kCtrlX, 60.f);
        break;
    case SettingsTab::Controls:
        contentBottom = renderControlsTab(window, kContentX, kCtrlX, 50.f);
        break;
    case SettingsTab::Game:
        contentBottom = renderGameTab(window, kContentX, kCtrlX, 60.f);
        break;
    case SettingsTab::Console:
        contentBottom = renderConsoleTab(window, kContentX, kCtrlX, 60.f);
        break;
    case SettingsTab::Advanced:
        contentBottom = renderAdvancedTab(window, kContentX, kCtrlX, 50.f);
        break;
    }
    contentTotalH_ = contentBottom;

    if (resetTabConfirm_) {
        resetTabConfirm_->relayout({kDesignW, kDesignH});
        resetTabConfirm_->render(native);
    }
    if (resetAllConfirm_) {
        resetAllConfirm_->relayout({kDesignW, kDesignH});
        resetAllConfirm_->render(native);
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

                // 光有一根白条，用户不知道那是"还能滚"的意思。
                // 贴在滚动条顶端，只在真的溢出时才出现。
                scrollHint_.setPosition({viewW - 260.f, 10.f});
                native.draw(scrollHint_);
            }
        }
    }

    // ⭐ 阶段 3：窗口坐标系底部一行（不随 UI 缩放）
    native.setView(native.getDefaultView());
    {
        auto size = window.native().getSize();
        float winW = static_cast<float>(size.x);
        float winH = static_cast<float>(size.y);

        const float gap = 20.f;
        const float margin = 20.f;

        float bw = backButton_->size().x;
        float bh = backButton_->size().y;
        float y = winH - bh - margin;

        float backX = winW - bw - margin;

        // ⭐ 「恢复本页默认」每一页都在，而且位置固定 ——
        //    用户不用先搞清楚"这一页支持哪些操作"，位置固定才形成肌肉记忆。
        //    以前只有「画面」页有这个按钮，别的页调坏一项只能全局重置。
        const float tw = resetTabButton_->size().x;
        const float tx = backX - gap - tw;
        resetTabButton_->setPosition({tx, y});
        resetTabButton_->render(native);

        // 「关于」与「恢复全部默认」只在「高级」页：前者是看一次就够的信息，
        // 后者影响全部设置、属于危险操作，都不该在每一页都杵着
        if (currentTab_ == SettingsTab::Advanced) {
            const float rw = resetAllButton_->size().x;
            const float aw = aboutButton_->size().x;
            const float resetX = tx - gap - rw;
            const float aboutX = resetX - gap - aw;

            aboutButton_->setPosition({aboutX, y});
            aboutButton_->render(native);

            resetAllButton_->setPosition({resetX, y});
            resetAllButton_->render(native);
        }

        // 返回按钮最后画，确保在最上层
        backButton_->setPosition({backX, y});
        backButton_->render(native);
    }
}