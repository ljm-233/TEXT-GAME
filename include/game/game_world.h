#pragma once
#include "camera.h"
#include "event_bus.h"
#include "game_object.h"
#include "level.h"
#include "particle_system.h"
#include "player.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "key.h"
#include "door.h"
#include "spike.h"
#include "checkpoint.h"
#include <algorithm>

class GameWorld {
public:
    enum class State { Playing, LevelComplete, GameOver };

    GameWorld(std::unique_ptr<Level> level, int levelIndex);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    void setViewSize(float w, float h);
    void setInitialLives(int n) {
        initialLives_ = std::max(1, n);
        lives_ = initialLives_;
    }
    void setShowColliders(bool b) { showColliders_ = b; }
    void setScreenShake(bool b) { screenShake_ = b; }
    void setParticles(bool b) { particlesEnabled_ = b; }
    void setPseudo3D(bool b) {
        pseudo3D_ = b;
        if (level_)
            level_->setPseudo3D(b);
    }
    void setPlayerAnimation(bool b) {
        if (player_)
            player_->setAnimationEnabled(b);
    }

    Player& player() { return *player_; }
    const Player& player() const { return *player_; }
    const Level& level() const { return *level_; }
    Vec2 cameraCenter() const { return camera_.center(); }

    // ===== 事件总线（供 GameScene 订阅）=====
    EventBus& bus() { return bus_; }
    ParticleSystem& particles() { return particles_; }

    void reset();
    void respawnAtCheckpoint();   // 从最近的存档点重生（生命重置，金币保留）

    int lives() const { return lives_; }
    int coins() const { return coins_; }
    int totalCoins() const { return totalCoins_; }
    State state() const { return state_; }
    int levelIndex() const { return levelIndex_; }

private:
    bool checkGoalReached() const;
    void spawnPlayer(Vec2 spawn);
    void spawnLevelObjects();
    void checkCollisionsSafe();
    void renderDebugColliders(sf::RenderTarget& target);
    void renderShadow(sf::RenderTarget& target, Vec2 worldPos, float width,
                      float height) const;

    std::unique_ptr<Level> level_;
    int levelIndex_ = 1;

    std::vector<std::unique_ptr<GameObject>> objects_;
    Player* player_ = nullptr;
    Checkpoint* activeCheckpoint_ = nullptr;   // 当前激活的存档点（最多一个）

    Camera camera_;
    ParticleSystem particles_;
    EventBus bus_;

    int lives_ = 3;
    int initialLives_ = 3;
    int coins_ = 0;
    int totalCoins_ = 0;
    State state_ = State::Playing;
    bool  pendingRestart_ = false;   // 生命耗尽，等待延迟后重生
    float respawnDelayTimer_ = 0.f;  // 生命耗尽后的重生延迟

    float accumulator_ = 0.f;

    bool showColliders_ = false;
    bool screenShake_ = true;
    bool particlesEnabled_ = true;
    bool pseudo3D_ = true;
    struct DoorEntry {
        Door* door;
        int   tx;
        int   ty;
    };
    std::vector<DoorEntry> doors_;
};