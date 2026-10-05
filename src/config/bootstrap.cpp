#include "config/bootstrap.h"

#include "config/bootstrap_config.h"
#include "config/preferences.h"
#include "config/runtime_config.h"
#include "core/paths.h"
#include "core/resource_manager.h"

#include <filesystem>
#include <memory>

namespace {

// 首次运行时，把随包发布的默认设置铺到用户配置目录。
//
// 为什么需要它：打包模式下 Config 读的是 ~/.config/text-game/，
// 而包里那份文件它根本不会看。想让发行版带上"预设设置"，只能在首次运行时
// 落一次盘 —— 也就是这里。
//
// 两条原则：
//   1. **只在目标文件不存在时写**。用户改过之后绝不再覆盖，否则每次升级
//      都会把玩家的设置冲掉。
//   2. 找不到默认文件就静默跳过，走代码里的内置默认值。
//      这样开发模式（没带 defaults/）和精简打包都不会出问题。
void seedDefaultIfMissing(const Paths& paths, const ResourceManager& resources,
                          const std::string& filename) {
    namespace fs = std::filesystem;
    std::error_code ec;

    const fs::path target = paths.configDir() / filename;
    if (fs::exists(target, ec))
        return;   // 已经有了：这是用户的文件，别动

    // 走别名，不自己拼 assets/defaults/ —— "默认配置放在包里的哪一层"
    // 只有别名表该知道
    const fs::path source = resources.get("defaults", filename);
    if (source.empty() || !fs::exists(source, ec))
        return;   // 没带默认值，用内置默认

    fs::create_directories(target.parent_path(), ec);
    fs::copy_file(source, target, fs::copy_options::none, ec);
}

} // namespace

void registerConfig(Container& container) {
    container.reg<BootstrapConfig>("bootstrap_config", [&container]() {
        return std::make_shared<BootstrapConfig>(
            *container.require<Paths>("paths"),
            *container.require<ResourceManager>("resources"));
    });

    container.reg<Preferences>("preferences", [&container]() {
        const Paths& paths = *container.require<Paths>("paths");
        const ResourceManager& resources =
            *container.require<ResourceManager>("resources");
        seedDefaultIfMissing(paths, resources, "preferences.conf");
        return std::make_shared<Preferences>(paths, resources);
    });

    container.reg<RuntimeConfig>("runtime_config", [&container]() {
        return std::make_shared<RuntimeConfig>(
            *container.require<Paths>("paths"),
            *container.require<ResourceManager>("resources"));
    });
}
