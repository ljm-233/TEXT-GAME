#pragma once
#include <filesystem>
#include <set>
#include <string>
#include <vector>

// 全局成就系统（跨存档，单文件持久化）
class AchievementManager {
public:
    static AchievementManager& instance();

    struct Entry {
        const char* id;
        const char* nameKey;   // i18n key
        const char* descKey;   // i18n key
    };

    // 初始化（传入存档文件路径，例如 config/achievements.conf）
    void init(const std::filesystem::path& filePath);

    // 解锁（已解锁则忽略），解锁时弹通知
    void unlock(const std::string& id);

    bool isUnlocked(const std::string& id) const;

    // 所有成就条目（静态目录）
    static const std::vector<Entry>& catalogue();

    int unlockedCount() const;
    int totalCount() const;

    // 落盘
    void flush();

private:
    AchievementManager() = default;

    void save();

    std::set<std::string> unlocked_;
    std::filesystem::path path_;
};