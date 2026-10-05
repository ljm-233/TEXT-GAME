#pragma once
#include "paths.h"
#include "resource_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class Config {
public:
    // 生产用：从 Paths 推出配置文件路径（configDir() / filename），
    // 并接上 ResourceManager —— `get()` 里会展开值中的 ${path:别名}。
    Config(const Paths& paths, const ResourceManager& resources,
           const std::string& filename)
          : resources_(&resources),
            filePath_(paths.configDir() / filename) {
        load();
    }

    // 测试用 / 独立用：直接指定配置文件路径，不依赖 Paths，也不做插值展开
    explicit Config(const std::filesystem::path& filePath)
          : resources_(nullptr),
            filePath_(filePath) {
        load();
    }

    virtual ~Config() { flush(); }

    // ---------- 读 ----------
    //
    // ⚠️ 返回前会展开 ${path:别名}（见 ResourceManager::expand）。
    //    没有 `${` 的值只多一次 find，可以放心在热路径上调。
    //    只对待字符串的值生效；getInt / getDouble 走原始值 ——
    //    数字字段里写插值没有意义。
    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = values_.find(key);
        const std::string& raw = it != values_.end() ? it->second : defaultValue;
        return resources_ ? resources_->expand(raw) : raw;
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

    // 测试用：直接访问本 Config 对应的文件路径
    const std::filesystem::path& filePath() const { return filePath_; }

    // ---------- 调试 ----------
    /// 已设置的键数量。
    size_t size() const { return values_.size(); }

    /// 多行快照，键按字典序排列。
    ///
    /// 排序而不是按哈希序输出，是为了让两次运行的快照能直接 diff ——
    /// "这次启动和上次差了哪个键"是排查配置问题时最常问的一句话。
    /// 对应 albuswall Configue 的 __str__。
    std::string str() const {
        std::vector<std::string> keys;
        keys.reserve(values_.size());
        for (const auto& entry : values_)
            keys.push_back(entry.first);
        std::sort(keys.begin(), keys.end());

        std::string head =
            filePath_.filename().string() + " (" + std::to_string(values_.size()) + " 项";
        if (dirty_)
            head += "，有未落盘改动";
        head += ")";

        std::string out = head;
        for (const auto& key : keys)
            out += "\n    " + key + " = " + values_.at(key);
        return out;
    }

protected:
    const ResourceManager* resources_ = nullptr; // 可空（裸路径构造时）
    std::filesystem::path filePath_;
    std::unordered_map<std::string, std::string> values_;

    void save() const {
        std::ofstream out(filePath_);
        if (!out) {
            std::cerr << "[Config] 无法写入: " << filePath_.string() << "\n";
            return;
        }
        for (const auto& [k, v] : values_) {
            out << k << '=' << v << '\n';
        }
        if (!out) {
            std::cerr << "[Config] 写入过程中出错: " << filePath_.string() << "\n";
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