#pragma once

// 场景 ID，用于在场景间切换
enum class SceneId {
    None,        // 不切换，保持当前
    Exit,        // 退出程序
    MainMenu,
    SaveSelect,
    Game
};