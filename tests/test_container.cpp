#include "doctest.h"
#include "common/exceptions.h"
#include "core/container.h"

#include <memory>
#include <string>

namespace {

struct Service {
    int value = 0;
};

struct Other {
    int value = 0;
};

/// 构造计数器：用来验证"单例只构造一次 / transient 每次都构造"。
int g_constructed = 0;

struct Counted {
    Counted() { ++g_constructed; }
};

} // namespace

TEST_CASE("Container - 单例只构造一次并复用") {
    g_constructed = 0;
    Container container;
    container.reg<Counted>("counted", []() { return std::make_shared<Counted>(); });

    CHECK(g_constructed == 0);  // 注册不构造

    auto first = container.require<Counted>("counted");
    auto second = container.get<Counted>("counted");

    CHECK(g_constructed == 1);
    CHECK(first.get() == second.get());
}

TEST_CASE("Container - transient 每次解析都新建") {
    g_constructed = 0;
    Container container;
    container.reg<Counted>("counted", []() { return std::make_shared<Counted>(); },
                           /*singleton=*/false);

    auto first = container.get<Counted>("counted");
    auto second = container.get<Counted>("counted");

    CHECK(g_constructed == 2);
    CHECK(first.get() != second.get());
}

TEST_CASE("Container - 重复注册抛 ContainerError") {
    Container container;
    container.reg<Service>("svc", []() { return std::make_shared<Service>(); });

    CHECK_THROWS_AS(
        container.reg<Service>("svc", []() { return std::make_shared<Service>(); }),
        ContainerError);
}

TEST_CASE("Container - 解析未注册的名字抛 ContainerError") {
    Container container;

    CHECK_THROWS_AS(container.get<Service>("missing"), ContainerError);

    // 异常里要带上名字，否则报错定位不到问题
    try {
        container.get<Service>("missing");
        FAIL("应当抛异常");
    } catch (const ContainerError& e) {
        CHECK(e.name() == "missing");
    }
}

TEST_CASE("Container - 类型不符抛 ContainerError") {
    Container container;
    container.reg<Service>("svc", []() { return std::make_shared<Service>(); });

    // 取错类型必须当场炸，而不是静默返回一个错位的指针
    CHECK_THROWS_AS(container.get<Other>("svc"), ContainerError);
}

TEST_CASE("Container - tryGet 永不抛异常") {
    Container container;
    container.reg<Service>("svc", []() { return std::make_shared<Service>(); });

    CHECK(container.tryGet<Service>("missing") == nullptr);   // 未注册
    CHECK(container.tryGet<Other>("svc") == nullptr);         // 类型不符
    CHECK(container.tryGet<Service>("svc") != nullptr);       // 正常路径
}

TEST_CASE("Container - 工厂内部抛异常时 tryGet 吞掉并返回 nullptr") {
    Container container;
    container.reg<Service>("boom", []() -> std::shared_ptr<Service> {
        throw AppError("工厂炸了");
    });

    CHECK(container.tryGet<Service>("boom") == nullptr);
    CHECK_THROWS_AS(container.get<Service>("boom"), AppError);
}

TEST_CASE("Container - peek 只看已缓存的单例，不触发构造") {
    g_constructed = 0;
    Container container;
    container.reg<Counted>("counted", []() { return std::make_shared<Counted>(); });

    CHECK(container.peek<Counted>("counted") == nullptr);
    CHECK(g_constructed == 0);  // 关键：peek 不能有副作用

    container.touch("counted");

    CHECK(container.peek<Counted>("counted") != nullptr);
    CHECK(g_constructed == 1);
}

TEST_CASE("Container - touch 只构造不返回") {
    g_constructed = 0;
    Container container;
    container.reg<Counted>("counted", []() { return std::make_shared<Counted>(); });

    CHECK(container.contains("counted"));
    CHECK_FALSE(container.hasInstance("counted"));

    container.touch("counted");

    CHECK(container.hasInstance("counted"));
    CHECK(g_constructed == 1);
}

TEST_CASE("Container - touch 未注册的名字同样抛错") {
    Container container;
    CHECK_THROWS_AS(container.touch("missing"), ContainerError);
}

TEST_CASE("Container - contains 不构造，hasInstance 反映构造状态") {
    Container container;
    container.reg<Service>("svc", []() { return std::make_shared<Service>(); });

    CHECK(container.contains("svc"));
    CHECK_FALSE(container.hasInstance("svc"));
    CHECK_FALSE(container.contains("missing"));

    container.get<Service>("svc");

    CHECK(container.hasInstance("svc"));
}

TEST_CASE("Container - names 按注册顺序返回") {
    Container container;
    container.reg<Service>("alpha", []() { return std::make_shared<Service>(); });
    container.reg<Service>("beta", []() { return std::make_shared<Service>(); });
    container.reg<Service>("gamma", []() { return std::make_shared<Service>(); });

    const auto names = container.names();

    REQUIRE(names.size() == 3);
    CHECK(names[0] == "alpha");
    CHECK(names[1] == "beta");
    CHECK(names[2] == "gamma");
}

TEST_CASE("Container - str 是给人看的调试快照") {
    Container container;
    container.reg<Service>("svc", []() { return std::make_shared<Service>(); });

    const std::string before = container.str();
    CHECK(before.find("svc") != std::string::npos);
    CHECK(before.find("singleton") != std::string::npos);

    container.get<Service>("svc");
    CHECK(container.str().find("已构造") != std::string::npos);
}

TEST_CASE("Container - 同名不同类型互不干扰") {
    Container container;
    container.reg<Service>("svc", []() { return std::make_shared<Service>(); });
    container.reg<Other>("other", []() { return std::make_shared<Other>(); });

    CHECK(container.get<Service>("svc") != nullptr);
    CHECK(container.get<Other>("other") != nullptr);
}
