#pragma once
//
// 配置层装配。
//
// 与 albuswall configue/bootstrap.py 的两阶段配置对应：
//
//     BootstrapConfig —— 静态：启动前读一次，改了要重启（日志级别、首次运行标记）
//     Preferences     —— 动态：运行时可改，由 flush() 统一决定何时落盘
//     RuntimeConfig   —— 运行时状态：窗口尺寸这类"退出时记住"的数据
//
// 三者的区别不在存储格式（都是 key=value 文本），而在**可变性与落盘时机**，
// 所以拆成三个类而不是一个带命名空间的大配置。
//
#include "core/container.h"

/// 把配置层注册进容器。键名约定：
///
///     "bootstrap_config" -> BootstrapConfig
///     "preferences"      -> Preferences
///     "runtime_config"   -> RuntimeConfig
///
/// 只注册工厂，不构造实例。
///
/// 依赖方向：config -> core（需要 Paths 定位配置文件）。这是**解析期**依赖
/// 而非注册期依赖 —— 工厂在首次解析时才 require<Paths>，所以本函数可以
/// 先于 registerCore() 调用，装配顺序仍然按"底层在前"摆放。
void registerConfig(Container& container);
