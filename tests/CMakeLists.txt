# ============================================================
# 单元测试
# ============================================================

add_executable(unit_tests
    main.cpp
    test_vec2.cpp
    test_aabb.cpp
    test_config.cpp
    test_level.cpp
)

# 测试需要访问项目的头文件
target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/config
    ${CMAKE_SOURCE_DIR}/include/ui
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/scene
    ${CMAKE_SOURCE_DIR}/tests
)

# PROJECT_ROOT 宏（Level 不用，但以防万一）
target_compile_definitions(unit_tests PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

# 只链接项目里需要的源文件（不链接 SFML 图形，避免依赖）
target_sources(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/game/level.cpp
)

target_link_libraries(unit_tests PRIVATE
    SFML::Graphics
)

# 注册到 CTest
add_test(NAME unit_tests COMMAND unit_tests)