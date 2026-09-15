#pragma once
#include "paths.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

class Config {
public:
    // 生产用：从 Paths 推导配置文件路径（configDir() / filename）
    Config(const Paths& paths, const std::string& filename)
          : paths_(&paths),
            filePath_(paths.configDir() / filename) {
        load();
    }

    // 测试用 / 独立用：直接指定配置文件路径，不依赖 Paths
    explicit Config(const std::filesystem::path& filePath)
          : paths_(nullptr),
            filePath_(filePath) {
        load();
    }

    virtual ~Config() { flush(); }

    // ---------- 读 ----------
    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = values_.find(key);
        return it != values_.end() ? it->second : defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = values_.find(key);
        if (it == values_.end())
            return defaultValue;
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }

    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto it = values_.find(key);
        if (it == values_.end())
            return defaultValue;
        try {
            return std::stod(it->second);
        } catch (...) {
            return defaultValue;
        }
    }

    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto v = get(key, defaultValue ? "true" : "false");
        return v == "true" || v == "1" || v == "yes";
    }

    // ---------- 写（只改内存，延迟到 flush 才落盘） ----------
    void set(const std::string& key, const std::string& value) {
        auto it = values_.find(key);
        if (it != values_.end() && it->second == value)
            return; // 值没变，不标脏
        values_[key] = value;
        dirty_ = true;
    }
    void setInt(const std::string& key, int v) { set(key, std::to_string(v)); }
    void setDouble(const std::string& key, double v) { set(key, std::to_string(v)); }
    void setBool(const std::string& key, bool v) { set(key, v ? "true" : "false"); }

    // 清空全部并落盘
    void resetAll() {
        values_.clear();
        dirty_ = true;
        flush();
    }

    // 立即写磁盘（dirty_ 为 false 时是空操作）
    void flush() {
        if (!dirty_)
            return;
        save();
        dirty_ = false;
    }

    bool isDirty() const { return dirty_; }

    // ---------- 资源路径 ----------
    // 当用"裸路径"构造时（paths_ == nullptr），
    // 除 configDir() 外的路径方法返回空路径。
    std::filesystem::path configDir() const {
        return paths_ ? paths_->configDir() : filePath_.parent_path();
    }
    std::filesystem::path cacheDir() const {
        return paths_ ? paths_->cacheDir() : std::filesystem::path{};
    }
    std::filesystem::path tempDir() const {
        return paths_ ? paths_->tempDir() : std::filesystem::path{};
    }
    std::filesystem::path savesDir() const {
        return paths_ ? paths_->savesDir() : std::filesystem::path{};
    }
    std::filesystem::path wallpaperDir() const {
        return paths_ ? paths_->wallpaperDir() : std::filesystem::path{};
    }
    std::filesystem::path assetsDir() const {
        return paths_ ? paths_->assetsDir() : std::filesystem::path{};
    }

    std::filesystem::path configFile(const std::string& name) const {
        return configDir() / name;
    }
    std::filesystem::path saveFile(const std::string& name) const {
        return savesDir() / name;
    }
    std::filesystem::path assetFile(const std::string& name) const {
        return assetsDir() / name;
    }

    // 测试用：直接访问本 Config 对应的文件路径
    const std::filesystem::path& filePath() const { return filePath_; }

protected:
    const Paths* paths_ = nullptr;   // 可空
    std::filesystem::path filePath_;
    std::unordered_map<std::string, std::string> values_;

    void save() const {
        std::ofstream out(filePath_);
        if (!out)
            return;
        for (const auto& [k, v] : values_) {
            out << k << '=' << v << '\n';
        }
    }

private:
    void load() {
        std::ifstream in(filePath_);
        if (!in)
            return;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#')
                continue;
            auto pos = line.find('=');
            if (pos == std::string::npos)
                continue;
            values_[line.substr(0, pos)] = line.substr(pos + 1);
        }
    }

    bool dirty_ = false;
};