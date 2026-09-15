#include "enemy_sprite_factory.h"

std::shared_ptr<sf::Texture> EnemySpriteFactory::sheet_;

namespace {

const sf::Color kBody    (220, 80, 80);
const sf::Color kDark    (140, 40, 40);
const sf::Color kDarker  (100, 20, 20);
const sf::Color kEye     (255, 255, 255);
const sf::Color kPupil   (20, 20, 20);

void drawEnemyFrame(sf::RenderTarget& rt, int frameIndex) {
    const float fx = static_cast<float>(frameIndex * 48);
    const float cx = fx + 24.f;

    // 身体：30×26，居中
    const float bodyW = 30.f;
    const float bodyH = 26.f;
    const float bodyX = fx + (48.f - bodyW) * 0.5f;
    const float bodyY = 4.f;

    // 腿偏移（根据帧决定）
    float legLeftX  = bodyX + 4.f;
    float legRightX = bodyX + bodyW - 14.f;
    float legLeftY  = bodyY + bodyH;
    float legRightY = bodyY + bodyH;

    switch (frameIndex) {
        case 0: /* 站立：腿并拢 */ break;
        case 1: /* 走路：左腿前，右腿后 */
            legLeftX  += 2.f; legLeftY  -= 1.f;
            legRightX -= 2.f;
            break;
        case 2: /* 站立 */ break;
        case 3: /* 走路：右腿前，左腿后 */
            legRightX -= 2.f; legRightY -= 1.f;
            legLeftX  += 2.f;
            break;
    }

    // ===== 腿（先画，在身体底下）=====
    sf::RectangleShape leg({10.f, 8.f});
    leg.setFillColor(kBody);
    leg.setOutlineThickness(2.f);
    leg.setOutlineColor(kDark);

    leg.setPosition({legLeftX, legLeftY});
    rt.draw(leg);
    leg.setPosition({legRightX, legRightY});
    rt.draw(leg);

    // ===== 身体 =====
    sf::RectangleShape body({bodyW, bodyH});
    body.setPosition({bodyX, bodyY});
    body.setFillColor(kBody);
    body.setOutlineThickness(2.f);
    body.setOutlineColor(kDark);
    rt.draw(body);

    // 底部阴影
    sf::RectangleShape bottom({bodyW, 3.f});
    bottom.setPosition({bodyX, bodyY + bodyH - 3.f});
    bottom.setFillColor(kDarker);
    rt.draw(bottom);

    // ===== 眼睛（朝右默认）=====
    // 白色眼球
    sf::RectangleShape eyeL({7.f, 7.f});
    eyeL.setFillColor(kEye);
    eyeL.setPosition({bodyX + 7.f, bodyY + 7.f});
    rt.draw(eyeL);

    sf::RectangleShape eyeR({7.f, 7.f});
    eyeR.setFillColor(kEye);
    eyeR.setPosition({bodyX + bodyW - 14.f, bodyY + 7.f});
    rt.draw(eyeR);

    // 瞳孔（偏右，表示朝右）
    sf::RectangleShape pupL({3.f, 3.f});
    pupL.setFillColor(kPupil);
    pupL.setPosition({bodyX + 10.f, bodyY + 9.f});
    rt.draw(pupL);

    sf::RectangleShape pupR({3.f, 3.f});
    pupR.setFillColor(kPupil);
    pupR.setPosition({bodyX + bodyW - 11.f, bodyY + 9.f});
    rt.draw(pupR);

    // ===== 眉毛（生气）=====
    sf::RectangleShape browL({7.f, 2.f});
    browL.setFillColor(kDarker);
    browL.setPosition({bodyX + 7.f, bodyY + 4.f});
    rt.draw(browL);

    sf::RectangleShape browR({7.f, 2.f});
    browR.setFillColor(kDarker);
    browR.setPosition({bodyX + bodyW - 14.f, bodyY + 4.f});
    rt.draw(browR);

    (void)cx;
}

} // namespace

std::shared_ptr<sf::Texture> EnemySpriteFactory::getSheet() {
    if (sheet_) return sheet_;

    sf::RenderTexture rt;
    if (!rt.resize({kFrameW * kFrameCount, kFrameH})) {
        return nullptr;
    }
    rt.clear(sf::Color::Transparent);

    for (int i = 0; i < kFrameCount; ++i) {
        drawEnemyFrame(rt, i);
    }
    rt.display();

    sheet_ = std::make_shared<sf::Texture>();
    (void)sheet_->loadFromImage(rt.getTexture().copyToImage());
    sheet_->setSmooth(false);

    return sheet_;
}