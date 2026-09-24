#include "achievement.h"
#include "notification.h"
#include "text_strings.h"

#include <fstream>

namespace {

// ⭐ 成就目录：新增成就只需在这里加一行 + text_strings.h 加两条字符串
const std::vector<AchievementManager::Entry> kAchievements = {
    {"first_coin",    Str::AchvFirstCoin,    Str::AchvFirstCoinDesc},
    {"first_level",   Str::AchvFirstLevel,   Str::AchvFirstLevelDesc},
    {"all_levels",    Str::AchvAllLevels,    Str::AchvAllLevelsDesc},
    {"perfect_level", Str::AchvPerfectLevel, Str::AchvPerfectLevelDesc},
    {"no_damage",     Str::AchvNoDamage,     Str::AchvNoDamageDesc},
    {"speedrun",      Str::AchvSpeedrun,     Str::AchvSpeedrunDesc},
    {"three_stars",   Str::AchvThreeStars,   Str::AchvThreeStarsDesc},
    {"all_stars",     Str::AchvAllStars,     Str::AchvAllStarsDesc},
    {"editor_used",   Str::AchvEditorUsed,   Str::AchvEditorUsedDesc},
    {"editor_export", Str::AchvEditorExport, Str::AchvEditorExportDesc},
    {"console_used",  Str::AchvConsoleUsed,  Str::AchvConsoleUsedDesc},
    {"debug_mode",    Str::AchvDebugMode,    Str::AchvDebugModeDesc},
};

} // namespace

AchievementManager& AchievementManager::instance() {
    static AchievementManager inst;
    return inst;
}

const std::vector<AchievementManager::Entry>& AchievementManager::catalogue() {
    return kAchievements;
}

void AchievementManager::init(const std::filesystem::path& filePath) {
    path_ = filePath;
    unlocked_.clear();

    std::ifstream in(path_);
    if (!in) return;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        unlocked_.insert(line);
    }
}

void AchievementManager::unlock(const std::string& id) {
    if (unlocked_.count(id)) return;
    unlocked_.insert(id);

    // 查找名称用于通知
    std::string name = id;
    for (const auto& e : kAchievements) {
        if (id == e.id) {
            name = Str::T(e.nameKey);
            break;
        }
    }

    std::string msg = std::string(Str::T(Str::AchvUnlockedPrefix)) + name;
    NotificationSystem::instance().push(msg, NotificationType::Success, 5.f);

    save();
}

bool AchievementManager::isUnlocked(const std::string& id) const {
    return unlocked_.count(id) > 0;
}

int AchievementManager::unlockedCount() const {
    return static_cast<int>(unlocked_.size());
}

int AchievementManager::totalCount() const {
    return static_cast<int>(kAchievements.size());
}

void AchievementManager::save() {
    if (path_.empty()) return;
    std::ofstream out(path_);
    if (!out) return;
    for (const auto& id : unlocked_) {
        out << id << '\n';
    }
    out.flush();
}

void AchievementManager::flush() {
    save();
}