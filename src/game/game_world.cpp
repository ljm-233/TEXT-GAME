#include "game_world.h"
#include "coin.h"
#include "enemy.h"
#include "jump_pad.h"
#include "checkpoint.h"
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
        for (auto& obj : objects_) {
            obj->update(GameConst::kFixedTimeStep, *level_);
        }
        checkCollisionsSafe();
        if (state_ != State::Playing) return;
        accumulator_ -= GameConst::kFixedTimeStep;
        ++iterations;
    }

    if (player_->consumeJustJumped()) SoundManager::instance().playJump();
    if (player_->consumeJustLanded()) SoundManager::instance().playLand();

    if (player_ && player_->consumeFellOut()) {
        --lives_;
        SoundManager::instance().playHurt();
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
}

void GameWorld::checkCollisionsSafe() {
    if (!player_) return;
    AABB pb = player_->bounds();

    for (auto& obj : objects_) {
        if (obj.get() == player_) continue;
        if (!obj->bounds().intersects(pb)) continue;

        switch (obj->type()) {
            case GameObject::Type::Coin: {
                auto* c = static_cast<Coin*>(obj.get());
                if (!c->collected()) {
                    c->collect();
                    ++coins_;
                    SoundManager::instance().playCoin();
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
                } else if (!player_->isInvincible()) {
                    player_->takeDamage();
                    --lives_;
                    SoundManager::instance().playHurt();
                    if (lives_ <= 0) {
                        state_ = State::GameOver;
                        return;
                    }
                }
                break;
            }
            case GameObject::Type::JumpPad: {
                // 只有从上方落下才触发
                if (player_->velocity().y >= 0.f) {
                    player_->setVelocityY(JumpPad::kLaunchSpeed);
                    SoundManager::instance().playJump();
                }
                break;
            }
            case GameObject::Type::Checkpoint: {
                auto* cp = static_cast<Checkpoint*>(obj.get());
                if (!cp->isActive()) {
                    cp->activate();
                    player_->setSpawn(cp->respawnPos());
                    SoundManager::instance().playCheckpoint();
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

void GameWorld::render(sf::RenderTarget& target) {
    Vec2 camTL = camera_.position();
    level_->render(target,
                   camTL.x, camTL.y,
                   camera_.viewWidth(), camera_.viewHeight());

    for (const auto& obj : objects_) {
        obj->render(target);
    }
}

void GameWorld::reset() {
    lives_ = 3;
    coins_ = 0;
    state_ = State::Playing;
    accumulator_ = 0.f;

    objects_.clear();
    player_ = nullptr;
    spawnPlayer(level_->playerSpawn());
    player_->setKillY(static_cast<float>(level_->pixelHeight() + 64));
    spawnLevelObjects();

    camera_.snapTo(player_->bounds().center());
}