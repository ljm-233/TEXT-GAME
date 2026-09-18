#include "save_manager.h"
#include "text_strings.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <cstdio>

namespace fs = std::filesystem;

SaveManager::SaveManager(std::shared_ptr<RuntimeConfig> config,
                         std::shared_ptr<Logger> logger)
    : config_(std::move(config)), logger_(std::move(logger)) {}

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

std::string SaveManager::serializeStars(const std::vector<int>& stars) {
    std::string out;
    for (size_t i = 0; i < stars.size(); ++i) {
        if (i > 0) out += ',';
        out += std::to_string(stars[i]);
    }
    return out;
}

std::vector<int> SaveManager::parseStars(const std::string& s) {
    std::vector<int> out;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ',')) {
        try { out.push_back(std::stoi(token)); }
        catch (...) { out.push_back(0); }
    }
    return out;
}

std::string SaveManager::serializeTimes(const std::vector<float>& times) {
    std::string out;
    for (std::size_t i = 0; i < times.size(); ++i) {
        if (i > 0) out += ',';
        // 用 snprintf 控制小数位，避免 to_string 产生 6 位小数
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", times[i]);
        out += buf;
    }
    return out;
}

std::vector<float> SaveManager::parseTimes(const std::string& s) {
    std::vector<float> out;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ',')) {
        try { out.push_back(std::stof(token)); }
        catch (...) { out.push_back(0.f); }
    }
    return out;
}

std::vector<SaveInfo> SaveManager::listSaves() const {
    std::vector<SaveInfo> result;
    auto dir = config_->savesDir();
    if (!fs::exists(dir)) return result;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        if (path.extension() != ".conf") continue;

        SaveInfo info;
        if (loadSave(path.filename().string(), info))
            result.push_back(info);
    }

    std::sort(result.begin(), result.end(),
              [](const SaveInfo& a, const SaveInfo& b) {
                  return a.lastPlayed > b.lastPlayed;
              });
    return result;
}

SaveInfo SaveManager::createSave(const std::string& customName) {
    auto now = currentTimestamp();
    auto ts = std::chrono::system_clock::now().time_since_epoch().count();
    std::string filename = "save_" + std::to_string(ts) + ".conf";

    SaveInfo info;
    info.filename   = filename;
    info.name       = customName.empty()
                        ? (std::string(Str::SaveNamePrefix) + now)
                        : customName;
    info.createdAt  = now;
    info.lastPlayed = now;
    info.coins        = 0;
    info.currentLevel = 1;
    info.levelStars   = std::vector<int>(9, 0);
    info.levelBestTimes = std::vector<float>(9, 0.f);

    auto path = config_->saveFile(filename);
    std::ofstream out(path);
    if (!out) {
        logger_->error("创建存档失败: 无法写入 " + path.string());
        return {};
    }
    out << "name="             << info.name         << '\n';
    out << "created_at="       << info.createdAt    << '\n';
    out << "last_played="      << info.lastPlayed   << '\n';
    out << "coins="            << info.coins        << '\n';
    out << "current_level="    << info.currentLevel << '\n';
    out << "level_stars="      << serializeStars(info.levelStars) << '\n';
    out << "level_best_times=" << serializeTimes(info.levelBestTimes) << '\n';
    out.flush();
    if (!out) {
        logger_->error("创建存档失败: 写入过程中断 " + path.string());
        return {};
    }

    logger_->info("创建存档: " + path.string());
    return info;
}

bool SaveManager::loadSave(const std::string& filename, SaveInfo& out) const {
    auto path = config_->saveFile(filename);
    std::ifstream in(path);
    if (!in) return false;

    out.filename   = filename;
    out.name       = Str::UnnamedSave;
    out.createdAt  = "";
    out.lastPlayed = "";
    out.coins        = 0;
    out.currentLevel = 1;
    out.levelStars   = std::vector<int>(9, 0);
    out.levelBestTimes = std::vector<float>(9, 0.f);

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 1);
        if      (k == "name")        out.name       = v;
        else if (k == "created_at")  out.createdAt  = v;
        else if (k == "last_played") out.lastPlayed = v;
        // ⭐ 兼容旧字段名 progress= 和新字段名 coins=
        else if (k == "coins" || k == "progress") {
            try { out.coins = std::stoi(v); } catch (...) { out.coins = 0; }
        }
        else if (k == "current_level") {
            try { out.currentLevel = std::stoi(v); } catch (...) { out.currentLevel = 1; }
        }
        else if (k == "level_stars") {
            out.levelStars = parseStars(v);
            if (out.levelStars.size() < 9)
                out.levelStars.resize(9, 0);
        }
        else if (k == "level_best_times") {
            out.levelBestTimes = parseTimes(v);
            if (out.levelBestTimes.size() < 9)
                out.levelBestTimes.resize(9, 0.f);
        }
    }
    return true;
}

bool SaveManager::deleteSave(const std::string& filename) {
    auto path = config_->saveFile(filename);
    if (!fs::exists(path)) return false;
    std::error_code ec;
    fs::remove(path, ec);
    if (ec) {
        logger_->error("删除存档失败: " + path.string() + " " + ec.message());
        return false;
    }
    logger_->info("删除存档: " + path.string());
    return true;
}

bool SaveManager::updateProgress(const std::string& filename,
                                 int coins,
                                 int currentLevel) {
    SaveInfo info;
    if (!loadSave(filename, info)) return false;
    info.coins        = std::max(info.coins, coins);
    info.currentLevel = std::max(info.currentLevel, currentLevel);
    info.lastPlayed   = currentTimestamp();

    auto path = config_->saveFile(filename);
    std::ofstream out(path);
    if (!out) return false;
    out << "name="             << info.name         << '\n';
    out << "created_at="       << info.createdAt    << '\n';
    out << "last_played="      << info.lastPlayed   << '\n';
    out << "coins="            << info.coins        << '\n';
    out << "current_level="    << info.currentLevel << '\n';
    out << "level_stars="      << serializeStars(info.levelStars) << '\n';
    out << "level_best_times=" << serializeTimes(info.levelBestTimes) << '\n';
    return true;
}

bool SaveManager::setLevelStar(const std::string& filename,
                               int level, int stars) {
    SaveInfo info;
    if (!loadSave(filename, info)) return false;
    if (level < 1 || level > static_cast<int>(info.levelStars.size()))
        return false;

    int idx = level - 1;
    if (stars > info.levelStars[idx]) {
        info.levelStars[idx] = stars;
        info.lastPlayed = currentTimestamp();

        auto path = config_->saveFile(filename);
        std::ofstream out(path);
        if (!out) return false;
        out << "name="             << info.name         << '\n';
        out << "created_at="       << info.createdAt    << '\n';
        out << "last_played="      << info.lastPlayed   << '\n';
        out << "coins="            << info.coins        << '\n';
        out << "current_level="    << info.currentLevel << '\n';
        out << "level_stars="      << serializeStars(info.levelStars) << '\n';
        out << "level_best_times=" << serializeTimes(info.levelBestTimes) << '\n';

        logger_->info("第 " + std::to_string(level) + " 关星级更新为 " +
                      std::to_string(stars));
    }
    return true;
}

bool SaveManager::setLevelBestTime(const std::string& filename,
                                   int level, float seconds) {
    SaveInfo info;
    if (!loadSave(filename, info)) return false;
    if (level < 1 || level > static_cast<int>(info.levelBestTimes.size()))
        return false;
    if (seconds <= 0.f) return false;

    int idx = level - 1;
    // 首次通关，或刷新 PB
    if (info.levelBestTimes[idx] <= 0.f || seconds < info.levelBestTimes[idx]) {
        info.levelBestTimes[idx] = seconds;
        info.lastPlayed = currentTimestamp();

        auto path = config_->saveFile(filename);
        std::ofstream out(path);
        if (!out) return false;
        out << "name="             << info.name         << '\n';
        out << "created_at="       << info.createdAt    << '\n';
        out << "last_played="      << info.lastPlayed   << '\n';
        out << "coins="            << info.coins        << '\n';
        out << "current_level="    << info.currentLevel << '\n';
        out << "level_stars="      << serializeStars(info.levelStars) << '\n';
        out << "level_best_times=" << serializeTimes(info.levelBestTimes) << '\n';

        logger_->info("第 " + std::to_string(level) + " 关 PB 更新为 " +
                      std::to_string(seconds) + " 秒");
        return true;
    }
    return false;   // 未刷新
}