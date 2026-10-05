#include "doctest.h"
#include "scene/playtest_request.h"

#include "level.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// 编辑器「试玩」的交接通道。
//
// 这个类本身很薄（就是一个可有可无的槽位），但它的**一次性语义**是关键的：
// 如果 take() 之后不清空，那么"试玩 → 退出回编辑器 → 从选关页进正式关卡"
// 这一串会让正式关卡又玩到那份旧草稿，而且没有任何提示 —— 看起来就像
// "关卡加载错了"。

namespace {

std::vector<std::string> draft() {
    return {"#####", "#P C#", "#####"};
}

} // namespace

TEST_CASE("试玩交接 - 取走一次之后就没有了") {
    PlaytestRequest req;
    CHECK(!req.has());

    req.request(draft(), "editor.txt");
    CHECK(req.has());

    auto first = req.take();
    REQUIRE(first.has_value());
    CHECK(first->lines.size() == 3);
    CHECK(first->sourceName == "editor.txt");

    // ⚠️ 这一条是整个类的存在理由：第二次必须是空的
    CHECK(!req.has());
    CHECK(!req.take().has_value());
}

TEST_CASE("试玩交接 - 没有请求时 take 返回空") {
    PlaytestRequest req;
    CHECK(!req.take().has_value());
    req.clear(); // 空的时候 clear 也不该有事
    CHECK(!req.has());
}

TEST_CASE("试玩交接 - 后一次请求覆盖前一次") {
    PlaytestRequest req;
    req.request({"aa"}, "first.txt");
    req.request({"bb", "cc"}, "second.txt");

    auto r = req.take();
    REQUIRE(r.has_value());
    CHECK(r->lines.size() == 2);
    CHECK(r->sourceName == "second.txt");
}

TEST_CASE("试玩交接 - clear 能丢掉待处理请求") {
    PlaytestRequest req;
    req.request(draft(), "editor.txt");
    req.clear();
    CHECK(!req.has());
    CHECK(!req.take().has_value());
}

TEST_CASE("试玩交接 - 没有玩家出生点的草稿要能识别出来") {
    // 没有 'P'：关卡能构造出来，但玩家会被摆在 (0,0)，大概率卡在边界墙里
    CHECK(!PlaytestRequest::hasPlayerSpawn({"#####", "#   #", "#####"}));
    CHECK(!PlaytestRequest::hasPlayerSpawn({}));

    CHECK(PlaytestRequest::hasPlayerSpawn({"#####", "#P  #", "#####"}));
    // 'P' 出现在任意位置都算（行中间、行尾）
    CHECK(PlaytestRequest::hasPlayerSpawn({"#####", "#  P#", "#####"}));
    CHECK(PlaytestRequest::hasPlayerSpawn({"P"}));
}

TEST_CASE("试玩交接 - 空草稿也能安全地放进通道") {
    PlaytestRequest req;
    req.request({}, "");
    REQUIRE(req.has());
    auto r = req.take();
    REQUIRE(r.has_value());
    CHECK(r->lines.empty());
    CHECK(r->sourceName.empty());
}

// ============================================================
// 「编辑器里的内容 → 关卡场景能解析」——这是试玩功能真正的风险点
// ============================================================
//
// 上面几条只验了交接通道本身。但试玩失败最可能的方式是：编辑器内存里的那份
// `lines_` 格式与 `Level::loadFromString` 对不上 —— 功能表面上是通的（F5 能按、
// 场景也切了），实际一进就弹"关卡解析失败"。
//
// 所以这里**原样复刻**两边的变换：
//   EditorScene::loadFile()     去掉 '\r'、按最长行补空格到等宽
//   EditorScene::startPlaytest()  逐行拼接、每行末尾补 '\n'
// 然后把结果交给真的 Level 解析，并且对 assets/levels 里的**真实文件**也跑一遍。

namespace {

/// 复刻 EditorScene::loadFile 的读盘变换（\r 去掉、补到等宽）
std::vector<std::string> readLikeEditor(const fs::path& path) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    REQUIRE(in.good());
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    std::size_t w = 0;
    for (const auto& l : lines)
        w = std::max(w, l.size());
    for (auto& l : lines)
        l.resize(w, ' ');
    return lines;
}

/// 复刻 EditorScene::startPlaytest 的拼接
std::string joinLikePlaytest(const std::vector<std::string>& lines) {
    std::string text;
    for (const auto& line : lines) {
        text += line;
        text += '\n';
    }
    return text;
}

} // namespace

TEST_CASE("试玩交接 - 编辑器读过的东西一定能被关卡解析（真实关卡文件）") {
    const fs::path dir = fs::path(PROJECT_ROOT) / "assets" / "levels";
    REQUIRE(fs::is_directory(dir));

    int checked = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".txt")
            continue;
        const auto lines = readLikeEditor(entry.path());
        REQUIRE(!lines.empty());

        Level level;
        const bool ok = level.loadFromString(joinLikePlaytest(lines));
        CHECK_MESSAGE(ok, "关卡解析失败: " << entry.path().filename().string());

        // 尺寸要对得上（编辑器是按"最长行"算宽度的）
        std::size_t w = 0;
        for (const auto& l : lines)
            w = std::max(w, l.size());
        CHECK(level.width() == static_cast<int>(w));
        CHECK(level.height() == static_cast<int>(lines.size()));

        // 而且必须能通过试玩的前置检查（有玩家出生点）
        const bool spawn = PlaytestRequest::hasPlayerSpawn(lines);
        CHECK_MESSAGE(spawn, "没有玩家出生点: " << entry.path().filename().string());
        ++checked;
    }
    CHECK(checked >= 5); // 仓库里现在有 6 个
}

TEST_CASE("试玩交接 - 编辑器新建关卡的初始格子也能被解析") {
    // 复刻 EditorScene::loadFile 在"文件不存在"时铺的那张空框 ——
    // 新建一个关卡直接按 F5 是最常见的用法之一
    constexpr int w = 40;
    constexpr int h = 22;
    std::vector<std::string> lines(h, std::string(w, ' '));
    for (int x = 0; x < w; ++x) {
        lines[0][x] = '#';
        lines[h - 1][x] = '#';
    }
    for (int y = 0; y < h; ++y) {
        lines[y][0] = '#';
        lines[y][w - 1] = '#';
    }
    lines[2][3] = 'P';

    CHECK(PlaytestRequest::hasPlayerSpawn(lines));

    Level level;
    CHECK(level.loadFromString(joinLikePlaytest(lines)));
    CHECK(level.width() == w);
    CHECK(level.height() == h);
}
