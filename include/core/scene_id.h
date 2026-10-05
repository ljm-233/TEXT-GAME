#pragma once
//
// 场景标识。
//
// 前三个是**控制值**，不是真正的场景，永远不会被注册进 SceneRegistry：
//   None —— "本帧不切场景"的哨兵
//   Back —— "弹出当前场景"的指令
//   Exit —— "退出程序"的指令
// 它们由 Game::switchScene 直接处理，不会走到场景工厂。
//
enum class SceneId {
    None,
    Back,
    Exit,
    MainMenu,
    SaveSelect,
    Game,
    Settings,
    Console,
    LevelSelect,
    Editor,
    Achievements,
    Stats
};

/// 稳定可读的名字。用于日志、错误消息与调试快照 ——
/// 直接打印枚举会得到数字，排查时还得回来对着枚举表数。
const char* sceneIdName(SceneId id);
