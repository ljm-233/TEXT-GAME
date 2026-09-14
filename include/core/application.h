#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

class Container {
public:
    template <typename T> void registerType(std::function<std::shared_ptr<T>()> factory) {
        factories_[std::type_index(typeid(T))] = [factory]() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(factory());
        };
    }

    template <typename T> std::shared_ptr<T> resolve() {
        auto it = instances_.find(std::type_index(typeid(T)));
        if (it != instances_.end())
            return std::static_pointer_cast<T>(it->second);

        auto fit = factories_.find(std::type_index(typeid(T)));
        if (fit == factories_.end())
            throw std::runtime_error("Container: 未注册的类型");

        auto obj = std::static_pointer_cast<T>(fit->second());
        instances_[std::type_index(typeid(T))] = obj;
        return obj;
    }

private:
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>>
        factories_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> instances_;
};

class Application {
public:
    static Application& instance();
    void run();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    Application();
    ~Application() = default;

    void registerDependencies();

    Container container_;
};