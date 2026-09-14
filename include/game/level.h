#pragma once
#include "vec2.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class Level {
public:
    bool loadFromString(const std::string& text);
    bool loadFromFile(const std::string& path);

    int width() const { return width_; }
    int height() const { return height_; }
    int tileSize() const { return tileSize_; }
    int pixelWidth() const { return width_ * tileSize_; }
    int pixelHeight() const { return height_ * tileSize_; }

    char tileAt(int tx, int ty) const;
    bool isSolid(int tx, int ty) const;

    Vec2 playerSpawn() const { return playerSpawn_; }
    const std::vector<Vec2>& enemySpawns() const { return enemySpawns_; }
    const std::vector<Vec2>& coinSpawns() const { return coinSpawns_; }
    Vec2 goalPos() const { return goalPos_; }
    bool hasGoal() const { return hasGoal_; }

    void render(sf::RenderTarget& target, float camLeft, float camTop, float camW,
                float camH) const;

private:
    int width_ = 0, height_ = 0, tileSize_ = 32;
    std::vector<char> tiles_;
    Vec2 playerSpawn_{0.f, 0.f};
    std::vector<Vec2> enemySpawns_;
    std::vector<Vec2> coinSpawns_;
    Vec2 goalPos_{0.f, 0.f};
    bool hasGoal_ = false;
};