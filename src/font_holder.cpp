#include "font_holder.h"

FontHolder::FontHolder(const std::filesystem::path& fontPath,
                       std::shared_ptr<Logger> logger) {
    // SFML 3 使用 openFromFile；若编译报错改成 loadFromFile
    if (!font_.openFromFile(fontPath)) {
        logger->error("无法加载字体: " + fontPath.string());
    } else {
        logger->info("字体加载: " + fontPath.string());
    }
}