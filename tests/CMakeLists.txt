add_executable(unit_tests
    main.cpp
    test_vec2.cpp
    test_aabb.cpp
    test_config.cpp
    test_level.cpp
    test_camera.cpp
    test_animator.cpp
    test_game_constants.cpp
    test_strings.cpp
    test_moving_platform.cpp
    test_jump_pad.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/config
    ${CMAKE_SOURCE_DIR}/include/ui
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/scene
    ${CMAKE_SOURCE_DIR}/tests
)

target_compile_definitions(unit_tests PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

# 只链接测试需要的源文件（不包含窗口、渲染等）
target_sources(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/game/level.cpp
    ${CMAKE_SOURCE_DIR}/src/game/camera.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/animator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/animation.cpp
    ${CMAKE_SOURCE_DIR}/src/game/moving_platform.cpp
    ${CMAKE_SOURCE_DIR}/src/game/jump_pad.cpp
)

target_link_libraries(unit_tests PRIVATE
    SFML::Graphics
)

add_test(NAME unit_tests COMMAND unit_tests)