#include "game_scene.h"
#include "utils/text_strings.h"
#include "utils/utf8.h"
#include "notification.h"
#include "sound_manager.h"
#include "infrastructure/keybindings.h"
#include "game_constants.h"
#include "game/score_rules.h"
#include "focus_group.h"
#include "infrastructure/gamepad.h"
#include "achievement.h"
#include "config/keys.h"
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

// ⭐ 作用域计时器（EMA 平滑写入）
struct ScopeTimer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point t0;
    float& out;
    explicit ScopeTimer(float& target)
          : t0(Clock::now()),
            out(target) {}
    ~ScopeTimer() {
        auto t1 = Clock::now();
        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
        out = out * 0.9f + ms * 0.1f;
    }
};
} // namespace

GameScene::GameScene(std::shared_ptr<Background> background, const sf::Font& font,
                     std::shared_ptr<Logger> logger,
                     std::shared_ptr<SaveManager> saveManager,
                     std::shared_ptr<Preferences> preferences,
                     std::shared_ptr<ResourceManager> resources,
                     std::shared_ptr<PlaytestRequest> playtest)
      : background_(std::move(background)),
        logger_(std::move(logger)),
        saveManager_(std::move(saveManager)),
        preferences_(std::move(preferences)),
        // 顺序必须与头文件里的声明顺序一致（playtest_ 声明在 resources_ 之前），
        // 否则 -Wreorder 会报
        playtest_(std::move(playtest)),
        resources_(std::move(resources)),
        font_(&font),
        hudText_(font, sf::String(), 20),
        overlayTitle_(font, sf::String(), 48),
        overlayHint_(font, sf::String(), 24),
        overlaySubHint_(font, sf::String(), 20),
        overlayTime_(font, sf::String(), 22),
        overlayStars_(font, sf::String(), 56),
        debugText_(font, sf::String(), 14),
        perfText_(font, sf::String(), 14) {
    debugText_.setFillColor(sf::Color(255, 255, 130));
    debugText_.setOutlineThickness(2.f);
    debugText_.setOutlineColor(sf::Color(0, 0, 0, 200));
    perfText_.setFillColor(sf::Color(160, 240, 255));
    perfText_.setOutlineThickness(2.f);
    perfText_.setOutlineColor(sf::Color(0, 0, 0, 200));

    // ⭐ PauseMenu 常驻，避免频繁析构 sf::Text
    pauseMenu_ = std::make_unique<PauseMenu>(font, preferences_,
                                             sf::Vector2f(kLogicalW, kLogicalH));

    hudText_.setFillColor(sf::Color::White);
    overlayTitle_.setFillColor(sf::Color(255, 255, 255));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    overlaySubHint_.setFillColor(sf::Color(180, 180, 200));
    overlayTime_.setFillColor(sf::Color(220, 220, 240));
    overlayStars_.setFillColor(sf::Color(255, 220, 80));
}

void GameScene::onEnter() {
    nextScene_ = SceneId::None;

    // ⭐ 「试玩」优先：编辑器按 F5 会把**未保存**的草稿放进交接通道。
    //    这条路必须在读 pending save 之前走 —— 试玩跟存档没有任何关系。
    //    用 take() 的返回值直接判断，而不是先 has() 再 take()：既少一次判空，
    //    也把"两次调用之间状态会变"这件事从根上消掉。
    if (auto req = playtest_ ? playtest_->take() : std::nullopt) {
        playtestMode_ = true;
        playtestSource_ = req->sourceName;

        std::string text;
        for (const auto& line : req->lines) {
            text += line;
            text += '\n';
        }
        auto level = std::make_unique<Level>();
        if (!level->loadFromString(text) || !buildWorld(std::move(level), 0)) {
            logger_->error("试玩关卡解析失败: " + playtestSource_);
            NotificationSystem::instance().push(Str::T(Str::NotifLevelLoadFailed),
                                                NotificationType::Error, 5.f);
            playtestMode_ = false;
            nextScene_ = SceneId::Back; // 退回编辑器，而不是把用户丢在主菜单
            return;
        }
        parallax_ = std::make_unique<ParallaxBackground>();
        logger_->info("试玩进入: " + playtestSource_);
        // 明说一句"这是试玩、不计进度"。只靠窗口标题不够显眼，
        // 而"为什么刚才那关的星星没记上"正是试玩最容易让人困惑的地方。
        NotificationSystem::instance().push(std::string(Str::T(Str::PlaytestBadge)) +
                                                " - " + playtestSource_,
                                            NotificationType::Info, 4.f);
        return;
    }

    // ⚠️ 正式关卡要把上一次试玩的标志清掉。不清的话，"试玩 → 退出 → 从选关页
    //    进正式关卡"这一串会让正式关卡也**不写存档**，而且完全不报错。
    playtestMode_ = false;
    playtestSource_.clear();
    // ⭐ 场景常驻后，onEnter 可能被多次调用。
    //    pending save 为空时沿用上次的 save_
    //
    // ⚠️ takePendingSave() 是**取走即清空**（save_manager.h:41 里 `pending_ = {}`）。
    //    这里曾经多出一行 `save_ = saveManager_->takePendingSave();`，
    //    于是第二次拿到的是空 SaveInfo，把刚读到的存档又冲掉了 ——
    //    后果是 levelIndex_ 恒为 1、save_.filename 恒为空，
    //    进而 updateProgress / setLevelStar / setLevelBestTime 全部写入失败，
    //    通关不记进度、星级与 PB 永不落盘。别再调第二次。
    SaveInfo pending = saveManager_->takePendingSave();
    if (!pending.filename.empty()) {
        save_ = pending;
    }
    syncFocus();

    levelIndex_ = std::max(1, save_.currentLevel);

    // ⭐ 读取本关之前的 PB
    if (levelIndex_ >= 1 &&
        levelIndex_ <= static_cast<int>(save_.levelBestTimes.size())) {
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

    // ⭐ 从 SettingsScene 返回：保持 paused_ 不变（仍显示 PauseMenu）
    //    但焦点被 SettingsScene 抢走了，要让 PauseMenu 重新接管
    if (paused_ && pauseMenu_) {
        pauseMenu_->syncFocus();
    } else {
        syncFocus();
    }
}

std::string GameScene::windowTitleHint() const {
    // 试玩时明确标出来：否则用户会奇怪"为什么刚才那关的进度没记上"
    if (playtestMode_) {
        return std::string(Str::T(Str::PlaytestBadge)) + " - " + playtestSource_;
    }
    if (paused_) {
        return Str::T(Str::WinTitleLevelPrefix) + std::to_string(levelIndex_) +
               Str::T(Str::WinTitlePausedSuffix);
    }
    return Str::T(Str::WinTitleLevelPrefix) + std::to_string(levelIndex_) +
           Str::T(Str::WinTitleLevelSuffix);
}

void GameScene::subscribeWorldEvents() {
    if (!world_)
        return;

    world_->bus().subscribe([this](const GameEvent& e) {
        const bool particlesOn = preferences_->getBool(ConfigKey::kParticles, true);

        std::visit(
            [this, particlesOn](const auto& ev) {
                using T = std::decay_t<decltype(ev)>;
                auto& gp = Gamepad::instance();

                if constexpr (std::is_same_v<T, EvJumped>) {
                    SoundManager::instance().playJump();
                    if (particlesOn)
                        particles_.emitJump(ev.pos);
                } else if constexpr (std::is_same_v<T, EvLanded>) {
                    SoundManager::instance().playLand();
                    if (particlesOn)
                        particles_.emitLand(ev.pos, ev.intensity);
                    // ⭐ 重落地才振动
                    if (ev.intensity > 1.0f) {
                        gp.vibrate(0.f, 0.20f, 0.05f);
                    }
                } else if constexpr (std::is_same_v<T, EvCoined>) {
                    SoundManager::instance().playCoin();
                    if (particlesOn)
                        particles_.emitCoin(ev.pos);
                    gp.vibrate(0.f, 0.15f, 0.03f);
                    AchievementManager::instance().unlock("first_coin");
                } else if constexpr (std::is_same_v<T, EvStomped>) {
                    SoundManager::instance().playStomp();
                    if (particlesOn)
                        particles_.emitStomp(ev.pos);
                    hitstopTimer_ = 0.06f;
                    gp.vibrate(0.f, 0.30f, 0.08f);
                } else if constexpr (std::is_same_v<T, EvHurt>) {
                    SoundManager::instance().playHurt();
                    if (particlesOn)
                        particles_.emitHurt(ev.pos);
                    hitstopTimer_ = 0.04f;
                    gp.vibrate(0.40f, 0.60f, 0.15f);
                    tookDamageThisLevel_ = true;
                } else if constexpr (std::is_same_v<T, EvCheckpoint>) {
                    SoundManager::instance().playCheckpoint();
                    if (particlesOn)
                        particles_.emitCoin(ev.pos);
                    gp.vibrate(0.f, 0.25f, 0.10f);
                } else if constexpr (std::is_same_v<T, EvJumpPad>) {
                    SoundManager::instance().playJump();
                    if (particlesOn)
                        particles_.emitJump(ev.pos);
                    gp.vibrate(0.30f, 0.50f, 0.10f);
                } else if constexpr (std::is_same_v<T, EvLevelComplete>) {
                    finalCoins_ = world_->coins();
                    finalTotalCoins_ = world_->totalCoins();
                    finalStars_ = calcStars();
                    // 试玩不写任何存档/PB/成就 —— 那是"看看改得怎么样"，
                    // 不该污染真实进度
                    if (!playtestMode_) {
                        applyStars();
                        saveManager_->updateProgress(save_.filename, world_->coins(),
                                                     levelIndex_);

                        newRecord_ = ScoreRules::isNewRecord(prevBestTime_, levelTime_);
                        if (newRecord_) {
                            saveManager_->setLevelBestTime(save_.filename, levelIndex_,
                                                           levelTime_);
                        }
                        // 每关最佳金币：成绩页要**按关**显示金币，而 save.coins 是跨关
                        // 累计总数、拆不出来。setter 内部只增不减，所以直接调即可。
                        saveManager_->setLevelBestCoins(save_.filename, levelIndex_,
                                                        world_->coins());
                    }

                    SoundManager::instance().playLevelComplete();
                    NotificationSystem::instance().push(
                        Str::T(Str::NotifLevelCompleteStars) +
                            std::to_string(finalStars_) + Str::T(Str::NotifStarSuffix),
                        NotificationType::Success, 5.f);

                    // ⭐ 庆祝振动
                    gp.vibrate(0.70f, 0.80f, 0.40f);

                    // ⭐ 检查成就（试玩不解锁）
                    if (!playtestMode_)
                        checkAchievements();

                    screenFlashTimer_ = 0.35f;
                    screenFlashDuration_ = 0.35f;
                    screenFlashColor_ = sf::Color(120, 255, 150);
                } else if constexpr (std::is_same_v<T, EvLifeExhausted>) {
                    // 重生时清空粒子：世界不再管这件事，交给持有者
                    particles_.clear();

                    SoundManager::instance().playGameOver();
                    NotificationSystem::instance().push(Str::T(Str::NotifLifeExhausted),
                                                        NotificationType::Error, 4.f);

                    // ⭐ 长振
                    gp.vibrate(0.60f, 0.70f, 0.30f);

                    levelTime_ = 0.f;
                    lastOverlayState_ = GameWorld::State::Playing;
                }
            },
            e);
    });
}

bool GameScene::loadLevel(int index) {
    auto levelPath = resources_->get("levels", "level" + std::to_string(index) + ".txt");
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

    logger_->info("关卡已加载: level" + std::to_string(index) + " " +
                  std::to_string(level->width()) + "x" + std::to_string(level->height()) +
                  " 瓦片");

    if (!buildWorld(std::move(level), index))
        return false;

    // ⭐ 读取新关的 PB（试玩不走这里 —— 试玩没有"关卡序号"这回事）
    if (index >= 1 && index <= static_cast<int>(save_.levelBestTimes.size())) {
        prevBestTime_ = save_.levelBestTimes[index - 1];
    } else {
        prevBestTime_ = 0.f;
    }

    if (preferences_->getBool(ConfigKey::kLevelIntro, true)) {
        if (!intro_) {
            intro_ = std::make_unique<LevelIntro>(*font_);
        }
        intro_->restart(index, static_cast<float>(world_->totalCoins()),
                        world_->level().name());
        introActive_ = true;
    } else {
        introActive_ = false;
    }
    return true;
}

bool GameScene::buildWorld(std::unique_ptr<Level> level, int index) {
    if (!level)
        return false;

    level->setFont(font_);

    world_ = std::make_unique<GameWorld>(std::move(level), index,
                                         GameWorld::SpriteSheets::fromFactories());
    world_->setViewSize(kLogicalW, kLogicalH);

    // ⭐ 校验初始生命值，非法值回退到 1
    int lives = preferences_->getInt(ConfigKey::kInitialLives, 1);
    if (lives != 1 && lives != 3 && lives != 5 && lives != 10 && lives != 100) {
        lives = 1;
        preferences_->setInt(ConfigKey::kInitialLives, 1);
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
    tookDamageThisLevel_ = false;
    // 试玩不放开场白：那个界面显示的是"关卡 N"，而试玩根本没有序号
    introActive_ = false;

    lastHudLives_ = -1;
    lastOverlayState_ = GameWorld::State::Playing;

    // ⭐ 调试：切关 / 重载时清除暂停
    paused_ = false;

    // ⭐ 调试：保持无敌状态
    if (debugInvincible_) {
        world_->player().setInvincible(true);
    }

    subscribeWorldEvents();
    syncFocus();

    return true;
}

bool GameScene::handleDebugKey(sf::Keyboard::Key k) {
    // ⭐ 任意调试键都算
    if (k >= sf::Keyboard::Key::F1 && k <= sf::Keyboard::Key::F10) {
        AchievementManager::instance().unlock("debug_mode");
    }

    switch (k) {
    case sf::Keyboard::Key::F1:
        debugHud_ = !debugHud_;
        NotificationSystem::instance().push(debugHud_ ? "调试 HUD: 开" : "调试 HUD: 关",
                                            NotificationType::Info, 1.2f);
        return true;

    case sf::Keyboard::Key::F2:
        debugInvincible_ = !debugInvincible_;
        if (world_)
            world_->player().setInvincible(debugInvincible_);
        NotificationSystem::instance().push(debugInvincible_ ? "无敌: 开" : "无敌: 关",
                                            NotificationType::Info, 1.2f);
        return true;

    case sf::Keyboard::Key::F3:
        if (world_)
            world_->killAllEnemies();
        NotificationSystem::instance().push("已清空敌人", NotificationType::Info, 1.2f);
        return true;

    case sf::Keyboard::Key::F4:
        loadLevel(levelIndex_);
        NotificationSystem::instance().push("重载关卡 " + std::to_string(levelIndex_),
                                            NotificationType::Info, 1.2f);
        return true;

    case sf::Keyboard::Key::F5:
        if (levelIndex_ > 1) {
            loadLevel(levelIndex_ - 1);
            NotificationSystem::instance().push("关卡 " + std::to_string(levelIndex_),
                                                NotificationType::Info, 1.2f);
        }
        return true;

    case sf::Keyboard::Key::F6:
        if (levelIndex_ < kMaxLevels) {
            loadLevel(levelIndex_ + 1);
            NotificationSystem::instance().push("关卡 " + std::to_string(levelIndex_),
                                                NotificationType::Info, 1.2f);
        }
        return true;

    case sf::Keyboard::Key::F7: {
        // 1.0 → 0.5 → 0.25 → 0.1 → 1.0
        if (debugTimeScale_ > 0.75f)
            debugTimeScale_ = 0.5f;
        else if (debugTimeScale_ > 0.4f)
            debugTimeScale_ = 0.25f;
        else if (debugTimeScale_ > 0.15f)
            debugTimeScale_ = 0.1f;
        else
            debugTimeScale_ = 1.f;

        char buf[32];
        std::snprintf(buf, sizeof(buf), "慢动作: %.2fx",
                      static_cast<double>(debugTimeScale_));
        NotificationSystem::instance().push(buf, NotificationType::Info, 1.2f);
        return true;
    }

    case sf::Keyboard::Key::F8:
        debugShowColliders_ = !debugShowColliders_;
        NotificationSystem::instance().push(debugShowColliders_ ? "碰撞盒: 开"
                                                                : "碰撞盒: 关",
                                            NotificationType::Info, 1.2f);
        return true;

    case sf::Keyboard::Key::F9:
        pendingScreenshot_ = true;
        return true;

    case sf::Keyboard::Key::F10:
        perfHud_ = !perfHud_;
        NotificationSystem::instance().push(perfHud_ ? "性能面板: 开" : "性能面板: 关",
                                            NotificationType::Info, 1.2f);
        return true;

    default:
        return false;
    }
}

void GameScene::renderDebugHud(sf::RenderTarget& rt, Window& window) {
    if (!world_)
        return;

    const auto& level = world_->level();
    const auto& player = world_->player();

    Vec2 pos = player.position();
    Vec2 vel = player.velocity();
    Vec2 cam = world_->cameraCenter();

    // 鼠标世界坐标
    sf::Vector2i mousePix = sf::Mouse::getPosition(window.native());
    sf::Vector2f mouseWorld = window.native().mapPixelToCoords(mousePix, lastWorldView_);
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
    if (!level.name().empty())
        oss << " [" << level.name() << "]";
    oss << "  " << level.width() << "x" << level.height();
    oss << "\n玩家 (" << pos.x << ", " << pos.y << ")";
    oss << "  vel (" << vel.x << ", " << vel.y << ")";
    oss << "\n" << (player.onGround() ? "地面" : "空中");
    if (player.isInvincible())
        oss << "  无敌";
    if (player.isInvinciblePersistent())
        oss << " ⚡";
    oss << "\n相机 (" << cam.x << ", " << cam.y << ")";
    oss << "\n金币 " << world_->coins() << "/" << world_->totalCoins() << "  生命 "
        << world_->lives();
    oss << "\n鼠标 tile(" << tx << ", " << ty << ") '" << tileChar << "'";
    oss << "\n[F1]HUD [F2]无敌 [F3]杀敌 [F4]重载 [F5]上关 [F6]下关";
    oss << " [F7]慢动作 [F8]碰撞盒 [F9]截图";

    debugText_.setString(toSf(oss.str()));

    // 背景
    auto b = debugText_.getLocalBounds();
    const float padX = 8.f, padY = 6.f;
    sf::RectangleShape bg({b.size.x + padX * 2.f, b.size.y + padY * 2.f});
    bg.setPosition({12.f, 42.f});
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(120, 120, 160, 180));
    rt.draw(bg);

    debugText_.setPosition({12.f + padX - b.position.x, 42.f + padY - b.position.y});
    rt.draw(debugText_);
}

void GameScene::renderPerfHud(sf::RenderTarget& rt, Window& window, float winW,
                              float /*winH*/) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    float fps = (perfFrameMs_ > 0.01f) ? (1000.f / perfFrameMs_) : 0.f;
    float endFrameMs = window.upscalePostMs();

    oss << "性能面板 [F10]\n";
    oss << "帧时间      " << perfFrameMs_ << " ms";
    oss << "  (" << std::setprecision(1) << fps << " FPS)\n";
    oss << std::setprecision(2);
    oss << "update      " << perfUpdateMs_ << " ms\n";
    oss << "render      " << perfRenderMs_ << " ms\n";
    oss << "上采样+后处理 " << endFrameMs << " ms\n";
    oss << "渲染缩放    " << std::setprecision(0)
        << static_cast<int>(window.getRenderScale() * 100.f) << "%\n";
    oss << "超分模式    ";
    switch (window.getUpscaleMode()) {
    case 0:
        oss << "关";
        break;
    case 1:
        oss << "双三次";
        break;
    case 2:
        oss << "FSR1";
        break;
    default:
        oss << "?";
        break;
    }

    perfText_.setString(toSf(oss.str()));

    auto b = perfText_.getLocalBounds();
    const float padX = 10.f, padY = 8.f;
    float boxW = b.size.x + padX * 2.f;
    float boxH = b.size.y + padY * 2.f;

    float bx = winW - boxW - 12.f;
    float by = 42.f;

    sf::RectangleShape bg({boxW, boxH});
    bg.setPosition({bx, by});
    bg.setFillColor(sf::Color(0, 0, 0, 170));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(160, 220, 255, 180));
    rt.draw(bg);

    perfText_.setPosition({bx + padX - b.position.x, by + padY - b.position.y});
    rt.draw(perfText_);
}

void GameScene::syncFocus() {
    // 暂停菜单打开时，焦点交给 PauseMenu 管理，这里不动
    if (paused_ && pauseMenu_)
        return;

    // 开场动画显示中，不设焦点
    if (introActive_) {
        FocusGroup::instance().clear();
        return;
    }

    // LevelComplete 状态：设 overlay 按钮
    if (world_ && world_->state() == GameWorld::State::LevelComplete &&
        !overlayButtons_.empty()) {
        std::vector<Button*> items;
        for (auto& b : overlayButtons_)
            items.push_back(b.get());
        FocusGroup::instance().setItems(items);
        return;
    }

    // Playing 状态：无焦点
    FocusGroup::instance().clear();
}

bool GameScene::advanceToNextLevel() {
    int next = levelIndex_ + 1;
    if (next > kMaxLevels)
        return false;

    // 先确认关卡文件存在，避免 loadLevel 打 error 日志
    auto nextPath = resources_->get("levels", "level" + std::to_string(next) + ".txt");
    if (!std::filesystem::exists(nextPath))
        return false;

    if (!loadLevel(next))
        return false;

    NotificationSystem::instance().push(Str::T(Str::NotifEnterLevel) +
                                            std::to_string(levelIndex_) +
                                            Str::T(Str::NotifLevelSuffix),
                                        NotificationType::Info);
    if (!playtestMode_)
        saveManager_->updateProgress(save_.filename, world_->coins(), levelIndex_);
    return true;
}

void GameScene::refreshHud() {
    int lives = world_->lives();
    int coins = world_->coins();

    if (lives == lastHudLives_ && coins == lastHudCoins_ &&
        levelIndex_ == lastHudLevel_) {
        return;
    }

    lastHudLives_ = lives;
    lastHudCoins_ = coins;
    lastHudLevel_ = levelIndex_;

    char timeBuf[32];
    std::snprintf(timeBuf, sizeof(timeBuf), "%.1f", levelTime_);

    std::string hud = Str::T(Str::HudSave) + save_.name + "   " + Str::T(Str::HudLevel) +
                      std::to_string(levelIndex_) + "   " + Str::T(Str::HudLives) +
                      std::to_string(lives) + "   " + Str::T(Str::HudCoins) +
                      std::to_string(coins) + " / " +
                      std::to_string(world_->totalCoins()) + "   " +
                      Str::T(Str::HudTime) + timeBuf + "s";

    if (world_->player().keys() > 0) {
        hud += "   " + Str::T(Str::HudKeys) + std::to_string(world_->player().keys());
    }
    hud += "   " + Str::T(Str::HudHelp);
    hudText_.setString(toSf(hud));
}

int GameScene::targetTime() const {
    // 没世界时给个中庸值（等价于 10 枚金币的目标时间）
    if (!world_)
        return 60;
    return ScoreRules::targetTimeSeconds(world_->totalCoins());
}

int GameScene::calcStars() const {
    if (!world_)
        return 1;
    // 规则本身在 game/score_rules.h —— 纯算术，放在那里才测得到
    return ScoreRules::starsFor(world_->coins(), world_->totalCoins(), levelTime_);
}

void GameScene::applyStars() {
    if (finalStars_ <= 0)
        return;
    saveManager_->setLevelStar(save_.filename, levelIndex_, finalStars_);
}

void GameScene::checkAchievements() {
    auto& am = AchievementManager::instance();

    // 第一次通关
    am.unlock("first_level");

    // 单关 3 星
    if (finalStars_ >= 3)
        am.unlock("three_stars");

    // 单关全金币
    if (finalTotalCoins_ > 0 && finalCoins_ == finalTotalCoins_) {
        am.unlock("perfect_level");
    }

    // 速通
    if (levelTime_ > 0.f && levelTime_ < 30.f) {
        am.unlock("speedrun");
    }

    // 无伤
    if (!tookDamageThisLevel_) {
        am.unlock("no_damage");
    }

    // 读最新存档，检查全关 / 全星
    SaveInfo updated;
    if (saveManager_->loadSave(save_.filename, updated)) {
        const auto& stars = updated.levelStars;
        if (!stars.empty()) {
            bool allCleared =
                std::all_of(stars.begin(), stars.end(), [](int s) { return s > 0; });
            if (allCleared)
                am.unlock("all_levels");

            bool allThree =
                std::all_of(stars.begin(), stars.end(), [](int s) { return s >= 3; });
            if (allThree)
                am.unlock("all_stars");
        }
    }
}

void GameScene::rebuildOverlayButtons() {
    overlayButtons_.clear();

    // 现在只有 LevelComplete 需要按钮
    if (world_->state() != GameWorld::State::LevelComplete)
        return;

    const float winW = lastOverlayWinW_;
    const float winH = lastOverlayWinH_;

    const float bw = 180.f;
    const float bh = 50.f;
    const float gap = 15.f;
    const float totalW = bw * 3.f + gap * 2.f;
    const float x0 = (winW - totalW) * 0.5f;
    const float y = winH * 0.5f + 130.f;

    overlayButtons_.push_back(std::make_unique<Button>(Str::T(Str::BtnNextLevel), *font_,
                                                       sf::Vector2f{x0, y},
                                                       sf::Vector2f{bw, bh}, 22));
    overlayButtons_.push_back(std::make_unique<Button>(Str::T(Str::BtnReplay), *font_,
                                                       sf::Vector2f{x0 + bw + gap, y},
                                                       sf::Vector2f{bw, bh}, 22));
    overlayButtons_.push_back(std::make_unique<Button>(
        Str::T(Str::Back), *font_, sf::Vector2f{x0 + (bw + gap) * 2.f, y},
        sf::Vector2f{bw, bh}, 22));
}

void GameScene::refreshOverlayLayout(float winW, float winH) {
    auto state = world_->state();
    if (winW == lastOverlayWinW_ && winH == lastOverlayWinH_ &&
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
    overlayTitle_.setOrigin(
        {tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f});
    overlayTitle_.setPosition({winW / 2.f, winH / 2.f - 140.f});

    // 第一行：金币
    std::string stats = Str::T(Str::OverlayCoins) + std::to_string(finalCoins_) + " / " +
                        std::to_string(finalTotalCoins_);
    overlayHint_.setString(toSf(stats));
    overlayHint_.setFillColor(sf::Color(200, 200, 220));
    auto hb = overlayHint_.getLocalBounds();
    overlayHint_.setOrigin(
        {hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f});
    overlayHint_.setPosition({winW / 2.f, winH / 2.f - 70.f});

    // 第二行：PB（只在通关时显示）
    std::string pbLine;
    if (newRecord_) {
        pbLine = Str::T(Str::OverlayNewRecord);
    } else if (prevBestTime_ > 0.f) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s%.2f%s", Str::T(Str::OverlayBestTime).c_str(),
                      prevBestTime_, Str::T(Str::OverlaySeconds).c_str());
        pbLine = buf;
    }
    overlaySubHint_.setString(toSf(pbLine));

    char timeBuf[128];
    std::snprintf(timeBuf, sizeof(timeBuf), "%s%.1f%s  /  %s%d%s",
                  Str::T(Str::OverlayTime).c_str(), levelTime_,
                  Str::T(Str::OverlaySeconds).c_str(), Str::T(Str::OverlayTarget).c_str(),
                  targetTime(), Str::T(Str::OverlaySeconds).c_str());
    overlayTime_.setString(toSf(timeBuf));
    auto tb2 = overlayTime_.getLocalBounds();
    overlayTime_.setOrigin(
        {tb2.position.x + tb2.size.x / 2.f, tb2.position.y + tb2.size.y / 2.f});
    overlayTime_.setPosition({winW / 2.f, winH / 2.f - 25.f});

    {
        std::string stars;
        for (int i = 0; i < 3; ++i) {
            stars += (i < finalStars_) ? "\u2605" : "\u2606";
            if (i < 2)
                stars += "  ";
        }
        overlayStars_.setString(toSf(stars));
        auto sb = overlayStars_.getLocalBounds();
        overlayStars_.setOrigin(
            {sb.position.x + sb.size.x / 2.f, sb.position.y + sb.size.y / 2.f});
        overlayStars_.setPosition({winW / 2.f, winH / 2.f + 60.f});
    }
    // PB 行位置（时间下面）
    {
        auto sb = overlaySubHint_.getLocalBounds();
        overlaySubHint_.setOrigin(
            {sb.position.x + sb.size.x / 2.f, sb.position.y + sb.size.y / 2.f});
        overlaySubHint_.setFillColor(newRecord_ ? sf::Color(255, 220, 80)
                                                : sf::Color(180, 180, 200));
        overlaySubHint_.setPosition({winW / 2.f, winH / 2.f + 20.f});
    }
}

void GameScene::handleEvent(const sf::Event& event) {
    // ⭐ 调试快捷键：优先响应
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (handleDebugKey(kp->code))
            return;
    }

    if (paused_) {
        pauseMenu_->handleEvent(event);
        return;
    }

    if (introActive_) {
        if (event.getIf<sf::Event::KeyPressed>() ||
            event.getIf<sf::Event::MouseButtonPressed>()) {
            intro_->skip();
            introActive_ = false;
        }
        return;
    }

    auto state = world_->state();

    if (state != GameWorld::State::Playing) {
        // ⭐ 先转发给 overlay 按钮
        for (auto& b : overlayButtons_)
            b->handleEvent(event);

        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == KeyBindings::instance().get(KeyBindings::Pause)) {
                nextScene_ = SceneId::Back;
                return;
            }
            if (kp->code == KeyBindings::instance().get(KeyBindings::Restart)) {
                world_->reset();
                particles_.clear();
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
                            Str::T(Str::NotifAllClear), NotificationType::Success, 5.f);
                    }
                }
            }
        }
        return;
    }

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == KeyBindings::instance().get(KeyBindings::Pause)) {
            paused_ = true;
            pauseMenu_->reset();
            return;
        }
        if (kp->code == KeyBindings::instance().get(KeyBindings::Restart)) {
            world_->reset();
            particles_.clear();
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
    // ⭐ 帧时间（含 vsync 等待）+ update 耗时
    perfFrameMs_ = perfFrameClock_.restart().asSeconds() * 1000.f;
    ScopeTimer timer(perfUpdateMs_);

    if (screenFlashTimer_ > 0.f)
        screenFlashTimer_ -= dt;

    if (paused_) {
        pauseMenu_->update(dt);
        auto action = pauseMenu_->consumeAction();
        if (action == PauseMenu::Action::Resume) {
            paused_ = false;
            syncFocus();
        } else if (action == PauseMenu::Action::SaveAndQuit) {
            saveManager_->updateProgress(save_.filename, world_->coins(), levelIndex_);
            nextScene_ = SceneId::Back;
        } else if (action == PauseMenu::Action::OpenSettings) {
            // ⭐ push SettingsScene。paused_ 保持 true，
            //    返回时 onResume 会 unpause
            nextScene_ = SceneId::Settings;
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

    if (introActive_) {
        intro_->update(dt);
        if (parallax_)
            parallax_->update(dt);
        if (intro_->isFinished())
            introActive_ = false;
        return;
    }

    auto state = world_->state();
    if (state != GameWorld::State::Playing) {
        // ⭐ 处理 overlay 按钮点击
        if (state == GameWorld::State::LevelComplete && overlayButtons_.size() >= 3) {
            if (overlayButtons_[0]->consumeClick()) {
                if (!advanceToNextLevel()) {
                    NotificationSystem::instance().push(Str::T(Str::NotifAllClear),
                                                        NotificationType::Success, 5.f);
                }
                return;
            }
            if (overlayButtons_[1]->consumeClick()) {
                world_->reset();
                particles_.clear();
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

    // 粒子开关每帧同步一次。update 先于 render，所以这一处赋值对两者都生效，
    // 不需要在 render 里再读一遍偏好。
    particlesOn_ = preferences_->getBool(ConfigKey::kParticles, true);

    // 开关关闭时直接清空，行为与旧实现一致（旧代码在 GameWorld::update 里）
    if (particlesOn_)
        particles_.update(dt);
    else
        particles_.clear();
    if (parallax_)
        parallax_->update(dt);
}

void GameScene::renderStateOverlay(sf::RenderTarget& rt, float winW, float winH) {
    refreshOverlayLayout(winW, winH);

    auto state = world_->state();
    if (state == GameWorld::State::Playing)
        return;

    rt.draw(overlayBg_);
    rt.draw(overlayTitle_);
    rt.draw(overlayHint_);
    if (state == GameWorld::State::LevelComplete) {
        rt.draw(overlayTime_);
        rt.draw(overlayStars_);
    }
    rt.draw(overlaySubHint_);

    for (auto& b : overlayButtons_)
        b->render(rt);
}

void GameScene::render(Window& window) {
    ScopeTimer timer(perfRenderMs_);

    auto& rt = window.target();
    auto winSize = window.native().getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    sf::View screenView(sf::FloatRect({0.f, 0.f}, {winW, winH}));

    rt.setView(screenView);
    rt.clear(sf::Color::Black);
    if (background_)
        background_->render(rt);

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

    world_->setShowColliders(debugShowColliders_ ||
                             preferences_->getBool(ConfigKey::kShowColliders, false));
    world_->setScreenShake(preferences_->getBool(ConfigKey::kScreenShake, true));
    // 抖动幅度：夹在 0~2 倍。0 等于关掉震动（但开关仍显示为"开"），
    // 所以上界给 2 倍而不是更高，免得画面抖到看不清角色在哪。
    world_->setShakeIntensity(static_cast<float>(
        std::clamp(preferences_->getDouble(ConfigKey::kShakeIntensity, 1.0), 0.0, 2.0)));
    // 粒子密度：0~3 档 → 0 / 0.5 / 1.0 / 1.5 倍。最高档略微超过 1 是为了
    // 让"多"这个档位看得出区别，而不是和"标准"一模一样。
    {
        const int d =
            std::clamp(preferences_->getInt(ConfigKey::kParticleDensity, 2), 0, 3);
        static const float kDensityScale[] = {0.f, 0.5f, 1.0f, 1.5f};
        particles_.setDensityScale(kDensityScale[d]);
    }
    world_->setPseudo3D(preferences_->getBool(ConfigKey::kPseudo3d, true));
    world_->setPlayerAnimation(preferences_->getBool(ConfigKey::kPlayerAnimation, true));

    Vec2 camCenter = world_->cameraCenter();
    worldView_.setCenter({camCenter.x, camCenter.y});
    rt.setView(worldView_);
    lastWorldView_ = worldView_; // ⭐ 供调试 HUD 用

    if (parallax_ && preferences_->getBool(ConfigKey::kParallax, true)) {
        float camLeft = camCenter.x - kLogicalW * 0.5f;
        float camTop = camCenter.y - kLogicalH * 0.5f;
        parallax_->render(rt, camLeft, camTop, kLogicalW, kLogicalH);
    }

    world_->render(rt);

    // 粒子画在场景对象之上。
    // 注意：调试碰撞盒是在 world_->render() 内部画的，所以粒子现在会盖住它 ——
    // 与改造前（GameWorld 里 粒子 -> 碰撞盒 的顺序）不同，仅影响 F8 调试视图。
    if (particlesOn_)
        particles_.render(rt);

    rt.setView(screenView);

    refreshHud();
    hudText_.setPosition({20.f, 16.f});
    rt.draw(hudText_);

    // ⭐ 调试 HUD
    if (debugHud_) {
        renderDebugHud(rt, window);
    }

    // ⭐ 性能面板
    if (perfHud_) {
        renderPerfHud(rt, window, winW, winH);
    }

    if (introActive_)
        intro_->render(rt, winW, winH);

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
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(vw) * vh * 4);
            glReadPixels(0, 0, vw, vh, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            // 显式写出 Vector2u：直接写 img({vw, vh}) 在 clang 和 MSVC 上会与
            // sf::Image 的其它重载产生歧义（GCC 接受，所以本地看不出来）。
            // 带 sf::Color 参数的那几处没这个问题，不用改。
            sf::Image img(
                sf::Vector2u{static_cast<unsigned>(vw), static_cast<unsigned>(vh)});
            // glReadPixels 原点在左下，sf::Image 原点在左上 → 翻转 Y
            for (int y = 0; y < vh; ++y) {
                int srcY = vh - 1 - y;
                for (int x = 0; x < vw; ++x) {
                    std::size_t si = (static_cast<std::size_t>(srcY) * vw + x) * 4;
                    img.setPixel({static_cast<unsigned>(x), static_cast<unsigned>(y)},
                                 sf::Color(pixels[si + 0], pixels[si + 1], pixels[si + 2],
                                           pixels[si + 3]));
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
                NotificationSystem::instance().push("截图已保存: screenshots/" +
                                                        file.filename().string(),
                                                    NotificationType::Success, 2.f);
            } else {
                NotificationSystem::instance().push("截图保存失败",
                                                    NotificationType::Error, 2.f);
            }
        } else {
            NotificationSystem::instance().push("截图失败: viewport 为空",
                                                NotificationType::Error, 2.f);
        }
    }
}