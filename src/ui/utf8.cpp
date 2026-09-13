#include "utf8.h"

sf::String toSf(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

std::string fromSf(const sf::String& s) {
    auto utf8 = s.toUtf8();
    return std::string(utf8.begin(), utf8.end());
}