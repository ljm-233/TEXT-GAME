#include "level_intro.h"
#include "text_strings.h"
#include "utf8.h"
#include <algorithm>
#include <cstdint>

LevelIntro::LevelIntro(const sf::Font& font, int levelIndex, float totalCoins,
                       const std::string& levelName)
      : title_(font, sf::String(), 72),
        subtitle_(font, sf::String(), 24) {

    // 标题：有自定义关卡名就用它，否则用 "关卡 N"
    std::string titleText;
    if (!levelName.empty()) {
        titleText = levelName;
    } else {
        titleText = Str::T(Str::IntroLevelPrefix) + std::to_string(levelIndex);
    }
    title_.setString(toSf(titleText));
    title_.setFillColor(sf::Color(255, 255, 255));

    std::string sub =
        Str::T(Str::IntroCollect) + std::to_string(static_cast<int>(totalCoins))
        + Str::T(Str::IntroCollectTail);
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

    sf::RectangleShape bg({winW, winH});
    bg.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha * 140.f)));
    target.draw(bg);

    {
        auto c = title_.getFillColor();
        title_.setFillColor(sf::Color(c.r, c.g, c.b, a8));
        auto b = title_.getLocalBounds();
        title_.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        title_.setPosition({winW / 2.f, winH / 2.f - 30.f});
        target.draw(title_);
    }

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