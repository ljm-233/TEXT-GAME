#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

// 程序化生成玩家精灵图集（不需要美术资源）
class PlayerSpriteFactory {
public:
    // 8 帧，每帧 32x32：
    //   0-1: 待机   2-5: 跑步   6: 跳跃   7: 下落
    static std::shared_ptr<sf::Texture> getSheet();

    static constexpr int kFrameW = 32;
    static constexpr int kFrameH = 32;
    static constexpr int kFrameCount = 8;

private:
    static std::shared_ptr<sf::Texture> sheet_;
};