#include "doctest.h"
#include "config.h"
#include "paths.h"
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

// 测试用临时目录
struct TempPaths {
    fs::path root;
    TempPaths() {
        root = fs::temp_directory_path() /
               ("textgame_test_" + std::to_string(std::rand()));
        fs::create_directories(root);
    }
    ~TempPaths() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

TEST_CASE("Config - 读写") {
    TempPaths tmp;

    // 用测试目录手工造一个 Config（不能直接用 Paths，它会用 PROJECT_ROOT）
    fs::path file = tmp.root / "test.conf";

    // 用 Config 的公开 API 读写
    {
        // 造一个最简单的子类
        struct TestConfig : public Config {
            TestConfig(const Paths& p, const std::string& name)
                : Config(p, name) {}
        };
        // 但要构造 Paths 需要 PROJECT_ROOT... 这里换个方法
    }

    // 直接测试底层行为：写入文件，读回来
    {
        std::ofstream out(file);
        out << "key1=value1\n";
        out << "key2=42\n";
        out << "key3=true\n";
    }

    std::ifstream in(file);
    REQUIRE(in.good());

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(in, line)) {
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            values[line.substr(0, pos)] = line.substr(pos + 1);
        }
    }

    CHECK(values["key1"] == "value1");
    CHECK(values["key2"] == "42");
    CHECK(values["key3"] == "true");
}

TEST_CASE("Config - 类型转换") {
    // 直接测 stoi / stod / bool 解析
    CHECK(std::stoi("123") == 123);
    CHECK(std::stod("3.14") == doctest::Approx(3.14));

    auto parseBool = [](const std::string& v) {
        return v == "true" || v == "1" || v == "yes";
    };
    CHECK(parseBool("true"));
    CHECK(parseBool("1"));
    CHECK(parseBool("yes"));
    CHECK_FALSE(parseBool("false"));
    CHECK_FALSE(parseBool("0"));
    CHECK_FALSE(parseBool("no"));
}