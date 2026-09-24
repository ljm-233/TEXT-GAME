#pragma once
#include <SFML/Graphics.hpp>
#include <string>

// 关卡开场文字：淡入 → 停留 → 淡出
class LevelIntro {
public:
    // ⭐ 只构造一次，之后复用 restart()
    explicit LevelIntro(const sf::Font& font);

    // ⭐ 重新设置文字并重置计时
    void restart(int levelIndex, float totalCoins,
                 const std::string& levelName = "");

    void update(float dt);
    void render(sf::RenderTarget& target, float winW, float winH);

    bool isFinished() const { return elapsed_ >= kTotalDuration; }
    void skip() { elapsed_ = kTotalDuration; }

private:
    sf::Text title_;
    sf::Text subtitle_;
    float elapsed_ = 0.f;

    static constexpr float kFadeIn = 0.4f;
    static constexpr float kHold = 1.4f;
    static constexpr float kFadeOut = 0.5f;
    static constexpr float kTotalDuration = kFadeIn + kHold + kFadeOut;
};