#include "tabs/advanced_tab.h"

#include "config/keys.h"
#include "config/settings_codec.h"
#include "core/platform.h"
#include "log/rotation.h"
#include "ui_scale.h"
#include "utils/text_strings.h"
#include "utils/utf8.h"

#include <SFML/Window/Clipboard.hpp>

#include <algorithm>
#include <functional>
#include <string>

namespace {

constexpr float kRowH = 50.f;

const LogLevel kLevels[] = {LogLevel::Trace, LogLevel::Debug, LogLevel::Info,
                            LogLevel::Warn, LogLevel::Error};
constexpr int kLevelCount = 5;
const char* kLevelLabels[] = {"Trace", "Debug", "Info", "Warn", "Error"};

// 展示用的标签。档位取值范围本身由日志层维护（log/rotation.h 是权威来源），
// UI 只保留展示标签 —— 两边各存一份档位表迟早漂移。
const char* kRotateLabels[] = {"无限", "1MB", "5MB", "10MB"};
static_assert(kLogRotationSteps == 4, "标签表必须与日志层的档位数一致");

int indexOfLevel(int l) {
    for (int i = 0; i < kLevelCount; ++i)
        if (static_cast<int>(kLevels[i]) == l)
            return i;
    return 2; // Info
}

} // namespace

AdvancedTab::AdvancedTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                         std::shared_ptr<Logger> logger, std::shared_ptr<Paths> paths)
      : font_(font),
        prefs_(std::move(prefs)),
        logger_(std::move(logger)),
        paths_(std::move(paths)),
        labelLog_(font, sf::String(), scaledFontSize(18)),
        labelOpen_(font, sf::String(), scaledFontSize(20)),
        labelShare_(font, sf::String(), scaledFontSize(20)),
        labelCode_(font, sf::String(), scaledFontSize(18)),
        hintText_(font, sf::String(), scaledFontSize(16)),
        statusText_(font, sf::String(), scaledFontSize(18)) {
    auto softColor = sf::Color(180, 180, 200);
    hintText_.setFillColor(softColor);
    labelCode_.setFillColor(softColor);
    statusText_.setFillColor(sf::Color(150, 220, 150));

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

    rowLogLevel_ = addMulti(Str::LabelLogLevel, [this](int i) {
        logLevel_ = static_cast<int>(kLevels[i]);
        refreshSelection();
        prefs_->setInt(ConfigKey::kLogLevel, logLevel_);
    });
    for (int i = 0; i < kLevelCount; ++i)
        rowLogLevel_->addButton(std::make_unique<Button>(kLevelLabels[i], font_,
                                                         sf::Vector2f{0.f, 0.f},
                                                         sf::Vector2f{86.f, 40.f}, 18));

    rowLogRotate_ = addMulti(Str::LabelLogRotate, [this](int i) {
        logRotateIdx_ = i;
        refreshSelection();
        applyLogRotation();
    });
    for (int i = 0; i < kLogRotationSteps; ++i)
        rowLogRotate_->addButton(std::make_unique<Button>(kRotateLabels[i], font_,
                                                          sf::Vector2f{0.f, 0.f},
                                                          sf::Vector2f{86.f, 40.f}, 18));

    rowLogKeep_ = addMulti(Str::LabelLogKeep, [this](int i) {
        logKeepIdx_ = i;
        refreshSelection();
        applyLogRotation();
    });
    for (int i = 0; i < kLogRotationSteps; ++i)
        rowLogKeep_->addButton(std::make_unique<Button>(
            std::to_string(logRotationKeepAt(i)), font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{86.f, 40.f}, 18));

    rowColliders_ = addToggle(Str::LabelShowColliders, [this](bool v) {
        showColliders_ = v;
        refreshSelection();
        prefs_->setBool(ConfigKey::kShowColliders, v);
    });

    openConfigButton_ =
        std::make_unique<Button>(Str::ButtonOpenConfigDir, font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{190.f, 40.f}, 17);
    openLogButton_ =
        std::make_unique<Button>(Str::ButtonOpenLogDir, font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{190.f, 40.f}, 17);
    exportButton_ =
        std::make_unique<Button>(Str::ButtonExportSettings, font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{150.f, 40.f}, 17);
    importButton_ =
        std::make_unique<Button>(Str::ButtonImportSettings, font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{150.f, 40.f}, 17);

    // 分享码最长能到几千字符（60 多项设置 base64 之后），maxLength 给足。
    // 框里显示的就是当前可导出的码 / 待导入的码，剪贴板只是方便路径。
    codeInput_ = std::make_unique<TextInput>(font_, sf::Vector2f{0.f, 0.f},
                                             sf::Vector2f{420.f, 40.f},
                                             Str::ShareCodePlaceholder, 15, 8192);

    loadFromPrefs();
    refreshLabels();
    refreshSelection();
}

void AdvancedTab::loadFromPrefs() {
    logLevel_ = prefs_->getInt(ConfigKey::kLogLevel, static_cast<int>(LogLevel::Info));
    logRotateIdx_ = clampLogRotationIndex(prefs_->getInt(ConfigKey::kLogRotate, 0));
    logKeepIdx_ = clampLogKeepIndex(prefs_->getInt(ConfigKey::kLogKeep, 1));
    showColliders_ = prefs_->getBool(ConfigKey::kShowColliders, false);
}

void AdvancedTab::applyLogRotation() {
    logger_->setRotation(logRotationSizeAt(logRotateIdx_),
                         logRotationKeepAt(logKeepIdx_));
    prefs_->setInt(ConfigKey::kLogRotate, logRotateIdx_);
    prefs_->setInt(ConfigKey::kLogKeep, logKeepIdx_);
}

void AdvancedTab::doExport() {
    const std::string code = SettingsCodec::encode(prefs_->exportPortable());
    if (code.empty()) {
        status_ = Str::T(Str::ShareExportFailed);
        return;
    }
    codeInput_->setText(code);

    // 剪贴板是"顺手"路径：某些环境（无剪贴板服务）拿不到也无所谓，
    // 文本框里已经有码，用户可以自己选中复制。
    sf::Clipboard::setString(sf::String::fromUtf8(code.begin(), code.end()));

    status_ = std::string(Str::T(Str::ShareExported)) + " (" +
              std::to_string(ConfigKey::portableKeyCount()) + ")";
}

void AdvancedTab::doImport() {
    const std::string decoded = SettingsCodec::decode(codeInput_->text());
    if (decoded.empty()) {
        status_ = Str::T(Str::ShareImportInvalid);
        return;
    }

    const int applied = prefs_->applyPortable(decoded);
    if (applied <= 0) {
        // 码本身是合法的，但里面一条可携带的键都没有 —— 多半是把别的
        // 游戏的码或者空设置码粘进来了，说清楚比"导入成功 0 项"友好
        status_ = Str::T(Str::ShareImportEmpty);
        return;
    }

    status_ = std::string(Str::T(Str::ShareImported)) + " " + std::to_string(applied) +
              " " + Str::T(Str::ShareImportedUnit);
    // 一批设置在启动阶段就固化了（分辨率、字体缩放、着色器…），
    // 和「恢复默认设置」一样走"退出后下次启动生效"。
    restartRequested_ = true;

    // 把界面上的选中态同步到刚导入的值
    loadFromPrefs();
    refreshSelection();
}

bool AdvancedTab::consumeRestartRequest() {
    const bool r = restartRequested_;
    restartRequested_ = false;
    return r;
}

void AdvancedTab::refreshLabels() {
    for (auto& row : toggles_) {
        row->label.setString(toSf(Str::T(row->labelKey.c_str())));
        row->onButton->setText(Str::T(Str::On));
        row->offButton->setText(Str::T(Str::Off));
    }
    for (auto& row : multiRows_)
        row->refreshLabel();

    labelLog_.setString(toSf(Str::T(Str::LabelAdvancedLog)));
    labelOpen_.setString(toSf(Str::T(Str::LabelAdvancedFiles)));
    labelShare_.setString(toSf(Str::T(Str::LabelShareSettings)));
    hintText_.setString(toSf(Str::T(Str::HintAdvanced)));
    codeInput_->setPlaceholder(Str::ShareCodePlaceholder);
    if (status_.empty())
        labelCode_.setString(toSf(Str::T(Str::LabelShareCode)));
}

void AdvancedTab::refreshSelection() {
    if (rowColliders_) {
        rowColliders_->currentValue = showColliders_;
        rowColliders_->onButton->setSelected(showColliders_);
        rowColliders_->offButton->setSelected(!showColliders_);
    }
    if (rowLogLevel_)
        rowLogLevel_->setSelected(indexOfLevel(logLevel_));
    if (rowLogRotate_)
        rowLogRotate_->setSelected(logRotateIdx_);
    if (rowLogKeep_)
        rowLogKeep_->setSelected(logKeepIdx_);
}

void AdvancedTab::handleEvent(const sf::Event& ev) {
    for (auto& row : toggles_) {
        row->onButton->handleEvent(ev);
        row->offButton->handleEvent(ev);
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            btn->handleEvent(ev);
    openConfigButton_->handleEvent(ev);
    openLogButton_->handleEvent(ev);
    exportButton_->handleEvent(ev);
    importButton_->handleEvent(ev);
    codeInput_->handleEvent(ev);
}

void AdvancedTab::update() {
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

    if (openConfigButton_->consumeClick()) {
        const bool ok = Platform::openDirectory(paths_->configDir());
        status_ = ok ? Str::T(Str::ShareOpenedDir) : Str::T(Str::ShareOpenDirFailed);
    }
    if (openLogButton_->consumeClick()) {
        // 日志就落在配置目录里（log/bootstrap.cpp 用的 configDir()/app.log）
        const bool ok = Platform::openDirectory(paths_->configDir());
        status_ = ok ? Str::T(Str::ShareOpenedDir) : Str::T(Str::ShareOpenDirFailed);
    }
    if (exportButton_->consumeClick())
        doExport();
    if (importButton_->consumeClick())
        doImport();
}

void AdvancedTab::registerFocus(std::vector<Button*>& out) {
    for (auto& row : toggles_) {
        out.push_back(row->onButton.get());
        out.push_back(row->offButton.get());
    }
    for (auto& row : multiRows_)
        for (auto& btn : row->buttons)
            out.push_back(btn.get());
    out.push_back(openConfigButton_.get());
    out.push_back(openLogButton_.get());
    out.push_back(exportButton_.get());
    out.push_back(importButton_.get());
}

bool AdvancedTab::anyEditing() const {
    return codeInput_ && codeInput_->isFocused();
}

float AdvancedTab::render(sf::RenderTarget& target, float contentX, float ctrlX,
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

    // 页首说明：这一页是给排查问题用的，普通玩家不用动
    hintText_.setPosition({contentX, y});
    target.draw(hintText_);
    y += 30.f;

    if (rowLogLevel_)
        drawMulti(*rowLogLevel_);
    if (rowLogRotate_)
        drawMulti(*rowLogRotate_);
    if (rowLogKeep_)
        drawMulti(*rowLogKeep_);
    if (rowColliders_)
        drawToggle(*rowColliders_);

    // ---- 打开目录 ----
    if (openConfigButton_ && openLogButton_) {
        labelOpen_.setPosition({contentX, y + 8.f});
        target.draw(labelOpen_);
        openConfigButton_->setPosition({ctrlX, y});
        openLogButton_->setPosition({ctrlX + 200.f, y});
        openConfigButton_->render(target);
        openLogButton_->render(target);
        y += kRowH;
    }

    // ---- 导出 / 导入 ----
    if (exportButton_ && importButton_) {
        labelShare_.setPosition({contentX, y + 8.f});
        target.draw(labelShare_);
        exportButton_->setPosition({ctrlX, y});
        importButton_->setPosition({ctrlX + 160.f, y});
        exportButton_->render(target);
        importButton_->render(target);
        y += kRowH;
    }

    // ---- 分享码文本框 ----
    if (codeInput_) {
        labelCode_.setPosition({contentX, y + 8.f});
        target.draw(labelCode_);
        codeInput_->setPosition({ctrlX, y});
        codeInput_->setSize({420.f, 40.f});
        codeInput_->render(target);
        y += kRowH + 6.f;
    }

    // ---- 上次操作的结果 ----
    if (!status_.empty()) {
        statusText_.setString(toSf(status_));
        statusText_.setPosition({contentX, y});
        target.draw(statusText_);
        y += 30.f;
    }

    return y + 20.f;
}

void AdvancedTab::reapply() {
    loadFromPrefs();
    applyLogRotation(); // 日志轮转要立刻作用到 Logger 上
    refreshSelection();
}
