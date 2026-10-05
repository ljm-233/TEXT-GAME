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
    test_scene_manager.cpp
    test_layers.cpp
    test_entities.cpp
    test_game_world.cpp
    test_level_codec.cpp
    test_save_manager.cpp
    test_score_rules.cpp
    test_editor_tools.cpp
    test_focus_nav.cpp
    test_resource_manager.cpp
    test_platform.cpp
    test_config_seeding.cpp
    test_settings_codec.cpp
    test_settings_tabs.cpp
    test_settings_share.cpp
    test_particle_system.cpp
    test_keybindings.cpp
    test_wallpaper_library.cpp
    test_wallpaper_thumbnail.cpp
)

# 只有 doctest.h 需要额外加路径；游戏源码的 include 路径、
# PROJECT_VERSION / BUILD_DATE、SFML / OpenGL / XInput
# 全部由 text_game_core 的 PUBLIC 传递过来。
target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests
)

# 测试要读源码树（分层测试扫 include/ 与 src/，关卡测试读 assets/levels/），
# 所以 PROJECT_ROOT 由**测试自己**定义 —— 库那边在打包构建里刻意不带它，
# 免得把构建机路径编进发出去的二进制。
target_compile_definitions(unit_tests PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

# ⭐ 不再手写源文件清单，直接链 text_game_core。
#
# 这里原本是几十行手写列表，和顶层 CMakeLists 的 GLOB 各维护一份。
# 手写那份一旦漏文件，本地可能没事、换个平台就是链接错误 ——
# macOS 上漏掉 gamepad_vibration_stub.cpp 就是这么挂的，当时还专门加了个
# GLOB 打补丁。现在两个目标共用同一个静态库，从结构上漏不了。
#
# 链接整个库不会破坏"无界面环境也能跑"：静态库只按需拉取被引用到的 .o，
# 而真正会创建 GL 上下文的只有 sprite factory 之类**被调用时**才建
# RenderTexture 的代码 —— 测试走的是默认的空 sheets，从不调用它们。
target_link_libraries(unit_tests PRIVATE text_game_core)

add_test(NAME unit_tests COMMAND unit_tests)
