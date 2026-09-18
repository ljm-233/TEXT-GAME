#pragma once
#include <string>
#include <vector>
#include <memory>
#include "runtime_config.h"
#include "logging.h"

struct SaveInfo {
    std::string filename;
    std::string name;
    std::string createdAt;
    std::string lastPlayed;
    int         coins        = 0;   // ⭐ 实际存的是金币数（旧名 progress）
    int         currentLevel = 1;
    std::vector<int> levelStars;   // 每关星级，索引 0 = 第 1 关，值 0~3

    // ⭐ 每关最佳通关时间（秒），0 = 无记录
    std::vector<float> levelBestTimes;
};

class SaveManager {
public:
    SaveManager(std::shared_ptr<RuntimeConfig> config,
                std::shared_ptr<Logger> logger);

    std::vector<SaveInfo> listSaves() const;
    SaveInfo createSave(const std::string& customName = "");
    bool loadSave(const std::string& filename, SaveInfo& out) const;
    bool deleteSave(const std::string& filename);
    bool updateProgress(const std::string& filename,
                        int coins,
                        int currentLevel = 1);

    // ⭐ 写入某关星级（取最高）
    bool setLevelStar(const std::string& filename, int level, int stars);

    // ⭐ 写入某关最佳时间（取最小，0 视为无记录）
    bool setLevelBestTime(const std::string& filename, int level, float seconds);

    void setPendingSave(const SaveInfo& info) { pending_ = info; }
    SaveInfo takePendingSave() { auto s = pending_; pending_ = {}; return s; }
    bool hasPendingSave() const { return !pending_.filename.empty(); }

private:
    std::string currentTimestamp() const;
    static std::string serializeStars(const std::vector<int>& stars);
    static std::vector<int> parseStars(const std::string& s);
    static std::string serializeTimes(const std::vector<float>& times);
    static std::vector<float> parseTimes(const std::string& s);

    std::shared_ptr<RuntimeConfig> config_;
    std::shared_ptr<Logger> logger_;
    SaveInfo pending_;
};