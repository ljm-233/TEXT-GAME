#pragma once
#include "scene.h"
#include "scene_id.h"
#include <functional>
#include <memory>
#include <vector>

// 场景管理器：负责场景栈的推入/弹出/替换
// 不处理 Exit（由 Game 检测并关闭窗口），不处理 Back（由本类弹栈）
class SceneManager {
public:
    using SceneFactory = std::function<std::unique_ptr<Scene>(SceneId)>;

    explicit SceneManager(SceneFactory factory);

    // 初始化：设置初始场景（清空历史）
    bool start(SceneId id);

    // 进入新场景：当前场景压入历史
    bool push(SceneId id);

    // 返回上一场景：当前场景销毁，从历史弹出并创建
    bool pop();

    // 替换当前场景（不入历史）
    bool replace(SceneId id);

    Scene& current();
    SceneId currentId() const { return currentId_; }
    bool empty() const { return current_ == nullptr; }
    size_t historySize() const { return history_.size(); }

private:
    SceneFactory factory_;
    std::vector<SceneId> history_;
    std::unique_ptr<Scene> current_;
    SceneId currentId_ = SceneId::None;
};