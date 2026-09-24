#pragma once
#include "scene.h"
#include "scene_id.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

// 场景管理器：场景常驻，避免 sf::Text 频繁析构触发 SFML 3.1.0 死锁
class SceneManager {
public:
    using SceneFactory = std::function<std::unique_ptr<Scene>(SceneId)>;

    explicit SceneManager(SceneFactory factory);
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    bool start(SceneId id);
    bool push(SceneId id);
    bool pop();
    bool replace(SceneId id);

    Scene& current();
    SceneId currentId() const { return currentId_; }
    bool empty() const { return current_ == nullptr; }
    size_t historySize() const { return history_.size(); }

private:
    struct HistoryEntry {
        SceneId id;
        Scene*  scene = nullptr;   // ⭐ 不持有所有权
    };

    Scene* getOrCreate(SceneId id);

    SceneFactory factory_;
    std::vector<HistoryEntry> history_;
    std::map<SceneId, std::unique_ptr<Scene>> cache_;   // ⭐ 场景常驻
    Scene*  current_ = nullptr;
    SceneId currentId_ = SceneId::None;
};