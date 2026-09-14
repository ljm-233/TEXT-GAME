#include "game_scene.h"
#include "strings.h"
#include "utf8.h"
#include "notification.h"
#include <algorithm>
#include <filesystem>

GameScene::GameScene(std::shared_ptr<Background>  background,
                     const sf::Font&              font,
                     std::shared_ptr<Logger>      logger,
                     SaveInfo                     save,
                     std::shared_ptr<SaveManager> saveManager,
                     std::shared_ptr<Preferences> preferences)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      saveManager_(std::move(saveManager)),
      preferences_(std::move(preferences)),
      save_(std::move(save)),
      font_(&font),
      hudText_(font, sf::String(), 20) {

    hudText_.setFillColor(sf::Color::White);

    auto level = std::make_unique<Level>();
    std::string path = std::string(PROJECT_ROOT) + "/assets/levels/level1.txt";

    if (!std::filesystem::exists(path)) {
        logger_->error("关卡文件不存在: " + path);
        NotificationSystem::instance().push("关卡文件缺失",
                                            NotificationType::Error, 5.f);
    } else if (!level->loadFromFile(path)) {
        logger_->error("关卡加载失败: " + path);
    } else {
        logger_->info("关卡已加载: " +
            std::to_string(level->width()) + "x" +
            std::to_string(level->height()) + " 瓦片");
    }

    world_ = std::make_unique<GameWorld>(std::move(level));
    world_->setViewSize(kLogicalW, kLogicalH);

    logger_->info("进入游戏场景，存档: " + save_.filename);
}

void GameScene::handleEvent(const sf::Event& event) {
    // ===== 暂停时，事件全给暂停菜单 =====
    if (paused_) {
        pauseMenu_->handleEvent(event);
        return;
    }

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            paused_ = true;
            auto winSize = sf::Vector2f(1280.f, 720.f); // relayout 时会修正
            pauseMenu_ = std::make_unique<PauseMenu>(
                *font_, preferences_, winSize);
            return;
        }
        if (kp->code == sf::Keyboard::Key::R) {
            world_->reset();
            NotificationSystem::instance().push("已重生",
                                                NotificationType::Info);
            return;
        }
    }
    world_->handleEvent(event);
}

void GameScene::update(float dt) {
    // ===== 暂停时不更新世界 =====
    if (paused_) {
        pauseMenu_->update(dt);
        auto action = pauseMenu_->consumeAction();
        if (action == PauseMenu::Action::Resume) {
            paused_ = false;
            pauseMenu_.reset();
        } else if (action == PauseMenu::Action::SaveAndQuit) {
            // 保存金币数作为进度
            saveManager_->updateProgress(save_.filename, world_->coins());
            NotificationSystem::instance().push("已保存并退出",
                                                NotificationType::Success);
            nextScene_ = SceneId::Back;
        }
        return;
    }

    // ===== 正常更新 =====
    world_->update(dt);

    int deaths = world_->consumeDeaths();
    if (deaths > 0) {
        NotificationSystem::instance().push(
            "掉出地图！剩余生命: " + std::to_string(world_->lives()),
            NotificationType::Warning);
    }

    if (world_->consumeLevelDone()) {
        if (world_->lives() > 0) {
            saveManager_->updateProgress(save_.filename, world_->coins());
            NotificationSystem::instance().push(
                "关卡完成！", NotificationType::Success, 5.f);
        } else {
            NotificationSystem::instance().push(
                "生命耗尽，按 R 重试", NotificationType::Error, 5.f);
        }
    }
}

void GameScene::render(Window& window) {
    auto& rt = window.native();
    auto winSize = rt.getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    // ⭐ 每次渲染都构造一个"屏幕空间 view"，尺寸 = 当前窗口尺寸
    //    这样窗口 resize 后，背景和 HUD 不会被拉伸
    sf::View screenView(sf::FloatRect({0.f, 0.f}, {winW, winH}));

    // ===== 阶段 1：全屏背景 =====
    rt.setView(screenView);
    rt.clear(sf::Color::Black);
    if (background_) background_->render(rt);

    // ===== 阶段 2：游戏世界 =====
    sf::View worldView(sf::FloatRect({0.f, 0.f}, {kLogicalW, kLogicalH}));
    float scale = std::min(winW / kLogicalW, winH / kLogicalH);
    float vpW = kLogicalW * scale / winW;
    float vpH = kLogicalH * scale / winH;
    float vpX = (1.f - vpW) * 0.5f;
    float vpY = (1.f - vpH) * 0.5f;
    worldView.setViewport(sf::FloatRect({vpX, vpY}, {vpW, vpH}));

    Vec2 camCenter = world_->cameraCenter();
    worldView.setCenter({camCenter.x, camCenter.y});
    rt.setView(worldView);

    world_->render(rt);

    // ===== 阶段 3：HUD =====
    rt.setView(screenView);   // ← 用同一个 screen view

    std::string hud =
        "存档: " + save_.name +
        "   生命: " + std::to_string(world_->lives()) +
        "   金币: " + std::to_string(world_->coins()) +
        " / " + std::to_string(world_->totalCoins()) +
        "   WASD/方向键 移动，Space/W 跳跃，R 重生，ESC 暂停";
    hudText_.setString(toSf(hud));
    hudText_.setPosition({20.f, 16.f});
    rt.draw(hudText_);

    // ===== 阶段 4：暂停菜单 =====
    if (paused_ && pauseMenu_) {
        pauseMenu_->relayout({winW, winH});
        pauseMenu_->render(rt);
    }
}