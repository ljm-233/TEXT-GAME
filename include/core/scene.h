#pragma once
#include "scene_id.h"
#include "window.h"
#include <SFML/Window/Event.hpp>
#include <string>

class Scene {
public:
    virtual ~Scene() = default;

    // ===== 生命周期钩子 =====
    // 场景是常驻的（SceneManager 只构造一次，之后复用），所以：
    // onEnter:  每次成为当前场景都调用（start / push / replace 后）——
    //           状态必须能在这里重置
    // onPause:  本场景被离开时调用（start 换掉当前 / push 到新场景 /
    //           pop 回上一层 / replace）。**离开场景的清理写这里**
    // onResume: 只在 pop 回到本场景时调用
    // onExit:   只在程序退出（~SceneManager）时调用 —— 不是"离开场景"。
    //           pop / replace 不会调它，别把清理只挂在这里
    virtual void onEnter()  {}
    virtual void onExit()   {}
    virtual void onPause()  {}
    virtual void onResume() {}

    virtual void handleEvent(const sf::Event& /*event*/) {}
    virtual void update(float /*dt*/) {}
    virtual void render(Window& window) = 0;

    virtual SceneId nextScene() const { return SceneId::None; }

    // 窗口标题后缀（返回空 = 使用默认 "TEXT-GAME"）
    virtual std::string windowTitleHint() const { return {}; }
};