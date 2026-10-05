#pragma once
#include "wallpaper/wallpaper_info.h"

#include <filesystem>
#include <string>
#include <vector>

/// 壁纸资源的纯逻辑层：扫盘、排序、匹配。
///
/// 把它从 Background 里抽出来有两个直接收益：
///   1. 不依赖 GL，于是**可以在无 DISPLAY 环境构造与测**（Background
///      内部要拿 `sf::Texture`，那是 GlResource，碰不得）
///   2. 切图/重命名/容错这些**配置相关**的逻辑全部沉到这里，UI 那层
///      只剩"画当前这张图"
///
/// 与 `ResourceManager` 的分工：
///   - `ResourceManager` 解决**路径**问题（"shaders/ 是哪一层、哪个文件"）
///   - `WallpaperLibrary` 解决**内容**问题（"这个目录下有几张图、哪张是用户选的"）
class WallpaperLibrary {
public:
    /// 从指定目录构造一份空库 —— 真正的扫描要等 `scan()` 触发。
    /// 空构造函数是为了测试里能"先造库、再放文件"地搭沙箱。
    WallpaperLibrary() = default;

    /// 立即扫描给定目录；之前的内容会被清空。
    ///
    /// 不存在的目录视为"0 张"，不抛异常 —— 启动时 `wallpaper/` 还没装好
    /// 或者用户在打包模式下文件被删了，不应该把游戏拒之门外。
    void scan(const std::filesystem::path& dir);

    /// 解析 `current_wallpaper` 这种存的是文件名（"wallpaper.jpg"）的配置项，
    /// 在库里找出对应的那张。
    ///
    /// 匹配规则（顺序）：
    ///   1. 全名匹配（"wallpaper.jpg"）
    ///   2. 主名匹配（"wallpaper"）—— 同一素材换扩展名（png → jpg）也能接上，
    ///      否则用户的设置会被静默打回默认、看着像"设置自己丢了"
    ///   3. 找不到返回 -1，调用方应回退到第一张（0）
    ///
    /// 返回 `info.index` 而不是 `info` 本身，是因为"index"这个语义对 UI 来说
    /// 更有用（显示"当前是第几张 / 共几张"），库内部表也用 index 索引。
    int resolveIndex(const std::string& requestedName) const;

    /// 取第 index 张；越界返回空 info。UI 渲染时拿到 index 后用这个取详情。
    WallpaperInfo at(int index) const;

    int size() const { return static_cast<int>(infos_.size()); }
    bool empty() const { return infos_.empty(); }

    /// 名字列表，按当前顺序 —— UI 用它显示候选项。
    /// 返回 `const std::vector<std::string>&` 是因为库是这些字符串的
    /// 唯一所有者，UI 不应该缓存副本。
    const std::vector<std::string>& filenames() const { return filenames_; }

    /// 扫描时用的目录（最后一次 scan 传进来的）。
    const std::filesystem::path& directory() const { return dir_; }

private:
    std::filesystem::path dir_;
    std::vector<WallpaperInfo> infos_; ///< 按扫描顺序排列
    std::vector<std::string>
        filenames_; ///< infos_[i].filename 的平行副本，省得 UI 反复 path 操作
};
