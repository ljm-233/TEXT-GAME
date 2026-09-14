#pragma once
#include "level.h"
#include "player.h"
#include "game_object.h"
#include "camera.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class GameWorld {
public:
    GameWorld(std::unique_ptr<Level> level);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    void setViewSize(float w, float h);

    Player& player() { return *player_; }
    const Player& player() const { return *player_; }

    const Level& level() const { return *level_; }

    Vec2 cameraCenter() const { return camera_.center(); }

    void reset();

    int  consumeDeaths()   { int d = deaths_; deaths_ = 0; return d; }
    bool consumeLevelDone(){ bool d = levelDone_; levelDone_ = false; return d; }

    int lives() const { return lives_; }
    int coins() const { return coins_; }
    int totalCoins() const { return totalCoins_; }

private:
    bool checkGoalReached() const;
    void spawnPlayer(Vec2 spawn);
    void spawnLevelObjects();
    void checkCollisions();

    std::unique_ptr<Level> level_;

    std::vector<std::unique_ptr<GameObject>> objects_;
    Player* player_ = nullptr;

    Camera camera_;

    int  lives_      = 3;
    int  deaths_     = 0;
    int  coins_      = 0;
    int  totalCoins_ = 0;
    bool levelDone_  = false;
};