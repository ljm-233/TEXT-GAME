#pragma once
#include "level.h"
#include "player.h"
#include "game_object.h"
#include "camera.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

class GameWorld {
public:
    enum class State {
        Playing,
        LevelComplete,
        GameOver
    };

    GameWorld(std::unique_ptr<Level> level, int levelIndex);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    void setViewSize(float w, float h);

    Player& player() { return *player_; }
    const Player& player() const { return *player_; }

    const Level& level() const { return *level_; }

    Vec2 cameraCenter() const { return camera_.center(); }

    void reset();

    int  lives() const { return lives_; }
    int  coins() const { return coins_; }
    int  totalCoins() const { return totalCoins_; }
    State state() const { return state_; }
    int  levelIndex() const { return levelIndex_; }

private:
    bool checkGoalReached() const;
    void spawnPlayer(Vec2 spawn);
    void spawnLevelObjects();
    void checkCollisionsSafe();

    std::unique_ptr<Level> level_;
    int levelIndex_ = 1;

    std::vector<std::unique_ptr<GameObject>> objects_;
    Player* player_ = nullptr;

    Camera camera_;

    int   lives_      = 3;
    int   coins_      = 0;
    int   totalCoins_ = 0;
    State state_      = State::Playing;

    float accumulator_ = 0.f;
};