cmake_minimum_required(VERSION 3.20)
project(text_game CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_compile_options(
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wpedantic>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wshadow>
)

add_executable(text_game
    src/core/main.cpp
    src/core/application.cpp
    src/core/game.cpp
    src/core/scene_manager.cpp
    src/ui/button.cpp
    src/ui/slider.cpp
    src/ui/confirm_dialog.cpp
    src/ui/theme.cpp
    src/ui/ui_scale.cpp
    src/ui/utf8.cpp
    src/ui/window.cpp
    src/ui/background.cpp
    src/ui/font_holder.cpp
    src/ui/console.cpp
    src/ui/button_style.cpp
    src/ui/text_input.cpp
    src/ui/new_save_dialog.cpp
    src/game/calculator.cpp
    src/game/save_manager.cpp
    src/scene/main_menu_scene.cpp
    src/scene/save_select_scene.cpp
    src/scene/game_scene.cpp
    src/scene/settings_scene.cpp
    src/scene/console_scene.cpp
)

target_include_directories(text_game PRIVATE
    include
    include/core
    include/config
    include/ui
    include/game
    include/scene
)

string(TIMESTAMP BUILD_DATE "%Y-%m-%d")

target_compile_definitions(text_game PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    PROJECT_VERSION="0.1.0"
    BUILD_DATE="${BUILD_DATE}"
)

target_link_libraries(text_game PRIVATE SFML::Graphics)