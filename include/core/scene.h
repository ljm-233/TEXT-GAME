#pragma once
#include "scene_id.h"
#include "window.h"
#include <SFML/Window/Event.hpp>

class Scene {
public:
    virtual ~Scene() = default;

    virtual void handleEvent(const sf::Event& /*event*/) {}
    virtual void update(float /*dt*/) {}
    virtual void render(Window& window) = 0;

    // 返回要切换到的下一个场景；SceneId::None 表示保持当前场景
    virtual SceneId nextScene() const { return SceneId::None; }
};