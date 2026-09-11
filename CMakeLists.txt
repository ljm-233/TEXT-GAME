cmake_minimum_required(VERSION 3.20)
project(text_game CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # 给 clangd 用

add_executable(text_game
    src/main.cpp 
    src/time.cpp
    src/logging.cpp
)
