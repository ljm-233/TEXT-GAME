#pragma once
#include "vec2.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class Level {
public:
    bool loadFromString(const std::string& text);
    bool loadFromFile(const std::string& path);

    int width()    const { return width_; }
    int height()   const { return height_; }
    int tileSize() const { return tileSize_; }
    int pixelWidth()  const { return width_  * tileSize_; }
    int pixelHeight() const { return height_ * tileSize_; }

    char tileAt(int tx, int ty) const;
    bool isSolid(int tx, int ty) const;
    // 运行时动态实体（门锁上 = true，解锁 = false）
    void setDynamicSolid(int tx, int ty, bool solid);
    bool isDynamicSolid(int tx, int ty) const;

    Vec2 playerSpawn() const { return playerSpawn_; }
    const std::vector<Vec2>& enemySpawns()      const { return enemySpawns_; }
    const std::vector<Vec2>& coinSpawns()       const { return coinSpawns_; }
    const std::vector<Vec2>& jumpPadSpawns()    const { return jumpPadSpawns_; }
    const std::vector<Vec2>& checkpointSpawns() const { return checkpointSpawns_; }
    const std::vector<Vec2>& movingPlatformSpawns() const { return movingPlatformSpawns_; }
    const std::vector<Vec2>& verticalPlatformSpawns() const { return verticalPlatformSpawns_; }
    const std::vector<Vec2>& keySpawns()  const { return keySpawns_; }
    const std::vector<Vec2>& doorSpawns() const { return doorSpawns_; }
    const std::vector<Vec2>& spikeSpawns() const { return spikeSpawns_; }

    Vec2 goalPos() const { return goalPos_; }
    bool hasGoal() const { return hasGoal_; }

    // 关卡元数据（从头部 `# name: xxx` 注释行读取）
    const std::string& name()   const { return name_; }
    const std::string& author() const { return author_; }

    void setFont(const sf::Font* f) { font_ = f; }
    void setPseudo3D(bool b);
    bool isPseudo3D() const { return pseudo3D_; }

    void render(sf::RenderTarget& target,
                float camLeft, float camTop,
                float camW,    float camH) const;

private:
    void buildGeometry();

    int width_ = 0, height_ = 0, tileSize_ = 32;
    std::vector<char> tiles_;
    std::vector<bool> dynamicSolid_;   // 运行时动态实体（门等）
    Vec2 playerSpawn_{0.f, 0.f};
    std::vector<Vec2> enemySpawns_;
    std::vector<Vec2> coinSpawns_;
    std::vector<Vec2> jumpPadSpawns_;
    std::vector<Vec2> checkpointSpawns_;
    std::vector<Vec2> movingPlatformSpawns_;
    std::vector<Vec2> verticalPlatformSpawns_;
    std::vector<Vec2> keySpawns_;
    std::vector<Vec2> doorSpawns_;
    std::vector<Vec2> spikeSpawns_;
    Vec2 goalPos_{0.f, 0.f};
    bool hasGoal_ = false;

    std::string name_;
    std::string author_;

    mutable sf::Clock animClock_;
    const sf::Font* font_ = nullptr;
    bool pseudo3D_ = true;

    // 一次性构建的整个关卡的三角形顶点数组
    sf::VertexArray vertexArray_;
};