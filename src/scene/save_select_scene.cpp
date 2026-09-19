#include "save_select_scene.h"
#include "text_strings.h"
#include "focus_group.h"

SaveSelectScene::SaveSelectScene(std::shared_ptr<Background>  background,
                                 std::shared_ptr<SaveManager> saveManager,
                                 const sf::Font&              font,
                                 std::shared_ptr<Logger>      logger)
    : background_(std::move(background)),
      saveManager_(std::move(saveManager)),
      logger_(std::move(logger)),
      font_(font) {

    newButton_  = std::make_unique<Button>(Str::T(Str::NewSave), font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{520.f, 60.f}, 26);
    backButton_ = std::make_unique<Button>(Str::T(Str::Back), font_,
                        sf::Vector2f{0.f, 0.f}, sf::Vector2f{160.f, 50.f}, 22);

    rebuildButtons();
    logger_->info("进入存档选择页，共 " + std::to_string(saves_.size()) + " 个存档");
}

void SaveSelectScene::rebuildButtons() {
    saves_ = saveManager_->listSaves();

    saveButtons_.clear();
    deleteButtons_.clear();

    for (const auto& s : saves_) {
        // 存档名是用户数据，不翻译
        saveButtons_.push_back(std::make_unique<Button>(
            s.name, font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{440.f, 55.f}, 22));

        // 删除按钮是 ×，不翻译
        deleteButtons_.push_back(std::make_unique<Button>(
            Str::DeleteMark, font_, sf::Vector2f{0.f, 0.f},
            sf::Vector2f{60.f, 55.f}, 26));
    }

    // 刷新固定按钮的文字（语言可能已变）
    newButton_->setText(Str::T(Str::NewSave));
    backButton_->setText(Str::T(Str::Back));
    syncFocus();
}

void SaveSelectScene::syncFocus() {
    if (confirm_ || newSaveDialog_) {
        FocusGroup::instance().clear();
        return;
    }
    std::vector<Button*> items;
    for (auto& b : saveButtons_)   items.push_back(b.get());
    for (auto& b : deleteButtons_) items.push_back(b.get());
    items.push_back(newButton_.get());
    items.push_back(backButton_.get());
    FocusGroup::instance().setItems(items);
}


void SaveSelectScene::onEnter() {
    nextScene_ = SceneId::None;
    syncFocus();
}

void SaveSelectScene::onResume() {
    nextScene_ = SceneId::None;
    // 语言可能变了，重建按钮刷新文字
    rebuildButtons();
}

void SaveSelectScene::handleEvent(const sf::Event& event) {
    if (newSaveDialog_) {
        newSaveDialog_->handleEvent(event);
        return;
    }
    if (confirm_) {
        confirm_->handleEvent(event);
        return;
    }

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
    }

    for (auto& b : saveButtons_)   b->handleEvent(event);
    for (auto& b : deleteButtons_) b->handleEvent(event);
    newButton_->handleEvent(event);
    backButton_->handleEvent(event);
}

void SaveSelectScene::update(float /*dt*/) {
    if (newSaveDialog_) {
        auto r = newSaveDialog_->consumeResult();
        if (r == NewSaveDialog::Result::Created) {
            std::string name = newSaveDialog_->getName();
            if (name.empty()) name = Str::T(Str::NewSavePlaceholder);
            auto info = saveManager_->createSave(name);
            saveManager_->setPendingSave(info);
            newSaveDialog_.reset();
            syncFocus();
            nextScene_ = SceneId::Game;
        } else if (r == NewSaveDialog::Result::Cancelled) {
            newSaveDialog_.reset();
            syncFocus();
        }
        return;
    }

    if (confirm_) {
        auto r = confirm_->consumeResult();
        if (r == ConfirmDialog::Result::Yes) {
            if (pendingDeleteIndex_ >= 0 &&
                pendingDeleteIndex_ < static_cast<int>(saves_.size())) {
                saveManager_->deleteSave(saves_[pendingDeleteIndex_].filename);
            }
            confirm_.reset();
            pendingDeleteIndex_ = -1;
            rebuildButtons();     // ⭐ rebuildButtons 内部已经调 syncFocus
        } else if (r == ConfirmDialog::Result::No) {
            confirm_.reset();
            pendingDeleteIndex_ = -1;
            syncFocus();
        }
        return;
    }

    for (size_t i = 0; i < saveButtons_.size(); ++i) {
        if (saveButtons_[i]->consumeClick()) {
            logger_->info("选择存档: " + saves_[i].filename);
            saveManager_->setPendingSave(saves_[i]);
            nextScene_ = SceneId::Game;
            return;
        }
    }

    for (size_t i = 0; i < deleteButtons_.size(); ++i) {
        if (deleteButtons_[i]->consumeClick()) {
            pendingDeleteIndex_ = static_cast<int>(i);
            std::string msg = Str::T(Str::DeleteConfirmHead) + saves_[i].name
                            + Str::T(Str::DeleteConfirmTail);
            confirm_ = std::make_unique<ConfirmDialog>(
                font_, msg, sf::Vector2f(1280.f, 720.f));
            syncFocus();
            logger_->info("请求删除存档: " + saves_[i].filename);
            return;
        }
    }

    if (newButton_->consumeClick()) {
        std::string defName = Str::T(Str::NewSavePlaceholder);
        newSaveDialog_ = std::make_unique<NewSaveDialog>(
            font_, defName, sf::Vector2f(1280.f, 720.f));
        syncFocus();
        return;
    }

    if (backButton_->consumeClick()) {
        nextScene_ = SceneId::Back;
    }
}

void SaveSelectScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.target());

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
        saveButtons_[i]->render(window.target());
        deleteButtons_[i]->render(window.target());
        startY += 55.f + gap;
    }

    newButton_->setPosition({(w - 520.f) / 2.f, startY});
    newButton_->render(window.target());

    backButton_->setPosition({(w - 160.f) / 2.f, h - 90.f});
    backButton_->render(window.target());

    if (confirm_) {
        confirm_->relayout({w, h});
        confirm_->render(window.target());
    }

    if (newSaveDialog_) {
        newSaveDialog_->relayout({w, h});
        newSaveDialog_->render(window.target());
    }
}