#include "game_scene.h"
#include "text_strings.h"
#include "utf8.h"
#include "notification.h"
#include "sound_manager.h"
#include "keybindings.h"
#include "game_constants.h"
#include "focus_group.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <SFML/OpenGL.hpp>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
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
      overlayStars_(font, sf::String(), 56),
      debugText_(font, sf::String(), 14) {
    debugText_.setFillColor(sf::Color(255, 255, 130));
    debugText_.setOutlineThickness(2.f);
    debugText_.setOutlineColor(sf::Color(0, 0, 0, 200));

    hudText_.setFillColor(sf::Color::White);
    overlayTitle_.setFillColor(sf::Color(255, 255, 255));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    overlaySubHint_.setFillColor(sf::Color(180, 180, 200));
    overlayTime_.setFillColor(sf::Color(220, 220, 240));
    overlayStars_.setFillColor(sf::Color(255, 220, 80));
}

void GameScene::onEnter() {
    nextScene_ = SceneId::None;
    // （原有的 save_ / parallax_ / loadLevel 逻辑保持不变）

    syncFocus();

    save_ = saveManager_->takePendingSave();
    levelIndex_ = std::max(1, save_.currentLevel);

    // ⭐ 读取本关之前的 PB
    if (levelIndex_ >= 1 && levelIndex_ <= static_cast<int>(save_.levelBestTimes.size())) {
        prevBestTime_ = save_.levelBestTimes[levelIndex_ - 1];
    } else {
        prevBestTime_ = 0.f;
    }

    parallax_ = std::make_unique<ParallaxBackground>();

    if (!loadLevel(levelIndex_)) {
        logger_->error("加载关卡 " + std::to_string(levelIndex_) + " 失败");
        NotificationSystem::instance().push(Str::T(Str::NotifLevelLoadFailed),
                                            NotificationType::Error, 5.f);
    }

    logger_->info("进入游戏场景，存档: " + save_.filename);
    syncFocus();
}

void GameScene::onResume() {
    nextScene_ = SceneId::None;
    syncFocus();
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
                hitstopTimer_ = 0.06f;
            } else if constexpr (std::is_same_v<T, EvHurt>) {
                SoundManager::instance().playHurt();
                if (particlesOn) world_->particles().emitHurt(ev.pos);
                hitstopTimer_ = 0.04f;
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

                // ⭐ PB 检测
                newRecord_ = (prevBestTime_ <= 0.f || levelTime_ < prevBestTime_);
                if (newRecord_) {
                    saveManager_->setLevelBestTime(save_.filename,
                                                   levelIndex_, levelTime_);
                }

                SoundManager::instance().playLevelComplete();
                NotificationSystem::instance().push(
                    Str::T(Str::NotifLevelCompleteStars) +
                        std::to_string(finalStars_) +
                        Str::T(Str::NotifStarSuffix),
                    NotificationType::Success, 5.f);

                screenFlashTimer_    = 0.35f;
                screenFlashDuration_ = 0.35f;
                screenFlashColor_    = sf::Color(120, 255, 150);
            } else if constexpr (std::is_same_v<T, EvLifeExhausted>) {
                SoundManager::instance().playGameOver();
                NotificationSystem::instance().push(
                    Str::T(Str::NotifLifeExhausted),
                    NotificationType::Error, 4.f);

                // 不闪屏——延迟由 GameWorld 处理
                levelTime_ = 0.f;
                lastOverlayState_ = GameWorld::State::Playing;
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

    // ⭐ 校验初始生命值，非法值回退到 1
    int lives = preferences_->getInt("initial_lives", 1);
    if (lives != 1 && lives != 3 && lives != 5 && lives != 10 && lives != 100) {
        lives = 1;
        preferences_->setInt("initial_lives", 1);
    }
    world_->setInitialLives(lives);
    levelIndex_ = index;

    levelTime_ = 0.f;
    hitstopTimer_ = 0.f;
    screenFlashTimer_ = 0.f;
    finalStars_ = 0;
    finalCoins_ = 0;
    finalTotalCoins_ = 0;
    newRecord_ = false;

    // ⭐ 读取新关的 PB
    if (index >= 1 && index <= static_cast<int>(save_.levelBestTimes.size())) {
        prevBestTime_ = save_.levelBestTimes[index - 1];
    } else {
        prevBestTime_ = 0.f;
    }

    if (preferences_->getBool("level_intro", true)) {
        intro_ = std::make_unique<LevelIntro>(
            *font_, index, static_cast<float>(world_->totalCoins()),
            world_->level().name());
    } else {
        intro_.reset();
    }

    lastHudLives_ = -1;
    lastOverlayState_ = GameWorld::State::Playing;

    // ⭐ 调试：切关 / 重载时清除暂停
    paused_ = false;
    if (pauseMenu_) pauseMenu_.reset();

    // ⭐ 调试：保持无敌状态
    if (debugInvincible_) {
        world_->player().setInvincible(true);
    }

    subscribeWorldEvents();
    syncFocus();

    return true;
}

bool GameScene::handleDebugKey(sf::Keyboard::Key k) {
    switch (k) {
        case sf::Keyboard::Key::F1:
            debugHud_ = !debugHud_;
            NotificationSystem::instance().push(
                debugHud_ ? "调试 HUD: 开" : "调试 HUD: 关",
                NotificationType::Info, 1.2f);
            return true;

        case sf::Keyboard::Key::F2:
            debugInvincible_ = !debugInvincible_;
            if (world_) world_->player().setInvincible(debugInvincible_);
            NotificationSystem::instance().push(
                debugInvincible_ ? "无敌: 开" : "无敌: 关",
                NotificationType::Info, 1.2f);
            return true;

        case sf::Keyboard::Key::F3:
            if (world_) world_->killAllEnemies();
            NotificationSystem::instance().push(
                "已清空敌人", NotificationType::Info, 1.2f);
            return true;

        case sf::Keyboard::Key::F4:
            loadLevel(levelIndex_);
            NotificationSystem::instance().push(
                "重载关卡 " + std::to_string(levelIndex_),
                NotificationType::Info, 1.2f);
            return true;

        case sf::Keyboard::Key::F5:
            if (levelIndex_ > 1) {
                loadLevel(levelIndex_ - 1);
                NotificationSystem::instance().push(
                    "关卡 " + std::to_string(levelIndex_),
                    NotificationType::Info, 1.2f);
            }
            return true;

        case sf::Keyboard::Key::F6:
            if (levelIndex_ < kMaxLevels) {
                loadLevel(levelIndex_ + 1);
                NotificationSystem::instance().push(
                    "关卡 " + std::to_string(levelIndex_),
                    NotificationType::Info, 1.2f);
            }
            return true;

        case sf::Keyboard::Key::F7: {
            // 1.0 → 0.5 → 0.25 → 0.1 → 1.0
            if (debugTimeScale_ > 0.75f)      debugTimeScale_ = 0.5f;
            else if (debugTimeScale_ > 0.4f)  debugTimeScale_ = 0.25f;
            else if (debugTimeScale_ > 0.15f) debugTimeScale_ = 0.1f;
            else                              debugTimeScale_ = 1.f;

            char buf[32];
            std::snprintf(buf, sizeof(buf), "慢动作: %.2fx",
                          static_cast<double>(debugTimeScale_));
            NotificationSystem::instance().push(buf, NotificationType::Info, 1.2f);
            return true;
        }

        case sf::Keyboard::Key::F8:
            debugShowColliders_ = !debugShowColliders_;
            NotificationSystem::instance().push(
                debugShowColliders_ ? "碰撞盒: 开" : "碰撞盒: 关",
                NotificationType::Info, 1.2f);
            return true;

        case sf::Keyboard::Key::F9:
            pendingScreenshot_ = true;
            return true;

        default:
            return false;
    }
}

void GameScene::renderDebugHud(sf::RenderTarget& rt, Window& window) {
    if (!world_) return;

    const auto& level  = world_->level();
    const auto& player = world_->player();

    Vec2 pos  = player.position();
    Vec2 vel  = player.velocity();
    Vec2 cam  = world_->cameraCenter();

    // 鼠标世界坐标
    sf::Vector2i mousePix = sf::Mouse::getPosition(window.native());
    sf::Vector2f mouseWorld =
        window.native().mapPixelToCoords(mousePix, lastWorldView_);
    int ts = level.tileSize();
    int tx = static_cast<int>(std::floor(mouseWorld.x / ts));
    int ty = static_cast<int>(std::floor(mouseWorld.y / ts));
    char tileChar = ' ';
    if (tx >= 0 && tx < level.width() && ty >= 0 && ty < level.height()) {
        tileChar = level.tileAt(tx, ty);
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "关卡 " << levelIndex_;
    if (!level.name().empty()) oss << " [" << level.name() << "]";
    oss << "  " << level.width() << "x" << level.height();
    oss << "\n玩家 (" << pos.x << ", " << pos.y << ")";
    oss << "  vel (" << vel.x << ", " << vel.y << ")";
    oss << "\n" << (player.onGround() ? "地面" : "空中");
    if (player.isInvincible()) oss << "  无敌";
    if (player.isInvinciblePersistent()) oss << " ⚡";
    oss << "\n相机 (" << cam.x << ", " << cam.y << ")";
    oss << "\n金币 " << world_->coins() << "/" << world_->totalCoins()
        << "  生命 " << world_->lives();
    oss << "\n鼠标 tile(" << tx << ", " << ty << ") '" << tileChar << "'";
    oss << "\n[F1]HUD [F2]无敌 [F3]杀敌 [F4]重载 [F5]上关 [F6]下关";
    oss << " [F7]慢动作 [F8]碰撞盒 [F9]截图";

    debugText_.setString(toSf(oss.str()));

    // 背景
    auto b = debugText_.getLocalBounds();
    const float padX = 8.f, padY = 6.f;
    sf::RectangleShape bg({
        b.size.x + padX * 2.f,
        b.size.y + padY * 2.f
    });
    bg.setPosition({12.f, 42.f});
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(120, 120, 160, 180));
    rt.draw(bg);

    debugText_.setPosition({12.f + padX - b.position.x, 42.f + padY - b.position.y});
    rt.draw(debugText_);
}


void GameScene::syncFocus() {
    // 暂停菜单打开时，焦点交给 PauseMenu 管理，这里不动
    if (paused_ && pauseMenu_) return;

    // 开场动画显示中，不设焦点
    if (intro_) {
        FocusGroup::instance().clear();
        return;
    }

    // LevelComplete 状态：设 overlay 按钮
    if (world_ && world_->state() == GameWorld::State::LevelComplete
        && !overlayButtons_.empty()) {
        std::vector<Button*> items;
        for (auto& b : overlayButtons_) items.push_back(b.get());
        FocusGroup::instance().setItems(items);
        return;
    }

    // Playing 状态：无焦点
    FocusGroup::instance().clear();
}


bool GameScene::advanceToNextLevel() {
    int next = levelIndex_ + 1;
    if (next > kMaxLevels) return false;

    // 先确认关卡文件存在，避免 loadLevel 打 error 日志
    auto nextPath = preferences_->assetFile(
        "levels/level" + std::to_string(next) + ".txt");
    if (!std::filesystem::exists(nextPath)) return false;

    if (!loadLevel(next)) return false;

    NotificationSystem::instance().push(
        Str::T(Str::NotifEnterLevel) + std::to_string(levelIndex_) +
            Str::T(Str::NotifLevelSuffix),
        NotificationType::Info);
    saveManager_->updateProgress(save_.filename, world_->coins(), levelIndex_);
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

    // 现在只有 LevelComplete 需要按钮
    if (world_->state() != GameWorld::State::LevelComplete) return;

    const float winW = lastOverlayWinW_;
    const float winH = lastOverlayWinH_;

    const float bw = 180.f;
    const float bh = 50.f;
    const float gap = 15.f;
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
        // ⭐ 暂停时不清焦点，否则会覆盖 PauseMenu 刚设好的
        if (!paused_) {
            FocusGroup::instance().clear();
        }
        return;
    }

    rebuildOverlayButtons();

    overlayBg_.setSize({winW, winH});
    overlayBg_.setFillColor(sf::Color(0, 0, 0, 180));

    std::string title = Str::T(Str::IntroLevelPrefix) + std::to_string(levelIndex_) +
                        " " + Str::T(Str::LevelCompleteTitle);
    overlayTitle_.setString(toSf(title));
    overlayTitle_.setFillColor(sf::Color(100, 240, 120));
    auto tb = overlayTitle_.getLocalBounds();
    overlayTitle_.setOrigin({tb.position.x + tb.size.x / 2.f,
                             tb.position.y + tb.size.y / 2.f});
    overlayTitle_.setPosition({winW / 2.f, winH / 2.f - 140.f});

    // 第一行：金币
    std::string stats = Str::T(Str::OverlayCoins) + std::to_string(finalCoins_) +
                        " / " + std::to_string(finalTotalCoins_);
    overlayHint_.setString(toSf(stats));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    auto hb = overlayHint_.getLocalBounds();
    overlayHint_.setOrigin({hb.position.x + hb.size.x / 2.f,
                            hb.position.y + hb.size.y / 2.f});
    overlayHint_.setPosition({winW / 2.f, winH / 2.f - 70.f});

    // 第二行：PB（只在通关时显示）
    std::string pbLine;
    if (newRecord_) {
        pbLine = Str::T(Str::OverlayNewRecord);
    } else if (prevBestTime_ > 0.f) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s%.2f%s",
                      Str::T(Str::OverlayBestTime).c_str(),
                      prevBestTime_,
                      Str::T(Str::OverlaySeconds).c_str());
        pbLine = buf;
    }
    overlaySubHint_.setString(toSf(pbLine));

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
    overlayTime_.setPosition({winW / 2.f, winH / 2.f - 25.f});

    {
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
    }
   // PB 行位置（时间下面）
    {
        auto sb = overlaySubHint_.getLocalBounds();
        overlaySubHint_.setOrigin({sb.position.x + sb.size.x / 2.f,
                                   sb.position.y + sb.size.y / 2.f});
        overlaySubHint_.setFillColor(newRecord_
            ? sf::Color(255, 220, 80)
            : sf::Color(180, 180, 200));
        overlaySubHint_.setPosition({winW / 2.f, winH / 2.f + 20.f});
    }
}

void GameScene::handleEvent(const sf::Event& event) {
    // ⭐ 调试快捷键：优先响应
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (handleDebugKey(kp->code)) return;
    }

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
                hitstopTimer_ = 0.f;
                screenFlashTimer_ = 0.f;
                finalStars_ = 0;
                finalCoins_ = 0;
                finalTotalCoins_ = 0;
                lastOverlayState_ = GameWorld::State::Playing;
                return;
            }
            if (state == GameWorld::State::LevelComplete) {
                if (kp->code == sf::Keyboard::Key::Enter ||
                    kp->code == sf::Keyboard::Key::Space) {
                    if (!advanceToNextLevel()) {
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
            // PauseMenu 构造末尾会调 syncFocus()，自动接管焦点
            pauseMenu_ = std::make_unique<PauseMenu>(
                *font_, preferences_, sf::Vector2f(kLogicalW, kLogicalH));
            return;
        }
        if (kp->code == KeyBindings::instance().get(KeyBindings::Restart)) {
            world_->reset();
            levelTime_ = 0.f;
            hitstopTimer_ = 0.f;
            screenFlashTimer_ = 0.f;
            lastOverlayState_ = GameWorld::State::Playing;
            NotificationSystem::instance().push(Str::T(Str::NotifRespawned),
                                                NotificationType::Info);
            return;
        }
    }
    world_->handleEvent(event);
}

void GameScene::update(float dt) {
    if (screenFlashTimer_ > 0.f) screenFlashTimer_ -= dt;

    if (paused_) {
        pauseMenu_->update(dt);
        auto action = pauseMenu_->consumeAction();
        if (action == PauseMenu::Action::Resume) {
            paused_ = false;
            pauseMenu_.reset();
            syncFocus();                              // ⭐ 恢复 GameScene 焦点
        } else if (action == PauseMenu::Action::SaveAndQuit) {
            saveManager_->updateProgress(save_.filename, world_->coins(),
                                         levelIndex_);
            nextScene_ = SceneId::Back;
        }
        return;
    }

    // ⭐ 慢动作（只在游戏逻辑生效，不影响暂停菜单和计时 UI）
    dt *= debugTimeScale_;

    // ⭐ 检测世界状态变化，同步焦点（Playing ↔ LevelComplete）
    if (world_) {
        auto state = world_->state();
        if (state != lastFocusState_) {
            lastFocusState_ = state;
            syncFocus();
        }
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
                if (!advanceToNextLevel()) {
                    NotificationSystem::instance().push(
                    Str::T(Str::NotifAllClear),
                    NotificationType::Success, 5.f);
                }
                return;
            }
            if (overlayButtons_[1]->consumeClick()) {
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

    for (auto& b : overlayButtons_) b->render(rt);
}

void GameScene::render(Window& window) {
    auto& rt = window.target();
    auto winSize = window.native().getSize();
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

    world_->setShowColliders(
        debugShowColliders_ || preferences_->getBool("show_colliders", false));
    world_->setScreenShake(preferences_->getBool("screen_shake", true));
    world_->setParticles(preferences_->getBool("particles", true));
    world_->setPseudo3D(preferences_->getBool("pseudo_3d", true));
    world_->setPlayerAnimation(preferences_->getBool("player_animation", true));

    Vec2 camCenter = world_->cameraCenter();
    worldView_.setCenter({camCenter.x, camCenter.y});
    rt.setView(worldView_);
    lastWorldView_ = worldView_;   // ⭐ 供调试 HUD 用

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

    // ⭐ 调试 HUD
    if (debugHud_) {
        renderDebugHud(rt, window);
    }

    if (intro_) intro_->render(rt, winW, winH);

    renderStateOverlay(rt, winW, winH);

    if (paused_ && pauseMenu_) {
        pauseMenu_->relayout({winW, winH});
        pauseMenu_->render(rt);
    }

    // 屏幕闪光（最上层）
    if (screenFlashTimer_ > 0.f) {
        float t = screenFlashTimer_ / screenFlashDuration_;
        t = std::clamp(t, 0.f, 1.f);

        sf::RectangleShape flash({winW, winH});
        sf::Color c = screenFlashColor_;
        c.a = static_cast<std::uint8_t>(t * 200.f);
        flash.setFillColor(c);
        rt.draw(flash);
    }

    // ⭐ 截图（用 glReadPixels 抓当前帧缓冲）
    if (pendingScreenshot_) {
        pendingScreenshot_ = false;

        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        int vw = vp[2];
        int vh = vp[3];

        if (vw > 0 && vh > 0) {
            std::vector<std::uint8_t> pixels(
                static_cast<std::size_t>(vw) * vh * 4);
            glReadPixels(0, 0, vw, vh, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            sf::Image img({static_cast<unsigned>(vw),
                           static_cast<unsigned>(vh)});
            // glReadPixels 原点在左下，sf::Image 原点在左上 → 翻转 Y
            for (int y = 0; y < vh; ++y) {
                int srcY = vh - 1 - y;
                for (int x = 0; x < vw; ++x) {
                    std::size_t si =
                        (static_cast<std::size_t>(srcY) * vw + x) * 4;
                    img.setPixel(
                        {static_cast<unsigned>(x), static_cast<unsigned>(y)},
                        sf::Color(pixels[si + 0], pixels[si + 1],
                                  pixels[si + 2], pixels[si + 3]));
                }
            }

            namespace fs = std::filesystem;
            fs::path dir = fs::current_path() / "screenshots";
            std::error_code ec;
            fs::create_directories(dir, ec);

            auto now = std::chrono::system_clock::now();
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &tt);
#else
            localtime_r(&tt, &tm);
#endif
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);

            fs::path file = dir / (std::string("shot_") + buf + ".png");
            if (img.saveToFile(file.string())) {
                NotificationSystem::instance().push(
                    "截图已保存: screenshots/" + file.filename().string(),
                    NotificationType::Success, 2.f);
            } else {
                NotificationSystem::instance().push(
                    "截图保存失败", NotificationType::Error, 2.f);
            }
        } else {
            NotificationSystem::instance().push(
                "截图失败: viewport 为空", NotificationType::Error, 2.f);
        }
    }
}