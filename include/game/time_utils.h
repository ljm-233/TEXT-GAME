#pragma once
#include <ctime>
#include <iostream>

inline void getime() {
    std::time_t now = std::time(nullptr);
    std::cout << std::ctime(&now);
}