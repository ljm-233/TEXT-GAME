#include "game_world.h"
#include "coin.h"
#include "enemy.h"
#include <algorithm>

GameWorld::GameWorld(std::unique_ptr<Level> level)
    : level_(std::move(level)) {

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
    // 金币
    for (const auto& pos : level_->coinSpawns()) {
        objects_.push_back(std::make_unique<Coin>(pos));
    }
    totalCoins_ = static_cast<int>(level_->coinSpawns().size());

    // 敌人
    for (const auto& pos : level_->enemySpawns()) {
        objects_.push_back(std::make_unique<Enemy>(pos, level_->tileSize()));
    }
}

void GameWorld::setViewSize(float w, float h) {
    camera_.setViewSize(w, h);
}

void GameWorld::handleEvent(const sf::Event& event) {
    if (player_) player_->handleEvent(event);
}

void GameWorld::update(float dt) {
    constexpr float kFixedStep = 1.f / 120.f;
    static float accumulator = 0.f;

    accumulator += dt;
    int iterations = 0;
    while (accumulator >= kFixedStep && iterations < 8) {
        for (auto& obj : objects_) {
            obj->update(kFixedStep, *level_);
        }
        checkCollisions();
        accumulator -= kFixedStep;
        ++iterations;
    }

    if (player_ && player_->consumeFellOut()) {
        --lives_;
        ++deaths_;
        if (lives_ <= 0) levelDone_ = true;
    }

    if (!levelDone_ && checkGoalReached()) {
        levelDone_ = true;
    }

    // 移除已收集的金币等
    objects_.erase(
        std::remove_if(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<GameObject>& o) {
                return o->isRemovable();
            }),
        objects_.end());

    if (player_) camera_.follow(player_->bounds().center(), dt);
}

void GameWorld::checkCollisions() {
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
                }
                break;
            }
            case GameObject::Type::Enemy: {
                if (!player_->isInvincible()) {
                    player_->takeDamage();
                    --lives_;
                    ++deaths_;
                    if (lives_ <= 0) levelDone_ = true;
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
    deaths_ = 0;
    coins_ = 0;
    levelDone_ = false;

    objects_.clear();
    player_ = nullptr;
    spawnPlayer(level_->playerSpawn());
    player_->setKillY(static_cast<float>(level_->pixelHeight() + 64));
    spawnLevelObjects();

    camera_.snapTo(player_->bounds().center());
}