#pragma once
#include "camera.h"
#include "event_bus.h"
#include "game_object.h"
#include "level.h"
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

    /// 场景里用到的贴图。
    ///
    /// 由调用方注入，而不是 GameWorld 自己去调全局 sprite factory —— 那些
    /// factory 内部用 RenderTexture 程序化画贴图，没有 GL 上下文时 SFML 会
    /// 直接 SIGABRT，于是整个类在构造阶段就依赖 GL，根本没法测。
    ///
    /// 全部留空 = "无 sprite 模式"：实体照常参与逻辑与碰撞，只是画不出东西。
    struct SpriteSheets {
        std::shared_ptr<sf::Texture> player;
        std::shared_ptr<sf::Texture> coin;
        std::shared_ptr<sf::Texture> enemy;

        /// 从三个 sprite factory 取贴图。**需要 GL 上下文**，生产代码走这条。
        static SpriteSheets fromFactories();
    };

    GameWorld(std::unique_ptr<Level> level, int levelIndex, SpriteSheets sheets = {});

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
    /// 镜头抖动幅度倍率（设置里的「震动强度」，0.0~2.0）。
    /// 注意与手柄振动强度无关 —— 那个是触觉反馈。
    void setShakeIntensity(float m) { shakeIntensity_ = m < 0.f ? 0.f : m; }
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

    /// 当前世界里的全部对象（测试 / 调试用）。
    /// 只给 const 引用：外部不能拿它增删，只能按 type() 过滤查看。
    const std::vector<std::unique_ptr<GameObject>>& objects() const { return objects_; }

    // ===== 事件总线（供 GameScene 订阅）=====
    //
    // 粒子系统**曾经也挂在这里**，结果 game 层直接持有了一个渲染器，
    // 还自己调 render()。现在粒子归 GameScene —— 本层只负责发事件，
    // "事件要不要变成火花"是表现层的事。
    EventBus& bus() { return bus_; }

    void reset();
    void respawnAtCheckpoint(); // 从最近的存档点重生（生命重置，金币保留）

    // ⭐ 调试：清空所有敌人
    void killAllEnemies();

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
    SpriteSheets sheets_;

    std::vector<std::unique_ptr<GameObject>> objects_;
    Player* player_ = nullptr;
    Checkpoint* activeCheckpoint_ = nullptr; // 当前激活的存档点（最多一个）

    Camera camera_;
    // ⚠️ 必须是 unique_ptr，不能是按值的 sf::Texture：
    //    sf::Texture 继承自 sf::GlResource，**它的构造函数就会确保 GL 上下文存在**，
    //    没有 DISPLAY 时 SFML 直接 abort。作为按值成员会让"构造 GameWorld"
    //    这件事本身依赖 GL，于是 GameWorld 根本没法在任何无界面环境（测试、CI）里用。
    //    阴影纹理本来就是懒生成的，改成指针不影响行为。
    mutable std::unique_ptr<sf::Texture> shadowTex_;
    mutable bool shadowTexReady_ = false;
    EventBus bus_;

    int lives_ = 1;
    int initialLives_ = 1;
    int coins_ = 0;
    int totalCoins_ = 0;
    State state_ = State::Playing;
    bool pendingRestart_ = false;   // 生命耗尽，等待延迟后重生
    float respawnDelayTimer_ = 0.f; // 生命耗尽后的重生延迟

    float accumulator_ = 0.f;

    bool showColliders_ = false;
    bool screenShake_ = true;
    float shakeIntensity_ = 1.f;
    bool pseudo3D_ = true;
    struct DoorEntry {
        Door* door;
        int tx;
        int ty;
    };
    std::vector<DoorEntry> doors_;
};