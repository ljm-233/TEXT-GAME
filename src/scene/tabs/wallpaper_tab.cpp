#include "scene/tabs/wallpaper_tab.h"

#include "config/keys.h"
#include "theme.h"
#include "ui_scale.h"
#include "utils/text_strings.h"
#include "utils/utf8.h"

#include <algorithm>
#include <string>

// 行布局常量统一放在 wallpaper_tab.h 的 wallpaper_tab_layout 里 ——
// 冒烟测试要按同样的坐标模拟点击，不能各写一份。
using namespace wallpaper_tab_layout;

WallpaperTab::WallpaperTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                           std::shared_ptr<Background> background,
                           std::shared_ptr<WallpaperLibrary> library,
                           std::shared_ptr<WallpaperLoader> loader)
      : font_(font),
        prefs_(std::move(prefs)),
        background_(std::move(background)),
        library_(std::move(library)),
        loader_(std::move(loader)),
        hint_(font, sf::String(), scaledFontSize(18)) {
    hint_.setFillColor(sf::Color(180, 180, 200));

    if (!library_)
        return;

    // 每张壁纸一个行按钮。名字用文件名 —— 壁纸没有别的元数据可显示，
    // 而文件名正是 current_wallpaper 里存的那个东西，对得上号最重要。
    const int n = library_->size();
    rowButtons_.reserve(n);
    thumbs_.resize(n);
    for (int i = 0; i < n; ++i) {
        rowButtons_.push_back(std::make_unique<Button>(library_->at(i).filename, font_,
                                                       sf::Vector2f{0.f, 0.f},
                                                       sf::Vector2f{kBtnW, kBtnH}, 18));
    }

    refreshLabels();
}

void WallpaperTab::refreshLabels() {
    if (!library_) {
        hint_.setString(toSf(std::string(Str::T(Str::WallpaperHint))));
        return;
    }

    // 把"共几张、当前第几张"和操作提示放一行，省一行空间
    std::string text = Str::T(Str::WallpaperHint);
    if (!library_->empty()) {
        const int cur = background_ ? background_->currentIndex() : -1;
        text += "    (" + std::to_string(cur >= 0 ? cur + 1 : 0) + "/" +
                std::to_string(library_->size()) + ")";
    }
    hint_.setString(toSf(text));
}

void WallpaperTab::ensureThumbnails() {
    if (!loader_ || !library_)
        return;

    for (int i = 0; i < library_->size(); ++i) {
        Thumb& t = thumbs_[static_cast<std::size_t>(i)];
        if (t.tex)
            continue; // 已经就位

        // 后台线程逐个产出缩略图，没好的下一次再来问
        auto img = loader_->thumbnail(i);
        if (!img || img->getSize().x == 0)
            continue;

        t.tex = std::make_unique<sf::Texture>();
        if (!t.tex->loadFromImage(*img)) {
            t.tex.reset();
            continue; // 这次没成功，下次重试
        }
        t.tex->setSmooth(true);
        // sprite 建在纹理之后，且纹理由 unique_ptr 持有（地址稳定）
        t.sprite = std::make_unique<sf::Sprite>(*t.tex);
    }
}

void WallpaperTab::syncSelection() {
    const int cur = background_ ? background_->currentIndex() : -1;
    for (std::size_t i = 0; i < rowButtons_.size(); ++i)
        rowButtons_[i]->setSelected(static_cast<int>(i) == cur);
}

void WallpaperTab::layoutRows() {
    if (!laidOut_)
        return;

    float y = originY_;
    for (auto& b : rowButtons_) {
        b->setPosition({originX_ + kThumbW + kGap, y + (kRowH - kBtnH) * 0.5f});
        y += kRowH;
    }
}

void WallpaperTab::handleEvent(const sf::Event& ev) {
    for (auto& b : rowButtons_)
        b->handleEvent(ev);
}

void WallpaperTab::applySelection(int index) {
    if (!library_ || !background_)
        return;
    const WallpaperInfo info = library_->at(index);
    if (!info.valid())
        return;

    // loadByName 内部会做淡入；成功之后 currentFile() **立刻**就是新名字
    // （选中项与显示项是分开的，见 background.h），所以这里存的就是用户
    // 刚点的那张，不会存到旧名字上。
    if (!background_->loadByName(info.filename))
        return; // 加载失败就别动配置，免得重启后指向一张打不开的图

    if (prefs_)
        prefs_->set(ConfigKey::kCurrentWallpaper, background_->currentFile());
    refreshLabels();
}

void WallpaperTab::update() {
    ensureThumbnails();
    syncSelection();

    for (std::size_t i = 0; i < rowButtons_.size(); ++i) {
        if (rowButtons_[i]->consumeClick())
            applySelection(static_cast<int>(i));
    }
}

void WallpaperTab::registerFocus(std::vector<Button*>& out) {
    // 焦点导航按按钮位置算，所以这里必须先把位置落好 ——
    // 切 Tab 时的 syncFocus 发生在本页第一次 render **之前**。
    layoutRows();
    for (auto& b : rowButtons_)
        out.push_back(b.get());
}

void WallpaperTab::refreshSelection() {
    syncSelection();
}

float WallpaperTab::render(sf::RenderTarget& target, float contentX, float startY) {
    originX_ = contentX;
    originY_ = startY + 34.f; // 提示行占掉一行
    laidOut_ = true;
    layoutRows();

    hint_.setPosition({contentX, startY});
    target.draw(hint_);

    if (!library_ || library_->empty()) {
        return originY_ + 40.f;
    }

    const Theme& th = getTheme();
    const int cur = background_ ? background_->currentIndex() : -1;

    float y = originY_;
    for (int i = 0; i < library_->size(); ++i) {
        const float rowY = y;
        const Thumb& t = thumbs_[static_cast<std::size_t>(i)];

        // ---- 缩略图（等比 contain 放进 kThumbW x kThumbH 的框里）----
        if (t.sprite && t.tex) {
            const auto ts = t.tex->getSize();
            if (ts.x > 0 && ts.y > 0) {
                const float scale = std::min(kThumbW / static_cast<float>(ts.x),
                                             kThumbH / static_cast<float>(ts.y));
                t.sprite->setScale({scale, scale});
                t.sprite->setPosition(
                    {contentX + (kThumbW - static_cast<float>(ts.x) * scale) * 0.5f,
                     rowY + (kThumbH - static_cast<float>(ts.y) * scale) * 0.5f});
                target.draw(*t.sprite);
            }
        } else {
            // 还没解码好：画个占位块。注意别在这儿建 sf::Text（会触发
            // HarfBuzz 析构那条坑），纯矩形就够了。
            sf::RectangleShape ph({kThumbW, kThumbH});
            ph.setFillColor(th.panelBg);
            ph.setOutlineThickness(1.f);
            ph.setOutlineColor(th.outline);
            ph.setPosition({contentX, rowY});
            target.draw(ph);
        }

        // ---- 选中项加一圈高亮，和右边按钮的选中态呼应 ----
        if (i == cur) {
            sf::RectangleShape hl({kThumbW, kThumbH});
            hl.setFillColor(sf::Color::Transparent);
            hl.setOutlineThickness(3.f);
            hl.setOutlineColor(th.buttonSelected);
            hl.setPosition({contentX, rowY});
            target.draw(hl);
        }

        y += kRowH;
    }

    // 按钮位置已经在 layoutRows() 里落好，这里只画
    for (auto& b : rowButtons_)
        b->render(target);

    return y + 20.f;
}

void WallpaperTab::reapply() {
    if (!library_ || !background_ || !prefs_)
        return;
    // 键被删掉之后应当回到"列表第一张"（新配置的默认行为）
    const std::string want = prefs_->get(ConfigKey::kCurrentWallpaper, "");
    int idx = library_->resolveIndex(want);
    if (idx < 0 && !library_->empty())
        idx = 0;
    if (idx >= 0)
        background_->loadByName(library_->at(idx).filename);
    refreshLabels();
}
