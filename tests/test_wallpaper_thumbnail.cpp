#include "doctest.h"
#include "wallpaper/wallpaper_thumbnail.h"

#include <SFML/Graphics/Image.hpp>

// 壁纸缩略图的降采样。
//
// `sf::Image` 不是 `sf::GlResource`，所以这一整套能在**无 DISPLAY** 的环境里跑
// （CLAUDE.md 里"测试必须能在没有 DISPLAY 的环境下全绿"那条）。
// Background 那边要 sf::Texture，就没这个待遇了，只能靠 wallpaper_smoke。

namespace {

sf::Image solid(unsigned w, unsigned h, sf::Color c) {
    // ⚠️ 必须写成 sf::Vector2u{...}：单参数花括号在 clang/MSVC 上与其它重载歧义
    sf::Image img(sf::Vector2u{w, h}, c);
    return img;
}

/// 4x4：左上 2x2 红，其余蓝 —— 降一半之后每个目标像素正好对应一个 2x2 块
sf::Image quadrants() {
    sf::Image img(sf::Vector2u{4, 4}, sf::Color::Blue);
    for (unsigned y = 0; y < 2; ++y)
        for (unsigned x = 0; x < 2; ++x)
            img.setPixel({x, y}, sf::Color::Red);
    return img;
}

bool sameColor(sf::Color a, sf::Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

} // namespace

TEST_CASE("缩略图 - 等比缩到最长边不超过 maxDim") {
    // 100x50 → 最长边 100 缩到 20，宽高比 2:1 必须保持
    const sf::Image src = solid(100, 50, sf::Color::White);
    const sf::Image dst = makeThumbnail(src, 20);
    CHECK(dst.getSize().x == 20);
    CHECK(dst.getSize().y == 10);
}

TEST_CASE("缩略图 - 竖图按高度那一边缩") {
    const sf::Image src = solid(50, 100, sf::Color::White);
    const sf::Image dst = makeThumbnail(src, 20);
    CHECK(dst.getSize().x == 10);
    CHECK(dst.getSize().y == 20);
}

TEST_CASE("缩略图 - 盒式平均：每个目标像素是源图对应块的平均值") {
    const sf::Image dst = makeThumbnail(quadrants(), 2);
    REQUIRE(dst.getSize().x == 2);
    REQUIRE(dst.getSize().y == 2);

    // 每块都是纯色，平均之后应当原样保留
    CHECK(sameColor(dst.getPixel({0, 0}), sf::Color::Red));
    CHECK(sameColor(dst.getPixel({1, 0}), sf::Color::Blue));
    CHECK(sameColor(dst.getPixel({0, 1}), sf::Color::Blue));
    CHECK(sameColor(dst.getPixel({1, 1}), sf::Color::Blue));
}

TEST_CASE("缩略图 - 会真的做平均，不是取其中一个像素") {
    // 2x1：左边黑、右边白。缩到 1x1 应当是两者的平均（127 左右），
    // 而不是"取左边"（0）或"取右边"（255）—— 这条专门挡"退化成最近邻"。
    sf::Image src(sf::Vector2u{2, 1}, sf::Color::Black);
    src.setPixel({1, 0}, sf::Color::White);

    const sf::Image dst = makeThumbnail(src, 1);
    REQUIRE(dst.getSize().x == 1);
    const sf::Color c = dst.getPixel({0, 0});
    const bool inRange = (c.r >= 120) && (c.r <= 135);
    CHECK_MESSAGE(inRange, "期望约 127，实得 " << static_cast<int>(c.r));
}

TEST_CASE("缩略图 - 纯色图缩小后还是那个颜色") {
    const sf::Image src = solid(64, 64, sf::Color(10, 200, 30, 255));
    const sf::Image dst = makeThumbnail(src, 8);
    CHECK(sameColor(dst.getPixel({0, 0}), sf::Color(10, 200, 30, 255)));
    CHECK(sameColor(dst.getPixel({7, 7}), sf::Color(10, 200, 30, 255)));
}

TEST_CASE("缩略图 - 本来就小于 maxDim 就原样返回，不放大") {
    const sf::Image src = solid(10, 10, sf::Color::Green);
    const sf::Image dst = makeThumbnail(src, 32);
    CHECK(dst.getSize().x == 10);
    CHECK(dst.getSize().y == 10);
    CHECK(sameColor(dst.getPixel({5, 5}), sf::Color::Green));
}

TEST_CASE("缩略图 - 空图 / maxDim 为 0 返回空图，不崩") {
    const sf::Image empty;
    CHECK(makeThumbnail(empty, 32).getSize().x == 0);

    const sf::Image src = solid(8, 8, sf::Color::White);
    CHECK(makeThumbnail(src, 0).getSize().x == 0);
}

TEST_CASE("缩略图 - 极端宽高比也不会缩成 0 宽或 0 高") {
    // 1000x1 缩到 8 → 高会算成 0.008，必须被夹到至少 1 像素
    const sf::Image wide = solid(1000, 1, sf::Color::White);
    const sf::Image d1 = makeThumbnail(wide, 8);
    CHECK(d1.getSize().x == 8);
    CHECK(d1.getSize().y >= 1);

    const sf::Image tall = solid(1, 1000, sf::Color::White);
    const sf::Image d2 = makeThumbnail(tall, 8);
    CHECK(d2.getSize().x >= 1);
    CHECK(d2.getSize().y == 8);
}

TEST_CASE("缩略图 - 实际尺寸的壁纸能缩到 256 且形状对") {
    // 3840x2160（4K，最长边的墙壁纸就是这么大）→ 256x144
    const sf::Image src = solid(3840, 2160, sf::Color(70, 57, 40, 255));
    const sf::Image dst = makeThumbnail(src, kThumbnailMaxDim);
    CHECK(dst.getSize().x == 256);
    CHECK(dst.getSize().y == 144);
    // 单张缩略图的内存占用 —— 这里顺便钉住"不是全尺寸纹理"这件事。
    // 注意先算成变量再比：doctest 不拆解三连乘的表达式，会直接静态断言报
    // "Expression Too Complex Please Rewrite As Binary Comparison"。
    const unsigned thumbBytes = dst.getSize().x * dst.getSize().y * 4;
    constexpr unsigned kThumbBudget = 256u * 256u * 4;
    constexpr unsigned kFullHdBytes = 1920u * 1080u * 4;
    constexpr unsigned kFiftieth = kFullHdBytes / 50;
    CHECK(thumbBytes <= kThumbBudget);
    // 实测 256x144x4 = 147KB，而一张 1080p 全尺寸纹理是 8.3MB —— 小约 56 倍。
    // 这里只要求"至少小 50 倍"，别把具体倍率写死。
    CHECK(thumbBytes < kFiftieth);
    CHECK(sameColor(dst.getPixel({100, 100}), sf::Color(70, 57, 40, 255)));
}
