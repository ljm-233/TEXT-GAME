#include "game_world.h"
#include "coin.h"
#include "enemy.h"
#include "jump_pad.h"
#include "checkpoint.h"
#include "moving_platform.h"
#include "sound_manager.h"
#include "game_constants.h"
#include <algorithm>
#include <cmath>

GameWorld::GameWorld(std::unique_ptr<Level> level, int levelIndex)
    : level_(std::move(level)),
      levelIndex_(levelIndex) {

    camera_.setLevelBounds(static_cast<float>(level_->pixelWidth()),
                           static_cast<float>(level_->pixelHeight()));

    spawnPlayer(level_->playerSpawn());
    player_->setKillY(static_cast<float>(level_->pixelHeight() + 64));
    spawnLevelObjects();

    camera_.snapTo(player_->bounds().center());
}

void GameWorld::spawnPlayer(Vec2 spawn) {
    auto p = std::make_unique<Player>(spawn);
    player_ = p.get();
    objects_.push_back(std::move(p));
}

void GameWorld::spawnLevelObjects() {
    for (const auto& pos : level_->coinSpawns())
        objects_.push_back(std::make_unique<Coin>(pos, level_->tileSize()));
    totalCoins_ = static_cast<int>(level_->coinSpawns().size());

    for (const auto& pos : level_->enemySpawns())
        objects_.push_back(std::make_unique<Enemy>(pos, level_->tileSize()));

    for (const auto& pos : level_->jumpPadSpawns())
        objects_.push_back(std::make_unique<JumpPad>(pos, level_->tileSize()));

    for (const auto& pos : level_->checkpointSpawns())
        objects_.push_back(std::make_unique<Checkpoint>(pos, level_->tileSize()));

    // 水平移动平台：范围 4 格
    for (const auto& pos : level_->movingPlatformSpawns())
        objects_.push_back(std::make_unique<MovingPlatform>(
            pos, level_->tileSize(), 4.f * level_->tileSize(), true, 80.f));

    // 垂直移动平台：范围 3 格
    for (const auto& pos : level_->verticalPlatformSpawns())
        objects_.push_back(std::make_unique<MovingPlatform>(
            pos, level_->tileSize(), 3.f * level_->tileSize(), false, 60.f));
}

void GameWorld::setViewSize(float w, float h) {
    camera_.setViewSize(w, h);
}

void GameWorld::handleEvent(const sf::Event& event) {
    if (state_ != State::Playing) return;
    if (player_) player_->handleEvent(event);
}

void GameWorld::update(float dt) {
    if (state_ != State::Playing) return;

    accumulator_ += dt;
    int iterations = 0;
    while (accumulator_ >= GameConst::kFixedTimeStep && iterations < 8) {
        float step = GameConst::kFixedTimeStep;

        // ① 移动平台先更新
        for (auto& obj : objects_) {
            if (obj->type() == GameObject::Type::Platform) {
                obj->update(step, *level_);
            }
        }

        // ② 玩家物理
        if (player_) player_->update(step, *level_);

        // ③ 玩家站台检测
        if (player_) {
            AABB pb = player_->bounds();
            for (auto& obj : objects_) {
                if (obj->type() != GameObject::Type::Platform) continue;
                auto* mp = static_cast<MovingPlatform*>(obj.get());
                AABB plat = mp->bounds();

                bool overlapX = pb.right() > plat.left() + 1.f &&
                                pb.left() < plat.right() - 1.f;
                bool nearTop  = pb.bottom() >= plat.top() - 4.f &&
                                pb.bottom() <= plat.top() + 10.f;

                if (overlapX && nearTop && player_->velocity().y >= -1.f) {
                    player_->landOnPlatform(plat.top());
                    player_->moveBy(mp->lastDelta());
                    break;
                }
            }
        }

        // ④ 其他对象（跳过平台和玩家——它们已经单独更新过了）
        for (auto& obj : objects_) {
            auto t = obj->type();
            if (t != GameObject::Type::Platform &&
                t != GameObject::Type::Player) {
                obj->update(step, *level_);
            }
        }

        checkCollisionsSafe();
        if (state_ != State::Playing) return;
        accumulator_ -= step;
        ++iterations;
    }

    // 玩家跳跃/落地音效 + 粒子
    if (player_) {
        Vec2 pb = player_->bounds().center();
        float footX = pb.x;
        float footY = player_->bounds().bottom();
        if (player_->consumeJustJumped()) {
            SoundManager::instance().playJump();
            if (particlesEnabled_) particles_.emitJump({footX, footY});
        }
        if (player_->consumeJustLanded()) {
            SoundManager::instance().playLand();
            if (particlesEnabled_) particles_.emitLand({footX, footY});
        }
    }

    if (player_ && player_->consumeFellOut()) {
        --lives_;
        SoundManager::instance().playHurt();
        if (screenShake_) camera_.shake(8.f, 0.3f);
        if (lives_ <= 0) {
            state_ = State::GameOver;
            return;
        }
    }

    if (checkGoalReached()) {
        state_ = State::LevelComplete;
        return;
    }

    objects_.erase(
        std::remove_if(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<GameObject>& o) {
                return o->isRemovable();
            }),
        objects_.end());

    if (player_) camera_.follow(player_->bounds().center(), dt);
    camera_.updateShake(dt);
    if (particlesEnabled_) particles_.update(dt);
    else                    particles_.clear();
}

void GameWorld::checkCollisionsSafe() {
    if (!player_) return;
    AABB pb = player_->bounds();

    for (auto& obj : objects_) {
        if (obj.get() == player_) continue;
        if (obj->type() == GameObject::Type::Platform) continue;
        if (!obj->bounds().intersects(pb)) continue;

        switch (obj->type()) {
            case GameObject::Type::Coin: {
                auto* c = static_cast<Coin*>(obj.get());
                if (!c->collected()) {
                    c->collect();
                    ++coins_;
                    SoundManager::instance().playCoin();
                    if (particlesEnabled_) particles_.emitCoin(c->bounds().center());
                }
                break;
            }
            case GameObject::Type::Enemy: {
                auto* e = static_cast<Enemy*>(obj.get());
                if (e->killed()) break;

                bool falling = player_->velocity().y > 0.f;
                float overlap = pb.bottom() - e->bounds().top();
                bool fromAbove = overlap < GameConst::kStompTolerance;

                if (falling && fromAbove) {
                    e->kill();
                    player_->bounce();
                    SoundManager::instance().playStomp();
                    if (particlesEnabled_) particles_.emitStomp(e->bounds().center());
                    if (screenShake_) camera_.shake(4.f, 0.15f);
                } else if (!player_->isInvincible()) {
                    player_->takeDamage();
                    --lives_;
                    SoundManager::instance().playHurt();
                    if (particlesEnabled_) particles_.emitHurt(pb.center());
                    if (screenShake_) camera_.shake(8.f, 0.3f);
                    if (lives_ <= 0) {
                        state_ = State::GameOver;
                        return;
                    }
                }
                break;
            }
            case GameObject::Type::JumpPad: {
                auto* jp = static_cast<JumpPad*>(obj.get());
                if (jp->canTrigger()) {
                    player_->setVelocityY(JumpPad::kLaunchSpeed);
                    jp->trigger();
                    SoundManager::instance().playJump();
                    if (particlesEnabled_) particles_.emitJump(pb.center());
                }
                break;
            }
            case GameObject::Type::Checkpoint: {
                auto* cp = static_cast<Checkpoint*>(obj.get());
                if (!cp->isActive()) {
                    cp->activate();
                    player_->setSpawn(cp->respawnPos());
                    SoundManager::instance().playCheckpoint();
                    if (particlesEnabled_) particles_.emitCoin(cp->bounds().center());
                }
                break;
            }
            default:
                break;
        }
    }
}

bool GameWorld::checkGoalReached() const {
    if (!level_->hasGoal() || !player_) return false;
    AABB goal{level_->goalPos().x, level_->goalPos().y,
              static_cast<float>(level_->tileSize()),
              static_cast<float>(level_->tileSize())};
    return player_->bounds().intersects(goal);
}

void GameWorld::renderDebugColliders(sf::RenderTarget& target) {
    sf::RectangleShape rect;
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(1.f);

    for (const auto& obj : objects_) {
        AABB b = obj->bounds();
        rect.setSize({b.w, b.h});
        rect.setPosition({b.x, b.y});

        sf::Color color;
        switch (obj->type()) {
            case GameObject::Type::Player:     color = sf::Color(0, 255, 0);   break;
            case GameObject::Type::Enemy:      color = sf::Color(255, 0, 0);   break;
            case GameObject::Type::Coin:       color = sf::Color(255, 255, 0); break;
            case GameObject::Type::JumpPad:    color = sf::Color(0, 200, 255); break;
            case GameObject::Type::Checkpoint: color = sf::Color(255, 128, 0); break;
            case GameObject::Type::Platform:   color = sf::Color(160, 120, 80); break;
            default:                            color = sf::Color(200, 200, 200); break;
        }
        rect.setOutlineColor(color);
        target.draw(rect);
    }
}

void GameWorld::render(sf::RenderTarget& target) {
    Vec2 camTL = camera_.effectivePosition();
    level_->render(target,
                   camTL.x, camTL.y,
                   camera_.viewWidth(), camera_.viewHeight());

    for (const auto& obj : objects_) {
        obj->render(target);
    }

    if (particlesEnabled_) particles_.render(target);

    if (showColliders_) renderDebugColliders(target);
}

void GameWorld::reset() {
    lives_ = 3;
    coins_ = 0;
    state_ = State::Playing;
    accumulator_ = 0.f;
    particles_.clear();

    objects_.clear();
    player_ = nullptr;
    spawnPlayer(level_->playerSpawn());
    player_->setKillY(static_cast<float>(level_->pixelHeight() + 64));
    spawnLevelObjects();

    camera_.snapTo(player_->bounds().center());
}