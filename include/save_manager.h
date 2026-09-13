#pragma once
#include <string>
#include <vector>
#include <memory>
#include "config.h"
#include "logging.h"

struct SaveInfo {
    std::string filename;    // 磁盘文件名，如 save_123.conf
    std::string name;        // 显示名，如 "存档 2026-09-13 10:00:00"
    std::string createdAt;
    std::string lastPlayed;
};

class SaveManager {
public:
    SaveManager(std::shared_ptr<Config> config,
                std::shared_ptr<Logger> logger);

    // 列出所有存档（按 lastPlayed 倒序）
    std::vector<SaveInfo> listSaves() const;

    // 创建新存档，返回其信息
    SaveInfo createSave();

    // 加载指定存档的元数据
    bool loadSave(const std::string& filename, SaveInfo& out) const;

    // 待进入游戏的存档（由存档选择场景设置，游戏场景读取）
    void setPendingSave(const SaveInfo& info) { pending_ = info; }
    SaveInfo takePendingSave() { auto s = pending_; pending_ = {}; return s; }
    bool hasPendingSave() const { return !pending_.filename.empty(); }

private:
    std::string currentTimestamp() const;

    std::shared_ptr<Config> config_;
    std::shared_ptr<Logger> logger_;
    SaveInfo pending_;
};