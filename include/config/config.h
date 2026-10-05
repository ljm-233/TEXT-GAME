#pragma once
#include "paths.h"
#include "resource_manager.h"
#include "config/keys.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

    // ---------- 恢复默认 ----------

    /// 把这些键**从配置里删掉**（而不是写回一份默认值）。
    ///
    /// 默认值只存在于读取处（`getInt(key, def)` 的 def），删掉之后自然回落。
    /// 这样"默认值"永远只有一个定义 —— 如果在别处再抄一份默认值表，
    /// 迟早会漂移成两边打架（CLAUDE.md 里 vsync/fps_limit 那件事就是这么来的）。
    void resetKeys(const std::vector<std::string>& keys) {
        bool changed = false;
        for (const auto& k : keys)
            changed = (values_.erase(k) > 0) || changed;
        if (changed)
            dirty_ = true;
    }

    // ---------- 设置的导出 / 导入 ----------

    /// 把**可携带**的设置序列化成 "key=value" 多行文本（按字典序，便于比对）。
    ///
    /// 只导出 `ConfigKey::isPortable()` 认的键。跟机器绑定的那些
    /// （分辨率 / 窗口模式 / ui_scale / 帧率上限…）导出去只会坑到别人，
    /// 逐条理由见 src/config/portable_keys.cpp。
    std::string exportPortable() const {
        std::vector<std::string> keys;
        keys.reserve(values_.size());
        for (const auto& entry : values_) {
            if (ConfigKey::isPortable(entry.first))
                keys.push_back(entry.first);
        }
        std::sort(keys.begin(), keys.end());

        // 值里如果没有 ${ 就不用展开，导出的是"存进去的原样文本" ——
        // 导出的东西要能原样导回来，展开过就回不去了
        std::string out;
        for (const auto& key : keys)
            out += key + "=" + values_.at(key) + "\n";
        return out;
    }

    /// 应用一段 "key=value" 文本，**只认白名单里的键**，其余静默忽略。
    /// 返回实际应用了多少条（给 UI 显示"导入了 N 项"）。
    ///
    /// 分享码是别人粘贴进来的，所以这里按"不可信输入"处理：
    ///   - 不认的键（尤其是跟机器绑定的）直接跳过，不会去改窗口/分辨率设置
    ///   - 行格式不对就跳过，不抛异常
    int applyPortable(const std::string& text) {
        int applied = 0;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line)) {
            // 兼容在 Windows 上手工编辑过的文本（行尾 \r）
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty() || line[0] == '#')
                continue;

            const auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;

            const std::string key = line.substr(0, eq);
            if (!ConfigKey::isPortable(key))
                continue;

            set(key, line.substr(eq + 1));
            ++applied;
        }
        return applied;
    }

    // 测试用：直接访问本 Config 对应的文件路径
    const std::filesystem::path& filePath() const { return filePath_; }

    // ---------- 调试 ----------
    /// 已设置的键数量。
    size_t size() const { return values_.size(); }

    /// 已设置的键名（不做排序 —— 调用方多半要拿去比对集合）。
    ///
    /// 给"点了这个控件到底改了哪个键"这类检查用：改动前后各取一次做差集，
    /// 就能看出某个控件有没有越界去改别的设置页的键。
    std::vector<std::string> keys() const {
        std::vector<std::string> out;
        out.reserve(values_.size());
        for (const auto& entry : values_)
            out.push_back(entry.first);
        return out;
    }

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