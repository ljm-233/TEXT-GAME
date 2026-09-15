#pragma once
#include "scene.h"
#include "scene_id.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

// 场景管理器：负责场景栈的推入/弹出/替换
// - push 时旧场景保留在 history_，不会销毁
// - pop 时从 history_ 取回旧场景，状态保留
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
        std::unique_ptr<Scene> scene;
    };

    SceneFactory factory_;
    std::vector<HistoryEntry> history_;
    std::unique_ptr<Scene> current_;
    SceneId currentId_ = SceneId::None;
};