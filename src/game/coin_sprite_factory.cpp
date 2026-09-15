#include "coin_sprite_factory.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

std::shared_ptr<sf::Texture> CoinSpriteFactory::sheet_;

namespace {

constexpr float kPi = 3.14159265358979323846f;

// ===== 颜色 =====
const sf::Color kFrontOuter  (170, 115, 15);
const sf::Color kFrontBody   (255, 215, 65);
const sf::Color kFrontInner  (240, 190, 45);
const sf::Color kFrontCenter (255, 235, 130);
const sf::Color kBackOuter   (120, 75, 15);
const sf::Color kBackBody    (185, 130, 30);
const sf::Color kBackInner   (160, 105, 20);
const sf::Color kHighlight   (255, 252, 220, 240);

// 每帧的横向缩放（模拟绕垂直轴旋转）
const float kScales[16] = {
    1.00f, 0.92f, 0.78f, 0.58f,
    0.35f, 0.15f, 0.05f, 0.15f,
    0.35f, 0.58f, 0.78f, 0.92f,
    1.00f, 0.92f, 0.78f, 0.58f
};

void drawCoinFrame(sf::RenderTarget& rt, int frameIndex) {
    const float fx = static_cast<float>(frameIndex * 48);
    const float cx = fx + 24.f;
    const float cy = 24.f;

    const float radius = 16.f;

    // ⭐ 用 cos 判定正/背面（前 8 帧正面，后 8 帧背面）
    const float angle = (2.f * kPi * frameIndex) / 16.f;
    const bool  frontFacing = (std::cos(angle) >= 0.f);
    const float scaleX = kScales[frameIndex];

    // ============================================================
    // 侧面正对观众：只画一条竖线（无椭圆）
    // ============================================================
    if (scaleX < 0.08f) {
        sf::RectangleShape bar({3.f, radius * 1.9f});
        bar.setOrigin({1.5f, radius * 0.95f});
        bar.setPosition({cx, cy});
        bar.setFillColor(frontFacing ? kFrontOuter : kBackOuter);
        rt.draw(bar);
        return;
    }

    // ============================================================
    // 椭圆（宽度 = scaleX）
    // ============================================================
    sf::Color outerCol = frontFacing ? kFrontOuter : kBackOuter;
    sf::Color bodyCol  = frontFacing ? kFrontBody  : kBackBody;

    // 外圈
    sf::CircleShape outer(radius);
    outer.setOrigin({radius, radius});
    outer.setPosition({cx, cy});
    outer.setScale({scaleX, 1.f});
    outer.setFillColor(outerCol);
    rt.draw(outer);

    // 主体
    float bodyR = radius - 2.f;
    sf::CircleShape body(bodyR);
    body.setOrigin({bodyR, bodyR});
    body.setPosition({cx, cy});
    body.setScale({scaleX, 1.f});
    body.setFillColor(bodyCol);
    rt.draw(body);

    // ============================================================
    // 正面装饰（只有正面 + 足够宽时）
    // ============================================================
    if (frontFacing && scaleX > 0.35f) {
        // 内圈
        float innerR = bodyR * 0.68f;
        sf::CircleShape inner(innerR);
        inner.setOrigin({innerR, innerR});
        inner.setPosition({cx, cy});
        inner.setScale({scaleX, 1.f});
        inner.setFillColor(sf::Color::Transparent);
        inner.setOutlineThickness(1.5f);
        inner.setOutlineColor(kFrontInner);
        rt.draw(inner);

        // 中心竖条（"$" 简化）
        float barW = 2.f;
        float barH = bodyR * 0.6f;
        sf::RectangleShape bar({barW, barH});
        bar.setOrigin({barW * 0.5f, barH * 0.5f});
        bar.setPosition({cx, cy});
        bar.setScale({scaleX, 1.f});
        bar.setFillColor(kFrontCenter);
        rt.draw(bar);

        // 高光
        if (scaleX > 0.55f) {
            float hlR = bodyR * 0.22f;
            float hlX = -radius * 0.42f * scaleX;
            float hlY = -radius * 0.42f;
            sf::CircleShape hl(hlR);
            hl.setOrigin({hlR, hlR});
            hl.setPosition({cx + hlX, cy + hlY});
            hl.setScale({scaleX, 1.f});
            hl.setFillColor(kHighlight);
            rt.draw(hl);
        }
    }

    // ============================================================
    // 背面装饰（只有背面 + 足够宽时）
    // ============================================================
    if (!frontFacing && scaleX > 0.35f) {
        float innerR = bodyR * 0.58f;
        sf::CircleShape inner(innerR);
        inner.setOrigin({innerR, innerR});
        inner.setPosition({cx, cy});
        inner.setScale({scaleX, 1.f});
        inner.setFillColor(sf::Color::Transparent);
        inner.setOutlineThickness(1.2f);
        inner.setOutlineColor(kBackInner);
        rt.draw(inner);
    }
}

} // namespace

std::shared_ptr<sf::Texture> CoinSpriteFactory::getSheet() {
    if (sheet_) return sheet_;

    sf::RenderTexture rt;
    if (!rt.resize({kFrameW * kFrameCount, kFrameH})) {
        return nullptr;
    }
    rt.clear(sf::Color::Transparent);

    for (int i = 0; i < kFrameCount; ++i) {
        drawCoinFrame(rt, i);
    }
    rt.display();

    sheet_ = std::make_shared<sf::Texture>();
    (void)sheet_->loadFromImage(rt.getTexture().copyToImage());
    sheet_->setSmooth(false);

    return sheet_;
}