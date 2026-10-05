#include "core/resource_manager.h"

#include <algorithm>

ResourceManager::ResourceManager(const Paths& paths) {
    // 标准别名表。名字即调用方嘴里的"哪块资源"，
    // 值是它在包里的位置 —— 只有这里知道 assets/ 底下还有什么子目录。
    const std::filesystem::path assets = paths.assetsDir();
    add("assets", assets);
    add("levels", assets / "levels");
    add("shaders", assets / "shaders");
    add("lang", assets / "lang");
    add("defaults", assets / "defaults");
    add("wallpaper", paths.wallpaperDir());
}

void ResourceManager::add(std::string alias, std::filesystem::path dir) {
    if (alias.empty())
        return;
    dirs_[std::move(alias)] = std::move(dir);
}

std::filesystem::path ResourceManager::dir(const std::string& alias) const {
    auto it = dirs_.find(alias);
    return it != dirs_.end() ? it->second : std::filesystem::path{};
}

bool ResourceManager::has(const std::string& alias) const {
    return dirs_.find(alias) != dirs_.end();
}

std::vector<std::string> ResourceManager::aliases() const {
    std::vector<std::string> out;
    out.reserve(dirs_.size());
    for (const auto& [alias, _] : dirs_)
        out.push_back(alias);
    std::sort(out.begin(), out.end());
    return out;
}

std::filesystem::path ResourceManager::get(const std::string& alias,
                                           const std::string& rel) const {
    const std::filesystem::path root = dir(alias);
    if (root.empty())
        return {};
    if (rel.empty())
        return root;

    const std::filesystem::path sub(rel);

    // 绝对路径先直接拒掉：/etc/passwd、C:\evil 都不该能借别名溜出去
    if (sub.is_absolute() || sub.has_root_name() || sub.has_root_directory())
        return {};

    // 词法归一化把 "." 与 ".." 消化掉，再看还认不认得出原来的根。
    // 逐段比较而不是比字符串前缀 —— 否则 "/a/bc" 会被当成 "/a/b" 的子路径。
    const std::filesystem::path norm = (root / sub).lexically_normal();
    const std::filesystem::path rootNorm = root.lexically_normal();

    auto ri = rootNorm.begin();
    auto ni = norm.begin();
    for (; ri != rootNorm.end(); ++ri, ++ni) {
        if (ni == norm.end() || *ni != *ri)
            return {}; // 爬出子树了
    }
    return norm;
}

namespace {

/// 把 "${path:xxx}" 里的 xxx 解析成实际路径。
/// 解析不出来就返回空串，调用方保留原文。
std::string resolveToken(const ResourceManager& rm, const std::string& token) {
    constexpr const char* kPrefix = "path:";
    constexpr std::size_t kPrefixLen = 5;
    if (token.rfind(kPrefix, 0) != 0)
        return {}; // 目前只认 path: 这一种命名空间

    std::string body = token.substr(kPrefixLen);
    if (body.empty())
        return {};

    // "别名" 或 "别名/子路径"
    const auto slash = body.find('/');
    const std::string alias = (slash == std::string::npos) ? body : body.substr(0, slash);
    const std::string rel =
        (slash == std::string::npos) ? std::string{} : body.substr(slash + 1);

    const std::filesystem::path p = rm.get(alias, rel);
    return p.empty() ? std::string{} : p.string();
}

} // namespace

std::string ResourceManager::expand(const std::string& value) const {
    const auto first = value.find("${");
    if (first == std::string::npos)
        return value; // 绝大多数值走这条：一次 find 就结束

    std::string out;
    std::size_t i = 0;
    while (i < value.size()) {
        const auto open = value.find("${", i);
        if (open == std::string::npos) {
            out += value.substr(i);
            break;
        }
        out += value.substr(i, open - i);

        const auto close = value.find('}', open);
        if (close == std::string::npos) {
            out += value.substr(open); // 没有收尾的 }，原样留着
            break;
        }

        const std::string token = value.substr(open + 2, close - open - 2);
        const std::string resolved = resolveToken(*this, token);
        // 解析不出来就保留原文：写错的别名要看得见，不能静默变空
        out += resolved.empty() ? value.substr(open, close - open + 1) : resolved;

        i = close + 1;
    }
    return out;
}

std::string ResourceManager::str() const {
    std::string out = "ResourceManager(" + std::to_string(dirs_.size()) + " 个别名)";
    for (const auto& alias : aliases())
        out += "\n    " + alias + " = " + dirs_.at(alias).string();
    return out;
}
