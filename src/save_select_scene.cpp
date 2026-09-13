#include "save_select_scene.h"

SaveSelectScene::SaveSelectScene(std::shared_ptr<Background>  background,
                                 std::shared_ptr<SaveManager> saveManager,
                                 const sf::Font&              font,
                                 std::shared_ptr<Logger>      logger)
    : background_(std::move(background)),
      saveManager_(std::move(saveManager)),
      logger_(std::move(logger)),
      font_(font) {

    newButton_  = std::make_unique<Button>("＋ 新建存档", font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{520.f, 60.f}, 26);
    backButton_ = std::make_unique<Button>("返回", font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 50.f}, 22);

    rebuildButtons();
    logger_->info("进入存档选择页，共 " + std::to_string(saves_.size()) + " 个存档");
}

void SaveSelectScene::rebuildButtons() {
    saves_ = saveManager_->listSaves();

    saveButtons_.clear();
    deleteButtons_.clear();

    for (const auto& s : saves_) {
        saveButtons_.push_back(std::make_unique<Button>(
            s.name, font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{440.f, 55.f}, 22));

        deleteButtons_.push_back(std::make_unique<Button>(
            "×", font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{60.f, 55.f}, 26));
    }
}

void SaveSelectScene::handleEvent(const sf::Event& event) {
    // 确认框存在时，事件优先给它
    if (confirm_) {
        confirm_->handleEvent(event);
        return;
    }

    for (auto& b : saveButtons_)   b->handleEvent(event);
    for (auto& b : deleteButtons_) b->handleEvent(event);
    newButton_->handleEvent(event);
    backButton_->handleEvent(event);
}

void SaveSelectScene::update(float /*dt*/) {
    // 确认框处理
    if (confirm_) {
        auto r = confirm_->consumeResult();
        if (r == ConfirmDialog::Result::Yes) {
            if (pendingDeleteIndex_ >= 0 &&
                pendingDeleteIndex_ < static_cast<int>(saves_.size())) {
                saveManager_->deleteSave(saves_[pendingDeleteIndex_].filename);
            }
            confirm_.reset();
            pendingDeleteIndex_ = -1;
            rebuildButtons();
        } else if (r == ConfirmDialog::Result::No) {
            confirm_.reset();
            pendingDeleteIndex_ = -1;
        }
        return;
    }

    // 点击加载存档
    for (size_t i = 0; i < saveButtons_.size(); ++i) {
        if (saveButtons_[i]->consumeClick()) {
            logger_->info("选择存档: " + saves_[i].filename);
            saveManager_->setPendingSave(saves_[i]);
            nextScene_ = SceneId::Game;
            return;
        }
    }

    // 点击删除按钮
    for (size_t i = 0; i < deleteButtons_.size(); ++i) {
        if (deleteButtons_[i]->consumeClick()) {
            pendingDeleteIndex_ = static_cast<int>(i);
            auto size = sf::Vector2f(0.f, 0.f); // 稍后在 relayout 里由窗口尺寸决定
            confirm_ = std::make_unique<ConfirmDialog>(
                font_,
                "确定删除存档「" + saves_[i].name + "」？",
                // 用当前窗口尺寸不合适，用一个大致的默认值，稍后 render 里更新
                sf::Vector2f(1280.f, 720.f));
            logger_->info("请求删除存档: " + saves_[i].filename);
            return;
        }
    }

    if (newButton_->consumeClick()) {
        auto info = saveManager_->createSave();
        saveManager_->setPendingSave(info);
        nextScene_ = SceneId::Game;
        return;
    }

    if (backButton_->consumeClick()) {
        nextScene_ = SceneId::MainMenu;
    }
}

void SaveSelectScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    const float btnW = 440.f;
    const float delW = 60.f;
    const float gap  = 12.f;
    const float gapX = 8.f;
    const float rowW = btnW + gapX + delW;

    float total = saves_.size() * (55.f + gap) + (60.f + gap);
    float startY = (h - total) / 2.f;
    float leftX = (w - rowW) / 2.f;

    for (size_t i = 0; i < saveButtons_.size(); ++i) {
        saveButtons_[i]->setPosition({leftX, startY});
        deleteButtons_[i]->setPosition({leftX + btnW + gapX, startY});
        saveButtons_[i]->render(window.native());
        deleteButtons_[i]->render(window.native());
        startY += 55.f + gap;
    }

    newButton_->setPosition({(w - 520.f) / 2.f, startY});
    newButton_->render(window.native());

    backButton_->setPosition({(w - 160.f) / 2.f, h - 90.f});
    backButton_->render(window.native());

    // 确认框在最后画，覆盖在上面
    if (confirm_) {
        confirm_->relayout({w, h});
        confirm_->render(window.native());
    }

    window.display();
}