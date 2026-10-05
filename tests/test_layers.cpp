#include "doctest.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// 分层架构测试。
//
// CLAUDE.md 里写着"game/ 层不应该 #include ui/ 层的东西"，但在这之前它只是
// 一句散文 —— 改造过程中实测发现两个方向都在违规，而且是靠写脚本逐个文件
// 扫描才找出来的。这条规则值得被机器守护，而不是靠人记得。
//
// 做法：扫描 include/ 与 src/ 下所有头文件与源文件，把 #include 解析成
// "来自哪一层、指向哪一层"，然后比对禁止表。
namespace {

const std::vector<std::string> kLayers = {
    "common", "utils", "infrastructure", "wallpaper", "config", "log",
    "core",   "ui",    "game",           "scene",
};

struct Rule {
    std::string from;
    std::vector<std::string> forbidden;
};

/// 禁止的依赖方向。
///
///   - ui 与 game 是兄弟层，**双向**都不许依赖（这是当初最容易被忽略的一条：
///     只查一个方向，另一个方向的违规就一直躺着）
///   - utils / common 是零内部依赖的叶子
///   - infrastructure 是设备层，不该认识应用层与前端
///   - config / log 只允许依赖 core 的 Container 契约与更底层
///
/// 如果将来某条边确实变得合理了，改这张表 —— 但那时应该是一次有意识的决定，
/// 而不是悄悄多出一行 include。
const std::vector<Rule> kRules = {
    {"ui", {"game"}},
    {"game", {"ui"}},
    {"utils",
     {"common", "infrastructure", "config", "log", "core", "ui", "game", "scene"}},
    {"infrastructure", {"config", "log", "core", "ui", "game", "scene"}},
    {"wallpaper", {"config", "log", "core", "ui", "game", "scene"}},
    {"common", {"infrastructure", "config", "log", "core", "ui", "game", "scene"}},
    {"config", {"log", "ui", "game", "scene"}},
    {"log", {"ui", "game", "scene"}},
};

/// basename -> 拥有它的层（可能多个层重名，比如 bootstrap.h）
using LayerIndex = std::map<std::string, std::set<std::string>>;

LayerIndex buildLayerIndex(const fs::path& includeRoot) {
    LayerIndex index;
    for (const auto& layer : kLayers) {
        const fs::path dir = includeRoot / layer;
        if (!fs::is_directory(dir))
            continue;
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".h")
                continue;
            index[entry.path().filename().string()].insert(layer);
        }
    }
    return index;
}

/// 解析一条 include 指向哪一层；判不出来返回空串。
std::string resolveLayer(const std::string& includeTarget, const LayerIndex& index) {
    if (includeTarget.find('/') != std::string::npos) {
        const std::string head = includeTarget.substr(0, includeTarget.find('/'));
        if (std::find(kLayers.begin(), kLayers.end(), head) != kLayers.end())
            return head;
        return {};
    }
    const auto it = index.find(includeTarget);
    // 重名的 basename 无法从平铺写法判定归属，只能跳过
    if (it != index.end() && it->second.size() == 1)
        return *it->second.begin();
    return {};
}

bool isForbidden(const std::string& from, const std::string& to) {
    for (const auto& rule : kRules) {
        if (rule.from != from)
            continue;
        if (std::find(rule.forbidden.begin(), rule.forbidden.end(), to) !=
            rule.forbidden.end())
            return true;
    }
    return false;
}

std::vector<std::string> collectFiles(const fs::path& dir) {
    std::vector<std::string> files;
    if (!fs::is_directory(dir))
        return files;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file())
            continue;
        const std::string ext = entry.path().extension().string();
        if (ext == ".h" || ext == ".cpp")
            files.push_back(entry.path().string());
    }
    return files;
}

/// 返回所有违规，格式 "相对路径:行号  from -> to: include"
std::vector<std::string> findViolations(const fs::path& root) {
    const fs::path includeRoot = root / "include";
    const LayerIndex index = buildLayerIndex(includeRoot);

    std::vector<std::string> violations;
    for (const auto& layer : kLayers) {
        for (const char* sub : {"include", "src"}) {
            const fs::path dir = root / sub / layer;
            for (const auto& file : collectFiles(dir)) {
                std::ifstream in(file);
                std::string line;
                int lineNo = 0;
                while (std::getline(in, line)) {
                    ++lineNo;
                    const auto pos = line.find("#include \"");
                    if (pos == std::string::npos)
                        continue;
                    const auto begin = pos + 10;
                    const auto end = line.find('"', begin);
                    if (end == std::string::npos)
                        continue;

                    const std::string target = line.substr(begin, end - begin);
                    const std::string to = resolveLayer(target, index);
                    if (to.empty() || to == layer)
                        continue;
                    if (!isForbidden(layer, to))
                        continue;

                    violations.push_back(
                        fs::path(file).lexically_relative(root).string() + ":" +
                        std::to_string(lineNo) + "  " + layer + " -> " + to + ": " +
                        target);
                }
            }
        }
    }
    std::sort(violations.begin(), violations.end());
    return violations;
}

std::string join(const std::vector<std::string>& items) {
    std::string out;
    for (const auto& item : items)
        out += "\n      " + item;
    return out;
}

} // namespace

TEST_CASE("分层架构 - include/ 下的每个目录都必须登记为已知层") {
    const fs::path includeRoot = fs::path(PROJECT_ROOT) / "include";
    REQUIRE(fs::is_directory(includeRoot));

    std::vector<std::string> unknown;
    for (const auto& entry : fs::directory_iterator(includeRoot)) {
        if (!entry.is_directory())
            continue;
        const std::string name = entry.path().filename().string();
        if (std::find(kLayers.begin(), kLayers.end(), name) == kLayers.end())
            unknown.push_back(name);
    }

    // 新增一层却没登记到 kLayers，这条规则就会被架空 —— 让它直接报错
    CHECK_MESSAGE(unknown.empty(),
                  "发现未登记的新层（请同步本文件的 kLayers 与 kRules）："
                      << join(unknown));
}

TEST_CASE("分层架构 - 没有跨层违规依赖") {
    const fs::path root = PROJECT_ROOT;
    REQUIRE(fs::is_directory(root / "include"));
    REQUIRE(fs::is_directory(root / "src"));

    const std::vector<std::string> violations = findViolations(root);

    CHECK_MESSAGE(violations.empty(), "发现 " << violations.size() << " 处跨层违规依赖："
                                              << join(violations));
}

TEST_CASE("分层架构 - 规则表本身是自洽的") {
    // 防止把层名拼错：规则里出现的每个层名都必须是 kLayers 的成员
    std::vector<std::string> typos;
    for (const auto& rule : kRules) {
        if (std::find(kLayers.begin(), kLayers.end(), rule.from) == kLayers.end())
            typos.push_back("from: " + rule.from);
        for (const auto& to : rule.forbidden) {
            if (std::find(kLayers.begin(), kLayers.end(), to) == kLayers.end())
                typos.push_back(rule.from + " -> " + to);
        }
    }
    CHECK_MESSAGE(typos.empty(), "规则表里有未知层名：" << join(typos));
}
