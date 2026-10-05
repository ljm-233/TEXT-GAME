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
#include "theme.h"
#include "ui/background.h"
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"
#include "scene/tabs/wallpaper_tab.h"

#include <SFML/Graphics.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <set>
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
        const int want = (before + 1) % n;

        // ⭐ 回归守卫：**点下去那一刻**选中项就该是新图。
        //    设置里的 "n/m" 标签和 current_wallpaper 都读 currentIndex()/currentFile()，
        //    它们要立刻变 —— 曾经这里返回的是"正在显示的那张"，于是点完
        //    "下一张"存进配置的还是旧名字，重启打回上一张（完全静默）。
        check(bg.currentIndex() == want, "第 " + std::to_string(i) +
                                             " 次切换：选中项立刻变成 " +
                                             std::to_string(want) + "（实得 " +
                                             std::to_string(bg.currentIndex()) + "）");
        check(bg.currentFile() == lib.filenames()[want],
              "第 " + std::to_string(i) + " 次切换：currentFile() 立刻是新名字（" +
                  bg.currentFile() + "）");
        check(bg.displayedIndex() == before,
              "第 " + std::to_string(i) + " 次切换：这时的画面还是旧图（淡入中）");

        // 等淡入走完，画面才追上选中项
        std::this_thread::sleep_for(kPastFade);
        renderAndSample(bg); // 驱动 render 把 fade 推到终点
        check(bg.displayedIndex() == want,
              "第 " + std::to_string(i) + " 次切换：淡入结束后画面追上了（" +
                  std::to_string(bg.displayedIndex()) + "）");

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
        //    写成 closeTo(renderAndSample(bg), sampleFresh(..., bg.displayedIndex()))
        //    会先算右边 —— 那时 renderAndSample 还没推进淡入，displayedIndex()
        //    拿到的还是旧索引，于是基准取错图、断言假红。
        const sf::Color c = renderAndSample(bg); // 先推进 fade 到终点
        const int idx = bg.displayedIndex();     // 再读"画面上那张"
        check(closeTo(c, sampleFresh(lib, logger, idx)),
              "淡入中途再切一次，最终与基准一致 (实得 " + rgb(c) + ")");
    }

    // ---- 老配置换过扩展名也能接上 ----
    const std::string first = lib.filenames()[0];
    const std::string stem = first.substr(0, first.find_last_of('.'));
    check(bg.loadByName(stem + ".tiff"), "换过扩展名的老配置能接上");
    check(bg.currentIndex() == 0, "接上的是第 0 张（选中项立刻生效）");
    std::this_thread::sleep_for(kPastFade);
    renderAndSample(bg);
    check(bg.displayedIndex() == 0, "淡入结束后画面也是第 0 张");

    // ---- 不存在的名字应当拒绝，且不破坏当前画面 ----
    check(!bg.loadByName("definitely-not-here.png"), "不存在的名字返回 false");
    {
        const sf::Color c = renderAndSample(bg);
        const int idx = bg.displayedIndex();
        check(closeTo(c, sampleFresh(lib, logger, idx)),
              "拒绝之后当前壁纸还在且与基准一致 (rgb=" + rgb(c) + ")");
    }

    // ============================================================
    // 设置里的「壁纸」页（0.3.7 从 InterfaceTab 独立出来）
    // ============================================================
    std::cout << "\n--- 设置里的壁纸页 ---\n";

    // 缩略图是 worker 后台逐个产出的，先把它们等齐
    {
        int ready = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (std::chrono::steady_clock::now() < deadline) {
            ready = 0;
            for (int i = 0; i < lib.size(); ++i)
                if (loader.thumbnail(i))
                    ++ready;
            if (ready == lib.size())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        check(ready == lib.size(), "全部 " + std::to_string(lib.size()) +
                                       " 张缩略图都生成了（实得 " +
                                       std::to_string(ready) + "）");
    }

    sf::Font font;
    check(font.openFromFile("assets/font.ttf"), "加载字体 assets/font.ttf");

    // 非拥有型 shared_ptr：这些对象在栈上，生命周期覆盖整段测试
    auto keep = [](auto*) {};
    WallpaperTab tab(font, nullptr /* 不落盘 */, std::shared_ptr<Background>(&bg, keep),
                     std::shared_ptr<WallpaperLibrary>(&lib, keep),
                     std::shared_ptr<WallpaperLoader>(&loader, keep));

    constexpr float kCX = 240.f; // 与 settings_scene 的 kContentX 一致
    constexpr float kSY = 50.f;

    // ⚠️ 顺序要和真实游戏一致：每帧先 update() 再 render()。
    //    缩略图纹理是在 update() 里上传的，先 render 的话只会画到占位块 ——
    //    这个坑我第一次就踩了（页面采样出来只有一种颜色）。
    tab.update();

    sf::RenderTexture page(sf::Vector2u{1280, 720});
    page.clear(sf::Color::Black);
    const float bottom = tab.render(page, kCX, kSY);
    page.display();

    check(bottom > kSY + 100.f,
          "壁纸页渲染出了内容（底部 y=" + std::to_string(bottom) + "）");

    const sf::Image shot = page.getTexture().copyToImage();

    // 缩略图那一格必须是**真实图像**。占位块是 panelBg 纯色 ——
    // 如果这里只采样到一两种颜色，说明缩略图根本没上传，页面看着是空的。
    {
        const sf::Vector2f tp = wallpaper_tab_layout::rowThumbPos(kCX, kSY, 0);
        std::set<std::string> colors;
        for (unsigned y = 8; y < 82; y += 8) {
            for (unsigned x = 8; x < 152; x += 8) {
                const sf::Color c = shot.getPixel(
                    {static_cast<unsigned>(tp.x) + x, static_cast<unsigned>(tp.y) + y});
                colors.insert(std::to_string(c.r) + "," + std::to_string(c.g) + "," +
                              std::to_string(c.b));
            }
        }
        check(colors.size() > 3, "第一行缩略图是真图不是占位块（采样到 " +
                                     std::to_string(colors.size()) + " 种颜色）");
    }

    // 点第 target 行 → 应当切到那一张，并且配置项也立刻跟上
    {
        const int target = (bg.currentIndex() + 1) % lib.size();
        const sf::Vector2f bp = wallpaper_tab_layout::rowButtonPos(kCX, kSY, target);
        const sf::Vector2i at{
            static_cast<int>(bp.x + wallpaper_tab_layout::kBtnW * 0.5f),
            static_cast<int>(bp.y + wallpaper_tab_layout::kBtnH * 0.5f)};

        // Button 的点击判定 = 在按钮内按下 + 在按钮内松开
        sf::Event press(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, at});
        sf::Event release(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, at});
        tab.handleEvent(press);
        tab.handleEvent(release);
        tab.update();

        check(bg.currentIndex() == target, "点第 " + std::to_string(target) +
                                               " 行切过去了（实得 " +
                                               std::to_string(bg.currentIndex()) + "）");
        check(bg.currentFile() == lib.filenames()[target],
              "点的就是那张（" + bg.currentFile() + "）");

        // 选中高亮必须跟着走 —— 重新渲染后，那一行的缩略图框应当有高亮描边
        page.clear(sf::Color::Black);
        tab.render(page, kCX, kSY);
        page.display();
        const sf::Image after = page.getTexture().copyToImage();
        const sf::Vector2f tp = wallpaper_tab_layout::rowThumbPos(kCX, kSY, target);
        const Theme& th = getTheme();
        // 在上边**中点**附近找主题选中色。
        //
        // 两个坑：
        //   1. 别取角上 —— 角落的描边接合形状不可靠，而且缩略图那个角本身
        //      可能就是黑的（第一次就是被这个骗过去的，"黑 != panelBg" 竟然算过了）
        //   2. SFML 的 setOutlineThickness 是**向外**画的（矩形 160x90 加 3px
        //      描边之后整体是 166x96），所以描边在矩形外面的那一圈，
        //      采矩形内侧是采不到的
        // 这里上下各扫几像素，兼容描边方向的实现差异，但要求颜色**必须等于**
        // 主题的选中色 —— 而不是"只要不是某个色就算过"。
        const int sx = static_cast<int>(tp.x) + 80;
        const int sy = static_cast<int>(tp.y);
        bool found = false;
        sf::Color foundColor =
            after.getPixel({static_cast<unsigned>(sx), static_cast<unsigned>(sy)});
        for (int dy = -3; dy <= 2 && !found; ++dy) {
            const int y = sy + dy;
            if (y < 0 || y >= 720)
                continue;
            const sf::Color c =
                after.getPixel({static_cast<unsigned>(sx), static_cast<unsigned>(y)});
            if (closeTo(c, th.buttonSelected, 3)) {
                found = true;
                foundColor = c;
            }
        }
        check(found, "选中行的缩略图框用了主题选中色 " + rgb(th.buttonSelected) +
                         " 做高亮（上边缘附近实得 " + rgb(foundColor) + "）");
    }

    std::cout << (failures == 0 ? "\n全部通过\n"
                                : "\n失败 " + std::to_string(failures) + " 条\n");
    return failures == 0 ? 0 : 1;
}
