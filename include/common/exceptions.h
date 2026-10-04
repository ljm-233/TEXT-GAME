#pragma once
//
// 领域异常基类。
//
// 约定：
//   - 基础设施层 / 服务层可以预期的失败，抛 AppError 的派生类；
//   - 调用方按类型捕获，不要靠字符串匹配 what()；
//   - 只有"不该发生"的程序性错误才用 std::logic_error。
//
// 与 albuswall 的 common/exceptions.py 对应：那边用 RuntimeError 派生，
// 这边用 std::runtime_error 派生，语义一致。
//
#include <stdexcept>
#include <string>
#include <utility>

/// 所有可预期失败的基类。
class AppError : public std::runtime_error {
public:
    explicit AppError(const std::string& what) : std::runtime_error(what) {}
};

/// 容器注册 / 解析失败。
///
/// 三种触发场景：
///   - 重复注册同名条目
///   - 解析未注册的名字
///   - get<T> 的 T 与注册时的返回类型不一致
///
/// 带上 name 与 type，是为了让报错本身能定位问题，
/// 而不是只丢一句 "未注册的类型"。
class ContainerError : public AppError {
public:
    ContainerError(std::string name, std::string reason)
        : AppError("Container[" + name + "]: " + reason),
          name_(std::move(name)),
          reason_(std::move(reason)) {}

    const std::string& name() const { return name_; }
    const std::string& reason() const { return reason_; }

private:
    std::string name_;
    std::string reason_;
};
