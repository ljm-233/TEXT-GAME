#pragma once
#include "logging.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>

// 字体持有者：加载一次，全局共享
class FontHolder {
public:
    FontHolder(const std::filesystem::path& fontPath, std::shared_ptr<Logger> logger);

    const sf::Font& get() const { return font_; }

private:
    sf::Font font_;
};