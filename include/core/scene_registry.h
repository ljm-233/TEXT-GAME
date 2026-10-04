#pragma once
//
// 场景工厂注册表：SceneId -> 构造函数 的显式映射。
//
// 替代 Game::createScene 里的 switch，动机有两个：
//   1. 加一个场景原本要同时改 Game 的 switch 和 scene_id.h 的枚举；
//      现在只需要在 scene 层的 registerScenes() 里加一行。
//   2. 更要紧的一条：core 不再需要 include 任何一个具体场景的头文件。
//      "核心不知道有哪些前端"是这套架构的前提，而原来的 switch 让
//      core 反向依赖了 scene 层的 8 个头文件。
//
// 对应 albuswall ui/protocol.py 的 _REGISTRY / register_ui / get_ui 三件套。
// 唯一的语义差异：那边 get_ui 对未注册抛 RuntimeError，这边同样抛异常
// 而不是返回空 —— 理由见 create() 的说明。
//
#include "core/scene_id.h"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Scene;

class SceneRegistry {
public:
    using Factory = std::function<std::unique_ptr<Scene>()>;

    // ---------------- 注册 ----------------

    /// 登记一个场景工厂。重复注册同一个 id 抛 AppError（不做隐式覆盖）。
    void add(SceneId id, Factory factory);

    // ---------------- 解析 ----------------

    /// 按 id 构造场景。
    ///
    /// 未注册抛 AppError，而不是返回 nullptr：场景没注册属于装配错误，
    /// 静默返回空只会表现成一块黑屏，排查成本远高于直接报错。
    std::unique_ptr<Scene> create(SceneId id) const;

    // ---------------- 查询 ----------------

    bool contains(SceneId id) const;
    std::size_t size() const { return factories_.size(); }

    /// 已注册的 id，按枚举顺序。
    std::vector<SceneId> ids() const;

    /// 调试用快照，对应 albuswall 各处的 __str__。
    std::string str() const;

private:
    std::map<SceneId, Factory> factories_;
};
