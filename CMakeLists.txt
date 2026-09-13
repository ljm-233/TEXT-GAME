cmake_minimum_required(VERSION 3.20)
project(text_game CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(text_game
    src/main.cpp
    src/application.cpp
    src/calculator.cpp
    src/game.cpp
    src/background.cpp
    src/font_holder.cpp
    src/button.cpp
    src/slider.cpp
    src/confirm_dialog.cpp
    src/save_manager.cpp
    src/main_menu_scene.cpp
    src/save_select_scene.cpp
    src/game_scene.cpp
    src/settings_scene.cpp
    src/console.cpp
    src/console_scene.cpp
    src/window.cpp
)

target_include_directories(text_game PRIVATE include src)

target_compile_definitions(text_game PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(text_game PRIVATE SFML::Graphics)