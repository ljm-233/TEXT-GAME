#include "doctest.h"
#include "ui/particle_system.h"

// 粒子密度的数值效果（0.3.8 新增的设置项）。
//
// 这个类是少数几个**没有 GlResource 成员**的 UI 类（只有一堆 Particle 值），
// 所以能在无 DISPLAY 的环境里直接构造与断言 —— 不像 Background / WallpaperTab
// 那样只能靠手动冒烟。
//
// 密度只缩放在 emit() 里传进来的 count，不碰其它参数：
// 语义是"同样的事件、差多少颗粒子"，而不是"特效有没有"。

namespace {

/// 一个远超容量上限的请求量（只用来验证"再怎么要也是同一个上限"）
constexpr int kMaxParticlesProbe = 1000000;

constexpr float kSpeedMin = 10.f;
constexpr float kSpeedMax = 20.f;
constexpr float kLifeMin = 0.5f;
constexpr float kLifeMax = 1.0f;
constexpr float kSizeMin = 2.f;
constexpr float kSizeMax = 4.f;

int emitAndCount(ParticleSystem& ps, int n) {
    ps.clear();
    ps.emit({0.f, 0.f}, n, sf::Color::White, kSpeedMin, kSpeedMax, kLifeMin, kLifeMax,
            kSizeMin, kSizeMax);
    return ps.count();
}

} // namespace

TEST_CASE("粒子密度 - 默认是 1 倍，数量原样") {
    ParticleSystem ps;
    CHECK(ps.densityScale() == doctest::Approx(1.0f));
    CHECK(emitAndCount(ps, 10) == 10);
}

TEST_CASE("粒子密度 - 半分 → 粒子减半") {
    ParticleSystem ps;
    ps.setDensityScale(0.5f);
    CHECK(emitAndCount(ps, 10) == 5);
}

TEST_CASE("粒子密度 - 两倍 → 粒子翻倍") {
    ParticleSystem ps;
    ps.setDensityScale(2.0f);
    CHECK(emitAndCount(ps, 10) == 20);
}

TEST_CASE("粒子密度 - 0 表示完全不生成") {
    ParticleSystem ps;
    ps.setDensityScale(0.f);
    CHECK(emitAndCount(ps, 10) == 0);
}

TEST_CASE("粒子密度 - 负数被夹到 0，不会算出负数量") {
    ParticleSystem ps;
    ps.setDensityScale(-3.f);
    CHECK(ps.densityScale() == doctest::Approx(0.0f));
    CHECK(emitAndCount(ps, 10) == 0);
}

TEST_CASE("粒子密度 - 小数量取整不会凭空多出来") {
    ParticleSystem ps;
    ps.setDensityScale(0.5f);
    // 1 * 0.5 = 0.5 → 截断成 0（宁可少一颗，也别在低密度下反而更多）
    CHECK(emitAndCount(ps, 1) == 0);
    // 3 * 0.5 = 1.5 → 1
    CHECK(emitAndCount(ps, 3) == 1);
}

TEST_CASE("粒子密度 - 上限仍受 kMaxParticles 约束") {
    ParticleSystem ps;
    ps.setDensityScale(2.0f);
    // 一次请求远超上限：密度调高不能绕过容量上限。
    // 上限本身在 particle_system.cpp 里（kMaxParticles），这里按"绝不越界"
    // 断言，不去抄那个常量的具体数值 —— 抄了就又多一份要同步的东西。
    const int n = emitAndCount(ps, 100000);
    CHECK(n > 0);
    // 先把上限算出来，再比 —— doctest 拆不了复杂表达式
    const int capped = emitAndCount(ps, kMaxParticlesProbe);
    CHECK(n == capped);
}
