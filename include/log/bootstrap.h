#pragma once
//
// 日志层装配。
//
// 与 albuswall 的 log/bootstrap.py 对应：每层自己提供装配函数，
// 由入口的 boot 阶段统一调用，层与层之间不互相 import 装配代码。
//
#include "core/container.h"

/// 把日志层注册进容器。键名约定：
///
///     "log_console" -> std::shared_ptr<ConsoleHandler>
///     "log_file"    -> std::shared_ptr<FileHandler>   （未注册 bootstrap_config 时不提供）
///     "logger"      -> std::shared_ptr<Logger>
///
/// 只注册工厂，不构造实例：日志级别、轮转档位都在首次解析 "logger"
/// 时才从 "preferences" 读取。
///
/// 依赖方向：log -> config（单向）。这是刻意保持的 ——
/// albuswall 那边 log 与 configue 双向 import，靠 __getattr__ 惰性续命，
/// 等价于 C++ 的静态初始化顺序陷阱，这里不复制那个问题。
void registerLog(Container& container);
