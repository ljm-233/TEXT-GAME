#include "stats_scene.h"
#include "focus_group.h"
#include "theme.h"
#include "ui_scale.h"
#include "utils/lang.h"
#include "utils/text_strings.h"
#include "utils/utf8.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

// 时间显示与游戏里既有的一致：LevelSelect 的每关 PB 与完成 overlay 都是两位小数
std::string fmtTime(float seconds) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2fs", seconds);
    return buf;
}

// 三星串：与 LevelSelect / 完成 overlay 同一种字符（实心 / 空心）
std::string fmtStars(int stars) {
    std::string out;
    for (int i = 0; i < 3; ++i)
        out += (i < stars) ? "\u2605" : "\u2606";
    return out;
}

void centerOn(sf::Text& t, float x, float y) {
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    t.setPosition({x, y});
}

void leftAt(sf::Text& t, float x, float y) {
    t.setOrigin({0.f, 0.f});
    t.setPosition({x, y});
}

// 表格几何
constexpr float kColW = 420.f;    // 一栏的理想宽度
constexpr float kColMinW = 240.f; // 再窄四列就挤了 —— 换栏而不是继续压
constexpr float kColGap = 20.f;
constexpr float kPaneGap = 24.f;
// 四列的横向位置是栏宽的**比例**而不是固定像素：栏窄了列间距跟着缩，
// 否则「最佳金币」会跑到下一栏里去。420 时正好是 80 / 189 / 332。
constexpr float kRatioStars = 0.19f;
constexpr float kRatioTime = 0.45f;
constexpr float kRatioCoins = 0.79f;

} // namespace

StatsScene::StatsScene(std::shared_ptr<Background> background,
                       std::shared_ptr<SaveManager> saveManager, const sf::Font& font,
                       std::shared_ptr<Logger> logger)
      : background_(std::move(background)),
        saveManager_(std::move(saveManager)),
        logger_(std::move(logger)),
        font_(font),
        titleText_(font, sf::String(), scaledFontSize(44)),
        saveText_(font, sf::String(), scaledFontSize(22)),
        emptyText_(font, sf::String(), scaledFontSize(24)),
        noRecordText_(font, sf::String(), scaledFontSize(24)),
        summaryText_(font, sf::String(), scaledFontSize(22)),
        progressText_(font, sf::String(), scaledFontSize(20)),
        colLevelText_(font, sf::String(), scaledFontSize(20)),
        colStarsText_(font, sf::String(), scaledFontSize(20)),
        colTimeText_(font, sf::String(), scaledFontSize(20)),
        colCoinsText_(font, sf::String(), scaledFontSize(20)) {
    titleText_.setFillColor(getTheme().textPrimary);
    saveText_.setFillColor(getTheme().textSecondary);
    emptyText_.setFillColor(getTheme().textSecondary);
    noRecordText_.setFillColor(getTheme().textSecondary);
    summaryText_.setFillColor(getTheme().textPrimary);
    summaryText_.setLineSpacing(1.35f);
    progressText_.setFillColor(getTheme().textPrimary);
    for (auto* t : {&colLevelText_, &colStarsText_, &colTimeText_, &colCoinsText_})
        t->setFillColor(getTheme().textSecondary);

    prevSaveButton_ =
        std::make_unique<Button>(Str::T(Str::StatsPrev), font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{48.f, 44.f}, scaledFontSize(22));
    nextSaveButton_ =
        std::make_unique<Button>(Str::T(Str::StatsNext), font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{48.f, 44.f}, scaledFontSize(22));
    backButton_ =
        std::make_unique<Button>(Str::T(Str::Back), font_, sf::Vector2f{0.f, 0.f},
                                 sf::Vector2f{180.f, 52.f}, scaledFontSize(22));
}

std::string StatsScene::windowTitleHint() const {
    return Str::T(Str::Stats);
}

void StatsScene::reload() {
    // listSaves() 已经按 lastPlayed 降序排好，第一个就是"最近玩的"——
    // 不需要在这里再比一遍时间戳。
    saves_ = saveManager_->listSaves();

    if (saves_.empty()) {
        index_ = -1;
        summary_ = {};
    } else {
        if (index_ < 0 || index_ >= static_cast<int>(saves_.size()))
            index_ = 0;
        summary_ = Stats::summarize(saves_[static_cast<std::size_t>(index_)]);
    }

    ensureRowTexts();
    refreshLabels();
    syncFocus();
}

void StatsScene::ensureRowTexts() {
    const auto need = summary_.levels.size();
    if (levelTexts_.size() >= need)
        return;

    const unsigned rowFont = scaledFontSize(18);
    const unsigned n = static_cast<unsigned>(need - levelTexts_.size());
    for (unsigned i = 0; i < n; ++i) {
        levelTexts_.push_back(std::make_unique<sf::Text>(font_, sf::String(), rowFont));
        starTexts_.push_back(std::make_unique<sf::Text>(font_, sf::String(), rowFont));
        timeTexts_.push_back(std::make_unique<sf::Text>(font_, sf::String(), rowFont));
        coinTexts_.push_back(std::make_unique<sf::Text>(font_, sf::String(), rowFont));
    }
}

void StatsScene::refreshLabels() {
    lastLangVersion_ = Lang::instance().version();

    titleText_.setString(toSf(Str::T(Str::Stats)));
    backButton_->setText(Str::T(Str::Back));

    if (index_ < 0) {
        saveText_.setString(sf::String());
        emptyText_.setString(toSf(Str::T(Str::NoSaveHint)));
        noRecordText_.setString(sf::String());
        summaryText_.setString(sf::String());
        progressText_.setString(sf::String());
        return;
    }
    emptyText_.setString(sf::String());

    std::string name =
        Str::T(Str::SaveLabel) + saves_[static_cast<std::size_t>(index_)].name;
    if (saves_.size() > 1) {
        name += "  (" + std::to_string(index_ + 1) + "/" + std::to_string(saves_.size()) +
                ")";
    }
    saveText_.setString(toSf(name));

    // ---- 总计（全部来自 summarize，不在这里重算） ----
    const auto& s = summary_;
    const std::string sep = Str::T(Str::AchvProgressSeparator); // " / "
    std::string sum;
    sum += Str::T(Str::StatsSummaryCleared) + std::to_string(s.clearedLevels) + sep +
           std::to_string(s.levels.size()) + "\n";
    sum += Str::T(Str::StatsSummaryStars) + std::to_string(s.totalStars) + sep +
           std::to_string(s.maxStars) + "\n";
    sum += Str::T(Str::StatsSummaryTime) + fmtTime(s.totalBestTime) + "\n";
    sum += Str::T(Str::StatsSummaryCoins) + std::to_string(s.totalBestCoins);
    summaryText_.setString(toSf(sum));

    const int pct = static_cast<int>(std::lround(s.progress() * 100.f));
    progressText_.setString(toSf(Str::T(Str::StatsProgress) + std::to_string(pct) + "%"));

    noRecordText_.setString(s.anyRecord ? sf::String()
                                        : toSf(Str::T(Str::StatsNoRecord)));

    // ---- 表头 ----
    colLevelText_.setString(toSf(Str::T(Str::StatsColLevel)));
    colStarsText_.setString(toSf(Str::T(Str::StatsColStars)));
    colTimeText_.setString(toSf(Str::T(Str::StatsColTime)));
    colCoinsText_.setString(toSf(Str::T(Str::StatsColCoins)));

    // ---- 每关一行 ----
    const sf::Color primary = getTheme().textPrimary;
    const sf::Color secondary = getTheme().textSecondary;
    const sf::Color accent = getTheme().buttonSelected;

    for (std::size_t i = 0; i < s.levels.size(); ++i) {
        const auto& row = s.levels[i];

        levelTexts_[i]->setString(toSf(std::to_string(row.level)));
        starTexts_[i]->setString(toSf(fmtStars(row.stars)));
        // 没有记录显示 "--"，别让人把 0.00s 当成"打过但零秒"
        timeTexts_[i]->setString(
            toSf(row.bestTime > 0.f ? fmtTime(row.bestTime) : Str::T(Str::StatsEmpty)));
        coinTexts_[i]->setString(toSf(row.bestCoins > 0 ? std::to_string(row.bestCoins)
                                                        : Str::T(Str::StatsEmpty)));

        const sf::Color c = row.cleared ? primary : secondary;
        levelTexts_[i]->setFillColor(c);
        timeTexts_[i]->setFillColor(c);
        coinTexts_[i]->setFillColor(c);
        starTexts_[i]->setFillColor(row.stars > 0 ? accent : secondary);
    }
}

void StatsScene::selectSave(int i) {
    const int n = static_cast<int>(saves_.size());
    if (n <= 0)
        return;
    index_ = ((i % n) + n) % n; // 环绕
    reload();                   // index_ 合法，reload 不会改它
}

void StatsScene::syncFocus() {
    std::vector<Button*> items;
    if (saves_.size() > 1) {
        items.push_back(prevSaveButton_.get());
        items.push_back(nextSaveButton_.get());
    }
    items.push_back(backButton_.get());
    FocusGroup::instance().setItems(items);
}

void StatsScene::onEnter() {
    nextScene_ = SceneId::None;
    index_ = -1; // 每次从主菜单进来都回到"最近玩的存档"
    reload();
}

void StatsScene::onResume() {
    nextScene_ = SceneId::None;
    reload(); // 期间可能玩过一关 / 删过存档
}

void StatsScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
    }

    // 只有一个存档时切换按钮不画也不收事件（位置还是 (0,0)，收点击会误触）
    if (saves_.size() > 1) {
        prevSaveButton_->handleEvent(event);
        nextSaveButton_->handleEvent(event);
    }
    backButton_->handleEvent(event);
}

void StatsScene::update(float /*dt*/) {
    if (Lang::instance().version() != lastLangVersion_)
        refreshLabels();

    if (saves_.size() > 1) {
        if (prevSaveButton_->consumeClick())
            selectSave(index_ - 1);
        if (nextSaveButton_->consumeClick())
            selectSave(index_ + 1);
    }

    if (backButton_->consumeClick())
        nextScene_ = SceneId::Back;
}

void StatsScene::render(Window& window) {
    window.clear();
    if (background_)
        background_->render(window.target());

    auto size = window.native().getSize();
    const float w = static_cast<float>(size.x);
    const float h = static_cast<float>(size.y);
    const auto& theme = getTheme();

    // ⚠️ 纵向布局按**实测文字高度**往下推，不写死 y —— 字号走 scaledFontSize，
    //    跟随 fontScale；写死的话大字号下总计会和进度条 / 表头叠在一起。
    const sf::Vector2f backSize = backButton_->size();
    backButton_->setPosition({w / 2.f - backSize.x / 2.f, h - 20.f - backSize.y});

    float y = 18.f;
    {
        auto b = titleText_.getLocalBounds();
        centerOn(titleText_, w / 2.f, y + b.size.y / 2.f);
        window.target().draw(titleText_);
        y += b.size.y + 14.f;
    }

    // ---- 存档名 + 上一个 / 下一个 ----
    const float saveRowH =
        std::max(prevSaveButton_->size().y, saveText_.getLocalBounds().size.y);
    if (saves_.size() > 1) {
        prevSaveButton_->setPosition({w / 2.f - 250.f, y});
        nextSaveButton_->setPosition({w / 2.f + 202.f, y});
        prevSaveButton_->render(window.target());
        nextSaveButton_->render(window.target());
    }
    centerOn(saveText_, w / 2.f, y + saveRowH / 2.f);
    window.target().draw(saveText_);
    y += saveRowH + 16.f;

    if (index_ < 0) {
        centerOn(emptyText_, w / 2.f, y + (h - 40.f - backSize.y - y) / 2.f);
        window.target().draw(emptyText_);
        backButton_->render(window.target());
        return;
    }

    // 标题 / 存档行以下是左右两栏：左栏放总计与进度条，右栏放表格。
    // 竖着堆的话 800x480 这种小窗口里表格只剩十几像素（9 关都放不下）。
    const float margin = 24.f;
    const float leftW = std::clamp(w * 0.28f, 200.f, 320.f);
    const float tableAvailW = std::max(240.f, w - margin * 2.f - leftW - kPaneGap);
    const float tableBottom = h - 20.f - backSize.y - 20.f;
    const float paneTop = y;

    const auto& rows = summary_.levels;
    const int n = static_cast<int>(rows.size());

    const float headerH = colLevelText_.getLocalBounds().size.y;
    const float tableTop = paneTop + headerH + 10.f;
    const float rowsH = std::max(20.f, tableBottom - tableTop);

    // 先把整块版面居中：知道了栏数才能知道总宽，所以先算栏数、再定 x
    const float rowH = (n > 0 ? levelTexts_[0]->getLocalBounds().size.y : 18.f) + 12.f;
    const int perColFit = std::max(1, static_cast<int>(rowsH / rowH));
    const int colsByHeight = std::max(1, (n + perColFit - 1) / perColFit);
    const int maxCols =
        std::max(1, static_cast<int>((tableAvailW + kColGap) / (kColMinW + kColGap)));
    const int cols = std::min(colsByHeight, maxCols);
    const int perCol = std::max(1, (n + cols - 1) / cols);
    // 栏数被宽度卡住时行高才会被压（关数极多的小窗口）。
    // ponytail: 那种情况会挤成一团，真到那一步再上滚动。
    const float rowStep = std::min(rowH, rowsH / static_cast<float>(perCol));
    const float colW =
        std::min(kColW, (tableAvailW - static_cast<float>(cols - 1) * kColGap) / cols);
    const float totalW = cols * colW + static_cast<float>(cols - 1) * kColGap;
    const float blockW = leftW + kPaneGap + totalW;
    const float startX = (w - blockW) / 2.f;
    const float tableX = startX + leftW + kPaneGap;

    const float offStars = colW * kRatioStars;
    const float offTime = colW * kRatioTime;
    const float offCoins = colW * kRatioCoins;

    // ---- 左栏：总计 + 进度条 ----
    leftAt(summaryText_, startX, paneTop);
    window.target().draw(summaryText_);

    float ly = paneTop + summaryText_.getLocalBounds().size.y + 14.f;
    {
        const auto b = progressText_.getLocalBounds();
        leftAt(progressText_, startX, ly);
        window.target().draw(progressText_);
        ly += b.size.y + 8.f;

        const float barH = std::max(12.f, b.size.y * 0.6f);
        progressBg_.setPosition({startX, ly});
        progressBg_.setSize({leftW, barH});
        progressBg_.setFillColor(theme.buttonNormal);
        progressBg_.setOutlineThickness(1.f);
        progressBg_.setOutlineColor(theme.outline);
        window.target().draw(progressBg_);

        const float fillW = leftW * std::clamp(summary_.progress(), 0.f, 1.f);
        if (fillW > 0.f) {
            progressFill_.setPosition({startX, ly});
            progressFill_.setSize({fillW, barH});
            progressFill_.setFillColor(theme.buttonSelected);
            window.target().draw(progressFill_);
        }
    }

    // ---- 右栏：表格 ----
    // 关数少时面板跟着内容收（不然一大块空面板看着像加载失败）
    const float panelH =
        std::min(tableBottom - paneTop + 16.f,
                 headerH + 10.f + static_cast<float>(perCol) * rowStep + 8.f);
    panel_.setPosition({tableX - 16.f, paneTop - 8.f});
    panel_.setSize({totalW + 32.f, panelH});
    panel_.setFillColor(theme.panelBg);
    panel_.setOutlineThickness(1.f);
    panel_.setOutlineColor(theme.outline);
    window.target().draw(panel_);

    if (!summary_.anyRecord) {
        // 全新存档：一句提示，而不是一屏空格子
        centerOn(noRecordText_, tableX + totalW / 2.f, tableTop + rowsH / 2.f);
        window.target().draw(noRecordText_);
        backButton_->render(window.target());
        return;
    }

    for (int c = 0; c < cols; ++c) {
        const float x = tableX + static_cast<float>(c) * (colW + kColGap);
        leftAt(colLevelText_, x, paneTop);
        leftAt(colStarsText_, x + offStars, paneTop);
        leftAt(colTimeText_, x + offTime, paneTop);
        leftAt(colCoinsText_, x + offCoins, paneTop);
        window.target().draw(colLevelText_);
        window.target().draw(colStarsText_);
        window.target().draw(colTimeText_);
        window.target().draw(colCoinsText_);
    }

    for (int i = 0; i < n; ++i) {
        const int col = i / perCol;
        const int row = i % perCol;
        const float x = tableX + static_cast<float>(col) * (colW + kColGap);
        const float rowY = tableTop + static_cast<float>(row) * rowStep;

        leftAt(*levelTexts_[static_cast<std::size_t>(i)], x, rowY);
        leftAt(*starTexts_[static_cast<std::size_t>(i)], x + offStars, rowY);
        leftAt(*timeTexts_[static_cast<std::size_t>(i)], x + offTime, rowY);
        leftAt(*coinTexts_[static_cast<std::size_t>(i)], x + offCoins, rowY);

        window.target().draw(*levelTexts_[static_cast<std::size_t>(i)]);
        window.target().draw(*starTexts_[static_cast<std::size_t>(i)]);
        window.target().draw(*timeTexts_[static_cast<std::size_t>(i)]);
        window.target().draw(*coinTexts_[static_cast<std::size_t>(i)]);
    }

    backButton_->render(window.target());
}
