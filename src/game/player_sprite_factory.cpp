#include "player_sprite_factory.h"

std::shared_ptr<sf::Texture> PlayerSpriteFactory::sheet_;

namespace {

// 画一帧
void drawFrame(sf::RenderTexture& rt, int frameIndex) {
    const int fx = frameIndex * 32;

    const sf::Color bodyColor   (80, 200, 120);
    const sf::Color outlineColor(40, 120, 70);

    // ========== 身体 ==========
    sf::RectangleShape body({20.f, 16.f});
    body.setPosition({fx + 6.f, 4.f});
    body.setFillColor(bodyColor);
    body.setOutlineThickness(2.f);
    body.setOutlineColor(outlineColor);
    rt.draw(body);

    // ========== 眼睛 ==========
    sf::RectangleShape eye({3.f, 3.f});
    eye.setFillColor(sf::Color::White);
    eye.setPosition({fx + 13.f, 9.f});
    rt.draw(eye);
    eye.setPosition({fx + 19.f, 9.f});
    rt.draw(eye);

    // ========== 腿 ==========
    float leftLegX  = fx + 8.f;
    float rightLegX = fx + 19.f;
    float legY = 20.f;
    float legH = 10.f;

    switch (frameIndex) {
        case 0: /* IDLE1 */ break;
        case 1: /* IDLE2：身体微下移 */
            body.setPosition({fx + 6.f, 5.f});
            rt.draw(body);
            legY = 21.f;
            break;
        case 2: /* RUN1 */
            leftLegX += 1.f; rightLegX -= 1.f;
            break;
        case 3: /* RUN2 */
            leftLegX += 3.f; rightLegX -= 3.f;
            break;
        case 4: /* RUN3 */
            leftLegX -= 1.f; rightLegX += 1.f;
            break;
        case 5: /* RUN4 */
            leftLegX -= 3.f; rightLegX += 3.f;
            break;
        case 6: /* JUMP：收腿 */
            legH = 5.f;
            break;
        case 7: /* FALL：伸腿 */
            legH = 12.f;
            break;
        default: break;
    }

    sf::RectangleShape leg({5.f, legH});
    leg.setFillColor(bodyColor);
    leg.setOutlineThickness(2.f);
    leg.setOutlineColor(outlineColor);

    leg.setPosition({leftLegX, legY});
    rt.draw(leg);
    leg.setPosition({rightLegX, legY});
    rt.draw(leg);
}

} // namespace

std::shared_ptr<sf::Texture> PlayerSpriteFactory::getSheet() {
    if (sheet_) return sheet_;

    sf::RenderTexture rt;
    // ⭐ SFML 3：create → resize
    if (!rt.resize({kFrameW * kFrameCount, kFrameH})) {
        return nullptr;
    }
    rt.clear(sf::Color::Transparent);

    for (int i = 0; i < kFrameCount; ++i) {
        drawFrame(rt, i);
    }
    rt.display();

    sheet_ = std::make_shared<sf::Texture>();
    // ⭐ 显式忽略返回值
    (void)sheet_->loadFromImage(rt.getTexture().copyToImage());
    sheet_->setSmooth(false);

    return sheet_;
}