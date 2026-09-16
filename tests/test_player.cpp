#include "doctest.h"
#include "player.h"
#include "level.h"
#include "game_constants.h"
#include <SFML/Window/Event.hpp>

namespace {

// 开阔的测试关卡：80 宽 × 10 高，中间一片空地
Level makeWideLevel() {
    const char* text =
        "################################################################################\n"
        "#                                                                              #\n"
        "#                                                                              #\n"
        "#                                                                              #\n"
        "#                                                                              #\n"
        "#            P                                                                 #\n"
        "#                                                                              #\n"
        "#                                                                              #\n"
        "#                                                                              #\n"
        "################################################################################";
    Level level;
    level.loadFromString(text);
    return level;
}

// 空关卡：玩家悬空，用于纯物理测试
Level makeEmptyLevel() {
    Level level;
    return level;
}

void pressSpace(Player& p) {
    sf::Event e(sf::Event::KeyPressed{
        sf::Keyboard::Key::Space, sf::Keyboard::Scan::Space,
        false, false, false, false});
    p.handleEvent(e);
}

void releaseSpace(Player& p) {
    sf::Event e(sf::Event::KeyReleased{
        sf::Keyboard::Key::Space, sf::Keyboard::Scan::Space,
        false, false, false, false});
    p.handleEvent(e);
}

void pressD(Player& p) {
    sf::Event e(sf::Event::KeyPressed{
        sf::Keyboard::Key::D, sf::Keyboard::Scan::D,
        false, false, false, false});
    p.handleEvent(e);
}

void releaseD(Player& p) {
    sf::Event e(sf::Event::KeyReleased{
        sf::Keyboard::Key::D, sf::Keyboard::Scan::D,
        false, false, false, false});
    p.handleEvent(e);
}

void pressA(Player& p) {
    sf::Event e(sf::Event::KeyPressed{
        sf::Keyboard::Key::A, sf::Keyboard::Scan::A,
        false, false, false, false});
    p.handleEvent(e);
}

void releaseA(Player& p) {
    sf::Event e(sf::Event::KeyReleased{
        sf::Keyboard::Key::A, sf::Keyboard::Scan::A,
        false, false, false, false});
    p.handleEvent(e);
}

} // namespace

TEST_CASE("Player - 初始状态") {
    Player p({32.f, 32.f});
    CHECK(p.position().x == 32.f);
    CHECK(p.position().y == 32.f);
    CHECK(p.velocity().x == 0.f);
    CHECK(p.velocity().y == 0.f);
    CHECK_FALSE(p.onGround());
    CHECK_FALSE(p.isInvincible());
}

TEST_CASE("Player - 重力使速度向下") {
    // 空关卡，玩家悬空自由落体
    Level level = makeEmptyLevel();
    Player p({100.f, 100.f});

    p.update(1.f / 120.f, level);

    CHECK(p.velocity().y > 0.f);
}

TEST_CASE("Player - 落地后停止下落") {
    Level level = makeWideLevel();
    Player p(level.playerSpawn());

    // 玩家会落到下方地板上
    for (int i = 0; i < 240; ++i)
        p.update(1.f / 120.f, level);

    CHECK(p.onGround());
    CHECK(p.velocity().y == 0.f);
}

TEST_CASE("Player - 按 D 向右移动") {
    Level level = makeWideLevel();
    Player p(level.playerSpawn());

    for (int i = 0; i < 120; ++i) p.update(1.f / 120.f, level);
    REQUIRE(p.onGround());

    const float x0 = p.position().x;
    pressD(p);

    for (int i = 0; i < 30; ++i) p.update(1.f / 120.f, level);

    CHECK(p.position().x > x0);
    releaseD(p);
}

TEST_CASE("Player - 按 A 向左移动") {
    Level level = makeWideLevel();
    Player p(level.playerSpawn());

    for (int i = 0; i < 120; ++i) p.update(1.f / 120.f, level);
    REQUIRE(p.onGround());

    const float x0 = p.position().x;
    pressA(p);

    for (int i = 0; i < 30; ++i) p.update(1.f / 120.f, level);

    CHECK(p.position().x < x0);
    releaseA(p);
}

TEST_CASE("Player - 松开方向键后停下") {
    Level level = makeWideLevel();
    Player p(level.playerSpawn());

    for (int i = 0; i < 120; ++i) p.update(1.f / 120.f, level);

    pressD(p);
    for (int i = 0; i < 30; ++i) p.update(1.f / 120.f, level);
    releaseD(p);

    for (int i = 0; i < 5; ++i) p.update(1.f / 120.f, level);

    CHECK(p.velocity().x == 0.f);
}

TEST_CASE("Player - 按跳离地") {
    Level level = makeWideLevel();
    Player p(level.playerSpawn());

    for (int i = 0; i < 120; ++i) p.update(1.f / 120.f, level);
    REQUIRE(p.onGround());

    pressSpace(p);
    p.update(1.f / 120.f, level);

    CHECK_FALSE(p.onGround());
    CHECK(p.velocity().y < 0.f);
    releaseSpace(p);
}

TEST_CASE("Player - 长按跳更高") {
    Level level = makeWideLevel();

    // 场景 A：按住跳不放
    Player pa(level.playerSpawn());
    for (int i = 0; i < 120; ++i) pa.update(1.f / 120.f, level);
    pressSpace(pa);
    for (int i = 0; i < 30; ++i) pa.update(1.f / 120.f, level);
    const float longY = pa.position().y;
    releaseSpace(pa);

    // 场景 B：跳起后立刻松手
    Player pb(level.playerSpawn());
    for (int i = 0; i < 120; ++i) pb.update(1.f / 120.f, level);
    pressSpace(pb);
    pb.update(1.f / 120.f, level);
    releaseSpace(pb);
    for (int i = 0; i < 29; ++i) pb.update(1.f / 120.f, level);
    const float shortY = pb.position().y;

    // 长按的 y 更小（跳得更高，y 轴向下）
    CHECK(longY < shortY);
}

TEST_CASE("Player - 跳跃缓冲") {
    Level level = makeWideLevel();
    Player p(level.playerSpawn());

    // 先落地
    for (int i = 0; i < 120; ++i) p.update(1.f / 120.f, level);
    REQUIRE(p.onGround());

    // 跳出
    pressSpace(p);
    p.update(1.f / 120.f, level);
    releaseSpace(p);

    // 落地前 0.1 秒内按跳
    for (int i = 0; i < 20; ++i) p.update(1.f / 120.f, level);

    bool jumped = false;
    for (int i = 0; i < 200; ++i) {
        p.update(1.f / 120.f, level);
        if (p.consumeJustJumped()) {
            jumped = true;
            break;
        }
        if (i == 10) pressSpace(p);   // 在空中按跳（模拟缓冲）
    }

    CHECK(jumped);
}

TEST_CASE("Player - 受伤无敌计时") {
    Player p({32.f, 32.f});
    CHECK_FALSE(p.isInvincible());

    p.takeDamage();
    CHECK(p.isInvincible());

    Level level;   // 空关卡
    for (int i = 0; i < 200; ++i)
        p.update(1.f / 120.f, level);

    CHECK_FALSE(p.isInvincible());
}

TEST_CASE("Player - 反弹速度向上") {
    Player p({32.f, 32.f});
    p.bounce();

    CHECK(p.velocity().y < 0.f);
    CHECK(p.velocity().y == GameConst::kPlayerBounceSpeed);
}

TEST_CASE("Player - 掉图触发 fellOut 并回到 spawn") {
    Level level = makeWideLevel();
    Player p(level.playerSpawn());
    p.setSpawn({100.f, 200.f});
    p.setKillY(500.f);

    CHECK_FALSE(p.consumeFellOut());

    p.moveBy({0.f, 1000.f});
    p.update(1.f / 120.f, level);

    CHECK(p.consumeFellOut());
    CHECK(p.position().x == 100.f);
    CHECK(p.position().y == 200.f);
}