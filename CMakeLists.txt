cmake_minimum_required(VERSION 3.20)
project(text_game CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # 给 clangd 用

# 找 SFML
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(text_game
    src/main.cpp
    src/application.cpp
    src/calculator.cpp
    src/window.cpp
    src/game.cpp 
)

target_include_directories(text_game PRIVATE include src)

# 把项目根目录作为宏传进去
target_compile_definitions(text_game PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

# 链接 SFML
target_link_libraries(text_game PRIVATE SFML::Graphics)