add_executable(unit_tests
    main.cpp
    test_vec2.cpp
    test_aabb.cpp
    test_config.cpp
    test_level.cpp
    test_camera.cpp
    test_animator.cpp
    test_game_constants.cpp
    test_moving_platform.cpp
    test_jump_pad.cpp
    test_player.cpp
    test_container.cpp
    test_logger.cpp
    test_application.cpp
    test_scene_registry.cpp
    test_layers.cpp
    test_entities.cpp
    test_game_world.cpp
    test_level_codec.cpp
    test_save_manager.cpp
    test_score_rules.cpp
    test_editor_tools.cpp
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
    ${CMAKE_SOURCE_DIR}/src/utils/animator.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/animation.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/gamepad.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/keybindings.cpp
    ${CMAKE_SOURCE_DIR}/src/game/moving_platform.cpp
    ${CMAKE_SOURCE_DIR}/src/game/jump_pad.cpp
    ${CMAKE_SOURCE_DIR}/src/game/player.cpp
    # 无纹理的四类实体：纯 RectangleShape/VertexArray 绘制，
    # 不碰 sprite factory，因此可以在无 GL 上下文的测试进程里构造
    ${CMAKE_SOURCE_DIR}/src/game/checkpoint.cpp
    ${CMAKE_SOURCE_DIR}/src/game/door.cpp
    ${CMAKE_SOURCE_DIR}/src/game/key.cpp
    ${CMAKE_SOURCE_DIR}/src/game/spike.cpp
    # Coin / Enemy 现在也把贴图作为构造参数，源码里不再调用 sprite factory，
    # 所以测试目标不需要链上那几个 factory 实现
    ${CMAKE_SOURCE_DIR}/src/game/coin.cpp
    ${CMAKE_SOURCE_DIR}/src/game/enemy.cpp
    ${CMAKE_SOURCE_DIR}/src/game/game_world.cpp
    ${CMAKE_SOURCE_DIR}/src/game/level_codec.cpp
    ${CMAKE_SOURCE_DIR}/src/game/save_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/game/score_rules.cpp
    ${CMAKE_SOURCE_DIR}/src/scene/editor_tools.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/lang.cpp
    # game_world.cpp 里定义了 SpriteSheets::fromFactories()，引用了这三个工厂，
    # 所以链接期必须有它们的实现。仅仅是"被链接"不会触发 GL ——
    # 工厂只在真正被调用时才建 RenderTexture，测试走的是默认的空 sheets。
    ${CMAKE_SOURCE_DIR}/src/game/player_sprite_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/game/coin_sprite_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/game/enemy_sprite_factory.cpp
    # gamepad.cpp 调用了 GamepadVibration，实现必须一起链上，
    # 否则 unit_tests 会在链接期报 undefined reference。
    # 两个平台文件内部各有 #ifdef 守卫，同时列出来是安全的。
    ${CMAKE_SOURCE_DIR}/src/infrastructure/gamepad_vibration.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/gamepad_vibration_linux.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/gamepad_vibration_windows.cpp
    # 地基：容器与日志层（不含 bootstrap —— 那层是装配胶水，依赖配置与窗口）
    ${CMAKE_SOURCE_DIR}/src/core/container.cpp
    ${CMAKE_SOURCE_DIR}/src/core/main_loop.cpp
    ${CMAKE_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_SOURCE_DIR}/src/core/scene_registry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/scene_id.cpp
    ${CMAKE_SOURCE_DIR}/src/log/type.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/log/rotation.cpp
    ${CMAKE_SOURCE_DIR}/src/log/formatters/text_formatter.cpp
    ${CMAKE_SOURCE_DIR}/src/log/formatters/console_formatter.cpp
    ${CMAKE_SOURCE_DIR}/src/log/handlers/console_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/log/handlers/file_handler.cpp
)

target_link_libraries(unit_tests PRIVATE
    SFML::Graphics
)

add_test(NAME unit_tests COMMAND unit_tests)