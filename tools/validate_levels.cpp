#include "level.h"
#include "level_validator.h"
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::string dir = "assets/levels";
    if (argc > 1) dir = argv[1];

    if (!fs::exists(dir)) {
        std::cerr << "目录不存在: " << dir << "\n"; // i18n-skip
        return 1;
    }

    int total = 0;
    int failed = 0;
    int unloadable = 0;

    // 按文件名排序
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".txt") continue;
        files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());

    for (const auto& path : files) {
        ++total;
        Level level;
        if (!level.loadFromFile(path.string())) {
            std::cerr << "[???] " << path.filename().string()
                      << ": 无法加载\n"; // i18n-skip
            ++unloadable;
            continue;
        }

        const auto report = LevelValidator::validate(level);
        if (report.ok()) {
            std::cout << "[ OK ] " << path.filename().string()
                      << ": " << report.summary() << "\n";
        } else {
            std::cout << "[FAIL] " << path.filename().string()
                      << ": " << report.summary() << "\n";
            ++failed;
        }
    }

    std::cout << "\n合计: " << total << " 个关卡, " // i18n-skip
              << failed << " 个失败, " // i18n-skip
              << unloadable << " 个无法加载\n"; // i18n-skip

    return (failed > 0 || unloadable > 0) ? 1 : 0;
}