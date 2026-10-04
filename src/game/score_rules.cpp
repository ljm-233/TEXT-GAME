#include "game/score_rules.h"

namespace ScoreRules {

int targetTimeSeconds(int totalCoins) {
    if (totalCoins < 0)
        totalCoins = 0;
    return static_cast<int>(kBaseTime + static_cast<float>(totalCoins) * kPerCoinTime);
}

int starsFor(int coins, int totalCoins, float levelTime) {
    const bool allCoins = (coins == totalCoins);
    const bool fastEnough =
        (levelTime <= static_cast<float>(targetTimeSeconds(totalCoins)));

    if (allCoins && fastEnough) return 3;
    if (allCoins || fastEnough) return 2;
    return 1;
}

bool isNewRecord(float previousBest, float levelTime) {
    return previousBest <= 0.f || levelTime < previousBest;
}

} // namespace ScoreRules
