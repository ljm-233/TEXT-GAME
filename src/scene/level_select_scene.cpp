#include "level_select_scene.h"
#include "strings.h"
#include "utf8.h"
#include "focus_group.h"
#include <algorithm>

LevelSelectScene::LevelSelectScene(std::shared_ptr<Background>  background,
                                   std::shared_ptr<SaveManager> saveManager,
                                   const sf::Font&              font,
                                   std::shared_ptr<Logger>      logger)
    : background_(std::move(background)),
      saveManager_(std::move(saveManager)),
      logger_(std::move(logger)),
      font_(font),
      titleText_(font, toSf(Str::LevelSelectTitle), 48),
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

        std::string line = std::string(Str::SaveLabel) + save_.name;
        saveNameText_.setString(toSf(line));

        logger_->info("关卡选择: 存档=" + save_.filename +
                      " 当前进度=" + std::to_string(currentLevel_));
    } else {
        hasSave_ = false;
        hintText_.setString(toSf(Str::NoSaveHint));
        logger_->warn("关卡选择: 没有存档");
    }

    for (int i = 1; i <= kMaxLevels; ++i) {
        levelButtons_.push_back(std::make_unique<Button>(
            std::to_string(i), font_,
            sf::Vector2f{0.f, 0.f}, sf::Vector2f{120.f, 120.f}, 40));
    }

    backButton_ = std::make_unique<Button>(Str::Back, font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{180.f, 52.f}, 22);

    refreshSelection();
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
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);
    float cx = w / 2.f;

    // ===== 标题 =====
    {
        auto b = titleText_.getLocalBounds();
        titleText_.setOrigin({b.position.x + b.size.x / 2.f,
                              b.position.y + b.size.y / 2.f});
        titleText_.setPosition({cx, 80.f});
        window.native().draw(titleText_);
    }

    // ===== 存档名 =====
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
        // ===== 关卡按钮 =====
        const float btnSize  = 120.f;
        const float gapX     = 30.f;
        const float gapY     = 60.f;   // ⭐ 从 40 加到 60，给星星留位置
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

            // ===== ⭐ 星级显示 =====
            int stars = 0;
            if (i < static_cast<int>(save_.levelStars.size())) {
                stars = save_.levelStars[i];
            }

            int level = i + 1;
            bool unlocked = level <= currentLevel_;

            // 只对已解锁的关卡显示星级
            if (unlocked) {
                std::string starStr;
                for (int s = 0; s < 3; ++s) {
                    starStr += (s < stars) ? "\u2605" : "\u2606";
                }
                starText_.setString(toSf(starStr));

                // 有星的用金色，没星的用灰色
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

    // ===== 返回按钮 =====
    backButton_->setPosition({cx - 90.f, h - 100.f});
    backButton_->render(window.native());

    // ===== 注册焦点 =====
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