#include "stats.h"

#include <algorithm>

namespace Stats {

float Summary::progress() const {
    if (levels.empty())
        return 0.f;
    return static_cast<float>(clearedLevels) / static_cast<float>(levels.size());
}

int levelCount(const SaveInfo& save) {
    return static_cast<int>(std::max({save.levelStars.size(), save.levelBestTimes.size(),
                                      save.levelBestCoins.size()}));
}

Summary summarize(const SaveInfo& save) {
    Summary out;

    const int n = levelCount(save);
    out.levels.reserve(static_cast<std::size_t>(n));
    out.maxStars = n * 3;

    for (int i = 0; i < n; ++i) {
        const auto idx = static_cast<std::size_t>(i);

        LevelRow row;
        row.level = i + 1;
        // 三个数组都可能比 i 短（老存档 / 手改坏的存档），逐个查长度
        if (idx < save.levelStars.size())
            row.stars = save.levelStars[idx];
        if (idx < save.levelBestTimes.size())
            row.bestTime = save.levelBestTimes[idx];
        if (idx < save.levelBestCoins.size())
            row.bestCoins = save.levelBestCoins[idx];

        // 星级越界（手改存档）夹回 0~3，否则进度条会算出 >100%
        row.stars = std::clamp(row.stars, 0, 3);
        row.cleared = row.stars > 0;

        if (row.cleared)
            ++out.clearedLevels;
        out.totalStars += row.stars;
        // 只累加**有记录**的：没打过的那几关是 0，加进去等于把平均值拉低
        if (row.bestTime > 0.f)
            out.totalBestTime += row.bestTime;
        if (row.bestCoins > 0)
            out.totalBestCoins += row.bestCoins;

        if (row.cleared || row.bestTime > 0.f || row.bestCoins > 0)
            out.anyRecord = true;

        out.levels.push_back(row);
    }

    return out;
}

} // namespace Stats
