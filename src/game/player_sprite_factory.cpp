#include "player_sprite_factory.h"

std::shared_ptr<sf::Texture> PlayerSpriteFactory::sheet_;

namespace {

const sf::Color kCreeperGreen(95, 190, 95);
const sf::Color kCreeperDark(45, 110, 45);
const sf::Color kBlack(15, 15, 15);

// 画一只苦力怕
//   x, y:     frame 左上角
//   legL,legR: 左右腿水平偏移（跑步用）
//   legH:     腿高（跳跃收起 / 下落伸展）
//   bodyBob:  身体上下浮动（呼吸用）
void drawCreeper(sf::RenderTarget& rt, float x, float y, float legL, float legR,
                 float legH, float bodyBob) {
    const float bodyY = y + 4.f + bodyBob;

    // ========== 头/身体（一体方块）==========
    sf::RectangleShape head({20.f, 20.f});
    head.setPosition({x + 6.f, bodyY});
    head.setFillColor(kCreeperGreen);
    head.setOutlineThickness(2.f);
    head.setOutlineColor(kCreeperDark);
    rt.draw(head);

    // ========== 眼睛（两个黑色方块）==========
    sf::RectangleShape eye({4.f, 4.f});
    eye.setFillColor(kBlack);

    eye.setPosition({x + 10.f, bodyY + 6.f});
    rt.draw(eye);
    eye.setPosition({x + 18.f, bodyY + 6.f});
    rt.draw(eye);

    // ========== 嘴（倒 U 形）==========
    sf::RectangleShape mouth({10.f, 2.f});
    mouth.setFillColor(kBlack);
    mouth.setPosition({x + 11.f, bodyY + 12.f});
    rt.draw(mouth);

    // 左竖
    mouth.setSize({2.f, 5.f});
    mouth.setPosition({x + 11.f, bodyY + 12.f});
    rt.draw(mouth);

    // 右竖
    mouth.setPosition({x + 19.f, bodyY + 12.f});
    rt.draw(mouth);

    // ========== 腿 ==========
    float legTop = bodyY + 20.f;

    sf::RectangleShape leg({5.f, legH});
    leg.setFillColor(kCreeperGreen);
    leg.setOutlineThickness(2.f);
    leg.setOutlineColor(kCreeperDark);

    leg.setPosition({x + 7.f + legL, legTop});
    rt.draw(leg);

    leg.setPosition({x + 20.f + legR, legTop});
    rt.draw(leg);
}

// 每帧对应参数
void drawFrame(sf::RenderTarget& rt, int frameIndex) {
    const float fx = static_cast<float>(frameIndex * 32);

    float legL = 0.f, legR = 0.f;
    float legH = 6.f;
    float bodyBob = 0.f;

    switch (frameIndex) {
    case 0: /* IDLE1：静止 */
        break;
    case 1: /* IDLE2：呼吸，身体下沉 1px */
        bodyBob = 1.f;
        break;
    case 2: /* RUN1：腿张开 */
        legL = -1.f;
        legR = 1.f;
        break;
    case 3: /* RUN2：腿张开更多 */
        legL = -3.f;
        legR = 3.f;
        break;
    case 4: /* RUN3：腿内收 */
        legL = 1.f;
        legR = -1.f;
        break;
    case 5: /* RUN4：腿内收更多 */
        legL = 3.f;
        legR = -3.f;
        break;
    case 6: /* JUMP：腿收起 */
        legH = 3.f;
        break;
    case 7: /* FALL：腿伸展 */
        legH = 8.f;
        break;
    default:
        break;
    }

    drawCreeper(rt, fx, 0.f, legL, legR, legH, bodyBob);
}

} // namespace

std::shared_ptr<sf::Texture> PlayerSpriteFactory::getSheet() {
    if (sheet_)
        return sheet_;

    sf::RenderTexture rt;
    if (!rt.resize({kFrameW * kFrameCount, kFrameH})) {
        return nullptr;
    }
    rt.clear(sf::Color::Transparent);

    for (int i = 0; i < kFrameCount; ++i) {
        drawFrame(rt, i);
    }
    rt.display();

    sheet_ = std::make_shared<sf::Texture>();
    (void)sheet_->loadFromImage(rt.getTexture().copyToImage());
    sheet_->setSmooth(false);

    return sheet_;
}