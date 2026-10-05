#pragma once
#include <filesystem>
#include <string>
#include <vector>

/// 一张壁纸的元信息（不含解码结果）。
///
/// 抽成 POD 是为了让它**不依赖任何层** —— `WallpaperLibrary`、
/// `WallpaperLoader`、UI、配置都在用它。整个 wallpaper 层的接口都围绕它转：
/// 列表里存的是这个；loader 返回的也是这个；`current_wallpaper` 配置项
/// 存的是它里面的 `filename`。
///
/// 完整路径放在 `path`、文件名放在 `filename` 是为了让 UI 想要"显示列表"
/// 时不用每次 `filename()` 重新拼一次 —— `std::filesystem::path::filename()`
/// 在某些平台上不是 O(1)。
struct WallpaperInfo {
    std::filesystem::path path; ///< 完整路径（绝对或相对，不作要求）
    std::string filename;       ///< 不含路径，例如 "wallpaper.jpg"
    int index = -1;             ///< 在扫描列表里的位置（0-based），便于 UI 显示

    bool valid() const { return index >= 0 && !filename.empty(); }
};

/// 扫描时接受的扩展名（与 Background 旧的 scanDirectory 保持一致）。
///
/// 抽成共享常量而不是各自写一遍：将来 Background 切换格式时这里同步改。
inline const std::vector<std::string>& wallpaperSupportedExtensions() {
    static const std::vector<std::string> kExts = {".jpg", ".jpeg", ".png"};
    return kExts;
}
