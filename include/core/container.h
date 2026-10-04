#pragma once
//
// 最小注册表：name -> factory -> instance。
//
// Container 只负责"注册"和"按需解析"。什么时候构造、什么时候拆除、
// 按什么顺序执行，由 Application 编排。Container 不认识生命周期。
//
// 与 albuswall 的 core/container.py 对应，两点 C++ 化的等价替换：
//   1. Python 的工厂没有返回类型声明，所以那边有 annotate() 事后补元数据；
//      这边 reg<T>() 在注册时就知道 T，annotate 没有必要，故不提供。
//   2. 那边 get() 返回 Any，类型错误要到运行期才炸；这边 get<T>() 会
//      比对注册时的 type_index，取错类型立刻抛 ContainerError。
//
// 线程约定：非线程安全。解析应集中在装配阶段（Application 的 boot/wire
// 两个 phase）完成，运行期只读已缓存的单例。
//
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "common/exceptions.h"

class Container {
public:
    // ---------------- 注册 ----------------

    /// 注册一个类型工厂。
    ///
    /// singleton = true 时，首次解析构造并缓存；false 时每次解析都新建。
    /// 重复注册同名条目抛 ContainerError（不做隐式覆盖，覆盖意图必须显式）。
    template <typename T>
    void reg(const std::string& name, std::function<std::shared_ptr<T>()> factory,
             bool singleton = true) {
        if (entries_.count(name) != 0)
            throw ContainerError(name, "重复注册");

        Entry entry;
        entry.factory = [factory]() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(factory());
        };
        entry.singleton = singleton;
        entry.type = std::type_index(typeid(T));

        entries_.emplace(name, std::move(entry));
        order_.push_back(name);
    }

    // ---------------- 解析 ----------------

    /// 按需构造。未注册或类型不符抛 ContainerError。
    template <typename T>
    std::shared_ptr<T> get(const std::string& name) {
        return resolve<T>(name, false);
    }

    /// 语义等同于 get<T>()：调用方在说"这个依赖是必须的"。
    /// 保留这个名字是为了让装配代码读起来能区分"必需"和"可选"。
    template <typename T>
    std::shared_ptr<T> require(const std::string& name) {
        return resolve<T>(name, false);
    }

    /// 永不抛异常的可选查询：未注册、类型不符、工厂内部抛异常，一律返回 nullptr。
    template <typename T>
    std::shared_ptr<T> tryGet(const std::string& name) noexcept {
        try {
            return resolve<T>(name, false);
        } catch (...) {
            return nullptr;
        }
    }

    /// 只看已缓存的单例；未构造或类型不符返回 nullptr。不触发构造。
    template <typename T>
    std::shared_ptr<T> peek(const std::string& name) const {
        auto it = entries_.find(name);
        if (it == entries_.end() || !it->second.instance)
            return nullptr;
        if (it->second.type != std::type_index(typeid(T)))
            return nullptr;
        return std::static_pointer_cast<T>(it->second.instance);
    }

    /// 名字是否注册过（不代表已构造）。
    bool contains(const std::string& name) const;

    /// 单例是否已经构造出来。
    bool hasInstance(const std::string& name) const;

    /// 只构造单例、不要返回值。用于把"构造顺序"显式写出来，
    /// 而不必为了触发构造去 include 对应的类型。未注册抛 ContainerError。
    void touch(const std::string& name);

    // ---------------- 调试 ----------------

    /// 已注册的名字，按注册顺序。
    std::vector<std::string> names() const;

    /// 多行快照：每个条目的名字、是否单例、是否已构造、实例类型。
    std::string str() const;

private:
    struct Entry {
        std::function<std::shared_ptr<void>()> factory;
        bool singleton = true;
        std::type_index type{typeid(void)};
        std::shared_ptr<void> instance;
    };

    /// 解析的共同实现：类型校验 -> 单例缓存 -> 构造。
    template <typename T>
    std::shared_ptr<T> resolve(const std::string& name, bool /*allowMissing*/) {
        auto it = entries_.find(name);
        if (it == entries_.end())
            throw ContainerError(name, "未注册");

        Entry& entry = it->second;

        if (entry.type != std::type_index(typeid(T))) {
            throw ContainerError(
                name, std::string("类型不符：注册为 ") + entry.type.name() +
                          "，请求为 " + std::type_index(typeid(T)).name());
        }

        if (!entry.singleton)
            return std::static_pointer_cast<T>(entry.factory());

        if (!entry.instance)
            entry.instance = entry.factory();

        return std::static_pointer_cast<T>(entry.instance);
    }

    std::unordered_map<std::string, Entry> entries_;
    std::vector<std::string> order_;  // 让 str()/names() 的输出稳定可读
};
