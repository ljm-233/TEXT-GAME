#pragma once
//
// 场景层装配。
//
// 把 8 个具体场景登记进 SceneRegistry。工厂在**首次进入该场景时**才被调用
// （SceneManager 常驻缓存场景），所以这里只登记"怎么造"，不构造任何场景。
//
// 分层意义：这里是整个工程里唯一知道"有哪些场景"的地方。
// core 层的 Game 只拿到一个 SceneRegistry，不再认识任何具体场景类型 ——
// 改造前 core/game.cpp 反向 include 了 scene 层的 8 个头文件。
//
#include "core/container.h"

/// 登记全部场景。
///
/// 注册表从容器里取（键名 "scene_registry"），所以**必须在 registerCore()
/// 之后调用** —— 这里的解析是立即的，不是惰性的。
///
/// 每个场景工厂在真正被调用时才从容器解析自己的依赖。
void registerScenes(Container& container);
