#include "doctest.h"
#include "checkpoint.h"
#include "coin.h"
#include "door.h"
#include "enemy.h"
#include "key.h"
#include "spike.h"

#include "level.h"

#include <algorithm>

// 全部 6 类实体都在这里测，前提是**不能碰 GL 上下文**。
//
// Checkpoint / Key / Spike / Door 用 RectangleShape / ConvexShape / VertexArray
// 绘制，本来就没有纹理。
//
// Coin / Enemy 走 sprite factory，而 factory 是用 RenderTexture 程序化画贴图的
// ——没有 DISPLAY 时 SFML 会直接 SIGABRT。所以它们的构造函数改成接收 sheet
// 参数（照 Player 的既有模式），传 nullptr 就进"无 sprite"模式：逻辑与碰撞
// 照常跑，只是画不出东西。测试因此可以完全绕开贴图。
//
// 这也是为什么本文件一个 render() 都不调：RenderTarget 本身就需要 GL。
namespace {

constexpr int kTile = 32;

/// 空关卡：这些实体都不查地形，只有 Enemy 查。
Level makeEmptyLevel() {
    Level level;
    return level;
}

/// 一行地板 + 四周墙壁的空房间。
Level makeRoomLevel() {
    const char* text =
        "####################\n"
        "#                  #\n"
        "#                  #\n"
        "#                  #\n"
        "####################";
    Level level;
    level.loadFromString(text);
    return level;
}

} // namespace

// ============================================================
// Checkpoint
// ============================================================

TEST_CASE("Checkpoint - 初始未激活") {
    Checkpoint cp({100.f, 200.f}, kTile);

    CHECK_FALSE(cp.isActive());
    CHECK(cp.type() == GameObject::Type::Checkpoint);
    // 存档点不该被移除
    CHECK_FALSE(cp.isRemovable());
}

TEST_CASE("Checkpoint - activate / deactivate 可来回切换") {
    Checkpoint cp({0.f, 0.f}, kTile);

    cp.activate();
    CHECK(cp.isActive());

    cp.deactivate();
    CHECK_FALSE(cp.isActive());
}

TEST_CASE("Checkpoint - 碰撞盒向上扩一格，覆盖玩家站在上方的情况") {
    Checkpoint cp({100.f, 200.f}, kTile);
    const AABB box = cp.bounds();

    // 上方扩一格：从 y - tileSize 开始，高度两格
    CHECK(box.x == doctest::Approx(100.f));
    CHECK(box.y == doctest::Approx(200.f - kTile));
    CHECK(box.w == doctest::Approx(static_cast<float>(kTile)));
    CHECK(box.h == doctest::Approx(static_cast<float>(kTile) * 2.f));
}

TEST_CASE("Checkpoint - respawnPos 落在自己那一格内") {
    const float px = 100.f;
    const float py = 200.f;
    Checkpoint cp({px, py}, kTile);

    const Vec2 spawn = cp.respawnPos();

    CHECK(spawn.x > px);
    CHECK(spawn.x < px + static_cast<float>(kTile));
    CHECK(spawn.y > py);
    CHECK(spawn.y < py + static_cast<float>(kTile));
}

// ============================================================
// Key
// ============================================================

TEST_CASE("Key - 初始未收集") {
    Key key({64.f, 64.f}, kTile);

    CHECK_FALSE(key.collected());
    CHECK_FALSE(key.isRemovable());   // 收集后才可移除
    CHECK(key.type() == GameObject::Type::Key);
}

TEST_CASE("Key - collect 之后标记为可移除") {
    Key key({64.f, 64.f}, kTile);

    key.collect();

    CHECK(key.collected());
    CHECK(key.isRemovable());
}

TEST_CASE("Key - 碰撞盒比格子小，居中") {
    Key key({64.f, 96.f}, kTile);
    const AABB box = key.bounds();

    CHECK(box.w < static_cast<float>(kTile));
    CHECK(box.h < static_cast<float>(kTile));
    // 居中：左右留白相等
    const float leftGap = box.x - 64.f;
    const float rightGap = (64.f + kTile) - box.right();
    CHECK(leftGap == doctest::Approx(rightGap));
}

// ============================================================
// Door
// ============================================================

TEST_CASE("Door - 初始关闭且未消失") {
    Door door({10.f, 20.f}, kTile);

    CHECK_FALSE(door.isUnlocked());
    CHECK_FALSE(door.isGone());
    CHECK(door.type() == GameObject::Type::Door);
}

TEST_CASE("Door - 解锁后要等淡出结束才算消失") {
    Door door({0.f, 0.f}, kTile);
    Level level = makeEmptyLevel();

    door.unlock();
    CHECK(door.isUnlocked());
    // 刚解锁：淡出动画还没跑完，门仍然挡路
    CHECK_FALSE(door.isGone());

    door.update(0.3f, level);
    CHECK_FALSE(door.isGone());

    door.update(0.2f, level);   // 累计 0.5s > 0.4s 淡出时长
    CHECK(door.isGone());
}

TEST_CASE("Door - unlock 是幂等的，不会重置淡出计时") {
    Door door({0.f, 0.f}, kTile);
    Level level = makeEmptyLevel();

    door.unlock();
    door.update(0.35f, level);   // 只剩 0.05s

    door.unlock();               // 重复解锁不应把计时器推回 0.4s

    door.update(0.1f, level);
    CHECK_MESSAGE(door.isGone(),
                  "重复 unlock 重置了淡出计时 —— 门会多挡一会儿");
}

TEST_CASE("Door - update 不会把计时器推到负数以下") {
    Door door({0.f, 0.f}, kTile);
    Level level = makeEmptyLevel();

    door.unlock();
    for (int i = 0; i < 100; ++i)
        door.update(0.1f, level);   // 累计 10 秒，远超淡出时长

    CHECK(door.isGone());

    // 再跑也不该出问题（isGone 只看"已解锁 且 计时器 <= 0"）
    door.update(1.f, level);
    CHECK(door.isGone());
}

TEST_CASE("Door - 碰撞盒正好一格") {
    Door door({100.f, 200.f}, kTile);
    const AABB box = door.bounds();

    CHECK(box.x == doctest::Approx(100.f));
    CHECK(box.y == doctest::Approx(200.f));
    CHECK(box.w == doctest::Approx(static_cast<float>(kTile)));
    CHECK(box.h == doctest::Approx(static_cast<float>(kTile)));
}

// ============================================================
// Spike
// ============================================================

TEST_CASE("Spike - 类型标记正确") {
    Spike spike({0.f, 0.f}, kTile);

    CHECK(spike.type() == GameObject::Type::Spike);
    CHECK_FALSE(spike.isRemovable());   // 尖刺永远留在场上
}

TEST_CASE("Spike - 碰撞盒比格子窄且偏下（只有刺尖伤人）") {
    Spike spike({100.f, 200.f}, kTile);
    const AABB box = spike.bounds();

    CHECK(box.w < static_cast<float>(kTile));
    CHECK(box.h < static_cast<float>(kTile));
    // 顶部留白多于底部：判定区贴着格子下半部分
    const float topGap = box.y - 200.f;
    CHECK(topGap > 0.f);
    CHECK(box.bottom() <= doctest::Approx(200.f + static_cast<float>(kTile)));
}

TEST_CASE("Spike - update 不改变碰撞盒") {
    Spike spike({100.f, 200.f}, kTile);
    Level level = makeEmptyLevel();
    const AABB before = spike.bounds();

    spike.update(0.5f, level);

    const AABB after = spike.bounds();
    CHECK(after.x == doctest::Approx(before.x));
    CHECK(after.y == doctest::Approx(before.y));
    CHECK(after.w == doctest::Approx(before.w));
    CHECK(after.h == doctest::Approx(before.h));
}

// ============================================================
// Coin（无 sprite 模式）
// ============================================================

TEST_CASE("Coin - 初始未收集，可以用空贴图构造") {
    // nullptr = 无 sprite 模式。这一步本身就是断言：
    // 之前构造函数无条件解引用 factory 的返回值，贴图生成失败就会段错误。
    Coin coin({100.f, 200.f}, kTile, nullptr);

    CHECK_FALSE(coin.collected());
    CHECK_FALSE(coin.isRemovable());
    CHECK(coin.type() == GameObject::Type::Coin);
}

TEST_CASE("Coin - collect 之后标记为可移除") {
    Coin coin({100.f, 200.f}, kTile, nullptr);

    coin.collect();

    CHECK(coin.collected());
    CHECK(coin.isRemovable());
}

TEST_CASE("Coin - 碰撞盒居中且比格子小") {
    Coin coin({100.f, 200.f}, kTile, nullptr);
    const AABB box = coin.bounds();

    CHECK(box.w < static_cast<float>(kTile));
    CHECK(box.h < static_cast<float>(kTile));
    const float leftGap = box.x - 100.f;
    const float rightGap = (100.f + kTile) - box.right();
    CHECK(leftGap == doctest::Approx(rightGap));
}

TEST_CASE("Coin - 无 sprite 模式下 update 不炸") {
    Coin coin({100.f, 200.f}, kTile, nullptr);
    Level level = makeEmptyLevel();

    for (int i = 0; i < 10; ++i)
        coin.update(0.05f, level);   // 动画照常推进，只是没有 sprite 可套

    CHECK_FALSE(coin.collected());
}

// ============================================================
// Enemy（无 sprite 模式）
// ============================================================

TEST_CASE("Enemy - 初始向左移动") {
    Level level = makeRoomLevel();
    Enemy enemy({320.f, 96.f}, level.tileSize(), nullptr);

    const float x0 = enemy.bounds().x;
    enemy.update(0.1f, level);

    CHECK(enemy.bounds().x < x0);
    CHECK(enemy.type() == GameObject::Type::Enemy);
    CHECK_FALSE(enemy.killed());
}

TEST_CASE("Enemy - 碰撞盒是一格内缩的正方形") {
    Level level = makeRoomLevel();
    Enemy enemy({320.f, 96.f}, level.tileSize(), nullptr);
    const AABB box = enemy.bounds();

    CHECK(box.w == doctest::Approx(28.f));
    CHECK(box.h == doctest::Approx(28.f));
    // 构造时做了居中偏移，所以不会正好等于传入的 tile 坐标
    CHECK(box.x > 320.f);
    CHECK(box.y > 96.f);
}

TEST_CASE("Enemy - 撞墙后掉头") {
    // 左边一堵墙（tile 1，第 3 行），敌人在 tile 3 一路向左
    const char* text =
        "####################\n"
        "#                  #\n"
        "#                  #\n"
        "##                 #\n"
        "####################";
    Level level;
    level.loadFromString(text);

    Enemy enemy({96.f, 96.f}, level.tileSize(), nullptr);

    bool turned = false;
    float minX = enemy.bounds().x;
    float prev = minX;
    for (int i = 0; i < 200; ++i) {
        enemy.update(0.02f, level);
        const float x = enemy.bounds().x;
        minX = std::min(minX, x);
        if (x > prev + 0.001f) {
            turned = true;
            break;
        }
        prev = x;
    }

    CHECK_MESSAGE(turned, "敌人撞到墙之后没有掉头，会一直贴墙走");

    // ⚠️ 只断言"转向了"是不够的：墙左边还有关卡边界，敌人即使完全无视墙，
    //    也会走到边界被悬崖检测拦下并转向 —— 这条用例会假通过。
    //    变异测试（把 wallAhead 从判断里摘掉）确认过这一点，所以要卡住转向位置：
    //    墙占 tile 1（x ∈ [32,64)），正常行为下转向点在它右侧，不会走到 x≈2 的边界。
    CHECK_MESSAGE(minX > 60.f,
                  "敌人穿过了墙才发现要掉头 —— wallAhead 没起作用");
}

TEST_CASE("Enemy - 走到平台边缘前掉头，不会走下去") {
    // 地板只在 tile 6 往右，左边是空的
    const char* text =
        "####################\n"
        "#                  #\n"
        "#                  #\n"
        "#                  #\n"
        "#     ##############";
    Level level;
    level.loadFromString(text);

    Enemy enemy({8 * kTile + 0.f, 3 * kTile + 0.f}, level.tileSize(), nullptr);

    bool turned = false;
    float minX = enemy.bounds().x;
    for (int i = 0; i < 300; ++i) {
        enemy.update(0.02f, level);
        const float x = enemy.bounds().x;
        minX = std::min(minX, x);
        if (x > minX + 0.001f && minX < enemy.bounds().x) {
            turned = true;
            break;
        }
    }

    CHECK_MESSAGE(turned, "敌人走到平台边缘没有掉头，会掉下去");

    // 掉头点必须还在平台范围内（不能已经走出去了才回头）
    CHECK_MESSAGE(minX > 6 * kTile - static_cast<float>(kTile),
                  "敌人在离开平台之后才掉头");
}

TEST_CASE("Enemy - killed 之后不再移动，且可被移除") {
    Level level = makeRoomLevel();
    Enemy enemy({320.f, 96.f}, level.tileSize(), nullptr);

    enemy.kill();
    CHECK(enemy.killed());
    CHECK(enemy.isRemovable());

    const float x0 = enemy.bounds().x;
    enemy.update(0.5f, level);

    CHECK(enemy.bounds().x == doctest::Approx(x0));   // 死了就不动
}
