#include "save_select_scene.h"

SaveSelectScene::SaveSelectScene(std::shared_ptr<Background> background,
                                 std::shared_ptr<SaveManager> saveManager,
                                 const sf::Font& font,
                                 std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      saveManager_(std::move(saveManager)),
      logger_(std::move(logger)),
      font_(font) {

    saves_ = saveManager_->listSaves();

    // 为每个存档创建一个按钮（位置稍后在 render 里根据窗口大小设置）
    for (const auto& s : saves_) {
        saveButtons_.push_back(std::make_unique<Button>(
            s.name, font_, sf::Vector2f{0.f, 0.f}, sf::Vector2f{520.f, 55.f}, 22));
    }

    newButton_  = std::make_unique<Button>("＋ 新建存档", font_,
                                           sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{520.f, 60.f}, 26);
    backButton_ = std::make_unique<Button>("返回", font_,
                                           sf::Vector2f{0.f, 0.f},
                                           sf::Vector2f{160.f, 50.f}, 22);

    logger_->info("进入存档选择页，共 " + std::to_string(saves_.size()) + " 个存档");
}

void SaveSelectScene::handleEvent(const sf::Event& event) {
    for (auto& b : saveButtons_) b->handleEvent(event);
    if (newButton_)  newButton_->handleEvent(event);
    if (backButton_) backButton_->handleEvent(event);
}

void SaveSelectScene::update(float /*dt*/) {
    // 点击已有存档
    for (size_t i = 0; i < saveButtons_.size(); ++i) {
        if (saveButtons_[i]->consumeClick()) {
            logger_->info("选择存档: " + saves_[i].filename);
            saveManager_->setPendingSave(saves_[i]);
            nextScene_ = SceneId::Game;
            return;
        }
    }
    // 新建存档
    if (newButton_ && newButton_->consumeClick()) {
        auto info = saveManager_->createSave();
        saveManager_->setPendingSave(info);
        nextScene_ = SceneId::Game;
        return;
    }
    // 返回主菜单
    if (backButton_ && backButton_->consumeClick()) {
        nextScene_ = SceneId::MainMenu;
    }
}

void SaveSelectScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    const float btnW = 520.f;
    const float btnH = 55.f;
    const float gap  = 12.f;

    // 所有存档按钮 + 新建按钮的整体高度
    float total = static_cast<float>(saveButtons_.size()) * (btnH + gap)
                  + (60.f + gap);
    float startY = (h - total) / 2.f;

    // 存档按钮
    for (size_t i = 0; i < saveButtons_.size(); ++i) {
        saveButtons_[i]->setPosition({(w - btnW) / 2.f, startY});
        saveButtons_[i]->render(window.native());
        startY += btnH + gap;
    }

    // 新建存档按钮
    newButton_->setPosition({(w - btnW) / 2.f, startY});
    newButton_->render(window.native());

    // 返回按钮（底部居中）
    float bw = backButton_->size().x;
    backButton_->setPosition({(w - bw) / 2.f, h - 100.f});
    backButton_->render(window.native());

    window.display();
}