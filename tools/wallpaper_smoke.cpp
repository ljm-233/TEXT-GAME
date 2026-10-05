// 壁纸系统冒烟测试（**需要 DISPLAY，手动跑**）。
//
//   ./build/release/wallpaper_smoke [壁纸目录]      # 默认 "wallpaper"
//
// 为什么不放进 tests/：Background 要建 sf::Texture，那是 sf::GlResource，
// 没有 GL 上下文连构造都会 SIGABRT —— 而 tests/ 下的单测有一条硬性要求
// （见 CLAUDE.md）：**没有 DISPLAY 也必须全绿**。所以这个只能单独一个工具。
//
// 它补的是单测覆盖不到的那一段：真实的纹理上传、窗口适配、淡入淡出。
// 这个工具是实打实抓过 bug 的，两个都不是纯逻辑测试能发现的：
//
//   1. `front_ = std::move(back_)` 时纹理若按值持有，sprite 内部那个
//      `const sf::Texture*` 会指向 moved-from 的纹理 —— 淡入一结束壁纸就没了
//   2. 新建的 back_ 图层没有立刻 fitToWindow，而 render() 里的重适配只在
//      "窗口尺寸变化"时触发，于是新图以原生像素尺寸从左上角画出来
//
// 判定手法：把背景渲染进 sf::RenderTexture 取中心像素，和"同一张壁纸、
// 不走淡入路径"的基准渲染对比。
//
// ⚠️ 为什么不能只判"不是纯黑"：上面第 1 个 bug 实测画出来是**纯白**，
//    "不是黑"照样通过。必须和基准逐位比。
#include "log/logger.h"
#include "ui/background.h"
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"

#include <SFML/Graphics.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok)
        ++failures;
}

constexpr unsigned kWidth = 640;
constexpr unsigned kHeight = 400;

/// 把背景渲染到 RT 并取中心像素。
sf::Color renderAndSample(Background& bg) {
    sf::RenderTexture rt(sf::Vector2u{kWidth, kHeight});
    rt.clear(sf::Color::Black);
    bg.render(rt);
    rt.display();
    const sf::Image img = rt.getTexture().copyToImage();
    return img.getPixel({kWidth / 2, kHeight / 2});
}

bool isBlackish(sf::Color c) {
    return c.r < 8 && c.g < 8 && c.b < 8;
}

/// 两个颜色是否几乎一样。同一张图走同样的缩放路径，理论上应当逐位相同，
/// 留一点余量防止驱动/滤波抖动。
bool closeTo(sf::Color a, sf::Color b, int tol = 2) {
    auto d = [](int x, int y) { return x > y ? x - y : y - x; };
    return d(a.r, b.r) <= tol && d(a.g, b.g) <= tol && d(a.b, b.b) <= tol;
}

std::string rgb(sf::Color c) {
    return std::to_string(c.r) + "," + std::to_string(c.g) + "," + std::to_string(c.b);
}

/// 基准：同一张壁纸**不走淡入路径**（构造时就 prime 好）渲染出来什么样。
sf::Color sampleFresh(WallpaperLibrary& lib, const std::shared_ptr<Logger>& logger,
                      int index) {
    WallpaperLoader freshLoader;
    Background fresh(lib, freshLoader, lib.filenames()[index], kWidth, kHeight, logger);
    return renderAndSample(fresh);
}

/// 淡入是 0.5s，等得比它久一点
constexpr auto kPastFade = std::chrono::milliseconds(700);

} // namespace

int main(int argc, char** argv) {
    const std::string dir = (argc > 1) ? argv[1] : "wallpaper";

    // 先建立一个 GL 上下文（RenderTexture 需要它已经存在）
    sf::RenderWindow window(sf::VideoMode({kWidth, kHeight}), "wallpaper_smoke");
    window.setVisible(false);

    WallpaperLibrary lib;
    lib.scan(dir);
    std::cout << "壁纸目录: " << dir << "  (" << lib.size() << " 张)\n";
    if (lib.size() < 2) {
        std::cout << "少于 2 张，无法验证切换；跳过\n";
        return 2;
    }

    WallpaperLoader loader;
    auto logger = std::make_shared<Logger>(LogLevel::Error); // 静音，别刷屏

    // 空配置应当回退到第一张（模拟新用户）
    Background bg(lib, loader, "", kWidth, kHeight, logger);

    check(bg.isLoaded(), "首图已加载");
    check(bg.currentIndex() == 0, "空配置回退到第 0 张");
    check(bg.totalWallpapers() == lib.size(), "总数与库一致");

    const sf::Color px0 = renderAndSample(bg);
    check(!isBlackish(px0), "首图渲染出来不是黑的 (rgb=" + rgb(px0) + ")");
    check(closeTo(px0, sampleFresh(lib, logger, 0)),
          "首图与基准渲染一致 (rgb=" + rgb(px0) + ")");

    // ---- 连续切图，每轮都等淡入走完 ----
    const int n = bg.totalWallpapers();
    for (int i = 0; i <= n; ++i) {
        const int before = bg.currentIndex();
        if (!bg.next()) {
            check(false, "next() 返回 true（第 " + std::to_string(i) + " 次）");
            break;
        }
        // 淡入期间 currentIndex 仍是旧的 —— 这是设计：旧图还在淡出
        std::this_thread::sleep_for(kPastFade);
        renderAndSample(bg); // 驱动 render 把 fade 推到终点

        const int want = (before + 1) % n;
        check(bg.currentIndex() == want, "第 " + std::to_string(i) + " 次切换: index " +
                                             std::to_string(before) + " -> " +
                                             std::to_string(bg.currentIndex()) +
                                             " (期望 " + std::to_string(want) + ")");

        // ⭐ 真正抓 bug 的一条：淡入结束后必须与"同图不淡入"的基准一致
        const sf::Color c = renderAndSample(bg);
        const sf::Color ref = sampleFresh(lib, logger, want);
        check(closeTo(c, ref), "第 " + std::to_string(i) +
                                   " 次淡入结束后与基准一致 (实得 " + rgb(c) +
                                   " / 期望 " + rgb(ref) + ")");
    }

    // ---- 淡入还没走完就再切一次：不该崩，最终也必须正确 ----
    bg.next();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bg.next();
    std::this_thread::sleep_for(kPastFade);
    {
        // ⚠️ 必须分成两条语句：函数实参的求值顺序是**未指定的**，
        //    写成 closeTo(renderAndSample(bg), sampleFresh(..., bg.currentIndex()))
        //    会先算右边 —— 那时 renderAndSample 还没推进淡入，currentIndex()
        //    拿到的还是旧索引，于是基准取错图、断言假红。
        const sf::Color c = renderAndSample(bg); // 先推进 fade 到终点
        const int idx = bg.currentIndex();       // 再读索引
        check(closeTo(c, sampleFresh(lib, logger, idx)),
              "淡入中途再切一次，最终与基准一致 (实得 " + rgb(c) + ")");
    }

    // ---- 老配置换过扩展名也能接上 ----
    const std::string first = lib.filenames()[0];
    const std::string stem = first.substr(0, first.find_last_of('.'));
    check(bg.loadByName(stem + ".tiff"), "换过扩展名的老配置能接上");
    std::this_thread::sleep_for(kPastFade);
    renderAndSample(bg);
    check(bg.currentIndex() == 0, "接上的是第 0 张");

    // ---- 不存在的名字应当拒绝，且不破坏当前画面 ----
    check(!bg.loadByName("definitely-not-here.png"), "不存在的名字返回 false");
    {
        const sf::Color c = renderAndSample(bg);
        const int idx = bg.currentIndex();
        check(closeTo(c, sampleFresh(lib, logger, idx)),
              "拒绝之后当前壁纸还在且与基准一致 (rgb=" + rgb(c) + ")");
    }

    std::cout << (failures == 0 ? "\n全部通过\n"
                                : "\n失败 " + std::to_string(failures) + " 条\n");
    return failures == 0 ? 0 : 1;
}
