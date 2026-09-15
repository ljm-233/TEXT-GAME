#include "level_intro.h"
#include "utf8.h"
#include <algorithm>
#include <cstdint>

LevelIntro::LevelIntro(const sf::Font& font, int levelIndex, float totalCoins)
      : title_(font, sf::String(), 72),
        subtitle_(font, sf::String(), 24) {
    title_.setString(sf::String::fromUtf8(("关卡 " + std::to_string(levelIndex)).begin(),
                                          ("关卡 " + std::to_string(levelIndex)).end()));
    title_.setFillColor(sf::Color(255, 255, 255));

    std::string sub =
        "收集 " + std::to_string(static_cast<int>(totalCoins)) + " 个金币，到达终点";
    subtitle_.setString(toSf(sub));
    subtitle_.setFillColor(sf::Color(200, 220, 255));
}

void LevelIntro::update(float dt) {
    elapsed_ += dt;
}

void LevelIntro::render(sf::RenderTarget& target, float winW, float winH) {
    float alpha = 0.f;
    if (elapsed_ < kFadeIn) {
        alpha = elapsed_ / kFadeIn;
    } else if (elapsed_ < kFadeIn + kHold) {
        alpha = 1.f;
    } else if (elapsed_ < kTotalDuration) {
        alpha = 1.f - (elapsed_ - kFadeIn - kHold) / kFadeOut;
    } else {
        return;
    }
    alpha = std::clamp(alpha, 0.f, 1.f);

    auto a8 = static_cast<std::uint8_t>(alpha * 255.f);

    // 半透明黑底
    sf::RectangleShape bg({winW, winH});
    bg.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha * 140.f)));
    target.draw(bg);

    // 标题
    {
        auto c = title_.getFillColor();
        title_.setFillColor(sf::Color(c.r, c.g, c.b, a8));
        auto b = title_.getLocalBounds();
        title_.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        title_.setPosition({winW / 2.f, winH / 2.f - 30.f});
        target.draw(title_);
    }

    // 副标题
    {
        auto c = subtitle_.getFillColor();
        subtitle_.setFillColor(sf::Color(c.r, c.g, c.b, a8));
        auto b = subtitle_.getLocalBounds();
        subtitle_.setOrigin(
            {b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        subtitle_.setPosition({winW / 2.f, winH / 2.f + 50.f});
        target.draw(subtitle_);
    }
}