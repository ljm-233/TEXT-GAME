#include "game_scene.h"
#include "strings.h"
#include "utf8.h"
#include "notification.h"
#include "sound_manager.h"
#include "game_constants.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>

namespace {
// ⭐ 目标时间公式：基础 30 秒 + 每金币 3 秒
constexpr float kBaseTime = 30.f;
constexpr float kPerCoinTime = 3.f;
}

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
      overlaySubHint_(font, sf::String(), 20),
      overlayTime_(font, sf::String(), 22),
      overlayStars_(font, sf::String(), 56) {

    hudText_.setFillColor(sf::Color::White);
    overlayTitle_.setFillColor(sf::Color(255, 255, 255));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    overlaySubHint_.setFillColor(sf::Color(180, 180, 200));
    overlayTime_.setFillColor(sf::Color(220, 220, 240));
    overlayStars_.setFillColor(sf::Color(255, 220, 80));

    levelIndex_ = std::max(1, save_.currentLevel);

    parallax_ = std::make_unique<ParallaxBackground>();

    if (!loadLevel(levelIndex_)) {
        logger_->error("加载关卡 " + std::to_string(levelIndex_) + " 失败");
        NotificationSystem::instance().push("关卡加载失败",
                                            NotificationType::Error, 5.f);
    }

    logger_->info("进入游戏场景，存档: " + save_.filename);
}

bool GameScene::loadLevel(int index) {
    auto levelPath = preferences_->assetFile(
        "levels/level" + std::to_string(index) + ".txt");
    std::string path = levelPath.string();

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
    lastLives_ = 3;

    // ⭐ 重置计时
    levelTime_ = 0.f;
    finalStars_ = 0;
    finalCoins_ = 0;
    finalTotalCoins_ = 0;

    if (preferences_->getBool("level_intro", true)) {
        intro_ = std::make_unique<LevelIntro>(
            *font_, index, static_cast<float>(world_->totalCoins()));
    } else {
        intro_.reset();
    }

    lastHudLives_ = -1;
    lastOverlayState_ = GameWorld::State::Playing;
    return true;
}

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

    char timeBuf[32];
    std::snprintf(timeBuf, sizeof(timeBuf), "%.1f", levelTime_);

    std::string hud =
        "存档: " + save_.name +
        "   关卡: " + std::to_string(levelIndex_) +
        "   生命: " + std::to_string(lives) +
        "   金币: " + std::to_string(coins) +
        " / " + std::to_string(world_->totalCoins()) +
        "   时间: " + timeBuf + "s";

    if (world_->player().keys() > 0) {
        hud += "   钥匙: " + std::to_string(world_->player().keys());
    }
    hud += "   WASD 移动，Space 跳跃，ESC 暂停";
    hudText_.setString(toSf(hud));
}

// ============================================================
// 星级计算
// ============================================================

int GameScene::targetTime() const {
    if (!world_) return 60;
    int totalCoins = world_->totalCoins();
    return static_cast<int>(kBaseTime + totalCoins * kPerCoinTime);
}

int GameScene::calcStars() const {
    if (!world_) return 1;

    bool allCoins = (world_->coins() == world_->totalCoins());
    bool fastEnough = (levelTime_ <= static_cast<float>(targetTime()));

    if (allCoins && fastEnough) return 3;
    if (allCoins || fastEnough) return 2;
    return 1;
}

void GameScene::applyStars() {
    if (finalStars_ <= 0) return;
    saveManager_->setLevelStar(save_.filename, levelIndex_, finalStars_);
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
    overlayTitle_.setPosition({winW / 2.f, winH / 2.f - 140.f});

    // 金币统计
    std::string stats = "金币: " + std::to_string(finalCoins_) +
                        " / " + std::to_string(finalTotalCoins_);
    overlayHint_.setString(toSf(stats));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    auto hb = overlayHint_.getLocalBounds();
    overlayHint_.setOrigin({hb.position.x + hb.size.x / 2.f,
                            hb.position.y + hb.size.y / 2.f});
    overlayHint_.setPosition({winW / 2.f, winH / 2.f - 60.f});

    // 时间统计
    char timeBuf[64];
    std::snprintf(timeBuf, sizeof(timeBuf),
                  "时间: %.1f 秒  /  目标: %d 秒",
                  levelTime_, targetTime());
    overlayTime_.setString(toSf(timeBuf));
    auto tb2 = overlayTime_.getLocalBounds();
    overlayTime_.setOrigin({tb2.position.x + tb2.size.x / 2.f,
                            tb2.position.y + tb2.size.y / 2.f});
    overlayTime_.setPosition({winW / 2.f, winH / 2.f - 15.f});

    // 星级
    if (state == GameWorld::State::LevelComplete) {
        std::string stars;
        for (int i = 0; i < 3; ++i) {
            stars += (i < finalStars_) ? "\u2605" : "\u2606";
            if (i < 2) stars += "  ";
        }
        overlayStars_.setString(toSf(stars));
        auto sb = overlayStars_.getLocalBounds();
        overlayStars_.setOrigin({sb.position.x + sb.size.x / 2.f,
                                 sb.position.y + sb.size.y / 2.f});
        overlayStars_.setPosition({winW / 2.f, winH / 2.f + 60.f});
    } else {
        overlayStars_.setString("");
    }

    // 提示
    std::string hint;
    if (state == GameWorld::State::LevelComplete) {
        hint = "Enter 继续    R 重玩本关    ESC 返回";
    } else {
        hint = "R 重试    ESC 返回";
    }
    overlaySubHint_.setString(toSf(hint));
    auto sb2 = overlaySubHint_.getLocalBounds();
    overlaySubHint_.setOrigin({sb2.position.x + sb2.size.x / 2.f,
                               sb2.position.y + sb2.size.y / 2.f});
    overlaySubHint_.setPosition({winW / 2.f, winH / 2.f + 140.f});
}

void GameScene::handleEvent(const sf::Event& event) {
    if (paused_) {
        pauseMenu_->handleEvent(event);
        return;
    }

    if (intro_) {
        if (event.getIf<sf::Event::KeyPressed>() ||
            event.getIf<sf::Event::MouseButtonPressed>()) {
            intro_->skip();
            intro_.reset();
        }
        return;
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
                lastLives_ = 3;
                levelTime_ = 0.f;
                finalStars_ = 0;
                finalCoins_ = 0;
                finalTotalCoins_ = 0;
                lastOverlayState_ = GameWorld::State::Playing;
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
            lastLives_ = 3;
            levelTime_ = 0.f;
            lastOverlayState_ = GameWorld::State::Playing;
            NotificationSystem::instance().push("已重生",
                                                NotificationType::Info);
            return;
        }
    }
    world_->handleEvent(event);
}

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

    if (intro_) {
        intro_->update(dt);
        if (parallax_) parallax_->update(dt);
        if (intro_->isFinished()) intro_.reset();
        return;
    }

    if (world_->state() != GameWorld::State::Playing) return;

    // ⭐ 累加计时
    levelTime_ += dt;

    world_->update(dt);
    if (parallax_) parallax_->update(dt);

    // 检测掉血
    int curLives = world_->lives();
    lastLives_ = curLives;

    GameWorld::State cur = world_->state();
    if (cur != lastState_) {
        if (cur == GameWorld::State::LevelComplete) {
            // ⭐ 计算并写入星级
            finalCoins_ = world_->coins();
            finalTotalCoins_ = world_->totalCoins();
            finalStars_ = calcStars();
            applyStars();

            saveManager_->updateProgress(save_.filename, world_->coins(),
                                         levelIndex_);
            SoundManager::instance().playLevelComplete();

            NotificationSystem::instance().push(
                "关卡完成！获得 " + std::to_string(finalStars_) + " 星",
                NotificationType::Success, 5.f);
        } else if (cur == GameWorld::State::GameOver) {
            finalCoins_ = world_->coins();
            finalTotalCoins_ = world_->totalCoins();
            finalStars_ = 0;
            SoundManager::instance().playGameOver();
            NotificationSystem::instance().push(
                "游戏失败，按 R 重试", NotificationType::Error, 5.f);
        }
        lastState_ = cur;
    }
}

void GameScene::renderStateOverlay(sf::RenderTarget& rt, float winW, float winH) {
    refreshOverlayLayout(winW, winH);

    auto state = world_->state();
    if (state == GameWorld::State::Playing) return;

    rt.draw(overlayBg_);
    rt.draw(overlayTitle_);
    rt.draw(overlayHint_);
    if (state == GameWorld::State::LevelComplete) {
        rt.draw(overlayTime_);
        rt.draw(overlayStars_);
    }
    rt.draw(overlaySubHint_);
}

void GameScene::render(Window& window) {
    auto& rt = window.native();
    auto winSize = rt.getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    sf::View screenView(sf::FloatRect({0.f, 0.f}, {winW, winH}));

    rt.setView(screenView);
    rt.clear(sf::Color::Black);
    if (background_) background_->render(rt);

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

    world_->setShowColliders(preferences_->getBool("show_colliders", false));
    world_->setScreenShake(preferences_->getBool("screen_shake", true));
    world_->setParticles(preferences_->getBool("particles", true));
    world_->setPseudo3D(preferences_->getBool("pseudo_3d", true));
    world_->setPlayerAnimation(preferences_->getBool("player_animation", true));

    Vec2 camCenter = world_->cameraCenter();
    worldView_.setCenter({camCenter.x, camCenter.y});
    rt.setView(worldView_);

    if (parallax_ && preferences_->getBool("parallax", true)) {
        float camLeft = camCenter.x - kLogicalW * 0.5f;
        float camTop  = camCenter.y - kLogicalH * 0.5f;
        parallax_->render(rt, camLeft, camTop, kLogicalW, kLogicalH);
    }

    world_->render(rt);

    rt.setView(screenView);

    refreshHud();
    hudText_.setPosition({20.f, 16.f});
    rt.draw(hudText_);

    if (intro_) intro_->render(rt, winW, winH);

    renderStateOverlay(rt, winW, winH);

    if (paused_ && pauseMenu_) {
        pauseMenu_->relayout({winW, winH});
        pauseMenu_->render(rt);
    }
}