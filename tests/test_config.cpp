#include "doctest.h"
#include "config.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// 测试用临时目录：构造时创建，析构时删除
struct TempDir {
    fs::path root;

    TempDir() {
        static int counter = 0;
        root = fs::temp_directory_path() /
               ("textgame_test_" + std::to_string(++counter));
        std::error_code ec;
        fs::remove_all(root, ec);
        fs::create_directories(root);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

} // namespace

TEST_CASE("Config - 不存在的文件返回默认值") {
    TempDir tmp;
    fs::path file = tmp.root / "nonexistent.conf";

    Config cfg(file);

    CHECK(cfg.get("missing") == "");
    CHECK(cfg.get("missing", "default") == "default");
    CHECK(cfg.getInt("missing", 42) == 42);
    CHECK(cfg.getDouble("missing", 3.14) == doctest::Approx(3.14));
    CHECK(cfg.getBool("missing", true) == true);
    CHECK(cfg.getBool("missing", false) == false);
}

TEST_CASE("Config - set + flush + 重新加载") {
    TempDir tmp;
    fs::path file = tmp.root / "test.conf";

    {
        Config cfg(file);
        cfg.set("name", "hello");
        cfg.setInt("count", 42);
        cfg.setDouble("pi", 3.14);
        cfg.setBool("enabled", true);
        cfg.flush();
    }

    {
        Config cfg(file);
        CHECK(cfg.get("name") == "hello");
        CHECK(cfg.getInt("count") == 42);
        CHECK(cfg.getDouble("pi") == doctest::Approx(3.14));
        CHECK(cfg.getBool("enabled") == true);
    }
}

TEST_CASE("Config - set 相同值不标脏") {
    TempDir tmp;
    fs::path file = tmp.root / "test.conf";

    Config cfg(file);

    cfg.set("key", "value");
    CHECK(cfg.isDirty());

    cfg.flush();
    CHECK_FALSE(cfg.isDirty());

    cfg.set("key", "value");   // 相同的值
    CHECK_FALSE(cfg.isDirty());

    cfg.set("key", "different");
    CHECK(cfg.isDirty());

    cfg.flush();
    CHECK_FALSE(cfg.isDirty());
}

TEST_CASE("Config - getInt 转换失败回退默认值") {
    TempDir tmp;
    fs::path file = tmp.root / "test.conf";

    {
        std::ofstream out(file);
        out << "not_a_number=abc\n";
        out << "float_val=3.14\n";
    }

    Config cfg(file);

    CHECK(cfg.getInt("not_a_number", -1) == -1);   // abc 无法解析
    CHECK(cfg.getInt("float_val", -1) == 3);       // stoi("3.14") 停在 '.' 之前，返回 3
    CHECK(cfg.getInt("missing", -1) == -1);
}

TEST_CASE("Config - 手工写文件后读取") {
    TempDir tmp;
    fs::path file = tmp.root / "manual.conf";

    {
        std::ofstream out(file);
        out << "# 这是注释\n";
        out << "key1=value1\n";
        out << "\n";                 // 空行
        out << "key2=42\n";
        out << "no_equals_sign\n";   // 没有 = 的行
        out << "key3=value3\n";
    }

    Config cfg(file);

    CHECK(cfg.get("key1") == "value1");
    CHECK(cfg.getInt("key2") == 42);
    CHECK(cfg.get("key3") == "value3");
    CHECK(cfg.get("no_equals_sign") == "");
}

TEST_CASE("Config - resetAll 清空并落盘") {
    TempDir tmp;
    fs::path file = tmp.root / "test.conf";

    {
        Config cfg(file);
        cfg.set("a", "1");
        cfg.set("b", "2");
        cfg.set("c", "3");
        cfg.flush();
    }

    {
        Config cfg(file);
        CHECK(cfg.get("a") == "1");
        CHECK(cfg.get("b") == "2");

        cfg.resetAll();   // 立即清空 + 落盘

        CHECK(cfg.get("a") == "");
        CHECK(cfg.get("b") == "");
        CHECK(cfg.get("c") == "");
    }

    // 重新加载验证落盘确实清空
    {
        Config cfg(file);
        CHECK(cfg.get("a") == "");
        CHECK(cfg.get("b") == "");
        CHECK(cfg.get("c") == "");
    }
}

TEST_CASE("Config - 修改部分 key 不影响其他 key") {
    TempDir tmp;
    fs::path file = tmp.root / "test.conf";

    {
        std::ofstream out(file);
        out << "keep1=original1\n";
        out << "keep2=original2\n";
        out << "change=old\n";
    }

    {
        Config cfg(file);
        CHECK(cfg.get("keep1") == "original1");
        CHECK(cfg.get("keep2") == "original2");
        CHECK(cfg.get("change") == "old");

        cfg.set("change", "new");
        cfg.flush();
    }

    {
        Config cfg(file);
        CHECK(cfg.get("keep1") == "original1");
        CHECK(cfg.get("keep2") == "original2");
        CHECK(cfg.get("change") == "new");
    }
}

TEST_CASE("Config - getBool 各种真值表示") {
    TempDir tmp;
    fs::path file = tmp.root / "test.conf";

    {
        std::ofstream out(file);
        out << "t1=true\n";
        out << "t2=1\n";
        out << "t3=yes\n";
        out << "f1=false\n";
        out << "f2=0\n";
        out << "f3=no\n";
        out << "f4=random\n";
    }

    Config cfg(file);

    CHECK(cfg.getBool("t1"));
    CHECK(cfg.getBool("t2"));
    CHECK(cfg.getBool("t3"));
    CHECK_FALSE(cfg.getBool("f1"));
    CHECK_FALSE(cfg.getBool("f2"));
    CHECK_FALSE(cfg.getBool("f3"));
    CHECK_FALSE(cfg.getBool("f4"));   // 非真值当 false
}

TEST_CASE("Config - 空 key 可读写") {
    TempDir tmp;
    fs::path file = tmp.root / "test.conf";

    Config cfg(file);

    CHECK(cfg.get("") == "");
    CHECK(cfg.getInt("", -1) == -1);

    cfg.set("", "empty_key_value");
    CHECK(cfg.get("") == "empty_key_value");
}

TEST_CASE("Config - filePath 访问器") {
    TempDir tmp;
    fs::path file = tmp.root / "access.conf";

    Config cfg(file);
    CHECK(cfg.filePath() == file);
}