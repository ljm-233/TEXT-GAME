#include "doctest.h"
#include "core/platform.h"

#include <filesystem>
#include <string>

// Platform::openDirectory 的真实行为会去启动文件管理器，没有 DISPLAY 的环境
// 根本测不了（而且不该在单测里弹窗）。能测的只有「参数不合法就直接返回 false、
// 连外部命令都不去执行」这条分支 —— 它恰好也是唯一不依赖桌面会话的一条。

namespace fs = std::filesystem;

TEST_CASE("Platform::openDirectory 目录不存在时返回 false") {
    // 名字带随机后缀，保证在正常的 TMPDIR 下不存在
    const fs::path missing =
        fs::temp_directory_path() / "text-game-绝对不存在的目录-9f3c1a";
    const bool exists = fs::exists(missing);
    REQUIRE_FALSE(exists);

    const bool opened = Platform::openDirectory(missing);
    CHECK_FALSE(opened);
}

TEST_CASE("Platform::openDirectory 空路径与文件返回 false") {
    const bool empty = Platform::openDirectory(fs::path{});
    CHECK_FALSE(empty);

    // 传目录以外的东西也一律拒绝，不去执行外部命令
    const fs::path file = fs::temp_directory_path() / "text-game-not-a-dir-9f3c1a";
    const bool isDir = fs::is_directory(file);
    REQUIRE_FALSE(isDir);
    const bool opened = Platform::openDirectory(file);
    CHECK_FALSE(opened);
}
