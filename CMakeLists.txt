cmake_minimum_required(VERSION 3.20)
project(text_game VERSION 0.1.0 LANGUAGES CXX)

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
# 编译器
# ============================================================
message(STATUS "=== 编译器检测 ===")
message(STATUS "  ID:       ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  版本:     ${CMAKE_CXX_COMPILER_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
    add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
endif()

# ============================================================
# SFML 3
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
    src/core/platform.cpp
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
    src/ui/animator.cpp
    src/ui/gamepad.cpp
    src/ui/focus_group.cpp
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
    src/game/moving_platform.cpp
    src/game/player_sprite_factory.cpp
    src/game/parallax.cpp
    src/game/level_intro.cpp
    src/game/key.cpp
    src/game/door.cpp
    src/game/spike.cpp
    src/game/coin_sprite_factory.cpp
    src/game/enemy_sprite_factory.cpp
    src/scene/main_menu_scene.cpp
    src/scene/save_select_scene.cpp
    src/scene/level_select_scene.cpp
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
    PROJECT_VERSION="${PROJECT_VERSION}"
    BUILD_DATE="${BUILD_DATE}"
)

if(WIN32)
    target_compile_definitions(text_game PRIVATE TEXTGAME_PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(text_game PRIVATE TEXTGAME_PLATFORM_MACOS)
elseif(UNIX)
    target_compile_definitions(text_game PRIVATE TEXTGAME_PLATFORM_LINUX)
endif()

target_link_libraries(text_game PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(text_game PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(APPLE)
    set_target_properties(text_game PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "TEXT-GAME"
    )
endif()

install(TARGETS text_game RUNTIME DESTINATION .)
# ============================================================
# Windows：把 SFML 的 DLL 一起装到输出目录
# ============================================================
if(WIN32)
    # 找到 SFML 的运行时 DLL 目录
    # 从 sfml-graphics 的 IMPORTED_LOCATION 推导
    get_target_property(_sfml_graphics_loc SFML::Graphics IMPORTED_LOCATION_RELEASE)
    if(NOT _sfml_graphics_loc)
        get_target_property(_sfml_graphics_loc SFML::Graphics IMPORTED_LOCATION)
    endif()

    if(_sfml_graphics_loc)
        get_filename_component(_sfml_dll_dir "${_sfml_graphics_loc}" DIRECTORY)

        # 把所有 sfml-*.dll 和 openal32.dll 一起安装
        install(DIRECTORY "${_sfml_dll_dir}/"
                DESTINATION .
                FILES_MATCHING
                PATTERN "sfml-*.dll"
                PATTERN "openal32.dll"
                PATTERN "*.dll"
                )
    endif()
endif()
install(DIRECTORY assets/    DESTINATION assets)
install(DIRECTORY wallpaper/ DESTINATION wallpaper)
install(FILES README.md LICENSE DESTINATION .)

set(CPACK_PACKAGE_NAME        "TEXT-GAME")
set(CPACK_PACKAGE_VENDOR      "ljm-233")
set(CPACK_PACKAGE_VERSION     "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "A 2D platformer built from scratch in C++20 + SFML 3")
set(CPACK_PACKAGE_CONTACT     "ljm-233")

if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "TEXT-GAME-${PROJECT_VERSION}-Windows")
elseif(APPLE)
    set(CPACK_PACKAGE_FILE_NAME "TEXT-GAME-${PROJECT_VERSION}-macOS")
else()
    set(CPACK_PACKAGE_FILE_NAME "TEXT-GAME-${PROJECT_VERSION}-Linux")
endif()

set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES TRUE)

include(CPack)

# ============================================================
# 测试（默认关闭，加 -DBUILD_TESTS=ON 开启）
# ============================================================
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "=== 单元测试已启用 ===")
endif()

message(STATUS "=== 构建配置完成 ===")