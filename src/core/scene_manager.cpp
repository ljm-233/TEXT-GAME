#include "scene_manager.h"

SceneManager::SceneManager(SceneFactory factory)
      : factory_(std::move(factory)) {}

SceneManager::~SceneManager() {
    // ⭐ 程序退出时统一 onExit
    for (auto& [id, scene] : cache_) {
        if (scene) scene->onExit();
    }
}

Scene* SceneManager::getOrCreate(SceneId id) {
    auto it = cache_.find(id);
    if (it != cache_.end()) return it->second.get();

    auto scene = factory_(id);
    if (!scene) return nullptr;

    Scene* ptr = scene.get();
    cache_[id] = std::move(scene);
    return ptr;
}

bool SceneManager::start(SceneId id) {
    if (current_) current_->onPause();
    history_.clear();

    Scene* scene = getOrCreate(id);
    if (!scene) return false;

    current_ = scene;
    currentId_ = id;
    current_->onEnter();
    return true;
}

bool SceneManager::push(SceneId id) {
    if (!current_) return start(id);

    Scene* scene = getOrCreate(id);
    if (!scene) return false;

    current_->onPause();
    history_.push_back({currentId_, current_});

    current_ = scene;
    currentId_ = id;
    current_->onEnter();
    return true;
}

bool SceneManager::pop() {
    if (history_.empty()) return false;

    if (current_) current_->onPause();

    auto entry = history_.back();
    history_.pop_back();

    current_ = entry.scene;
    currentId_ = entry.id;
    current_->onResume();
    return true;
}

bool SceneManager::replace(SceneId id) {
    Scene* scene = getOrCreate(id);
    if (!scene) return false;

    if (current_) current_->onPause();

    current_ = scene;
    currentId_ = id;
    current_->onEnter();
    return true;
}

Scene& SceneManager::current() {
    return *current_;
}