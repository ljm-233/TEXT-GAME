#include "achievement_scene.h"
#include "text_strings.h"
#include "utf8.h"
#include "theme.h"
#include "achievement.h"
#include "lang.h"
#include "focus_group.h"

AchievementScene::AchievementScene(std::shared_ptr<Background> background,
                                   const sf::Font& font,
                                   std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      font_(font),
      titleText_(font, sf::String(), 48),
      progressText_(font, sf::String(), 22) {

    titleText_.setFillColor(sf::Color(240, 240, 250));
    progressText_.setFillColor(sf::Color(200, 220, 255));

    backButton_ = std::make_unique<Button>(
        Str::T(Str::Back), font, sf::Vector2f{0.f, 0.f},
        sf::Vector2f{180.f, 52.f}, 22);

    // ⭐ 一次性为每个成就创建 Text 对象（避免频繁析构触发 HarfBuzz 死锁）
    const auto& cat = AchievementManager::catalogue();
    nameTexts_.reserve(cat.size());
    descTexts_.reserve(cat.size());
    for (size_t i = 0; i < cat.size(); ++i) {
        auto name = std::make_unique<sf::Text>(font, sf::String(), 20);
        name->setFillColor(sf::Color(240, 240, 250));
        nameTexts_.push_back(std::move(name));

        auto desc = std::make_unique<sf::Text>(font, sf::String(), 14);
        desc->setFillColor(sf::Color(180, 180, 200));
        descTexts_.push_back(std::move(desc));
    }
}

void AchievementScene::onEnter() {
    nextScene_ = SceneId::None;
    lastLangVersion_ = -1;       // ⭐ 强制刷新
    lastUnlockedCount_ = -1;
    refreshLabels();
    syncFocus();
}

void AchievementScene::onResume() {
    nextScene_ = SceneId::None;
    lastLangVersion_ = -1;
    lastUnlockedCount_ = -1;
    refreshLabels();
    syncFocus();
}

void AchievementScene::refreshLabels() {
    auto& am = AchievementManager::instance();

    // 语言版本或解锁数变化才刷新文字
    int v = Lang::instance().version();
    int uc = am.unlockedCount();
    if (v == lastLangVersion_ && uc == lastUnlockedCount_) return;

    lastLangVersion_ = v;
    lastUnlockedCount_ = uc;

    titleText_.setString(toSf(Str::T(Str::AchvTitle)));

    std::string prog = std::string(Str::T(Str::AchvProgressPrefix))
                     + std::to_string(uc)
                     + Str::T(Str::AchvProgressSeparator)
                     + std::to_string(am.totalCount());
    progressText_.setString(toSf(prog));

    const auto& cat = AchievementManager::catalogue();
    for (size_t i = 0; i < cat.size() && i < nameTexts_.size(); ++i) {
        nameTexts_[i]->setString(toSf(Str::T(cat[i].nameKey)));
        descTexts_[i]->setString(toSf(Str::T(cat[i].descKey)));
    }

    backButton_->setText(Str::T(Str::Back));
}

void AchievementScene::syncFocus() {
    FocusGroup::instance().setItems({backButton_.get()});
}

void AchievementScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
    }
    backButton_->handleEvent(event);
}

void AchievementScene::update(float /*dt*/) {
    if (backButton_->consumeClick()) {
        nextScene_ = SceneId::Back;
    }
}

void AchievementScene::render(Window& window) {
    refreshLabels();

    window.clear();
    if (background_) background_->render(window.target());

    auto size = window.native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);
    float cx = w / 2.f;

    // 标题
    {
        auto b = titleText_.getLocalBounds();
        titleText_.setOrigin({b.position.x + b.size.x / 2.f,
                              b.position.y + b.size.y / 2.f});
        titleText_.setPosition({cx, 60.f});
        window.target().draw(titleText_);
    }

    // 进度
    {
        auto b = progressText_.getLocalBounds();
        progressText_.setOrigin({b.position.x + b.size.x / 2.f,
                                 b.position.y + b.size.y / 2.f});
        progressText_.setPosition({cx, 115.f});
        window.target().draw(progressText_);
    }

    // 成就卡片网格
    const auto& cat = AchievementManager::catalogue();
    auto& am = AchievementManager::instance();

    constexpr int   kCols  = 4;
    constexpr float kCardW = 280.f;
    constexpr float kCardH = 140.f;
    constexpr float kGapX  = 20.f;
    constexpr float kGapY  = 20.f;

    float totalW = kCols * kCardW + (kCols - 1) * kGapX;
    float startX = (w - totalW) / 2.f;
    float startY = 160.f;

    for (size_t i = 0; i < cat.size(); ++i) {
        int row = static_cast<int>(i) / kCols;
        int col = static_cast<int>(i) % kCols;
        float x = startX + col * (kCardW + kGapX);
        float y = startY + row * (kCardH + kGapY);

        bool unlocked = am.isUnlocked(cat[i].id);

        // 卡片背景
        sf::RectangleShape card({kCardW, kCardH});
        card.setPosition({x, y});
        card.setFillColor(unlocked ? sf::Color(35, 55, 40, 210)
                                   : sf::Color(28, 28, 38, 210));
        card.setOutlineThickness(2.f);
        card.setOutlineColor(unlocked ? sf::Color(120, 220, 140)
                                      : sf::Color(70, 70, 90));
        window.target().draw(card);

        // 图标：圆形
        float iconR = 28.f;
        sf::CircleShape icon(iconR);
        icon.setOrigin({iconR, iconR});
        icon.setPosition({x + 50.f, y + kCardH / 2.f});
        icon.setFillColor(unlocked ? sf::Color(255, 210, 60)
                                   : sf::Color(60, 60, 80));
        icon.setOutlineThickness(2.f);
        icon.setOutlineColor(unlocked ? sf::Color(180, 140, 30)
                                      : sf::Color(50, 50, 70));
        window.target().draw(icon);

        // 锁标记
        if (!unlocked) {
            sf::RectangleShape body({16.f, 12.f});
            body.setOrigin({8.f, 6.f});
            body.setPosition({x + 50.f, y + kCardH / 2.f + 2.f});
            body.setFillColor(sf::Color(150, 150, 170));
            window.target().draw(body);

            sf::RectangleShape shackle({10.f, 6.f});
            shackle.setOrigin({5.f, 6.f});
            shackle.setPosition({x + 50.f, y + kCardH / 2.f - 6.f});
            shackle.setFillColor(sf::Color::Transparent);
            shackle.setOutlineThickness(2.f);
            shackle.setOutlineColor(sf::Color(150, 150, 170));
            window.target().draw(shackle);
        } else {
            // 解锁标记：中间一个白色实心小方块
            sf::RectangleShape star({10.f, 10.f});
            star.setOrigin({5.f, 5.f});
            star.setPosition({x + 50.f, y + kCardH / 2.f});
            star.setFillColor(sf::Color(255, 255, 220));
            window.target().draw(star);
        }

        // 名字
        nameTexts_[i]->setFillColor(unlocked ? sf::Color(240, 240, 250)
                                              : sf::Color(140, 140, 160));
        nameTexts_[i]->setPosition({x + 100.f, y + 28.f});
        window.target().draw(*nameTexts_[i]);

        // 描述
        descTexts_[i]->setFillColor(unlocked ? sf::Color(180, 210, 190)
                                              : sf::Color(110, 110, 130));
        descTexts_[i]->setPosition({x + 100.f, y + 62.f});
        window.target().draw(*descTexts_[i]);
    }

    // 返回按钮
    backButton_->setPosition({cx - 90.f, h - 80.f});
    backButton_->render(window.target());
}