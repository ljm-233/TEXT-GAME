#include "core/scene_registry.h"

#include "common/exceptions.h"
#include "core/scene.h"

#include <utility>

void SceneRegistry::add(SceneId id, Factory factory) {
    if (!factory)
        throw AppError(std::string("SceneRegistry: ") + sceneIdName(id) +
                       " 的工厂是空的");

    if (factories_.count(id) != 0)
        throw AppError(std::string("SceneRegistry: ") + sceneIdName(id) +
                       " 重复注册");

    factories_.emplace(id, std::move(factory));
}

std::unique_ptr<Scene> SceneRegistry::create(SceneId id) const {
    auto it = factories_.find(id);
    if (it == factories_.end())
        throw AppError(std::string("SceneRegistry: 未注册的场景 ") + sceneIdName(id));

    return it->second();
}

bool SceneRegistry::contains(SceneId id) const {
    return factories_.count(id) != 0;
}

std::vector<SceneId> SceneRegistry::ids() const {
    // std::map 按 key 有序，而 SceneId 是枚举 —— 所以这里天然是枚举顺序，
    // 输出稳定，不依赖注册的先后。
    std::vector<SceneId> result;
    result.reserve(factories_.size());
    for (const auto& entry : factories_)
        result.push_back(entry.first);
    return result;
}

std::string SceneRegistry::str() const {
    std::string out = "SceneRegistry(" + std::to_string(factories_.size()) +
                      " registered)";
    for (SceneId id : ids())
        out += std::string("\n    ") + sceneIdName(id);
    return out;
}
