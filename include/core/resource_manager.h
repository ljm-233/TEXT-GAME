#pragma once
#include "paths.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

/// 静态资源寻址：**别名 → 包内路径**。
///
/// 为什么要有它：以前调用方各自手拼字符串 ——
/// `assetFile("levels/editor.txt")`、`assetsDir() / "shaders"`、
/// `assetsDir() / "lang"`、`assetsDir() / "defaults" / x ——
/// 于是"资源根在哪"这件事散落在十几个地方，而且没有任何穿越防护。
/// 现在收敛成一张别名表：调用方说「我要 lang 下的 zh.txt」，
/// 不再关心它在包里的哪一层。
///
/// ⚠️ 只管**随包发布的只读资源**（字体/关卡/着色器/语言/默认配置/壁纸）。
///    用户可写目录（config / saves / cache / temp）刻意不在这里 ——
///    那些跟机器绑定、落在 XDG 下，跟"资源"是两回事，仍然由 Paths 管。
///    对应 albuswall 的分法：`resources/` 管静态资源，
///    `configue/utils/platform_dir.py` 管用户目录。
///
/// 归 core 层，跟 Paths 并排；由 `registerCore` 注册成 `"resources"` 服务。
class ResourceManager {
public:
    ResourceManager() = default;

    /// 装好标准别名表（见 .cpp 里的名字清单）
    explicit ResourceManager(const Paths& paths);

    /// 注册一个别名。重复注册会被替换。
    void add(std::string alias, std::filesystem::path dir);

    /// 别名的根目录。别名不存在返回空路径。
    std::filesystem::path dir(const std::string& alias) const;

    /// 取别名下的某个文件或子目录。
    ///
    /// ⚠️ **带路径穿越防护**：`rel` 归一化之后必须仍在该别名的子树内，
    ///    否则返回空路径。`"../../etc/passwd"`、`"/etc/passwd"`、
    ///    `"C:\\evil"` 都会被拒。
    ///    纯词法判断（lexically_normal），不碰文件系统 —— 所以能直接单元测试，
    ///    也不会因为资源还不存在而误判。
    std::filesystem::path get(const std::string& alias,
                              const std::string& rel = {}) const;

    bool has(const std::string& alias) const;

    /// 已注册的别名，按字典序。
    std::vector<std::string> aliases() const;

    /// 展开配置值里的 `${path:别名}` 与 `${path:别名/子路径}`。
    ///
    /// 没有 `${` 时原样返回 —— 所以热路径上只是一次 `find`。
    /// 别名不存在或语法不完整时**保留原文**（而不是换成空串）：
    /// 写错的名字应该在下一次日志里看得见，而不是静默变成空路径。
    std::string expand(const std::string& value) const;

    /// 多行快照，别名按字典序（对应 Config::str() 的风格）
    std::string str() const;

private:
    std::unordered_map<std::string, std::filesystem::path> dirs_;
};
