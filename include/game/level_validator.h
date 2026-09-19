#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>

class Level;

struct ValidationReport {
    struct UnreachableItem {
        std::string kind;   // "coin" / "enemy" / "goal" / ...
        int tileX = 0;
        int tileY = 0;
    };

    bool spawnValid = false;         // 出生点下方是否有平台
    bool goalReachable = false;      // 终点是否可达
    std::vector<UnreachableItem> unreachable;

    int reachablePlatforms = 0;
    int totalPlatforms = 0;

    // ⭐ 可达性可视化数据
    std::set<std::pair<int,int>> allPlatformTops;       // 所有平台顶面 tile
    std::set<std::pair<int,int>> reachablePlatformTops; // 从出生点可达的平台顶面
    std::set<std::pair<int,int>> reachableSpawns;       // 可达的 spawn tile
    std::pair<int,int> spawnTile{-1, -1};               // 出生点 tile

    bool ok() const {
        return spawnValid && goalReachable && unreachable.empty();
    }

    std::string summary() const;
};

class LevelValidator {
public:
    // 静态入口：给定 Level，返回可达性报告
    static ValidationReport validate(const Level& level);

    LevelValidator() = delete;
};