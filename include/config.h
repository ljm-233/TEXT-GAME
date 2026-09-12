#pragma once
#include "paths.h"
#include <string>
#include <unordered_map>
#include <fstream>

class Config {
public:
    explicit Config(const Paths& paths)
        : paths_(paths),
          filePath_(paths.configDir() / "settings.conf") {
        load();
    }

    // ---------- 读配置 ----------
    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = values_.find(key);
        return it != values_.end() ? it->second : defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = values_.find(key);
        if (it == values_.end()) return defaultValue;
        try { return std::stoi(it->second); }
        catch (...) { return defaultValue; }
    }

    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto it = values_.find(key);
        if (it == values_.end()) return defaultValue;
        try { return std::stod(it->second); }
        catch (...) { return defaultValue; }
    }

    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto v = get(key, defaultValue ? "true" : "false");
        return v == "true" || v == "1" || v == "yes";
    }

    // ---------- 写配置 ----------
    void set(const std::string& key, const std::string& value) {
        values_[key] = value;
        save();
    }
    void setInt(const std::string& key, int v)    { set(key, std::to_string(v)); }
    void setDouble(const std::string& key, double v) { set(key, std::to_string(v)); }
    void setBool(const std::string& key, bool v)  { set(key, v ? "true" : "false"); }

    // ---------- 资源路径统一出口 ----------
    std::filesystem::path configDir() const { return paths_.configDir(); }
    std::filesystem::path cacheDir()  const { return paths_.cacheDir(); }
    std::filesystem::path tempDir()   const { return paths_.tempDir(); }
    std::filesystem::path savesDir()  const { return paths_.savesDir(); }

    // 常用便捷方法：拼一个 config 下的文件路径
    std::filesystem::path configFile(const std::string& name) const {
        return paths_.configDir() / name;
    }
    std::filesystem::path saveFile(const std::string& name) const {
        return paths_.savesDir() / name;
    }

private:
    void load() {
        std::ifstream in(filePath_);
        if (!in) return;   // 首次运行没有文件，就用默认值

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            values_[key] = val;
        }
    }

    void save() const {
        std::ofstream out(filePath_);
        if (!out) return;
        for (const auto& [k, v] : values_) {
            out << k << '=' << v << '\n';
        }
    }

    const Paths& paths_;
    std::filesystem::path filePath_;
    std::unordered_map<std::string, std::string> values_;
};