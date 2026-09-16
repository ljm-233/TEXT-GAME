#include "level_validator.h"
#include "level.h"
#include "vec2.h"
#include "game_constants.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <set>
#include <sstream>
#include <vector>

namespace {

// 平台：一组水平连续的"顶面"瓦片（上方为空）
struct Platform {
    int minX = 0;
    int maxX = 0;
    int y    = 0;

    // 两个平台之间的最小水平距离（瓦片为单位）
    // 若重叠或相邻，返回 0
    int horizDistTo(const Platform& other) const {
        if (other.minX > maxX) return other.minX - maxX;
        if (other.maxX < minX) return minX - other.maxX;
        return 0;
    }
};

// 扫描关卡，找出所有平台
std::vector<Platform> findPlatforms(const Level& level) {
    std::vector<Platform> result;
    const int w = level.width();
    const int h = level.height();

    for (int ty = 1; ty < h; ++ty) {   // 跳过 y=0（顶部墙顶不算平台）
        int x = 0;
        while (x < w) {
            bool isTop = level.isSolid(x, ty) && !level.isSolid(x, ty - 1);
            if (!isTop) { ++x; continue; }

            const int startX = x;
            while (x < w && level.isSolid(x, ty) && !level.isSolid(x, ty - 1)) {
                ++x;
            }
            Platform p;
            p.minX = startX;
            p.maxX = x - 1;
            p.y    = ty;
            result.push_back(p);
        }
    }
    return result;
}

// 从 (tx, minY) 往下找最近的平台（返回索引，-1 表示没有）
int findPlatformBelow(const std::vector<Platform>& platforms, int tx, int minY) {
    int bestIdx = -1;
    int bestY = 1000000;
    for (size_t i = 0; i < platforms.size(); ++i) {
        const auto& p = platforms[i];
        if (p.y < minY) continue;
        if (tx < p.minX || tx > p.maxX) continue;
        if (p.y < bestY) {
            bestY = p.y;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

// 判断 from 平台能否跳到 to 平台（忽略弹跳板、移动平台）
bool canReach(const Platform& from, const Platform& to, int tileSize) {
    // 用瓦片为单位计算物理量
    const float g          = GameConst::kPlayerGravity     / tileSize;   // 格/s²
    const float v          = -GameConst::kPlayerJumpVelocity / tileSize; // 格/s
    const float moveSpeed  = GameConst::kPlayerMoveSpeed   / tileSize;   // 格/s
    const float maxFall    = GameConst::kPlayerMaxFall     / tileSize;   // 格/s

    const float maxJumpH = v * v / (2.f * g);   // 最大跳跃高度（格）
    const float upTime   = v / g;               // 上升时间（秒）

    // 达到最大下落速度所需高度和对应时间
    const float h1 = maxFall * maxFall / (2.f * g);
    const float t1 = maxFall / g;

    const float dy = static_cast<float>(from.y - to.y);   // 正 = to 更高
    const float dx = static_cast<float>(from.horizDistTo(to));

    if (dy >= 0.f) {
        // to 更高或同高：需要跳
        if (dy > maxJumpH) return false;

        const float fallHeight = std::max(0.f, maxJumpH - dy);
        float fallTime;
        if (fallHeight <= h1) {
            fallTime = std::sqrt(2.f * fallHeight / g);
        } else {
            fallTime = t1 + (fallHeight - h1) / maxFall;
        }
        const float maxDx = moveSpeed * (upTime + fallTime);
        return dx <= maxDx;
    } else {
        // to 更低：直接走下去或从边缘掉落
        const float fallDist = -dy;
        float fallTime;
        if (fallDist <= h1) {
            fallTime = std::sqrt(2.f * fallDist / g);
        } else {
            fallTime = t1 + (fallDist - h1) / maxFall;
        }
        const float maxDx = moveSpeed * fallTime;
        return dx <= maxDx;
    }
}

// 从起始平台出发，BFS 找所有可达平台
std::set<int> findReachablePlatforms(const std::vector<Platform>& platforms,
                                     int startIdx, int tileSize) {
    std::set<int> reachable;
    if (startIdx < 0) return reachable;

    std::deque<int> queue;
    queue.push_back(startIdx);
    reachable.insert(startIdx);

    while (!queue.empty()) {
        int cur = queue.front();
        queue.pop_front();

        for (size_t i = 0; i < platforms.size(); ++i) {
            const int idx = static_cast<int>(i);
            if (reachable.count(idx)) continue;
            if (canReach(platforms[cur], platforms[i], tileSize)) {
                reachable.insert(idx);
                queue.push_back(idx);
            }
        }
    }
    return reachable;
}

} // namespace

// ============================================================
// LevelValidator
// ============================================================

ValidationReport LevelValidator::validate(const Level& level) {
    ValidationReport report;
    const int ts = level.tileSize();

    const std::vector<Platform> platforms = findPlatforms(level);
    report.totalPlatforms = static_cast<int>(platforms.size());

    // 出生点所在平台
    const Vec2 spawn = level.playerSpawn();
    const int spawnTileX = static_cast<int>(spawn.x) / ts;
    const int spawnTileY = static_cast<int>(spawn.y) / ts;

    const int spawnPlatformIdx =
        findPlatformBelow(platforms, spawnTileX, spawnTileY);
    if (spawnPlatformIdx < 0) {
        report.spawnValid = false;
        return report;
    }
    report.spawnValid = true;

    const std::set<int> reachable =
        findReachablePlatforms(platforms, spawnPlatformIdx, ts);
    report.reachablePlatforms = static_cast<int>(reachable.size());

    // 检查所有 spawn 元素
    auto checkSpawn = [&](const std::vector<Vec2>& spawns, const char* kindName) {
        for (const auto& s : spawns) {
            const int tx = static_cast<int>(s.x) / ts;
            const int ty = static_cast<int>(s.y) / ts;
            const int pIdx = findPlatformBelow(platforms, tx, ty);
            if (pIdx < 0 || !reachable.count(pIdx)) {
                report.unreachable.push_back({kindName, tx, ty});
            }
        }
    };

    checkSpawn(level.coinSpawns(),       "coin");
    checkSpawn(level.enemySpawns(),      "enemy");
    checkSpawn(level.keySpawns(),        "key");
    checkSpawn(level.doorSpawns(),       "door");
    checkSpawn(level.checkpointSpawns(), "checkpoint");
    checkSpawn(level.jumpPadSpawns(),    "jumppad");

    // 终点
    if (level.hasGoal()) {
        const Vec2 goal = level.goalPos();
        const int gx = static_cast<int>(goal.x) / ts;
        const int gy = static_cast<int>(goal.y) / ts;
        const int pIdx = findPlatformBelow(platforms, gx, gy);
        if (pIdx >= 0 && reachable.count(pIdx)) {
            report.goalReachable = true;
        } else {
            report.unreachable.push_back({"goal", gx, gy});
        }
    } else {
        report.goalReachable = false;
    }

    return report;
}

// ============================================================
// ValidationReport::summary
// ============================================================

std::string ValidationReport::summary() const {
    std::ostringstream oss;

    if (ok()) {
        oss << "OK (" << reachablePlatforms << "/" << totalPlatforms
            << " platforms reachable)";
        return oss.str();
    }

    oss << "FAIL";
    if (!spawnValid)          oss << " | spawn is floating";
    if (!goalReachable)       oss << " | goal unreachable";
    oss << " | platforms " << reachablePlatforms << "/" << totalPlatforms
        << " reachable";

    if (!unreachable.empty()) {
        oss << "\n  " << unreachable.size() << " unreachable items:";
        for (const auto& u : unreachable) {
            oss << "\n    - " << u.kind << " @ (" << u.tileX << "," << u.tileY << ")";
        }
    }
    return oss.str();
}