#include "doctest.h"
#include "core/application.h"
#include "core/main_loop.h"

#include <stdexcept>
#include <string>
#include <vector>

// 注意：Application 是单例，钩子表只增不减，所以针对 Application 的用例
// **只能有一个**。拆成多个的话，第二次 exec() 会重跑第一次注册的钩子，
// 而那些钩子捕获的是已经析构的局部变量。
// （下面的 HeadlessMainLoop 用例不碰 Application，可以独立存在。）
TEST_CASE("Application - 生命周期：五阶段顺序、钩子失败隔离、Headless 兜底") {
    Application& app = Application::instance();
    std::vector<std::string> trace;

    app.boot([&trace](Application& a) {
        // ---- phase 1: boot，此时容器是空的，也不该有主循环 ----
        trace.push_back("phase:boot");
        CHECK_FALSE(a.hasMainLoop());

        // ---- on_boot 钩子：逐个 try/catch，坏一个不挡其余 ----
        a.onBoot([&trace]() { trace.push_back("on_boot:1"); });
        a.onBoot([&trace]() {
            trace.push_back("on_boot:2");
            throw std::runtime_error("故意失败");
        });
        a.onBoot([&trace]() { trace.push_back("on_boot:3"); });

        // ---- on_loop 钩子：拿到主循环后请求退出 ----
        a.onLoop([&trace, &a](MainLoop& /*loop*/) {
            trace.push_back("on_loop");
            a.requestQuit("测试收尾");
        });

        a.onQuit([&trace](const std::string& reason) {
            trace.push_back("on_quit:" + reason);
        });

        a.onFinal([&trace]() { trace.push_back("on_final"); });
    });

    const int code = app.exec();

    // 没有注册 "main_loop"，所以走 HeadlessMainLoop 兜底并正常返回 0
    CHECK(code == 0);
    CHECK(app.hasMainLoop());

    const std::vector<std::string> expected = {
        "phase:boot",
        "on_boot:1",
        "on_boot:2",
        "on_boot:3",
        "on_loop",
        "on_quit:测试收尾",
        "on_final",
    };
    CHECK(trace == expected);
}

TEST_CASE("HeadlessMainLoop - quit 之后 run 立刻返回 0") {
    HeadlessMainLoop loop;

    CHECK(loop.exitCode() == -1);  // 还没跑
    loop.quit();

    CHECK(loop.run() == 0);
    CHECK(loop.str().find("HeadlessMainLoop") != std::string::npos);
}
