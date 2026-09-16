#pragma once
#include "scene_id.h"
#include "window.h"
#include <SFML/Window/Event.hpp>
#include <string>

class Scene {
public:
    virtual ~Scene() = default;

    // ===== 生命周期钩子 =====
    // onEnter:  场景首次进入（start / push / replace 后）
    // onExit:   场景被销毁（pop 后旧场景、replace 后旧场景、程序退出）
    // onPause:  场景被上层覆盖（push 时对旧场景调用）
    // onResume: 上层场景被弹出，本场景重新成为当前场景
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