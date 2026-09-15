#include "game_scene.h"
#include "strings.h"
#include "utf8.h"
#include "notification.h"
#include "sound_manager.h"
#include "game_constants.h"
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
      hudText_(font, sf::String(), 20),
      overlayTitle_(font, sf::String(), 48),
      overlayHint_(font, sf::String(), 24),
      overlaySubHint_(font, sf::String(), 20) {

    hudText_.setFillColor(sf::Color::White);
    overlayTitle_.setFillColor(sf::Color(255, 255, 255));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    overlaySubHint_.setFillColor(sf::Color(180, 180, 200));

    levelIndex_ = std::max(1, save_.currentLevel);

    // 视差背景
    parallax_ = std::make_unique<ParallaxBackground>();

    if (!loadLevel(levelIndex_)) {
        logger_->error("加载关卡 " + std::to_string(levelIndex_) + " 失败");
        NotificationSystem::instance().push("关卡加载失败",
                                            NotificationType::Error, 5.f);
    }

    logger_->info("进入游戏场景，存档: " + save_.filename);
}

bool GameScene::loadLevel(int index) {
    std::string path = std::string(PROJECT_ROOT) +
                       "/assets/levels/level" + std::to_string(index) + ".txt";

    if (!std::filesystem::exists(path)) {
        logger_->error("关卡文件不存在: " + path);
        return false;
    }

    auto level = std::make_unique<Level>();
    if (!level->loadFromFile(path)) {
        logger_->error("关卡解析失败: " + path);
        return false;
    }

    level->setFont(font_);

    logger_->info("关卡已加载: level" + std::to_string(index) + " " +
        std::to_string(level->width()) + "x" +
        std::to_string(level->height()) + " 瓦片");

    world_ = std::make_unique<GameWorld>(std::move(level), index);
    world_->setViewSize(kLogicalW, kLogicalH);
    levelIndex_ = index;
    lastState_ = GameWorld::State::Playing;

    // 创建开场文字
    intro_ = std::make_unique<LevelIntro>(
        *font_, index, static_cast<float>(world_->totalCoins()));

    // 强制刷新缓存
    lastHudLives_ = -1;
    lastOverlayState_ = GameWorld::State::Playing;
    return true;
}

// ============================================================
// 缓存刷新
// ============================================================

void GameScene::refreshHud() {
    int lives = world_->lives();
    int coins = world_->coins();

    if (lives == lastHudLives_ &&
        coins == lastHudCoins_ &&
        levelIndex_ == lastHudLevel_) {
        return;
    }

    lastHudLives_ = lives;
    lastHudCoins_ = coins;
    lastHudLevel_ = levelIndex_;

    std::string hud =
        "存档: " + save_.name +
        "   关卡: " + std::to_string(levelIndex_) +
        "   生命: " + std::to_string(lives) +
        "   金币: " + std::to_string(coins) +
        " / " + std::to_string(world_->totalCoins()) +
        "   WASD 移动，Space 跳跃，ESC 暂停";
    hudText_.setString(toSf(hud));
}

void GameScene::refreshOverlayLayout(float winW, float winH) {
    auto state = world_->state();
    if (winW == lastOverlayWinW_ &&
        winH == lastOverlayWinH_ &&
        state == lastOverlayState_) {
        return;
    }
    lastOverlayWinW_ = winW;
    lastOverlayWinH_ = winH;
    lastOverlayState_ = state;

    if (state == GameWorld::State::Playing) return;

    overlayBg_.setSize({winW, winH});
    overlayBg_.setFillColor(sf::Color(0, 0, 0, 180));

    std::string title;
    if (state == GameWorld::State::LevelComplete) {
        title = "关卡 " + std::to_string(levelIndex_) + " 完成！";
    } else {
        title = "游戏失败";
    }
    overlayTitle_.setString(toSf(title));
    overlayTitle_.setFillColor(
        state == GameWorld::State::LevelComplete
            ? sf::Color(100, 240, 120)
            : sf::Color(240, 100, 100));
    auto tb = overlayTitle_.getLocalBounds();
    overlayTitle_.setOrigin({tb.position.x + tb.size.x / 2.f,
                             tb.position.y + tb.size.y / 2.f});
    overlayTitle_.setPosition({winW / 2.f, winH / 2.f - 60.f});

    std::string stats = "金币: " + std::to_string(world_->coins()) +
                        " / " + std::to_string(world_->totalCoins());
    overlayHint_.setString(toSf(stats));
    auto hb = overlayHint_.getLocalBounds();
    overlayHint_.setOrigin({hb.position.x + hb.size.x / 2.f,
                            hb.position.y + hb.size.y / 2.f});
    overlayHint_.setPosition({winW / 2.f, winH / 2.f + 10.f});

    std::string hint;
    if (state == GameWorld::State::LevelComplete) {
        hint = "Enter 继续    R 重玩本关    ESC 返回";
    } else {
        hint = "R 重试    ESC 返回";
    }
    overlaySubHint_.setString(toSf(hint));
    auto sb = overlaySubHint_.getLocalBounds();
    overlaySubHint_.setOrigin({sb.position.x + sb.size.x / 2.f,
                               sb.position.y + sb.size.y / 2.f});
    overlaySubHint_.setPosition({winW / 2.f, winH / 2.f + 70.f});
}

// ============================================================
// 事件
// ============================================================

void GameScene::handleEvent(const sf::Event& event) {
    if (paused_) {
        pauseMenu_->handleEvent(event);
        return;
    }

    // ===== 开场动画：任意键/点击跳过 =====
    if (intro_) {
        if (event.getIf<sf::Event::KeyPressed>() ||
            event.getIf<sf::Event::MouseButtonPressed>()) {
            intro_->skip();
            intro_.reset();
        }
        return;   // 开场期间不吃其他事件
    }

    auto state = world_->state();

    if (state != GameWorld::State::Playing) {
        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Escape) {
                nextScene_ = SceneId::Back;
                return;
            }
            if (kp->code == sf::Keyboard::Key::R) {
                world_->reset();
                lastState_ = GameWorld::State::Playing;
                return;
            }
            if (state == GameWorld::State::LevelComplete) {
                if (kp->code == sf::Keyboard::Key::Enter ||
                    kp->code == sf::Keyboard::Key::Space) {
                    if (levelIndex_ + 1 <= kMaxLevels &&
                        loadLevel(levelIndex_ + 1)) {
                        NotificationSystem::instance().push(
                            "进入第 " + std::to_string(levelIndex_) + " 关",
                            NotificationType::Info);
                        saveManager_->updateProgress(
                            save_.filename, world_->coins(), levelIndex_);
                    } else {
                        NotificationSystem::instance().push(
                            "全部通关！", NotificationType::Success, 5.f);
                    }
                }
            }
        }
        return;
    }

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            paused_ = true;
            pauseMenu_ = std::make_unique<PauseMenu>(
                *font_, preferences_, sf::Vector2f(kLogicalW, kLogicalH));
            return;
        }
        if (kp->code == sf::Keyboard::Key::R) {
            world_->reset();
            lastState_ = GameWorld::State::Playing;
            NotificationSystem::instance().push("已重生",
                                                NotificationType::Info);
            return;
        }
    }
    world_->handleEvent(event);
}

// ============================================================
// 更新
// ============================================================

void GameScene::update(float dt) {
    if (paused_) {
        pauseMenu_->update(dt);
        auto action = pauseMenu_->consumeAction();
        if (action == PauseMenu::Action::Resume) {
            paused_ = false;
            pauseMenu_.reset();
        } else if (action == PauseMenu::Action::SaveAndQuit) {
            saveManager_->updateProgress(save_.filename, world_->coins(),
                                         levelIndex_);
            nextScene_ = SceneId::Back;
        }
        return;
    }

    // ===== 开场动画期间：更新视差和开场，冻结游戏 =====
    if (intro_) {
        intro_->update(dt);
        if (parallax_) parallax_->update(dt);
        if (intro_->isFinished()) intro_.reset();
        return;
    }

    if (world_->state() != GameWorld::State::Playing) return;

    world_->update(dt);
    if (parallax_) parallax_->update(dt);

    GameWorld::State cur = world_->state();
    if (cur != lastState_) {
        if (cur == GameWorld::State::LevelComplete) {
            saveManager_->updateProgress(save_.filename, world_->coins(),
                                         levelIndex_);
            SoundManager::instance().playLevelComplete();
            NotificationSystem::instance().push(
                "关卡完成！按 Enter 继续", NotificationType::Success, 5.f);
        } else if (cur == GameWorld::State::GameOver) {
            SoundManager::instance().playGameOver();
            NotificationSystem::instance().push(
                "游戏失败，按 R 重试", NotificationType::Error, 5.f);
        }
        lastState_ = cur;
    }
}

// ============================================================
// 渲染
// ============================================================

void GameScene::renderStateOverlay(sf::RenderTarget& rt, float winW, float winH) {
    refreshOverlayLayout(winW, winH);

    auto state = world_->state();
    if (state == GameWorld::State::Playing) return;

    rt.draw(overlayBg_);
    rt.draw(overlayTitle_);
    rt.draw(overlayHint_);
    rt.draw(overlaySubHint_);
}

void GameScene::render(Window& window) {
    auto& rt = window.native();
    auto winSize = rt.getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    sf::View screenView(sf::FloatRect({0.f, 0.f}, {winW, winH}));

    // ===== 阶段 1：全屏背景（贴图） =====
    rt.setView(screenView);
    rt.clear(sf::Color::Black);
    if (background_) background_->render(rt);

    // ===== 阶段 2：游戏世界 =====
    if (winW != lastViewWinW_ || winH != lastViewWinH_) {
        lastViewWinW_ = winW;
        lastViewWinH_ = winH;

        worldView_ = sf::View(sf::FloatRect({0.f, 0.f}, {kLogicalW, kLogicalH}));
        float scale = std::min(winW / kLogicalW, winH / kLogicalH);
        float vpW = kLogicalW * scale / winW;
        float vpH = kLogicalH * scale / winH;
        float vpX = (1.f - vpW) * 0.5f;
        float vpY = (1.f - vpH) * 0.5f;
        worldView_.setViewport(sf::FloatRect({vpX, vpY}, {vpW, vpH}));
    }

    Vec2 camCenter = world_->cameraCenter();
    worldView_.setCenter({camCenter.x, camCenter.y});
    rt.setView(worldView_);

    // ⭐ 视差背景（在世界 view 下，用摄像机偏移驱动）
    if (parallax_) {
        float camLeft = camCenter.x - kLogicalW * 0.5f;
        float camTop  = camCenter.y - kLogicalH * 0.5f;
        parallax_->render(rt, camLeft, camTop, kLogicalW, kLogicalH);
    }

    world_->render(rt);

    // ===== 阶段 3：HUD =====
    rt.setView(screenView);

    refreshHud();
    hudText_.setPosition({20.f, 16.f});
    rt.draw(hudText_);

    // ⭐ 开场文字
    if (intro_) {
        intro_->render(rt, winW, winH);
    }

    renderStateOverlay(rt, winW, winH);

    if (paused_ && pauseMenu_) {
        pauseMenu_->relayout({winW, winH});
        pauseMenu_->render(rt);
    }
}