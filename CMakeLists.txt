cmake_minimum_required(VERSION 3.20)
project(text_game CXX)

# ============================================================
# 平台检测
# ============================================================
message(STATUS "=== 平台检测 ===")
message(STATUS "  系统:     ${CMAKE_SYSTEM_NAME}")
message(STATUS "  版本:     ${CMAKE_SYSTEM_VERSION}")
message(STATUS "  架构:     ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  构建类型: ${CMAKE_BUILD_TYPE}")

if(WIN32)
    set(TEXTGAME_PLATFORM "Windows")
elseif(APPLE)
    set(TEXTGAME_PLATFORM "macOS")
elseif(UNIX)
    if(EXISTS "/etc/arch-release")
        set(TEXTGAME_PLATFORM "Arch Linux")
    elseif(EXISTS "/etc/debian_version")
        set(TEXTGAME_PLATFORM "Debian/Ubuntu")
    elseif(EXISTS "/etc/fedora-release")
        set(TEXTGAME_PLATFORM "Fedora")
    else()
        set(TEXTGAME_PLATFORM "Linux")
    endif()
else()
    set(TEXTGAME_PLATFORM "Unknown")
endif()
message(STATUS "  识别为:   ${TEXTGAME_PLATFORM}")

# ============================================================
# 编译器检测
# ============================================================
message(STATUS "=== 编译器检测 ===")
message(STATUS "  ID:       ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  版本:     ${CMAKE_CXX_COMPILER_VERSION}")

# ============================================================
# C++ 标准
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# 编译选项：按编译器区分
# ============================================================
if(MSVC)
    # MSVC 用 /W4，不用 -Wall
    add_compile_options(/W4 /permissive- /utf-8)
    # Windows 定义 UNICODE
    add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
    )
    # GCC 专用：UTF-8 编码
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
        )
    endif()
endif()

# ============================================================
# 查找 SFML 3
# ============================================================
find_package(SFML 3 COMPONENTS Graphics Window System Audio QUIET)

if(NOT SFML_FOUND)
    message(FATAL_ERROR
        "未找到 SFML 3。\n"
        "  Arch Linux:  sudo pacman -S sfml\n"
        "  Ubuntu:      sudo apt install libsfml-dev  (注意：默认可能是 2.6)\n"
        "  Windows:     vcpkg install sfml:x64-windows\n"
        "  macOS:       brew install sfml")
endif()

message(STATUS "=== SFML 检测 ===")
message(STATUS "  版本: ${SFML_VERSION}")
message(STATUS "  路径: ${SFML_DIR}")

# ============================================================
# 可执行文件
# ============================================================
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
    src/ui/animation.cpp
    src/ui/notification.cpp
    src/ui/pause_menu.cpp
    src/ui/sound_manager.cpp
    src/ui/particle_system.cpp
    src/game/calculator.cpp
    src/game/save_manager.cpp
    src/game/level.cpp
    src/game/camera.cpp
    src/game/player.cpp
    src/game/game_world.cpp
    src/game/coin.cpp
    src/game/enemy.cpp
    src/game/jump_pad.cpp
    src/game/checkpoint.cpp
    src/scene/main_menu_scene.cpp
    src/scene/save_select_scene.cpp
    src/scene/game_scene.cpp
    src/scene/settings_scene.cpp
    src/scene/console_scene.cpp
    src/core/platform.cpp
    src/game/parallax.cpp
    src/game/level_intro.cpp
    src/game/moving_platform.cpp
    src/ui/animator.cpp
    src/game/player_sprite_factory.cpp
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

# 平台宏（代码里可以 #ifdef TEXTGAME_PLATFORM_WINDOWS）
if(WIN32)
    target_compile_definitions(text_game PRIVATE TEXTGAME_PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(text_game PRIVATE TEXTGAME_PLATFORM_MACOS)
elseif(UNIX)
    target_compile_definitions(text_game PRIVATE TEXTGAME_PLATFORM_LINUX)
endif()

# ============================================================
# 链接：SFML 目标（不同平台名字一样）
# ============================================================
target_link_libraries(text_game PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

# ============================================================
# Windows：GUI 程序（不弹控制台）
# ============================================================
if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(text_game PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ============================================================
# macOS：应用包（可选）
# ============================================================
if(APPLE)
    set_target_properties(text_game PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "TEXT-GAME"
    )
endif()

# ============================================================
# 完成
# ============================================================
message(STATUS "=== 构建配置完成 ===")