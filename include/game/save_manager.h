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
    int         progress = 0;   // 0~100 或关卡数，看你怎么用
};

class SaveManager {
public:
    SaveManager(std::shared_ptr<RuntimeConfig> config,
                std::shared_ptr<Logger> logger);

    std::vector<SaveInfo> listSaves() const;
    SaveInfo createSave(const std::string& customName = "");
    bool loadSave(const std::string& filename, SaveInfo& out) const;
    bool deleteSave(const std::string& filename);
    bool updateProgress(const std::string& filename, int progress);

    void setPendingSave(const SaveInfo& info) { pending_ = info; }
    SaveInfo takePendingSave() { auto s = pending_; pending_ = {}; return s; }
    bool hasPendingSave() const { return !pending_.filename.empty(); }

private:
    std::string currentTimestamp() const;

    std::shared_ptr<RuntimeConfig> config_;
    std::shared_ptr<Logger> logger_;
    SaveInfo pending_;
};