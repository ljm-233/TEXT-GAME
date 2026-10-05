#include "wallpaper/wallpaper_thumbnail.h"

#include <algorithm>
#include <cstdint>

sf::Image makeThumbnail(const sf::Image& src, unsigned maxDim) {
    const sf::Vector2u srcSize = src.getSize();
    if (srcSize.x == 0 || srcSize.y == 0 || maxDim == 0)
        return {};

    // 本来就够小就原样返回 —— 不放大。放大只会浪费内存，还会更糊。
    if (srcSize.x <= maxDim && srcSize.y <= maxDim)
        return src;

    const float scale =
        static_cast<float>(maxDim) / static_cast<float>(std::max(srcSize.x, srcSize.y));

    const unsigned dstW = std::max(1u, static_cast<unsigned>(srcSize.x * scale));
    const unsigned dstH = std::max(1u, static_cast<unsigned>(srcSize.y * scale));

    // ⚠️ 必须写成 sf::Vector2u{...}，不能写 ({dstW, dstH}) ——
    //    单参数花括号在 clang/MSVC 上会与其它重载歧义（见 CLAUDE.md 跨平台那节）
    sf::Image dst(sf::Vector2u{dstW, dstH}, sf::Color::Black);

    // 读走原始字节：getPixel() 每个像素都有边界检查，这里要读 800 万个像素，
    // 用裸指针快得多。RGBA 四字节连续排列，逐行从上到下。
    const std::uint8_t* pixels = src.getPixelsPtr();

    for (unsigned y = 0; y < dstH; ++y) {
        // 源图上这一段矩形由哪些行组成（整数边界，保证恰好铺满、不重不漏）
        const unsigned sy0 = y * srcSize.y / dstH;
        const unsigned sy1 = std::max(sy0 + 1, (y + 1) * srcSize.y / dstH);

        for (unsigned x = 0; x < dstW; ++x) {
            const unsigned sx0 = x * srcSize.x / dstW;
            const unsigned sx1 = std::max(sx0 + 1, (x + 1) * srcSize.x / dstW);

            std::uint64_t r = 0, g = 0, b = 0, a = 0;
            unsigned count = 0;
            for (unsigned sy = sy0; sy < sy1; ++sy) {
                const std::uint8_t* row =
                    pixels + (static_cast<std::size_t>(sy) * srcSize.x + sx0) * 4;
                for (unsigned sx = sx0; sx < sx1; ++sx) {
                    r += row[0];
                    g += row[1];
                    b += row[2];
                    a += row[3];
                    row += 4;
                    ++count;
                }
            }
            if (count == 0)
                count = 1;

            dst.setPixel({x, y}, sf::Color(static_cast<std::uint8_t>(r / count),
                                           static_cast<std::uint8_t>(g / count),
                                           static_cast<std::uint8_t>(b / count),
                                           static_cast<std::uint8_t>(a / count)));
        }
    }

    return dst;
}
