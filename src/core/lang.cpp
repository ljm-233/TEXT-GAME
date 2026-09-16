#include "lang.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

Lang& Lang::instance() {
    static Lang inst;
    return inst;
}

bool Lang::load(const std::string& code) {
    map_.clear();
    current_ = code;
    ++version_;

    // zh 是 fallback（key 本身就是中文），不需要映射表
    if (code == "zh") {
        std::cerr << "[Lang] 切换到中文（原文）\n";
        return true;
    }

    if (langDir_.empty()) {
        std::cerr << "[Lang] 错误：语言目录为空\n";
        return false;
    }

    fs::path path = fs::path(langDir_) / (code + ".txt");
    std::ifstream in(path);
    if (!in) {
        std::cerr << "[Lang] 错误：无法打开 " << path << "\n";
        return false;
    }

    int count = 0;
    std::string line;
    while (std::getline(in, line)) {
        // 处理 CRLF
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        // ⭐ 关键：key 和 value 都不 trim
        // 因为 key 是"中文原文一字不差"，可能带尾随空格
        // value 也可能带尾随空格（拼接时有用，如 "存档 " + name）
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        if (key.empty()) continue;

        map_[key] = val;
        ++count;
    }
    std::cerr << "[Lang] 已加载 " << path << "（" << count << " 条翻译）\n";
    return true;
}

std::string Lang::tr(const std::string& key) const {
    auto it = map_.find(key);
    if (it != map_.end()) return it->second;
    return key;
}

void Lang::scanAvailable() {
    available_.clear();
    available_.push_back("zh");   // 始终有 zh

    if (langDir_.empty()) {
        std::cerr << "[Lang] 语言目录为空，只显示中文\n";
        return;
    }

    std::error_code ec;
    if (!fs::exists(langDir_, ec)) {
        std::cerr << "[Lang] 错误：目录不存在 " << langDir_ << "\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(langDir_, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".txt") continue;

        std::string code = entry.path().stem().string();
        if (code == "zh") continue;
        available_.push_back(code);
    }
    std::sort(available_.begin(), available_.end());

    std::cerr << "[Lang] 语言目录: " << langDir_ << "\n";
    std::cerr << "[Lang] 可用语言:";
    for (const auto& c : available_) std::cerr << " " << c;
    std::cerr << "\n";
}