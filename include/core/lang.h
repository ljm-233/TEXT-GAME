#pragma once
#include <string>
#include <unordered_map>
#include <vector>

// 极简 i18n
// - 用中文原文作为 key（中文=中文）
// - 语言文件 assets/lang/<code>.txt 格式：中文=翻译
// - 找不到翻译时返回 key 本身
class Lang {
public:
    static Lang& instance();

    // 设置语言文件所在目录（一般 = assets/lang）
    void setLangDir(const std::string& dir) { langDir_ = dir; }

    // 加载语言；code == "zh" 时等价于关闭翻译（key 即原文）
    // 返回是否找到语言文件
    bool load(const std::string& code);

    const std::string& current() const { return current_; }

    // 版本号，每次 load() 递增；UI 可据此判断语言是否变了
    int version() const { return version_; }

    // 翻译。找不到时返回 key 本身
    std::string tr(const std::string& key) const;

    // 扫描 langDir 里的 <code>.txt
    void scanAvailable();
    const std::vector<std::string>& available() const { return available_; }

private:
    Lang() = default;

    std::unordered_map<std::string, std::string> map_;
    std::string langDir_;
    std::string current_ = "zh";
    std::vector<std::string> available_;
    int version_ = 0;
};