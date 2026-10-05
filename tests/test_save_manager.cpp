#include "doctest.h"
#include "core/paths.h"
#include "log/logger.h"
#include "save_manager.h"
#include "utils/text_strings.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// SaveManager 管存档的读写与进度，属于"错了会悄悄丢玩家存档"的那类代码，
// 所以这里除了正常路径，重点钉的是几条容易写错的语义：
//   - 金币与关卡进度只增不减（不会因为重玩低关而被改小）
//   - 星级取最高、PB 取最小
//   - 旧字段名 progress= 仍能读出（历史存档兼容）
//   - 字段缺失 / 星级数组短于 9 关时怎么补
//
// 以前它被标成"依赖运行环境"没测，是因为 Paths 把目录钉死在 PROJECT_ROOT，
// 测试只能去动真实存档目录。现在 Paths 支持传入根目录，于是有了沙箱。
namespace {

/// 每个用例一个独立沙箱：config/saves/... 全建在临时目录下。
struct Sandbox {
    fs::path root;
    std::shared_ptr<Paths> paths;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<SaveManager> saves;

    Sandbox() {
        static int counter = 0;
        root = fs::temp_directory_path() /
               ("textgame_save_test_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(root, ec);

        paths = std::make_shared<Paths>(root);
        // Error 级别 + 不挂任何 handler = 完全静默，免得刷屏
        logger = std::make_shared<Logger>(LogLevel::Error);
        // SaveManager 现在只收 Paths —— 以前收 RuntimeConfig 只是为了借它的
        // saveFile/savesDir 两个转发，顺带还带来一个坑：Config 析构会 flush，
        // 所以沙箱必须先收 config 再删目录。现在没有这个顺序要求了。
        saves = std::make_unique<SaveManager>(paths, logger);
    }

    ~Sandbox() {
        // 先把还攥着 paths 的对象收掉，再删目录。顺序不能反。
        saves.reset();
        logger.reset();
        paths.reset();

        std::error_code ec;
        fs::remove_all(root, ec);
    }

    fs::path savePath(const std::string& filename) const {
        return root / "saves" / filename;
    }

    /// 手工写一个存档文件，用来构造"历史格式"或残缺内容
    void writeRawSave(const std::string& filename, const std::string& content) {
        std::ofstream out(savePath(filename));
        out << content;
    }
};

const char* kGoodSave =
    "name=测试存档\n"
    "created_at=2026-01-01 00:00:00\n"
    "last_played=2026-01-02 00:00:00\n"
    "coins=42\n"
    "current_level=3\n"
    "level_stars=3,2,1,0,0,0,0,0,0\n"
    "level_best_times=12.34,20.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00\n";

} // namespace

// ============================================================
// 沙箱自检
// ============================================================

TEST_CASE("SaveManager - 沙箱不会碰到真实存档目录") {
    Sandbox box;

    // Paths(root) 之下才该有这些目录
    CHECK(box.paths->savesDir() == box.root / "saves");
    CHECK(box.paths->mode() == "test");
    CHECK(fs::exists(box.paths->savesDir()));

    // 真实工程目录不该被这次构造动过
    const fs::path realSaves = fs::path(PROJECT_ROOT) / "saves";
    CHECK(box.paths->savesDir() != realSaves);
}

// ============================================================
// createSave / loadSave
// ============================================================

TEST_CASE("SaveManager - 创建存档后能再读回来") {
    Sandbox box;

    const SaveInfo created = box.saves->createSave("我的存档");

    CHECK_FALSE(created.filename.empty());
    CHECK(created.name == "我的存档");
    CHECK(created.coins == 0);
    CHECK(created.currentLevel == 1);
    CHECK(created.levelStars.size() == 9);
    CHECK(created.levelBestTimes.size() == 9);
    CHECK(fs::exists(box.savePath(created.filename)));

    SaveInfo loaded;
    REQUIRE(box.saves->loadSave(created.filename, loaded));

    CHECK(loaded.filename == created.filename);
    CHECK(loaded.name == "我的存档");
    CHECK(loaded.currentLevel == 1);
    CHECK(loaded.levelStars == created.levelStars);
}

TEST_CASE("SaveManager - 不传名字时用前缀加时间戳") {
    Sandbox box;

    const SaveInfo created = box.saves->createSave();

    CHECK_FALSE(created.name.empty());
    CHECK(created.name.rfind(Str::SaveNamePrefix, 0) == 0);
}

TEST_CASE("SaveManager - 读不存在的存档返回 false 而不是抛异常") {
    Sandbox box;

    SaveInfo info;
    CHECK_FALSE(box.saves->loadSave("nope.conf", info));
}

TEST_CASE("SaveManager - 读回全部字段") {
    Sandbox box;
    box.writeRawSave("full.conf", kGoodSave);

    SaveInfo info;
    REQUIRE(box.saves->loadSave("full.conf", info));

    CHECK(info.name == "测试存档");
    CHECK(info.coins == 42);
    CHECK(info.currentLevel == 3);
    REQUIRE(info.levelStars.size() == 9);
    CHECK(info.levelStars[0] == 3);
    CHECK(info.levelStars[1] == 2);
    CHECK(info.levelStars[2] == 1);
    REQUIRE(info.levelBestTimes.size() == 9);
    CHECK(info.levelBestTimes[0] == doctest::Approx(12.34f));
    CHECK(info.levelBestTimes[1] == doctest::Approx(20.f));
    CHECK(info.levelBestTimes[2] == doctest::Approx(0.f));
}

TEST_CASE("SaveManager - 兼容旧字段名 progress（历史存档）") {
    Sandbox box;
    // 老版本把金币数存在 progress= 里
    box.writeRawSave("legacy.conf",
                     "name=老存档\n"
                     "progress=99\n"
                     "current_level=5\n");

    SaveInfo info;
    REQUIRE(box.saves->loadSave("legacy.conf", info));

    CHECK(info.coins == 99);
    CHECK(info.currentLevel == 5);
}

TEST_CASE("SaveManager - 关卡数组短于 9 关时补齐") {
    Sandbox box;
    box.writeRawSave("short.conf",
                     "name=短\n"
                     "level_stars=3,1\n"
                     "level_best_times=5.00\n");

    SaveInfo info;
    REQUIRE(box.saves->loadSave("short.conf", info));

    REQUIRE(info.levelStars.size() == 9);
    CHECK(info.levelStars[0] == 3);
    CHECK(info.levelStars[1] == 1);
    CHECK(info.levelStars[2] == 0);
    CHECK(info.levelStars[8] == 0);

    REQUIRE(info.levelBestTimes.size() == 9);
    CHECK(info.levelBestTimes[0] == doctest::Approx(5.f));
    CHECK(info.levelBestTimes[8] == doctest::Approx(0.f));
}

TEST_CASE("SaveManager - 字段缺失时用默认值填充") {
    Sandbox box;
    box.writeRawSave("thin.conf", "name=只有名字\n");

    SaveInfo info;
    REQUIRE(box.saves->loadSave("thin.conf", info));

    CHECK(info.name == "只有名字");
    CHECK(info.coins == 0);
    CHECK(info.currentLevel == 1);
    CHECK(info.levelStars.size() == 9);
    CHECK(info.levelBestTimes.size() == 9);
}

TEST_CASE("SaveManager - 字段值不是数字时回退默认而不是抛异常") {
    Sandbox box;
    box.writeRawSave("bad.conf",
                     "name=坏的\n"
                     "coins=abc\n"
                     "current_level=xyz\n"
                     "level_stars=3,abc,1\n");

    SaveInfo info;
    REQUIRE(box.saves->loadSave("bad.conf", info));

    CHECK(info.coins == 0);
    CHECK(info.currentLevel == 1);
    CHECK(info.levelStars[0] == 3);
    CHECK(info.levelStars[1] == 0);   // 解析失败的那一项当 0
}

// ============================================================
// listSaves
// ============================================================

TEST_CASE("SaveManager - 空目录列出空列表") {
    Sandbox box;

    CHECK(box.saves->listSaves().empty());
}

TEST_CASE("SaveManager - 只列 .conf，且按最后游玩时间倒序") {
    Sandbox box;
    box.writeRawSave("old.conf",
                     "name=旧的\nlast_played=2026-01-01 00:00:00\n");
    box.writeRawSave("new.conf",
                     "name=新的\nlast_played=2026-06-01 00:00:00\n");
    box.writeRawSave("notes.txt", "不是存档\n");

    const auto list = box.saves->listSaves();

    REQUIRE(list.size() == 2);
    CHECK(list[0].name == "新的");   // 最近玩过的排前面
    CHECK(list[1].name == "旧的");
}

// ============================================================
// updateProgress
// ============================================================

TEST_CASE("SaveManager - 进度只增不减") {
    Sandbox box;
    box.writeRawSave("p.conf", kGoodSave);   // coins=42, level=3

    CHECK(box.saves->updateProgress("p.conf", 10, 1));   // 更低的成绩

    SaveInfo info;
    REQUIRE(box.saves->loadSave("p.conf", info));
    CHECK(info.coins == 42);          // 没被改小
    CHECK(info.currentLevel == 3);
}

TEST_CASE("SaveManager - 进度提高时会被写入") {
    Sandbox box;
    box.writeRawSave("p.conf", kGoodSave);

    CHECK(box.saves->updateProgress("p.conf", 80, 5));

    SaveInfo info;
    REQUIRE(box.saves->loadSave("p.conf", info));
    CHECK(info.coins == 80);
    CHECK(info.currentLevel == 5);
}

TEST_CASE("SaveManager - 对不存在的存档更新进度返回 false") {
    Sandbox box;

    CHECK_FALSE(box.saves->updateProgress("missing.conf", 10, 2));
}

// ============================================================
// setLevelStar
// ============================================================

TEST_CASE("SaveManager - 星级取最高，不会被更差的成绩覆盖") {
    Sandbox box;
    box.writeRawSave("s.conf", kGoodSave);   // 第 1 关 3 星

    CHECK(box.saves->setLevelStar("s.conf", 1, 1));   // 更差

    SaveInfo info;
    REQUIRE(box.saves->loadSave("s.conf", info));
    CHECK(info.levelStars[0] == 3);
}

TEST_CASE("SaveManager - 星级提高时会被写入") {
    Sandbox box;
    box.writeRawSave("s.conf", kGoodSave);   // 第 4 关 0 星

    CHECK(box.saves->setLevelStar("s.conf", 4, 2));

    SaveInfo info;
    REQUIRE(box.saves->loadSave("s.conf", info));
    CHECK(info.levelStars[3] == 2);
}

TEST_CASE("SaveManager - 越界的关卡号被拒绝") {
    Sandbox box;
    box.writeRawSave("s.conf", kGoodSave);

    CHECK_FALSE(box.saves->setLevelStar("s.conf", 0, 3));
    CHECK_FALSE(box.saves->setLevelStar("s.conf", 10, 3));
    CHECK_FALSE(box.saves->setLevelStar("s.conf", -1, 3));
}

// ============================================================
// setLevelBestTime
// ============================================================

TEST_CASE("SaveManager - PB 取最小，更慢的成绩不算刷新") {
    Sandbox box;
    box.writeRawSave("t.conf", kGoodSave);   // 第 1 关 12.34 秒

    // 注意：未刷新时返回 false，与 setLevelStar（未提升也返回 true）不一致，
    // 这里把当前行为钉住 —— 改语义时至少会有一条测试提醒。
    CHECK_FALSE(box.saves->setLevelBestTime("t.conf", 1, 30.f));

    SaveInfo info;
    REQUIRE(box.saves->loadSave("t.conf", info));
    CHECK(info.levelBestTimes[0] == doctest::Approx(12.34f));
}

TEST_CASE("SaveManager - 更快的成绩会刷新 PB") {
    Sandbox box;
    box.writeRawSave("t.conf", kGoodSave);

    CHECK(box.saves->setLevelBestTime("t.conf", 1, 8.5f));

    SaveInfo info;
    REQUIRE(box.saves->loadSave("t.conf", info));
    CHECK(info.levelBestTimes[0] == doctest::Approx(8.5f));
}

TEST_CASE("SaveManager - 0 秒视为无记录，第一次通关一定写入") {
    Sandbox box;
    box.writeRawSave("t.conf", kGoodSave);   // 第 5 关是 0

    CHECK(box.saves->setLevelBestTime("t.conf", 5, 60.f));

    SaveInfo info;
    REQUIRE(box.saves->loadSave("t.conf", info));
    CHECK(info.levelBestTimes[4] == doctest::Approx(60.f));
}

TEST_CASE("SaveManager - 非正的时间被拒绝") {
    Sandbox box;
    box.writeRawSave("t.conf", kGoodSave);

    CHECK_FALSE(box.saves->setLevelBestTime("t.conf", 1, 0.f));
    CHECK_FALSE(box.saves->setLevelBestTime("t.conf", 1, -5.f));
}

TEST_CASE("SaveManager - 越界的关卡号被拒绝（时间）") {
    Sandbox box;
    box.writeRawSave("t.conf", kGoodSave);

    CHECK_FALSE(box.saves->setLevelBestTime("t.conf", 0, 10.f));
    CHECK_FALSE(box.saves->setLevelBestTime("t.conf", 10, 10.f));
}

// ============================================================
// deleteSave
// ============================================================

TEST_CASE("SaveManager - 删除存档") {
    Sandbox box;
    box.writeRawSave("gone.conf", kGoodSave);
    REQUIRE(fs::exists(box.savePath("gone.conf")));

    CHECK(box.saves->deleteSave("gone.conf"));
    CHECK_FALSE(fs::exists(box.savePath("gone.conf")));
    CHECK(box.saves->listSaves().empty());
}

TEST_CASE("SaveManager - 删除不存在的存档返回 false") {
    Sandbox box;

    CHECK_FALSE(box.saves->deleteSave("never.conf"));
}

// ============================================================
// pending save（场景之间传存档的通道）
// ============================================================

TEST_CASE("SaveManager - pending save 取走之后就没了") {
    Sandbox box;

    CHECK_FALSE(box.saves->hasPendingSave());

    SaveInfo info;
    info.filename = "x.conf";
    info.name = "待进入的存档";
    box.saves->setPendingSave(info);

    CHECK(box.saves->hasPendingSave());

    const SaveInfo taken = box.saves->takePendingSave();
    CHECK(taken.filename == "x.conf");
    CHECK(taken.name == "待进入的存档");

    // 取走即清空，避免下一次进游戏又跳回同一个存档
    CHECK_FALSE(box.saves->hasPendingSave());

    // ⭐ 也正因为"取走即清空"，连着调两次是致命的：第二次拿到的是空 SaveInfo。
    //    GameScene::onEnter 里就多调了一次，第二次把刚读到的 save_ 冲成空 ——
    //    filename 为空 → saveFile("") 指向 saves 目录本身 →
    //    updateProgress / setLevelStar / setLevelBestTime 全部写入失败，
    //    通关不记进度、星级与 PB 永不落盘。
    const SaveInfo again = box.saves->takePendingSave();
    CHECK(again.filename.empty());
}
