#include "scene_manager.h"

SceneManager::SceneManager(SceneFactory factory)
    : factory_(std::move(factory)) {}

bool SceneManager::start(SceneId id) {
    auto scene = factory_(id);
    if (!scene) return false;
    history_.clear();
    current_ = std::move(scene);
    currentId_ = id;
    return true;
}

bool SceneManager::push(SceneId id) {
    if (!current_) return start(id);
    auto scene = factory_(id);
    if (!scene) return false;
    history_.push_back(currentId_);
    current_ = std::move(scene);
    currentId_ = id;
    return true;
}

bool SceneManager::pop() {
    if (history_.empty()) return false;
    SceneId prev = history_.back();
    history_.pop_back();

    auto scene = factory_(prev);
    if (!scene) return false;
    current_ = std::move(scene);
    currentId_ = prev;
    return true;
}

bool SceneManager::replace(SceneId id) {
    auto scene = factory_(id);
    if (!scene) return false;
    current_ = std::move(scene);
    currentId_ = id;
    return true;
}

Scene& SceneManager::current() {
    return *current_;
}