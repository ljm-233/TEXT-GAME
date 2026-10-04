#pragma once
//
// 核心层装配与接线。
//
// 与 albuswall 的 __main__.py 里 `_boot()` 的分工一致：
//   registerCore —— 只往容器里放工厂。不构造实例、不产生副作用。
//   wireCore     —— 把"偏好生效 / 子系统初始化 / 退出收尾"挂成生命周期钩子。
//                   注册钩子本身也不执行，真正执行发生在
//                   Application 的 wire / teardown 阶段。
//
// 旧实现把这两件事混在 Application::registerDependencies() 里：注册到一半
// 就开始改 UI 缩放、切主题、初始化音效。后果是"构造 Application"等于
// "启动半个程序"，没法在测试里单独装配容器。拆开之后 boot 阶段是纯的。
//
#include "core/application.h"
#include "core/container.h"

/// 注册核心层单例。键名约定：
///
///     "paths"            -> Paths
///     "window"           -> Window
///     "background"       -> Background
///     "font_holder"      -> FontHolder
///     "save_manager"     -> SaveManager
///     "scene_registry"   -> SceneRegistry（空表，由 scene 层的 registerScenes 填）
///     "game"             -> Game
///     "main_loop"        -> MainLoop（就是 Game 本身，前端交出主循环的入口）
///
/// 三个 Config（"bootstrap_config" / "preferences" / "runtime_config"）
/// 由 config 层的 registerConfig() 负责注册。
///
void registerCore(Container& container);

/// 挂核心层的生命周期钩子。
void wireCore(Application& app);
