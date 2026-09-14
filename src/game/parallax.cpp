#include "parallax.h"
#include <cmath>
#include <cstdlib>

namespace {
float randf() { return static_cast<float>(std::rand()) / RAND_MAX; }
float randRange(float a, float b) { return a + randf() * (b - a); }
}

ParallaxBackground::ParallaxBackground() {
    for (int i = 0; i < 8; ++i) {
        Cloud c;
        c.x     = randRange(0.f, 4000.f);
        c.y     = randRange(40.f, 200.f);
        c.speed = randRange(6.f, 14.f);
        c.scale = randRange(0.6f, 1.4f);
        clouds_.push_back(c);
    }
}

void ParallaxBackground::update(float dt) {
    cloudTimer_ += dt;
    for (auto& c : clouds_) {
        c.x += c.speed * dt;
        if (c.x > 5000.f) c.x = -200.f;
    }
}

void ParallaxBackground::render(sf::RenderTarget& target,
                                float camLeft, float camTop,
                                float viewW,   float viewH) {
    // ========== 第 1 层：远山（视差 0.15）==========
    {
        float px = camLeft * 0.85f;   // 1 - 0.15
        float py = camTop  * 0.90f;
        float groundY = py + viewH + 20.f;

        mountainShape_.setPointCount(3);
        mountainShape_.setFillColor(sf::Color(40, 55, 85));

        for (int i = 0; i < 14; ++i) {
            float mx = px + static_cast<float>(i) * 380.f - 200.f;
            float mh = 200.f + static_cast<float>((i * 37) % 120);

            mountainShape_.setPoint(0, {mx, groundY});
            mountainShape_.setPoint(1, {mx + 190.f, groundY - mh});
            mountainShape_.setPoint(2, {mx + 380.f, groundY});
            target.draw(mountainShape_);
        }
    }

    // ========== 第 2 层：云（视差 0.30）==========
    {
        float px = camLeft * 0.70f;   // 1 - 0.30
        float py = camTop  * 0.80f;

        cloudShape_.setFillColor(sf::Color(200, 215, 240, 90));
        cloudShape_.setOutlineThickness(0.f);

        for (const auto& c : clouds_) {
            float cx = px + c.x;
            float cy = py + c.y;

            // 视口裁剪：超出摄像机范围就跳过
            if (cx < camLeft - 200.f || cx > camLeft + viewW + 200.f) continue;

            cloudShape_.setRadius(50.f * c.scale);
            cloudShape_.setPosition({cx, cy});
            target.draw(cloudShape_);

            cloudShape_.setRadius(35.f * c.scale);
            cloudShape_.setPosition({cx + 60.f * c.scale, cy + 10.f});
            target.draw(cloudShape_);
        }
    }

    // ========== 第 3 层：树（视差 0.50）==========
    {
        float px = camLeft * 0.50f;   // 1 - 0.50
        float py = camTop  * 0.65f;
        float groundY = py + viewH + 10.f;

        treeShape_.setPointCount(3);
        treeShape_.setFillColor(sf::Color(30, 70, 50));

        for (int i = 0; i < 20; ++i) {
            float tx = px + static_cast<float>(i) * 280.f - 100.f;
            float th = 90.f + static_cast<float>((i * 53) % 60);

            treeShape_.setPoint(0, {tx, groundY});
            treeShape_.setPoint(1, {tx + 50.f, groundY - th});
            treeShape_.setPoint(2, {tx + 100.f, groundY});
            target.draw(treeShape_);
        }
    }
}