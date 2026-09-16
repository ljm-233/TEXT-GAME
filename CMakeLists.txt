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
# Sanitizer（-DSANITIZE=ON 开启）
# ============================================================
option(SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer + LeakSanitizer" OFF)

if(SANITIZE)
    if(MSVC)
        message(WARNING "Sanitizer 不支持 MSVC，已忽略")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "=== Sanitizer 已启用 ===")
        message(STATUS "  ASan  (AddressSanitizer)")
        message(STATUS "  UBSan (UndefinedBehaviorSanitizer)")
        message(STATUS "  LSan  (LeakSanitizer, 仅 Linux)")
        message(STATUS "  运行时可设置 ASAN_OPTIONS 环境变量:")
        message(STATUS "    禁用泄漏检测: ASAN_OPTIONS=detect_leaks=0")
        message(STATUS "    关闭 UBSan:   UBSAN_OPTIONS=halt_on_error=0")

        add_compile_options(
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-common
            -g
        )
        add_link_options(-fsanitize=address,undefined)
    else()
        message(WARNING "Sanitizer 只在 GCC / Clang 上支持，已忽略")
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
# 源文件收集（自动扫描，加新文件无需改本文件）
# ============================================================
file(GLOB_RECURSE TEXTGAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_SOURCE_DIR}/src/config/*.cpp
    ${CMAKE_SOURCE_DIR}/src/game/*.cpp
    ${CMAKE_SOURCE_DIR}/src/scene/*.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/*.cpp
)
list(LENGTH TEXTGAME_SOURCES TEXTGAME_SOURCES_COUNT)
message(STATUS "=== 源文件扫描 ===")
message(STATUS "  共 ${TEXTGAME_SOURCES_COUNT} 个 .cpp 文件")

# ============================================================
# 可执行文件
# ============================================================
add_executable(text_game ${TEXTGAME_SOURCES})

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
    get_target_property(_sfml_graphics_loc SFML::Graphics IMPORTED_LOCATION_RELEASE)
    if(NOT _sfml_graphics_loc)
        get_target_property(_sfml_graphics_loc SFML::Graphics IMPORTED_LOCATION)
    endif()

    if(_sfml_graphics_loc)
        get_filename_component(_sfml_dll_dir "${_sfml_graphics_loc}" DIRECTORY)

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
# 关卡验证工具（独立目标）
# ============================================================
add_executable(validate_levels
    tools/validate_levels.cpp
    src/game/level.cpp
    src/game/level_validator.cpp
)
target_include_directories(validate_levels PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_compile_definitions(validate_levels PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)
target_link_libraries(validate_levels PRIVATE
    SFML::Graphics
)

# ============================================================
# 测试（默认关闭，加 -DBUILD_TESTS=ON 开启）
# ============================================================
# ============================================================
# clang-tidy（-DCLANG_TIDY=ON 开启）
# ============================================================
option(CLANG_TIDY "Run clang-tidy during build" OFF)

if(CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(NOT CLANG_TIDY_EXE)
        message(WARNING "未找到 clang-tidy，CLANG_TIDY 选项被忽略")
    else()
        message(STATUS "=== clang-tidy 已启用: ${CLANG_TIDY_EXE} ===")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endif()

# ============================================================
# 测试覆盖率（-DCOVERAGE=ON 开启）
# ============================================================
option(COVERAGE "Enable code coverage (gcov/lcov)" OFF)

if(COVERAGE)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "覆盖率构建建议使用 Debug 模式（当前: ${CMAKE_BUILD_TYPE}）")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "=== 覆盖率已启用（gcov / lcov）===")

        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)

        find_program(LCOV_EXE NAMES lcov)
        find_program(GENHTML_EXE NAMES genhtml)

        if(LCOV_EXE AND GENHTML_EXE)
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E rm -rf coverage.info coverage_html
                COMMAND ${LCOV_EXE} --zerocounters
                        --directory ${CMAKE_BINARY_DIR}
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${LCOV_EXE}
                        --capture
                        --directory ${CMAKE_BINARY_DIR}
                        --output-file coverage.info
                        --rc branch_coverage=1
                        --ignore-errors mismatch
                COMMAND ${LCOV_EXE}
                        --remove coverage.info
                        '/usr/*'
                        '*/tests/doctest.h'
                        '*/tests/*'
                        '*/_deps/*'
                        --output-file coverage_filtered.info
                        --rc branch_coverage=1
                        --ignore-errors unused
                COMMAND ${GENHTML_EXE}
                        coverage_filtered.info
                        --output-directory coverage_html
                        --branch-coverage
                        --legend
                COMMAND ${CMAKE_COMMAND} -E echo
                        "覆盖率报告: file://${CMAKE_BINARY_DIR}/coverage_html/index.html"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "生成覆盖率报告"
                USES_TERMINAL
            )

            message(STATUS "  运行: cmake --build ${CMAKE_BINARY_DIR} --target coverage")
            message(STATUS "  报告: ${CMAKE_BINARY_DIR}/coverage_html/index.html")
        else()
            message(WARNING "未找到 lcov / genhtml，coverage target 未创建")
            message(WARNING "  Arch Linux: sudo pacman -S lcov")
            message(WARNING "  Ubuntu:     sudo apt install lcov")
        endif()
    else()
        message(WARNING "覆盖率只在 GCC / Clang 上支持，已忽略")
    endif()
endif()

option(BUILD_TESTS "Build unit tests" OFF)

if(COVERAGE AND NOT BUILD_TESTS)
    message(WARNING "COVERAGE=ON 但 BUILD_TESTS=OFF，无法生成有意义覆盖率报告")
    message(WARNING "  建议同时 -DBUILD_TESTS=ON")
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "=== 单元测试已启用 ===")
endif()

message(STATUS "=== 构建配置完成 ===")