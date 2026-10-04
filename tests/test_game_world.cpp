#include "doctest.h"
#include "game_world.h"
#include "event_bus.h"
#include "level.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

// GameWorld 串起了物理、碰撞、事件三件事，是剩下最大的测试空白。
//
// 它能在这里被构造，靠的是把贴图改成**注入**：
// GameWorld 不再自己去调 sprite factory（那些 factory 内部用 RenderTexture
// 画贴图，没有 GL 上下文时 SFML 会直接 SIGABRT）。传默认的空 SpriteSheets
// 就进"无 sprite 模式"—— 实体照常参与逻辑与碰撞，只是画不出东西。
//
// 因此这个文件同样不许调 render()（RenderTarget 本身就要 GL）。
namespace {

constexpr int kTile = 32;

/// 每次 update 推进两个固定步（GameConst::kFixedTimeStep = 1/120）
constexpr float kStep = 1.f / 60.f;

/// 造一个 20×5 的房间。row2 是第 2 行的中间 18 个字符（首尾的 '#' 由这里补）。
std::unique_ptr<Level> makeRoom(const std::string& row2) {
    REQUIRE(row2.size() == 18);
    const std::string text = "####################\n"
                             "#                  #\n"
                             "#" + row2 + "#\n"
                             "#                  #\n"
                             "####################";
    auto level = std::make_unique<Level>();
    level->loadFromString(text);
    return level;
}

/// row2 里第 nth 次出现的 ch 所在的 tile 像素坐标（对象都摆在第 2 行）。
///
/// 关卡行首尾各有一个 '#'，所以 tile 下标 = row2 下标 + 1 —— 手写坐标时
/// 最容易在这里差一格，所以测试一律用这个函数取位置，不写死数字。
/// 找不到字符时 REQUIRE 会让用例立刻失败，而不是悄悄退化成"没碰撞"。
Vec2 tileOf(const std::string& row2, char ch, int nth = 1) {
    int seen = 0;
    for (std::size_t i = 0; i < row2.size(); ++i) {
        if (row2[i] == ch && ++seen == nth) {
            return {static_cast<float>(static_cast<int>(i) + 1) * kTile,
                    2.f * kTile};
        }
    }
    REQUIRE_MESSAGE(false, "关卡行里找不到字符 '" << ch << "': " << row2);
    return {-1.f, -1.f};
}

/// 记录所有经过总线的事件，便于断言"该发的发了 / 不该发的没发"。
struct EventLog {
    std::vector<GameEvent> events;

    void attach(GameWorld& world) {
        world.bus().subscribe([this](const GameEvent& e) { events.push_back(e); });
    }

    template <typename T>
    int count() const {
        int n = 0;
        for (const auto& e : events)
            if (std::holds_alternative<T>(e))
                ++n;
        return n;
    }
};

/// 把玩家瞬移到指定位置（respawn 会清空速度，方便构造确定的状态）
void teleport(GameWorld& world, Vec2 pos) {
    world.player().respawn(pos);
}

} // namespace

// ============================================================
// 构造与初始状态
// ============================================================

TEST_CASE("GameWorld - 构造后生成玩家与关卡对象") {
    const std::string row = "  P   C C     E   ";
    GameWorld world(makeRoom(row), 1);

    CHECK(world.state() == GameWorld::State::Playing);
    CHECK(world.levelIndex() == 1);
    CHECK_FALSE(world.level().hasGoal());

    CHECK(world.totalCoins() == 2);   // 两个 C
    CHECK(world.coins() == 0);

    // 玩家出生在 P 那一格
    const Vec2 spawn = world.player().position();
    CHECK(spawn.x == doctest::Approx(tileOf(row, 'P').x));
    CHECK(spawn.y == doctest::Approx(tileOf(row, 'P').y));
}

TEST_CASE("GameWorld - setInitialLives 同时重置当前生命") {
    GameWorld world(makeRoom("  P               "), 1);

    world.setInitialLives(3);

    CHECK(world.lives() == 3);
}

// ============================================================
// 金币
// ============================================================

TEST_CASE("GameWorld - 玩家碰到金币：计数增加并发事件") {
    const std::string row = "  P   C           ";
    GameWorld world(makeRoom(row), 1);
    EventLog log;
    log.attach(world);

    teleport(world, tileOf(row, 'C'));
    world.update(kStep);

    CHECK(world.coins() == 1);
    CHECK(log.count<EvCoined>() == 1);
}

TEST_CASE("GameWorld - 同一枚金币不会被重复计数") {
    const std::string row = "  P   C           ";
    GameWorld world(makeRoom(row), 1);
    EventLog log;
    log.attach(world);

    teleport(world, tileOf(row, 'C'));
    for (int i = 0; i < 30; ++i)
        world.update(kStep);   // 站在原地蹭 30 帧

    CHECK(world.coins() == 1);
    CHECK(log.count<EvCoined>() == 1);
}

// ============================================================
// 敌人
// ============================================================

TEST_CASE("GameWorld - 从上方踩敌人：击杀 + 弹起 + 不扣血") {
    const std::string row = "  P          E    ";
    GameWorld world(makeRoom(row), 1);
    world.setInitialLives(3);
    EventLog log;
    log.attach(world);

    // 敌人 bounds 顶边在 y = 2*32 + 2 = 66；玩家 24×32，
    // 让 bottom 落在 66 以内一点点，再给一个向下速度，
    // 才满足"下落中 + 从上方"的踩踏判定
    const Vec2 enemyPos = tileOf(row, 'E');
    teleport(world, {enemyPos.x, 40.f});
    world.player().setVelocityY(400.f);
    world.update(kStep);

    CHECK(log.count<EvStomped>() == 1);
    CHECK(log.count<EvHurt>() == 0);
    CHECK(world.lives() == 3);                    // 没扣血
    CHECK(world.player().velocity().y < 0.f);     // 被弹起
}

TEST_CASE("GameWorld - 侧面撞敌人：扣血 + 受伤事件") {
    const std::string row = "  P          E    ";
    GameWorld world(makeRoom(row), 1);
    world.setInitialLives(3);
    EventLog log;
    log.attach(world);

    // 和敌人同一高度：重叠量远大于踩踏容差，判定为"被撞"而不是"踩"
    const Vec2 enemyPos = tileOf(row, 'E');
    teleport(world, {enemyPos.x, enemyPos.y - 2.f});
    world.update(kStep);

    CHECK(log.count<EvHurt>() == 1);
    CHECK(log.count<EvStomped>() == 0);
    CHECK(world.lives() == 2);
}

TEST_CASE("GameWorld - killAllEnemies 之后撞上去不再受伤") {
    const std::string row = "  P          E    ";
    GameWorld world(makeRoom(row), 1);
    world.setInitialLives(3);

    world.killAllEnemies();

    EventLog log;
    log.attach(world);
    const Vec2 enemyPos = tileOf(row, 'E');
    teleport(world, {enemyPos.x, enemyPos.y - 2.f});
    world.update(kStep);

    CHECK(log.count<EvHurt>() == 0);
    CHECK(world.lives() == 3);
}

// ============================================================
// 尖刺
// ============================================================

TEST_CASE("GameWorld - 碰到尖刺扣血") {
    const std::string row = "  P     ^         ";
    GameWorld world(makeRoom(row), 1);
    world.setInitialLives(3);
    EventLog log;
    log.attach(world);

    teleport(world, tileOf(row, '^'));
    world.update(kStep);

    CHECK(log.count<EvHurt>() == 1);
    CHECK(world.lives() == 2);
}

TEST_CASE("GameWorld - 无敌状态下踩尖刺不掉血") {
    const std::string row = "  P     ^         ";
    GameWorld world(makeRoom(row), 1);
    world.setInitialLives(3);

    EventLog log;
    log.attach(world);
    teleport(world, tileOf(row, '^'));
    world.player().setInvincible(true);   // respawn 会清掉无敌，所以放在它后面
    world.update(kStep);

    CHECK(log.count<EvHurt>() == 0);
    CHECK(world.lives() == 3);
}

// ============================================================
// 存档点
// ============================================================

TEST_CASE("GameWorld - 碰到存档点：发事件并把重生点挪过去") {
    const std::string row = "  P     S         ";
    GameWorld world(makeRoom(row), 1);
    EventLog log;
    log.attach(world);

    teleport(world, tileOf(row, 'S'));
    world.update(kStep);

    CHECK(log.count<EvCheckpoint>() == 1);

    // 重生点应当落在存档点那一格内
    const Vec2 spawn = world.player().spawn();
    const Vec2 cp = tileOf(row, 'S');
    CHECK(spawn.x >= cp.x);
    CHECK(spawn.x < cp.x + kTile);
    CHECK(spawn.y >= cp.y - kTile);
    CHECK(spawn.y < cp.y + kTile);
}

// ============================================================
// 钥匙与门
// ============================================================

TEST_CASE("GameWorld - 拿到钥匙：钥匙数 +1") {
    const std::string row = "  P  K    L       ";
    GameWorld world(makeRoom(row), 1);
    EventLog log;
    log.attach(world);

    CHECK(world.player().keys() == 0);

    teleport(world, tileOf(row, 'K'));
    world.update(kStep);

    CHECK(world.player().keys() == 1);
    CHECK(log.count<EvCoined>() == 1);   // 钥匙复用金币的拾取事件
    CHECK_FALSE(world.player().isInvincible());   // 开门不影响玩家状态
}

TEST_CASE("GameWorld - reset 会清空钥匙数") {
    const std::string row = "  P  K            ";
    GameWorld world(makeRoom(row), 1);

    teleport(world, tileOf(row, 'K'));
    world.update(kStep);
    REQUIRE(world.player().keys() == 1);

    world.reset();

    CHECK(world.player().keys() == 0);
}

// ============================================================
// 终点
// ============================================================

TEST_CASE("GameWorld - 到达终点：状态切到 LevelComplete 并发事件") {
    const std::string row = "  P        G      ";
    GameWorld world(makeRoom(row), 1);
    EventLog log;
    log.attach(world);

    teleport(world, tileOf(row, 'G'));
    world.update(kStep);

    CHECK(world.state() == GameWorld::State::LevelComplete);
    CHECK(log.count<EvLevelComplete>() == 1);
}

TEST_CASE("GameWorld - 没有终点标记的关卡永远不会通关") {
    GameWorld world(makeRoom("  P               "), 1);

    for (int i = 0; i < 60; ++i)
        world.update(kStep);

    CHECK(world.state() == GameWorld::State::Playing);
}

// ============================================================
// reset / 生命耗尽
// ============================================================

TEST_CASE("GameWorld - reset 恢复生命与金币计数") {
    const std::string row = "  P   C           ";
    GameWorld world(makeRoom(row), 1);
    world.setInitialLives(3);

    teleport(world, tileOf(row, 'C'));
    world.update(kStep);
    REQUIRE(world.coins() == 1);

    world.reset();

    CHECK(world.coins() == 0);
    CHECK(world.lives() == 3);
    CHECK(world.state() == GameWorld::State::Playing);
    CHECK(world.player().position().x == doctest::Approx(tileOf(row, 'P').x));
}

TEST_CASE("GameWorld - 生命耗尽后延迟重生，并重置生命") {
    const std::string row = "  P     ^         ";
    GameWorld world(makeRoom(row), 1);
    world.setInitialLives(1);   // 一碰就死

    EventLog log;
    log.attach(world);

    teleport(world, tileOf(row, '^'));
    world.update(kStep);
    REQUIRE(world.lives() == 0);

    // 0.5 秒延迟期间不该触发重生
    world.update(0.2f);
    CHECK(log.count<EvLifeExhausted>() == 0);

    // 延迟走完的那一帧只是把计时器减到 0 以下，真正重生发生在下一帧
    world.update(0.4f);
    world.update(kStep);

    CHECK(log.count<EvLifeExhausted>() == 1);
    CHECK(world.lives() == 1);   // 生命重置
}
