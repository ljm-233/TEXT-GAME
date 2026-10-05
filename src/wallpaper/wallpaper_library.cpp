#include "wallpaper/wallpaper_library.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {

bool hasSupportedExtension(const std::filesystem::path& p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    for (const auto& supported : wallpaperSupportedExtensions()) {
        if (ext == supported)
            return true;
    }
    return false;
}

} // namespace

void WallpaperLibrary::scan(const std::filesystem::path& dir) {
    namespace fs = std::filesystem;

    dir_ = dir;
    infos_.clear();
    filenames_.clear();

    std::error_code ec;
    if (!fs::exists(dir_, ec))
        return;

    std::vector<fs::path> found;
    for (const auto& entry : fs::directory_iterator(dir_, ec)) {
        if (ec)
            break;
        if (!entry.is_regular_file(ec))
            continue;
        if (!hasSupportedExtension(entry.path()))
            continue;
        found.push_back(entry.path());
    }
    // 排序要稳定 —— 不同文件系统大小写规则不同，先按小写文件名比，再按原名。
    // 这样 dev/release 顺序一致，CI 也不会因为临时目录大小写乱跳。
    std::sort(found.begin(), found.end(), [](const fs::path& a, const fs::path& b) {
        auto la = a.filename().string();
        auto lb = b.filename().string();
        std::transform(la.begin(), la.end(), la.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::transform(lb.begin(), lb.end(), lb.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (la != lb)
            return la < lb;
        return a.filename().string() < b.filename().string();
    });

    infos_.reserve(found.size());
    filenames_.reserve(found.size());
    for (size_t i = 0; i < found.size(); ++i) {
        WallpaperInfo info;
        info.path = found[i];
        info.filename = found[i].filename().string();
        info.index = static_cast<int>(i);
        infos_.push_back(info);
        filenames_.push_back(info.filename);
    }
}

int WallpaperLibrary::resolveIndex(const std::string& requestedName) const {
    if (requestedName.empty())
        return -1;

    // 全名匹配
    for (size_t i = 0; i < infos_.size(); ++i) {
        if (infos_[i].filename == requestedName)
            return static_cast<int>(i);
    }

    // 主名匹配：png 改 jpg 之类的换扩展名时仍然能接上
    std::filesystem::path want(requestedName);
    const auto wantStem = want.stem().string();
    if (wantStem.empty())
        return -1;
    for (size_t i = 0; i < infos_.size(); ++i) {
        if (infos_[i].path.stem().string() == wantStem)
            return static_cast<int>(i);
    }

    return -1;
}

WallpaperInfo WallpaperLibrary::at(int index) const {
    if (index < 0 || index >= static_cast<int>(infos_.size()))
        return {};
    return infos_[index];
}
