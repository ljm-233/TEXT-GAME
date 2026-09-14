#include "save_manager.h"
#include "strings.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

SaveManager::SaveManager(std::shared_ptr<RuntimeConfig> config,
                         std::shared_ptr<Logger> logger)
      : config_(std::move(config)),
        logger_(std::move(logger)) {}

std::string SaveManager::currentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

std::vector<SaveInfo> SaveManager::listSaves() const {
    std::vector<SaveInfo> result;
    auto dir = config_->savesDir();
    if (!fs::exists(dir))
        return result;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file())
            continue;
        auto path = entry.path();
        if (path.extension() != ".conf")
            continue;

        SaveInfo info;
        if (loadSave(path.filename().string(), info))
            result.push_back(info);
    }

    std::sort(result.begin(), result.end(), [](const SaveInfo& a, const SaveInfo& b) {
        return a.lastPlayed > b.lastPlayed;
    });
    return result;
}

SaveInfo SaveManager::createSave(const std::string& customName) {
    auto now = currentTimestamp();
    auto ts = std::chrono::system_clock::now().time_since_epoch().count();
    std::string filename = "save_" + std::to_string(ts) + ".conf";

    SaveInfo info;
    info.filename = filename;
    info.name =
        customName.empty() ? (std::string(Str::SaveNamePrefix) + now) : customName;
    info.createdAt = now;
    info.lastPlayed = now;
    info.progress = 0;
    info.currentLevel = 1;

    auto path = config_->saveFile(filename);
    std::ofstream out(path);
    if (out) {
        out << "name=" << info.name << '\n';
        out << "created_at=" << info.createdAt << '\n';
        out << "last_played=" << info.lastPlayed << '\n';
        out << "progress=" << info.progress << '\n';
        out << "current_level=" << info.currentLevel << '\n';
    }

    logger_->info("创建存档: " + path.string());
    return info;
}

bool SaveManager::loadSave(const std::string& filename, SaveInfo& out) const {
    auto path = config_->saveFile(filename);
    std::ifstream in(path);
    if (!in)
        return false;

    out.filename = filename;
    out.name = Str::UnnamedSave;
    out.createdAt = "";
    out.lastPlayed = "";
    out.progress = 0;
    out.currentLevel = 1;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;
        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 1);
        if (k == "name")
            out.name = v;
        else if (k == "created_at")
            out.createdAt = v;
        else if (k == "last_played")
            out.lastPlayed = v;
        else if (k == "progress") {
            try {
                out.progress = std::stoi(v);
            } catch (...) {
                out.progress = 0;
            }
        } else if (k == "current_level") {
            try {
                out.currentLevel = std::stoi(v);
            } catch (...) {
                out.currentLevel = 1;
            }
        }
    }
    return true;
}

bool SaveManager::deleteSave(const std::string& filename) {
    auto path = config_->saveFile(filename);
    if (!fs::exists(path))
        return false;
    std::error_code ec;
    fs::remove(path, ec);
    if (ec) {
        logger_->error("删除存档失败: " + path.string() + " " + ec.message());
        return false;
    }
    logger_->info("删除存档: " + path.string());
    return true;
}

bool SaveManager::updateProgress(const std::string& filename, int progress,
                                 int currentLevel) {
    SaveInfo info;
    if (!loadSave(filename, info))
        return false;
    info.progress = std::max(info.progress, progress);
    info.currentLevel = std::max(info.currentLevel, currentLevel);
    info.lastPlayed = currentTimestamp();

    auto path = config_->saveFile(filename);
    std::ofstream out(path);
    if (!out)
        return false;
    out << "name=" << info.name << '\n';
    out << "created_at=" << info.createdAt << '\n';
    out << "last_played=" << info.lastPlayed << '\n';
    out << "progress=" << info.progress << '\n';
    out << "current_level=" << info.currentLevel << '\n';

    logger_->info("存档进度更新: " + filename +
                  " progress=" + std::to_string(info.progress) +
                  " level=" + std::to_string(info.currentLevel));
    return true;
}