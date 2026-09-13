#pragma once
#include <SFML/Window/Event.hpp>
#include "window.h"

// 场景基类：后续 SettingsScene / PauseScene 都继承它
class Scene {
public:
    virtual ~Scene() = default;

    virtual void handleEvent(const sf::Event& /*event*/) {}
    virtual void update(float /*dt*/) {}
    virtual void render(Window& window) = 0;

    // 返回 true 表示场景结束，可以切换到下一个场景
    virtual bool isFinished() const { return false; }
};