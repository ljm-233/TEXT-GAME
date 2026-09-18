#include "level_select_scene.h"
#include "text_strings.h"
#include "utf8.h"
#include "focus_group.h"
#include <algorithm>
#include <cstdio>

LevelSelectScene::LevelSelectScene(std::shared_ptr<Background>  background,
                                   std::shared_ptr<SaveManager> saveManager,
                                   const sf::Font&              font,
                                   std::shared_ptr<Logger>      logger)
    : background_(std::move(background)),
      saveManager_(std::move(saveManager)),
      logger_(std::move(logger)),
      font_(font),
      titleText_(font, sf::String(), 48),
      saveNameText_(font, sf::String(), 22),
      hintText_(font, sf::String(), 20),
      starText_(font, sf::String(), 22) {

    titleText_.setFillColor(sf::Color(240, 240, 250));
    saveNameText_.setFillColor(sf::Color(200, 220, 255));
    hintText_.setFillColor(sf::Color(255, 180, 80));
    starText_.setFillColor(sf::Color(255, 210, 60));

    auto saves = saveManager_->listSaves();
    if (!saves.empty()) {
        hasSave_ = true;
        save_ = saves[0];
        currentLevel_ = std::max(1, save_.currentLevel);

        logger_->info("关卡选择: 存档=" + save_.filename +
                      " 当前进度=" + std::to_string(currentLevel_));
    } else {
        hasSave_ = false;
        logger_->warn("关卡选择: 没有存档");
    }

    for (int i = 1; i <= kMaxLevels; ++i) {
        levelButtons_.push_back(std::make_unique<Button>(
            std::to_string(i), font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{120.f, 120.f}, 40));
    }

    backButton_ = std::make_unique<Button>(Str::T(Str::Back), font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 52.f}, 22);

    refreshSelection();
}

void LevelSelectScene::onEnter() {
    nextScene_ = SceneId::None;
    syncFocus();
}

void LevelSelectScene::onResume() {
    nextScene_ = SceneId::None;
    syncFocus();
}

void LevelSelectScene::refreshLabels() {
    titleText_.setString(toSf(Str::T(Str::LevelSelectTitle)));
    if (hasSave_) {
        saveNameText_.setString(toSf(Str::T(Str::SaveLabel) + save_.name));
    } else {
        hintText_.setString(toSf(Str::T(Str::NoSaveHint)));
    }
    backButton_->setText(Str::T(Str::Back));
}

void LevelSelectScene::syncFocus() {
    std::vector<Button*> items;
    for (int i = 0; i < kMaxLevels; ++i) {
        int level = i + 1;
        bool unlocked = hasSave_ && level <= currentLevel_;
        if (unlocked) {
            items.push_back(levelButtons_[i].get());
        }
    }
    items.push_back(backButton_.get());
    FocusGroup::instance().setItems(items);
}


void LevelSelectScene::refreshSelection() {
    for (int i = 0; i < kMaxLevels; ++i) {
        int level = i + 1;
        bool unlocked = hasSave_ && level <= currentLevel_;
        if (!unlocked) {
            levelButtons_[i]->setText("—");
        } else {
            levelButtons_[i]->setText(std::to_string(level));
        }
        levelButtons_[i]->setSelected(false);
    }
}

void LevelSelectScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
    }

    for (auto& b : levelButtons_) b->handleEvent(event);
    backButton_->handleEvent(event);
}

void LevelSelectScene::update(float /*dt*/) {
    for (int i = 0; i < kMaxLevels; ++i) {
        if (!levelButtons_[i]->consumeClick()) continue;

        int level = i + 1;
        bool unlocked = hasSave_ && level <= currentLevel_;
        if (!unlocked) continue;

        SaveInfo chosen = save_;
        chosen.currentLevel = level;
        saveManager_->setPendingSave(chosen);

        logger_->info("选关: 进入第 " + std::to_string(level) + " 关");
        nextScene_ = SceneId::Game;
        return;
    }

    if (backButton_->consumeClick()) {
        nextScene_ = SceneId::Back;
    }
}

void LevelSelectScene::render(Window& window) {
    // ⭐ 每帧刷新字符串，保证语言切换后立即生效
    refreshLabels();
    
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);
    float cx = w / 2.f;

    {
        auto b = titleText_.getLocalBounds();
        titleText_.setOrigin({b.position.x + b.size.x / 2.f,
                              b.position.y + b.size.y / 2.f});
        titleText_.setPosition({cx, 80.f});
        window.native().draw(titleText_);
    }

    {
        auto b = saveNameText_.getLocalBounds();
        saveNameText_.setOrigin({b.position.x + b.size.x / 2.f,
                                 b.position.y + b.size.y / 2.f});
        saveNameText_.setPosition({cx, 150.f});
        window.native().draw(saveNameText_);
    }

    if (!hasSave_) {
        auto b = hintText_.getLocalBounds();
        hintText_.setOrigin({b.position.x + b.size.x / 2.f,
                             b.position.y + b.size.y / 2.f});
        hintText_.setPosition({cx, h / 2.f});
        window.native().draw(hintText_);
    } else {
        const float btnSize  = 120.f;
        const float gapX     = 30.f;
        const float gapY     = 60.f;
        const int   perRow   = 5;

        for (int i = 0; i < kMaxLevels; ++i) {
            int row = i / perRow;
            int col = i % perRow;

            int colsInThisRow = std::min(perRow, kMaxLevels - row * perRow);
            float rowW = colsInThisRow * btnSize + (colsInThisRow - 1) * gapX;
            float startX = cx - rowW / 2.f;

            float x = startX + col * (btnSize + gapX);
            float y = h / 2.f - 60.f + row * (btnSize + gapY);

            levelButtons_[i]->setPosition({x, y});
            levelButtons_[i]->render(window.native());

            int stars = 0;
            if (i < static_cast<int>(save_.levelStars.size())) {
                stars = save_.levelStars[i];
            }

            int level = i + 1;
            bool unlocked = level <= currentLevel_;

            if (unlocked) {
                std::string starStr;
                for (int s = 0; s < 3; ++s) {
                    starStr += (s < stars) ? "\u2605" : "\u2606";
                }

                // ⭐ 如果有关卡 PB，显示在星星后面
                if (i < static_cast<int>(save_.levelBestTimes.size()) &&
                    save_.levelBestTimes[i] > 0.f) {
                    char buf[32];
                    std::snprintf(buf, sizeof(buf), "  %.2fs",
                                  save_.levelBestTimes[i]);
                    starStr += buf;
                }

                starText_.setString(toSf(starStr));

                if (stars > 0) {
                    starText_.setFillColor(sf::Color(255, 210, 60));
                } else {
                    starText_.setFillColor(sf::Color(120, 120, 130));
                }

                auto sb = starText_.getLocalBounds();
                starText_.setOrigin({sb.position.x + sb.size.x / 2.f,
                                     sb.position.y});
                starText_.setPosition({
                    x + btnSize * 0.5f,
                    y + btnSize + 6.f
                });
                window.native().draw(starText_);
            }
        }
    }

    backButton_->setPosition({cx - 90.f, h - 100.f});
    backButton_->render(window.native());
}