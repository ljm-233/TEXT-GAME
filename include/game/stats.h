#pragma once
#include "save_manager.h"

#include <vector>

//
// 成绩/统计页要显示的东西：把存档里的原始数组整理成"每关一行 + 总计"。
//
// 抽成纯函数的理由与 ScoreRules / EditorTools 一样：成绩页是个 Scene，
// 构造就要 sf::Font（GlResource），无头环境连构造都做不到。
// 而"某关没打过时那一行该显示什么""存档里数组长度不齐时怎么办"
// 恰恰是最容易写错、也最该被测住的部分。
//
namespace Stats {

/// 一关的成绩。字段全部是可以直接显示的原值：
/// 没有记录时 bestTime 是 0、stars 是 0 —— 由**界面**决定显示成"--"还是"未通关"。
struct LevelRow {
    int   level     = 1;
    int   stars     = 0;
    float bestTime  = 0.f;  ///< 秒；0 = 无记录
    int   bestCoins = 0;    ///< 单次最多拿到过多少；0 = 无记录
    bool  cleared   = false; ///< stars > 0 即视为通关过
};

struct Summary {
    std::vector<LevelRow> levels;

    int clearedLevels  = 0;    ///< 通关过的关数
    int totalStars     = 0;    ///< 星级之和
    int maxStars       = 0;    ///< 满星上限（3 × 关数）
    int totalBestCoins = 0;    ///< 每关最佳金币之和（只统计有记录的）
    float totalBestTime = 0.f; ///< 每关最佳时间之和（只统计有记录的）
    bool anyRecord     = false; ///< 一条记录都没有（新存档）

    /// 通关进度 0~1（界面拿去画进度条）。没有关卡时返回 0。
    float progress() const;
};

/// 从存档整理出统计结果。
///
/// ⚠️ **容忍长度不齐**：老存档可能只有 levelStars 而没有 levelBestCoins
/// （那个字段是新加的），手改坏的存档也可能短一截。取三个数组长度的最大值
/// 作为关数，缺的按 0 处理 —— 而不是去读越界或者干脆少显示几关。
Summary summarize(const SaveInfo& save);

/// 关数：三个数组长度的最大值（至少 0）
int levelCount(const SaveInfo& save);

} // namespace Stats
