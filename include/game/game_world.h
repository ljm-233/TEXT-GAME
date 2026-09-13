#pragma once
#include "level.h"
#include "player.h"
#include "camera.h"
#include <SFML/Graphics.hpp>
#include <memory>

class GameWorld {
public:
    GameWorld(std::unique_ptr<Level> level);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    void setViewSize(float w, float h);

    const Player& player() const { return player_; }
    const Level&  level()  const { return *level_; }

    Vec2 cameraCenter() const { return camera_.center(); }
    Vec2 cameraTopLeft() const { return camera_.position(); }
    float cameraW() const { return camera_.viewWidth(); }
    float cameraH() const { return camera_.viewHeight(); }

    void reset();

private:
    std::unique_ptr<Level> level_;
    Player player_;
    Camera camera_;
};