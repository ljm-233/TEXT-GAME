#pragma once
#include <SFML/System/String.hpp>
#include <string>

// std::string（UTF-8 字节）→ sf::String
sf::String toSf(const std::string& s);

// sf::String → std::string（UTF-8 字节）
std::string fromSf(const sf::String& s);