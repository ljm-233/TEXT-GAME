#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

class CoinSpriteFactory {
public:
    // 16 帧 × 48×48（更大，细节更清楚）
    static std::shared_ptr<sf::Texture> getSheet();

    static constexpr int kFrameW = 48;
    static constexpr int kFrameH = 48;
    static constexpr int kFrameCount = 16;

private:
    static std::shared_ptr<sf::Texture> sheet_;
};