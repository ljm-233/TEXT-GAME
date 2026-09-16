#include "game_scene.h"
#include "text_strings.h"
#include "focus_group.h"
#include "utf8.h"
#include "notification.h"
#include "sound_manager.h"
#include "keybindings.h"
#include "game_constants.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <type_traits>
#include <variant>

namespace {
// ⭐ 目标时间公式：基础 30 秒 + 每金币 3 秒
constexpr float kBaseTime = 30.f;
constexpr float kPerCoinTime = 3.f;
}

GameScene::GameScene(std::shared_ptr<Background>  background,
                     const sf::Font&              font,
                     std::shared_ptr<Logger>      logger,
                     std::shared_ptr<SaveManager> saveManager,
                     std::shared_ptr<Preferences> preferences)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      saveManager_(std::move(saveManager)),
      preferences_(std::move(preferences)),
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
}

void GameScene::onEnter() {
    nextScene_ = SceneId::None;

    save_ = saveManager_->takePendingSave();
    levelIndex_ = std::max(1, save_.currentLevel);

    parallax_ = std::make_unique<ParallaxBackground>();

    if (!loadLevel(levelIndex_)) {
        logger_->error("加载关卡 " + std::to_string(levelIndex_) + " 失败");
        NotificationSystem::instance().push(Str::T(Str::NotifLevelLoadFailed),
                                            NotificationType::Error, 5.f);
    }

    logger_->info("进入游戏场景，存档: " + save_.filename);
}

void GameScene::onResume() {
    nextScene_ = SceneId::None;
}

std::string GameScene::windowTitleHint() const {
    if (paused_) {
        return Str::T(Str::WinTitleLevelPrefix) +
               std::to_string(levelIndex_) +
               Str::T(Str::WinTitlePausedSuffix);
    }
    return Str::T(Str::WinTitleLevelPrefix) +
           std::to_string(levelIndex_) +
           Str::T(Str::WinTitleLevelSuffix);
}

void GameScene::subscribeWorldEvents() {
    if (!world_) return;

    world_->bus().subscribe([this](const GameEvent& e) {
        const bool particlesOn = preferences_->getBool("particles", true);

        std::visit([this, particlesOn](const auto& ev) {
            using T = std::decay_t<decltype(ev)>;

            if constexpr (std::is_same_v<T, EvJumped>) {
                SoundManager::instance().playJump();
                if (particlesOn) world_->particles().emitJump(ev.pos);
            } else if constexpr (std::is_same_v<T, EvLanded>) {
                SoundManager::instance().playLand();
                if (particlesOn) world_->particles().emitLand(ev.pos, ev.intensity);
            } else if constexpr (std::is_same_v<T, EvCoined>) {
                SoundManager::instance().playCoin();
                if (particlesOn) world_->particles().emitCoin(ev.pos);
            } else if constexpr (std::is_same_v<T, EvStomped>) {
                SoundManager::instance().playStomp();
                if (particlesOn) world_->particles().emitStomp(ev.pos);
                hitstopTimer_ = 0.06f;   // 60ms 冻结
            } else if constexpr (std::is_same_v<T, EvHurt>) {
                SoundManager::instance().playHurt();
                if (particlesOn) world_->particles().emitHurt(ev.pos);
                hitstopTimer_ = 0.04f;   // 40ms 冻结
            } else if constexpr (std::is_same_v<T, EvCheckpoint>) {
                SoundManager::instance().playCheckpoint();
                if (particlesOn) world_->particles().emitCoin(ev.pos);
            } else if constexpr (std::is_same_v<T, EvJumpPad>) {
                SoundManager::instance().playJump();
                if (particlesOn) world_->particles().emitJump(ev.pos);
            } else if constexpr (std::is_same_v<T, EvLevelComplete>) {
                finalCoins_      = world_->coins();
                finalTotalCoins_ = world_->totalCoins();
                finalStars_      = calcStars();
                applyStars();
                saveManager_->updateProgress(save_.filename,
                                             world_->coins(), levelIndex_);
                SoundManager::instance().playLevelComplete();
                NotificationSystem::instance().push(
                    Str::T(Str::NotifLevelCompleteStars) +
                        std::to_string(finalStars_) +
                        Str::T(Str::NotifStarSuffix),
                    NotificationType::Success, 5.f);
            } else if constexpr (std::is_same_v<T, EvGameOver>) {
                finalCoins_      = world_->coins();
                finalTotalCoins_ = world_->totalCoins();
                finalStars_      = 0;
                SoundManager::instance().playGameOver();
                NotificationSystem::instance().push(
                    Str::T(Str::NotifGameOverRetry), NotificationType::Error, 5.f);
            }
        }, e);
    });
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

    levelTime_ = 0.f;
    hitstopTimer_ = 0.f;
    finalStars_ = 0;
    finalCoins_ = 0;
    finalTotalCoins_ = 0;

    if (preferences_->getBool("level_intro", true)) {
        intro_ = std::make_unique<LevelIntro>(
            *font_, index, static_cast<float>(world_->totalCoins()),
            world_->level().name());
    } else {
        intro_.reset();
    }

    lastHudLives_ = -1;
    lastOverlayState_ = GameWorld::State::Playing;

    // ⭐ 新建 world 后立即订阅事件（每次 loadLevel 都会重建 world）
    subscribeWorldEvents();

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
        Str::T(Str::HudSave) + save_.name +
        "   " + Str::T(Str::HudLevel) + std::to_string(levelIndex_) +
        "   " + Str::T(Str::HudLives) + std::to_string(lives) +
        "   " + Str::T(Str::HudCoins) + std::to_string(coins) +
        " / " + std::to_string(world_->totalCoins()) +
        "   " + Str::T(Str::HudTime) + timeBuf + "s";

    if (world_->player().keys() > 0) {
        hud += "   " + Str::T(Str::HudKeys) + std::to_string(world_->player().keys());
    }
    hud += "   " + Str::T(Str::HudHelp);
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

void GameScene::rebuildOverlayButtons() {
    overlayButtons_.clear();

    const float winW = lastOverlayWinW_;
    const float winH = lastOverlayWinH_;
    const auto  state = world_->state();

    const float bw = 180.f;
    const float bh = 50.f;
    const float gap = 15.f;

    if (state == GameWorld::State::LevelComplete) {
        const float totalW = bw * 3.f + gap * 2.f;
        const float x0 = (winW - totalW) * 0.5f;
        const float y  = winH * 0.5f + 130.f;

        overlayButtons_.push_back(std::make_unique<Button>(
            Str::T(Str::BtnNextLevel), *font_,
            sf::Vector2f{x0, y}, sf::Vector2f{bw, bh}, 22));
        overlayButtons_.push_back(std::make_unique<Button>(
            Str::T(Str::BtnReplay), *font_,
            sf::Vector2f{x0 + bw + gap, y}, sf::Vector2f{bw, bh}, 22));
        overlayButtons_.push_back(std::make_unique<Button>(
            Str::T(Str::Back), *font_,
            sf::Vector2f{x0 + (bw + gap) * 2.f, y}, sf::Vector2f{bw, bh}, 22));
    } else if (state == GameWorld::State::GameOver) {
        const float totalW = bw * 2.f + gap;
        const float x0 = (winW - totalW) * 0.5f;
        const float y  = winH * 0.5f + 60.f;

        overlayButtons_.push_back(std::make_unique<Button>(
            Str::T(Str::BtnRetry), *font_,
            sf::Vector2f{x0, y}, sf::Vector2f{bw, bh}, 22));
        overlayButtons_.push_back(std::make_unique<Button>(
            Str::T(Str::Back), *font_,
            sf::Vector2f{x0 + bw + gap, y}, sf::Vector2f{bw, bh}, 22));
    }
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

    if (state == GameWorld::State::Playing) {
        overlayButtons_.clear();
        FocusGroup::instance().clear();
        return;
    }

    // ⭐ 状态变化时重建按钮
    rebuildOverlayButtons();

    overlayBg_.setSize({winW, winH});
    overlayBg_.setFillColor(sf::Color(0, 0, 0, 180));

    std::string title;
    if (state == GameWorld::State::LevelComplete) {
        title = Str::T(Str::IntroLevelPrefix) + std::to_string(levelIndex_) +
                " " + Str::T(Str::LevelCompleteTitle);
    } else {
        title = Str::T(Str::GameOverTitle);
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

    std::string stats = Str::T(Str::OverlayCoins) + std::to_string(finalCoins_) +
                        " / " + std::to_string(finalTotalCoins_);
    overlayHint_.setString(toSf(stats));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    auto hb = overlayHint_.getLocalBounds();
    overlayHint_.setOrigin({hb.position.x + hb.size.x / 2.f,
                            hb.position.y + hb.size.y / 2.f});
    overlayHint_.setPosition({winW / 2.f, winH / 2.f - 60.f});

    char timeBuf[128];
    std::snprintf(timeBuf, sizeof(timeBuf),
                  "%s%.1f%s  /  %s%d%s",
                  Str::T(Str::OverlayTime).c_str(),
                  levelTime_,
                  Str::T(Str::OverlaySeconds).c_str(),
                  Str::T(Str::OverlayTarget).c_str(),
                  targetTime(),
                  Str::T(Str::OverlaySeconds).c_str());
    overlayTime_.setString(toSf(timeBuf));
    auto tb2 = overlayTime_.getLocalBounds();
    overlayTime_.setOrigin({tb2.position.x + tb2.size.x / 2.f,
                            tb2.position.y + tb2.size.y / 2.f});
    overlayTime_.setPosition({winW / 2.f, winH / 2.f - 15.f});

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

    std::string hint;
    if (state == GameWorld::State::LevelComplete) {
        hint = Str::T(Str::OverlayHintComplete);
    } else {
        hint = Str::T(Str::OverlayHintFailed);
    }
    overlaySubHint_.setString(toSf(hint));
    auto sb2 = overlaySubHint_.getLocalBounds();
    overlaySubHint_.setOrigin({sb2.position.x + sb2.size.x / 2.f,
                               sb2.position.y + sb2.size.y / 2.f});

    float hintY = winH / 2.f + 140.f;
    if (state == GameWorld::State::LevelComplete) hintY = winH / 2.f + 210.f;
    overlaySubHint_.setPosition({winW / 2.f, hintY});
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
        // ⭐ 先转发给 overlay 按钮
        for (auto& b : overlayButtons_) b->handleEvent(event);

        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == KeyBindings::instance().get(KeyBindings::Pause)) {
                nextScene_ = SceneId::Back;
                return;
            }
            if (kp->code == KeyBindings::instance().get(KeyBindings::Restart)) {
                world_->reset();
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
                            Str::T(Str::NotifEnterLevel) +
                                std::to_string(levelIndex_) +
                                Str::T(Str::NotifLevelSuffix),
                            NotificationType::Info);
                        saveManager_->updateProgress(
                            save_.filename, world_->coins(), levelIndex_);
                    } else {
                        NotificationSystem::instance().push(
                            Str::T(Str::NotifAllClear),
                            NotificationType::Success, 5.f);
                    }
                }
            }
        }
        return;
    }

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == KeyBindings::instance().get(KeyBindings::Pause)) {
            paused_ = true;
            pauseMenu_ = std::make_unique<PauseMenu>(
                *font_, preferences_, sf::Vector2f(kLogicalW, kLogicalH));
            return;
        }
        if (kp->code == KeyBindings::instance().get(KeyBindings::Restart)) {
            world_->reset();
            levelTime_ = 0.f;
            hitstopTimer_ = 0.f;
            lastOverlayState_ = GameWorld::State::Playing;
            NotificationSystem::instance().push(Str::T(Str::NotifRespawned),
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

    auto state = world_->state();
    if (state != GameWorld::State::Playing) {
        // ⭐ 处理 overlay 按钮点击
        if (state == GameWorld::State::LevelComplete && overlayButtons_.size() >= 3) {
            if (overlayButtons_[0]->consumeClick()) {
                // 下一关
                if (levelIndex_ + 1 <= kMaxLevels && loadLevel(levelIndex_ + 1)) {
                    NotificationSystem::instance().push(
                        Str::T(Str::NotifEnterLevel) +
                            std::to_string(levelIndex_) +
                            Str::T(Str::NotifLevelSuffix),
                        NotificationType::Info);
                    saveManager_->updateProgress(
                        save_.filename, world_->coins(), levelIndex_);
                } else {
                    NotificationSystem::instance().push(
                        Str::T(Str::NotifAllClear),
                        NotificationType::Success, 5.f);
                }
                return;
            }
            if (overlayButtons_[1]->consumeClick()) {
                // 重玩
                world_->reset();
                levelTime_ = 0.f;
                finalStars_ = 0;
                finalCoins_ = 0;
                finalTotalCoins_ = 0;
                lastOverlayState_ = GameWorld::State::Playing;
                return;
            }
            if (overlayButtons_[2]->consumeClick()) {
                nextScene_ = SceneId::Back;
                return;
            }
        } else if (state == GameWorld::State::GameOver && overlayButtons_.size() >= 2) {
            if (overlayButtons_[0]->consumeClick()) {
                // 重试
                world_->reset();
                levelTime_ = 0.f;
                lastOverlayState_ = GameWorld::State::Playing;
                return;
            }
            if (overlayButtons_[1]->consumeClick()) {
                nextScene_ = SceneId::Back;
                return;
            }
        }
        return;
    }

    // 受击停顿：冻结游戏逻辑和计时
    if (hitstopTimer_ > 0.f) {
        hitstopTimer_ -= dt;
        return;
    }

    levelTime_ += dt;

    world_->update(dt);
    if (parallax_) parallax_->update(dt);

    // 状态变化的响应由 EventBus 的 EvLevelComplete / EvGameOver 处理
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

    // ⭐ 渲染按钮 + 注册焦点（手柄导航）
    for (auto& b : overlayButtons_) b->render(rt);

    std::vector<Button*> items;
    for (auto& b : overlayButtons_) items.push_back(b.get());
    FocusGroup::instance().setItems(items);
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