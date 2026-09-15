#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// 多层视差背景：程序生成的远山、云、树
class ParallaxBackground {
public:
    ParallaxBackground();

    void update(float dt);
    void render(sf::RenderTarget& target, float camLeft, float camTop, float viewW,
                float viewH);

private:
    struct Cloud {
        float x, y, speed, scale;
    };
    std::vector<Cloud> clouds_;
    float cloudTimer_ = 0.f;

    // 预生成的背景形状
    mutable sf::ConvexShape mountainShape_;
    mutable sf::CircleShape cloudShape_;
    mutable sf::ConvexShape treeShape_;
};