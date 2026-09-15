#include "scene_manager.h"

SceneManager::SceneManager(SceneFactory factory)
      : factory_(std::move(factory)) {}

SceneManager::~SceneManager() {
    if (current_)
        current_->onExit();
    for (auto& entry : history_) {
        if (entry.scene)
            entry.scene->onExit();
    }
}

bool SceneManager::start(SceneId id) {
    // 清理现有场景
    if (current_)
        current_->onExit();
    for (auto& entry : history_) {
        if (entry.scene)
            entry.scene->onExit();
    }
    history_.clear();
    current_.reset();

    auto scene = factory_(id);
    if (!scene)
        return false;

    current_ = std::move(scene);
    currentId_ = id;
    current_->onEnter();
    return true;
}

bool SceneManager::push(SceneId id) {
    if (!current_)
        return start(id);

    auto scene = factory_(id);
    if (!scene)
        return false;

    // 暂停当前场景，保留在历史栈
    current_->onPause();
    history_.push_back({currentId_, std::move(current_)});

    current_ = std::move(scene);
    currentId_ = id;
    current_->onEnter();
    return true;
}

bool SceneManager::pop() {
    if (history_.empty())
        return false;

    // 销毁当前场景
    current_->onExit();

    auto entry = std::move(history_.back());
    history_.pop_back();

    current_ = std::move(entry.scene);
    currentId_ = entry.id;
    current_->onResume();
    return true;
}

bool SceneManager::replace(SceneId id) {
    auto scene = factory_(id);
    if (!scene)
        return false;

    if (current_)
        current_->onExit();

    current_ = std::move(scene);
    currentId_ = id;
    current_->onEnter();
    return true;
}

Scene& SceneManager::current() {
    return *current_;
}