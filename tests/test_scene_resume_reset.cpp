// 场景的"待跳转请求"（`nextScene_`）必须在**自己被 resume 时**清掉。
//
// 为什么单独守这一条：`Game::run` 每帧都会问 `current->nextScene()`，而场景是
// **常驻**的 —— 一个场景 push 出别的场景、之后又被 pop 回来时，它上次写下的请求
// 还留着，于是"刚回来"的下一帧会再把那个场景推一次。
//
// 编辑器的真实翻车：F5 试玩 push 进 GameScene，点「保存并退出游戏」pop 回编辑器
// —— 编辑器的 nextScene_ 还停在 Game 上，下一帧立刻又 push 一次 GameScene。
// 用户看到的是**点退出直接重开一局**，而且不报任何错。
//
// 这条路径只有真的把场景跑起来才看得见（构造要 `sf::Font`），而 CI 不跑冒烟工具
// （`tools/scene_smoke.cpp` 的用例 E 守着同一条约定，但需要 DISPLAY、手动跑）。
// 所以这里用源码扫描守住它 —— 与 `tests/test_layers.cpp` 是同一套路子。
#include "doctest.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

namespace fs = std::filesystem;

/// 取出 `...::<signature>()` 的函数体。
/// 项目里顶层函数的右花括号永远在第 0 列（clang-format 保证），按行扫就够。
std::string functionBody(const std::string& source, const std::string& signature) {
    const std::size_t at = source.find(signature);
    if (at == std::string::npos)
        return {};
    const std::size_t open = source.find('{', at);
    if (open == std::string::npos)
        return {};
    const std::size_t close = source.find("\n}", open);
    if (close == std::string::npos)
        return {};
    return source.substr(open, close - open);
}

constexpr const char* kClear = "nextScene_ = SceneId::None";

/// 这条约定至少覆盖多少个场景。少了就说明扫描本身失效了（比如签名写法变了），
/// 而"扫不到"和"没问题"在这条测试里必须能区分开。
constexpr int kMinScenes = 8;

} // namespace

TEST_CASE("场景被 pop 回来时必须清掉自己的待跳转请求") {
    const fs::path dir = fs::path(PROJECT_ROOT) / "src" / "scene";
    REQUIRE(fs::is_directory(dir));

    int checked = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".cpp")
            continue;

        std::ifstream in(entry.path());
        std::ostringstream buf;
        buf << in.rdbuf();
        const std::string src = buf.str();

        const std::string enter = functionBody(src, "::onEnter()");
        if (enter.find(kClear) == std::string::npos)
            continue; // 不关心：要么没有 onEnter，要么它本来就不请求跳转

        ++checked;

        const std::string resume = functionBody(src, "::onResume()");
        // ⚠️ 这里只能用 `<<` 拼，不能用 `+`：doctest 的 INFO 展开成
        //    `MessageBuilder * 消息表达式`，而 `*` 比 `+` 优先级高 ——
        //    写 `INFO("文件: " + name)` 会变成 `MessageBuilder + std::string`，编译不过。
        INFO("文件: " << entry.path().filename().string());
        CHECK_MESSAGE(
            resume.find(kClear) != std::string::npos,
            "onEnter 清了 nextScene_，onResume 却没清 —— 这个场景被 pop 回来时"
            "会把上次的跳转请求再执行一遍（编辑器就因此「点退出 = 重开一局」）");
    }

    CHECK(checked >= kMinScenes);
}
