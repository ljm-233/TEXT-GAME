#include "doctest.h"
#include "enemy.h"
#include "game_constants.h"
#include "game_world.h"
#include "level.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

// 三种新敌人（W 巡逻 / F 飞行 / B 跳跃）的行为与生成。
//
// 和 test_entities.cpp 一样，这里**一个 render() 都不调**，全部走
// "无 sprite 模式"（构造 Enemy 时 sheet 传 nullptr）—— sprite factory
// 内部用 RenderTexture，没有 GL 上下文时 SFML 会直接 SIGABRT。
namespace {

constexpr int kTile = 32;
constexpr float kStep = 1.f / 60.f;

/// 造一个 20×5 的房间。row2 是第 2 行的中间 18 个字符（首尾 '#' 由这里补）。
/// 第 4 行整行是地板，第 3 行是空的 —— 所以第 2 行的敌人脚下隔了一格。
std::unique_ptr<Level> makeRoom(const std::string& row2) {
    REQUIRE(row2.size() == 18);
    const std::string text = "####################\n"
                             "#                  #\n"
                             "#" +
                             row2 +
                             "#\n"
                             "#                  #\n"
                             "####################";
    auto level = std::make_unique<Level>();
    level->loadFromString(text);
    return level;
}

/// row2 里第 nth 次出现的 ch 所在的 tile 像素坐标
Vec2 tileOf(const std::string& row2, char ch, int nth = 1) {
    int seen = 0;
    for (std::size_t i = 0; i < row2.size(); ++i) {
        if (row2[i] == ch && ++seen == nth)
            return {static_cast<float>(static_cast<int>(i) + 1) * kTile, 2.f * kTile};
    }
    REQUIRE_MESSAGE(false, "关卡行里找不到字符 '" << ch << "': " << row2);
    return {-1.f, -1.f};
}

/// 地板只从 tile 6 往右的一段平台，左边是空的（和 test_entities.cpp 用的是同一张图）
Level makeLedgeLevel() {
    const char* text = "####################\n"
                       "#                  #\n"
                       "#                  #\n"
                       "#                  #\n"
                       "#     ##############";
    Level level;
    level.loadFromString(text);
    return level;
}

/// 左右两边都有墙、地面只有 tile 1 一格空档的房间
Level makeWallLevel() {
    const char* text = "####################\n"
                       "#                  #\n"
                       "#                  #\n"
                       "##                 #\n"
                       "####################";
    Level level;
    level.loadFromString(text);
    return level;
}

/// 统计 world 里某种 kind 的敌人数量
int countKind(const GameWorld& world, EnemyKind kind) {
    int n = 0;
    for (const auto& o : world.objects())
        if (o->type() == GameObject::Type::Enemy &&
            static_cast<const Enemy*>(o.get())->kind() == kind)
            ++n;
    return n;
}

} // namespace

// ============================================================
// 关卡字符 → spawn 容器
// ============================================================

TEST_CASE("Level - W/F/B 各自进自己的容器，E 一个都不受影响") {
    const std::string row = " W  F  B  E       ";
    auto level = makeRoom(row);

    CHECK(level->patrolSpawns().size() == 1);
    CHECK(level->flyerSpawns().size() == 1);
    CHECK(level->jumperSpawns().size() == 1);
    CHECK(level->enemySpawns().size() == 1); // E 还是走老容器

    CHECK(level->patrolSpawns()[0].x == doctest::Approx(tileOf(row, 'W').x));
    CHECK(level->patrolSpawns()[0].y == doctest::Approx(tileOf(row, 'W').y));
    CHECK(level->flyerSpawns()[0].x == doctest::Approx(tileOf(row, 'F').x));
    CHECK(level->jumperSpawns()[0].x == doctest::Approx(tileOf(row, 'B').x));
    CHECK(level->enemySpawns()[0].x == doctest::Approx(tileOf(row, 'E').x));
}

TEST_CASE("Level - 同一种新敌人可以放多个") {
    const std::string row = " W W  F  B  E     ";
    auto level = makeRoom(row);

    REQUIRE(level->patrolSpawns().size() == 2);
    CHECK(level->patrolSpawns()[0].x == doctest::Approx(tileOf(row, 'W', 1).x));
    CHECK(level->patrolSpawns()[1].x == doctest::Approx(tileOf(row, 'W', 2).x));
    CHECK(level->enemySpawns().size() == 1);
}

TEST_CASE("Level - 重新加载会清空三种新敌人的容器") {
    Level level;
    level.loadFromString("####\n#WF#\n#B #\n####");
    REQUIRE(level.patrolSpawns().size() == 1);
    REQUIRE(level.flyerSpawns().size() == 1);
    REQUIRE(level.jumperSpawns().size() == 1);

    level.loadFromString("####\n#  #\n#  #\n####");

    CHECK(level.patrolSpawns().empty());
    CHECK(level.flyerSpawns().empty());
    CHECK(level.jumperSpawns().empty());
}

// ============================================================
// GameWorld 按 kind 生成
// ============================================================

TEST_CASE("GameWorld - 按关卡字符生成对应 kind 的敌人，数量与位置正确") {
    const std::string row = " W  F  B  E       ";
    GameWorld world(makeRoom(row), 1);

    int enemies = 0;
    for (const auto& o : world.objects())
        if (o->type() == GameObject::Type::Enemy)
            ++enemies;
    CHECK(enemies == 4);
    CHECK(countKind(world, EnemyKind::Basic) == 1);
    CHECK(countKind(world, EnemyKind::Patrol) == 1);
    CHECK(countKind(world, EnemyKind::Flyer) == 1);
    CHECK(countKind(world, EnemyKind::Jumper) == 1);

    // 位置：敌人碰撞盒是 28×28，在格子里居中 → 中心 = 格子左上 + 16
    for (const auto& o : world.objects()) {
        if (o->type() != GameObject::Type::Enemy)
            continue;
        const auto* e = static_cast<const Enemy*>(o.get());
        char ch = '?';
        switch (e->kind()) {
        case EnemyKind::Basic:
            ch = 'E';
            break;
        case EnemyKind::Patrol:
            ch = 'W';
            break;
        case EnemyKind::Flyer:
            ch = 'F';
            break;
        case EnemyKind::Jumper:
            ch = 'B';
            break;
        }
        const Vec2 expected = tileOf(row, ch);
        CHECK(e->bounds().center().x == doctest::Approx(expected.x + 16.f));
        CHECK(e->bounds().center().y == doctest::Approx(expected.y + 16.f));
    }
}

TEST_CASE("Enemy - 旧的三参数构造等价于 EnemyKind::Basic") {
    Enemy legacy({320.f, 96.f}, kTile, nullptr);

    CHECK(legacy.kind() == EnemyKind::Basic);
}

// ============================================================
// W 巡逻
// ============================================================

TEST_CASE("Enemy - 巡逻(W) 走到平台边缘会掉头，不会掉下去") {
    Level level = makeLedgeLevel();
    Enemy enemy(EnemyKind::Patrol, {8 * kTile + 0.f, 3 * kTile + 0.f}, kTile, nullptr);

    bool turned = false;
    float minLeft = enemy.bounds().left();
    for (int i = 0; i < 600; ++i) {
        enemy.update(0.02f, level);
        const float left = enemy.bounds().left();
        if (left > minLeft + 0.5f) { // 开始往回走
            turned = true;
            break;
        }
        minLeft = std::min(minLeft, left);
    }

    CHECK_MESSAGE(turned, "巡逻兵走到平台边缘没有掉头");
    // 平台从 tile 6 开始（x = 192），掉头点必须还在平台上
    CHECK_MESSAGE(minLeft >= 6.f * kTile - 0.001f, "巡逻兵已经走出平台才掉头，会掉下去");
}

TEST_CASE("Enemy - 巡逻(W) 掉头时会在端点停一下") {
    Level level = makeWallLevel();
    Enemy enemy(EnemyKind::Patrol, {3 * kTile + 0.f, 3 * kTile + 0.f}, kTile, nullptr);

    // 先向左走，直到第一帧"没有继续向左"——那就是掉头帧
    bool reversed = false;
    int stalled = 0;
    for (int i = 0; i < 400; ++i) {
        const float before = enemy.bounds().x;
        enemy.update(0.02f, level);
        const float after = enemy.bounds().x;

        if (!reversed) {
            if (after < before) // 还在向左
                continue;
            reversed = true; // 掉头帧（不论有没有停顿，这一帧都不动）
            continue;
        }
        if (std::abs(after - before) < 0.001f)
            ++stalled;
        else
            break;
    }

    REQUIRE(reversed);
    // kPatrolPauseTime = 0.35s，0.02s 一帧 → 大约 17 帧
    CHECK(stalled >= 10);
}

TEST_CASE("Enemy - 现有 E(Basic) 掉头后立刻反向，一个新字段都没碰") {
    Level level = makeWallLevel();
    Enemy enemy({3 * kTile + 0.f, 3 * kTile + 0.f}, kTile, nullptr);

    bool reversed = false;
    int stalled = 0;
    for (int i = 0; i < 400; ++i) {
        const float before = enemy.bounds().x;
        enemy.update(0.02f, level);
        const float after = enemy.bounds().x;

        if (!reversed) {
            if (after < before)
                continue;
            reversed = true;
            continue;
        }
        if (std::abs(after - before) < 0.001f)
            ++stalled;
        else
            break;
    }

    REQUIRE(reversed);
    CHECK_MESSAGE(stalled == 0, "E 掉头之后不该有任何停顿 —— 它的行为必须逐字不变");
}

TEST_CASE("Enemy - E 的行为与新增 kind 之前一致：不受重力，y 恒定") {
    auto level = makeRoom("                  ");
    // 第 3 行（地板上面那一格）—— 第 2 行离地一格多，cliffAhead 会一直掉头
    Enemy enemy({8 * kTile + 0.f, 3 * kTile + 0.f}, kTile, nullptr);
    const float y0 = enemy.bounds().y;

    bool movedX = false;
    for (int i = 0; i < 120; ++i) {
        enemy.update(kStep, *level);
        CHECK(enemy.bounds().y == doctest::Approx(y0)); // 没有重力
        movedX = movedX || (enemy.bounds().x < 8 * kTile);
    }
    CHECK(movedX); // 还是照常左右走
}

// ============================================================
// F 飞行
// ============================================================

TEST_CASE("Enemy - 飞行(F) 不受重力：正弦上下浮动，永不落地") {
    auto level = makeRoom("                  ");
    Enemy enemy(EnemyKind::Flyer, {8 * kTile + 0.f, 1 * kTile + 0.f}, kTile, nullptr);

    const float y0 = enemy.bounds().y;
    float minY = y0, maxY = y0;
    bool touchedGround = false;
    for (int i = 0; i < 240; ++i) { // 4 秒 > 一个正弦周期
        enemy.update(kStep, *level);
        minY = std::min(minY, enemy.bounds().y);
        maxY = std::max(maxY, enemy.bounds().y);
        if (enemy.bounds().bottom() >= 4.f * kTile)
            touchedGround = true;
    }

    CHECK_FALSE(touchedGround); // 地板在第 4 行，飞行兵永远够不着
    CHECK_MESSAGE(maxY - minY > 30.f, "飞行兵没有上下浮动（正弦没生效）");
    // 浮动中心就是出生高度，振幅不超过 kFlyerBobAmp
    CHECK(minY >= y0 - GameConst::kFlyerBobAmp - 0.5f);
    CHECK(maxY <= y0 + GameConst::kFlyerBobAmp + 0.5f);
}

TEST_CASE("Enemy - 飞行(F) 水平巡游，撞墙反向") {
    auto level = makeRoom("                  ");
    // 贴着左墙出生，一路向左，很快就会撞墙
    Enemy enemy(EnemyKind::Flyer, {2 * kTile + 0.f, 1 * kTile + 0.f}, kTile, nullptr);

    bool movedLeft = false;
    bool turned = false;
    float minX = enemy.bounds().x;
    bool leftLevel = false;
    for (int i = 0; i < 600; ++i) {
        enemy.update(kStep, *level);
        const float x = enemy.bounds().x;
        if (x < minX - 0.001f) {
            minX = x;
            movedLeft = true;
        } else if (movedLeft && x > minX + 0.5f) {
            turned = true;
            break;
        }
        if (enemy.bounds().left() < 0.f ||
            enemy.bounds().right() > static_cast<float>(level->pixelWidth()))
            leftLevel = true;
    }

    CHECK(movedLeft);
    CHECK_MESSAGE(turned, "飞行兵撞到墙没有反向");
    CHECK_FALSE(leftLevel);
}

TEST_CASE("Enemy - 飞行(F) 走到关卡边界也会反向（没有墙的关卡）") {
    // 故意做一个左右都没有墙的关卡
    const char* text = "                    \n"
                       "                    \n"
                       "                    \n"
                       "                    \n"
                       "####################";
    Level level;
    level.loadFromString(text);

    Enemy enemy(EnemyKind::Flyer, {1 * kTile + 0.f, 1 * kTile + 0.f}, kTile, nullptr);

    bool turned = false;
    float minX = enemy.bounds().x;
    for (int i = 0; i < 600; ++i) {
        enemy.update(kStep, level);
        const float x = enemy.bounds().x;
        if (x < minX) {
            minX = x;
        } else if (x > minX + 0.5f) {
            turned = true;
            break;
        }
    }

    CHECK(turned);
    // 边界检查是在移动之前做的，所以最多越界一帧的位移
    CHECK(minX > -GameConst::kFlyerSpeed * kStep - 0.001f);
}

// ============================================================
// B 跳跃
// ============================================================

TEST_CASE("Enemy - 跳跃(B) 会落到地面，然后周期性起跳") {
    auto level = makeRoom("                  ");
    Enemy enemy(EnemyKind::Jumper, {8 * kTile + 0.f, 2 * kTile + 0.f}, kTile, nullptr);

    // 先落地并站稳（地板在第 4 行，顶面 y = 128 → 敌人 y = 128 - 28 = 100）。
    // 40 帧 = 0.67s：下落只要 ~0.18s，而落地后的待机是 0.9s，还不会起跳。
    for (int i = 0; i < 40; ++i)
        enemy.update(kStep, *level);
    const float groundY = enemy.bounds().y;
    REQUIRE(groundY == doctest::Approx(100.f));

    int jumps = 0;
    float minY = groundY;
    float maxY = groundY;
    bool onGround = true;
    for (int i = 0; i < 300; ++i) { // 5 秒：起跳周期 ≈ 0.9s 待机 + 0.47s 滞空
        enemy.update(kStep, *level);
        const float y = enemy.bounds().y;
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);

        const bool nowOnGround = y >= groundY - 0.5f;
        if (onGround && !nowOnGround)
            ++jumps;
        onGround = nowOnGround;
    }

    CHECK_MESSAGE(minY < groundY - 30.f, "跳跃兵根本没有离地（起跳没生效）");
    CHECK_MESSAGE(jumps >= 2, "跳跃兵只跳了一次 —— 不是周期性的");
    // 每次都落回同一块地板，从不陷进去
    CHECK(maxY - groundY < 0.5f);
}

TEST_CASE("Enemy - E 不会跳（重力只对跳跃(B) 生效）") {
    auto level = makeRoom("                  ");
    Enemy enemy({8 * kTile + 0.f, 3 * kTile + 0.f}, kTile, nullptr);
    const float y0 = enemy.bounds().y;

    for (int i = 0; i < 300; ++i) {
        enemy.update(kStep, *level);
        CHECK(enemy.bounds().y == doctest::Approx(y0));
    }
}
