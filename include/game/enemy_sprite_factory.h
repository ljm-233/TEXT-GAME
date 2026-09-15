#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

class EnemySpriteFactory {
public:
    // 4 帧 × 48×48：
    //   0: 站立
    //   1: 走路（左腿前）
    //   2: 站立
    //   3: 走路（右腿前）
    static std::shared_ptr<sf::Texture> getSheet();

    static constexpr int kFrameW = 48;
    static constexpr int kFrameH = 48;
    static constexpr int kFrameCount = 4;

private:
    static std::shared_ptr<sf::Texture> sheet_;
};