cmake_minimum_required(VERSION 3.23)
project(text_game VERSION 0.3.4 LANGUAGES CXX)

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
    add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
                            _CRT_SECURE_NO_WARNINGS)
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
    ${CMAKE_SOURCE_DIR}/src/common/*.cpp
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_SOURCE_DIR}/src/config/*.cpp
    ${CMAKE_SOURCE_DIR}/src/game/*.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/*.cpp
    ${CMAKE_SOURCE_DIR}/src/log/*.cpp
    ${CMAKE_SOURCE_DIR}/src/scene/*.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/*.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/*.cpp
)
# main() 只能留在可执行文件里 —— 进了库会让两个可执行目标各带一份 main
list(REMOVE_ITEM TEXTGAME_SOURCES ${CMAKE_SOURCE_DIR}/src/core/main.cpp)
list(LENGTH TEXTGAME_SOURCES TEXTGAME_SOURCES_COUNT)
message(STATUS "=== 源文件扫描 ===")
message(STATUS "  text_game_core: ${TEXTGAME_SOURCES_COUNT} 个 .cpp（不含 main.cpp）")

# ============================================================
# 核心静态库
# ============================================================
#
# ⭐ 全部源码**只列一次**（就是上面那份 GLOB），三个目标共用：
#
#     text_game       游戏本体（只剩 main.cpp）
#     validate_levels 关卡校验工具
#     unit_tests      单元测试
#
# 以前是"顶层 GLOB + validate_levels 手写 3 个文件 + tests/CMakeLists.txt
# 手写几十行"三份清单。手写那两份漏一个文件，本地可能没事、换个平台就是
# 链接错误 —— macOS 上漏掉 gamepad_vibration_stub.cpp 就是这么挂的，
# 而且当时还专门加了个 GLOB 打补丁。
#
# 现在漏不了：新文件进了上面的 GLOB 就自动进这三个目标。
add_library(text_game_core STATIC ${TEXTGAME_SOURCES})

# 这些 .o 既要进 PIE 可执行文件，也（将来）可能进共享库，统一开 PIC 省心
set_target_properties(text_game_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 注意：utils / infrastructure 两层刻意**不**加入下面的 include 路径。
# 不在路径里，就只能写限定形式 #include "utils/utf8.h"，
# 于是跨层依赖在 include 行上直接可见、也能被 grep 出来，
# 而不是像平铺写法那样"看着像同层"。老层保留平铺写法是为了不制造无谓改动。
target_include_directories(text_game_core PUBLIC
    include
    include/common
    include/core
    include/config
    include/ui
    include/game
    include/scene
    include/log
)

string(TIMESTAMP BUILD_DATE "%Y-%m-%d")

target_compile_definitions(text_game_core PUBLIC
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    PROJECT_VERSION="${PROJECT_VERSION}"
    BUILD_DATE="${BUILD_DATE}"
)

if(WIN32)
    target_compile_definitions(text_game_core PUBLIC TEXTGAME_PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(text_game_core PUBLIC TEXTGAME_PLATFORM_MACOS)
elseif(UNIX)
    target_compile_definitions(text_game_core PUBLIC TEXTGAME_PLATFORM_LINUX)
endif()

find_package(OpenGL REQUIRED)

target_link_libraries(text_game_core PUBLIC
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    OpenGL::GL
)

# Windows 的振动实现用 XInput。MSVC 靠源码里的
# #pragma comment(lib, "xinput.lib") 自动链接，MinGW 没这个机制、
# 必须显式给。放在库里 PUBLIC，所有消费者就都有了 ——
# 以前只在 text_game 上写了这一句，MinGW 下 unit_tests 会链接失败。
if(WIN32)
    target_link_libraries(text_game_core PUBLIC xinput)
endif()

# ============================================================
# 主可执行文件：只剩 main.cpp
# ============================================================

# ⭐ Windows 必须先启用 RC 语言（在任何 target_sources 之前）
if(WIN32)
    enable_language(RC)
endif()

add_executable(text_game src/core/main.cpp)
target_link_libraries(text_game PRIVATE text_game_core)

# ⭐ Windows: 添加 .rc 资源文件（在 add_executable 之后）
if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/packaging/windows/text-game.rc")
    target_sources(text_game PRIVATE packaging/windows/text-game.rc)
endif()

# ⭐ Windows: 隐藏控制台窗口（Release / RelWithDebInfo / MinSizeRel）
#   注意用生成器表达式，避免 Debug 下也隐藏
if(WIN32)
    set_target_properties(text_game PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)

    # ⭐ MSVC 下 WIN32_EXECUTABLE 会要求 WinMain，但我们只有 main()
    #    用 /ENTRY:mainCRTStartup 强制用 main 作为入口
    #    MinGW 不需要（libmingw32.a 自带桥接）
    if(MSVC)
        target_link_options(text_game PRIVATE
            "$<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>")
    endif()
endif()

# ⭐ macOS: bundle
if(APPLE)
    set_target_properties(text_game PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "TEXT-GAME"
    )
endif()

# ============================================================
# install：可执行文件（三平台各自处理）
# ============================================================
if(APPLE)
    install(TARGETS text_game
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION .)
elseif(WIN32)
    # ⭐ 关键：用 $<TARGET_FILE:...> + RENAME 强制 .exe 后缀，
    #   绕过 NSIS 有时会剥离扩展名的 bug
    install(FILES $<TARGET_FILE:text_game>
            DESTINATION .
            RENAME text_game.exe
            COMPONENT Runtime)
else()
    install(TARGETS text_game
            RUNTIME DESTINATION .
            COMPONENT Runtime)
endif()

# ============================================================
# Linux / macOS: 打包运行时依赖库
# ============================================================
#
# Windows 的 vcpkg 构建会拷 DLL，AppImage 也自带一整套 .so —— 只有 Linux 的
# deb / rpm / tar.gz 一直是"裸二进制 + 假设系统装了 SFML"。而这个假设在
# Debian / Ubuntu 上根本不成立：**SFML 3 还没进它们的仓库**，所以打出来的包
# 装上能启动、一跑就报 "libsfml-graphics.so.3.1: cannot open shared object"。
# macOS 那份同理，一直要用户自己 `brew install sfml`。
#
# 打开这个开关就把依赖一起打进包里。
# Linux：给可执行文件设 $ORIGIN/lib 的 RPATH + 启动器设 LD_LIBRARY_PATH。
# 注意 **GL 驱动相关的库绝不能打**（libGL / libGLX / libGLdispatch / libOpenGL /
# libEGL / libdrm / libgbm）—— 那些和显卡驱动绑定，打进去换台机器就崩。
# glibc 家族同理，必须用系统那份。
# macOS：Mach-O 一个字节都不改，全靠启动器设 DYLD_LIBRARY_PATH（见下面的注释）。
option(BUNDLE_RUNTIME_DEPS "把运行时依赖库一起打进包（让 Linux / macOS 包自带 SFML）" OFF)

if(BUNDLE_RUNTIME_DEPS AND UNIX AND NOT APPLE)
    set_target_properties(text_game PROPERTIES
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH FALSE)

    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES "$<TARGET_FILE:text_game>"
            RESOLVED_DEPENDENCIES_VAR _deps
            UNRESOLVED_DEPENDENCIES_VAR _unresolved
            PRE_EXCLUDE_REGEXES
                "^ld-linux.*"
                "^libc\\.so"
                "^libm\\.so"
                "^libdl\\.so"
                "^libpthread\\.so"
                "^librt\\.so"
                "^libgcc_s\\.so"
                "^libstdc\\+\\+\\.so"
            POST_EXCLUDE_REGEXES
                ".*/ld-linux.*"
                ".*/libc\\.so.*"
                ".*/libm\\.so.*"
                ".*/libdl\\.so.*"
                ".*/libpthread\\.so.*"
                ".*/librt\\.so.*"
                ".*/libgcc_s\\.so.*"
                ".*/libstdc\\+\\+\\.so.*"
                # GL：跟显卡驱动绑定，必须用系统那份
                ".*/libGL\\.so.*"
                ".*/libGLX.*"
                ".*/libGLdispatch.*"
                ".*/libOpenGL\\.so.*"
                ".*/libEGL\\.so.*"
                ".*/libgbm\\.so.*"
                ".*/libdrm\\.so.*"
                # 系统服务/驱动层，各发行版差异大，不掺和
                ".*/libudev\\.so.*"
                ".*/libsystemd\\.so.*"
        )
        # FOLLOW_SYMLINK_CHAIN 是必须的：GET_RUNTIME_DEPENDENCIES 给出的往往是
        # 软链（libsfml-graphics.so.3.1 -> libsfml-graphics.so.3.1.0），
        # 不加这个选项就只拷软链、不拷目标文件，装出来是个**断链** ——
        # 加载器找不到就静默回退到系统库，在开发机上一切正常，
        # 到了没装 SFML 的机器上照样起不来。
        foreach(_dep ${_deps})
            file(INSTALL "${_dep}"
                 DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
                 FOLLOW_SYMLINK_CHAIN)
        endforeach()
        list(LENGTH _deps _n)
        message(STATUS "已捆绑 ${_n} 个运行时依赖库到 lib/")
        if(_unresolved)
            message(WARNING "有未解析的依赖，可能是构建机的 RPATH 问题: ${_unresolved}")
        endif()
    ]])

    # 只设可执行文件的 RPATH 不够：间接依赖（SFML 依赖的 freetype / harfbuzz /
    # X11 …）用的是【那个库自己的 RUNPATH】，包里那些库没设，于是会回退到系统。
    # 所以额外装一个启动器，靠 LD_LIBRARY_PATH 让直接 + 间接依赖都走包内。
    install(PROGRAMS "${CMAKE_SOURCE_DIR}/packaging/linux/launch.sh"
            DESTINATION .
            RENAME TEXT-GAME
            COMPONENT Runtime)

elseif(BUNDLE_RUNTIME_DEPS AND APPLE)
    # macOS：依赖收进 <bundle>/Contents/Frameworks/，改写 install name，
    # 最后 ad-hoc 重签名。
    #
    # 为什么必须"改写 + 重签名"成套做：
    #   Apple Silicon 要求一切可执行代码都带有效签名（ld 会自动加 ad-hoc），
    #   而 install_name_tool 一写文件签名就失效，内核随后直接 SIGKILL
    #   （"Code Signature Invalid"）。所以改写完必须 codesign --force --sign -
    #   重新签一遍，只做前半截等于把包做坏。
    #
    # 为什么不用 DYLD_LIBRARY_PATH + 启动器：
    #   那样只有从终端跑才拿得到环境变量，Finder 双击 .app 是没有的。
    #   改写 install name 之后双击也能跑，还省掉一个启动器脚本。
    #
    # ⚠️ 资源必须在**签名之前**装进 bundle：签名会给 bundle 盖封条
    #    （Contents/_CodeSignature/CodeResources），签完再往里塞文件，
    #    封条就盖不住它们了。所以这两条 install 规则必须在 install(CODE) 前面
    #    —— 文件里的顺序就是执行顺序。
    #
    # ⚠️ 目标必须是 Contents/Resources/，不能是 Contents/MacOS/。
    #    放 MacOS/ 里 codesign 会把那些目录当成嵌套代码去验签，直接失败：
    #      code object is not signed at all
    #      In subcomponent: .../Contents/MacOS/wallpaper/CREDITS.md
    #    Paths::resourceRootFor() 就是为这个布局加的。
    #
    # bundle 目录名 = <OUTPUT_NAME>.app（MACOSX_BUNDLE_BUNDLE_NAME 只写进
    # Info.plist，不影响目录名），所以从 target 属性取，不写死。
    # 写死过一次 TEXT-GAME.app，结果包里凭空多出一个只装资源的空 bundle。
    get_target_property(_tg_out_name text_game OUTPUT_NAME)
    if(NOT _tg_out_name)
        set(_tg_out_name text_game)
    endif()
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/"
            DESTINATION "${_tg_out_name}.app/Contents/Resources/assets"
            COMPONENT Runtime)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/wallpaper/"
            DESTINATION "${_tg_out_name}.app/Contents/Resources/wallpaper"
            COMPONENT Runtime)

    install(CODE [[
        # ⚠️ bundle 的目录名不是 target 名 —— MACOSX_BUNDLE_BUNDLE_NAME 只写进
        #    Info.plist，不影响 .app 的目录名（实际是 text_game.app）。所以这里
        #    一律从 install 的结果里问出来，不写死。
        file(GLOB _bundles "${CMAKE_INSTALL_PREFIX}/*.app")
        if(NOT _bundles)
            message(FATAL_ERROR "install 之后在 ${CMAKE_INSTALL_PREFIX} 里找不到 .app")
        endif()
        list(GET _bundles 0 _bundle)
        list(LENGTH _bundles _bn)
        if(NOT _bn EQUAL 1)
            # 包根出现第二个 .app，通常就是某处把 bundle 目录名写死了
            message(FATAL_ERROR "包根下应当只有 1 个 .app，实际 ${_bn} 个: ${_bundles}")
        endif()
        set(_bin "${_bundle}/Contents/MacOS/$<TARGET_FILE_NAME:text_game>")
        if(NOT EXISTS "${_bin}")
            message(FATAL_ERROR "bundle 里找不到可执行文件: ${_bin}")
        endif()
        set(_fw "${_bundle}/Contents/Frameworks")

        # /usr/lib 与 /System 是系统自带的（真机上走 dyld 共享缓存），不打
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES "$<TARGET_FILE:text_game>"
            RESOLVED_DEPENDENCIES_VAR _deps
            UNRESOLVED_DEPENDENCIES_VAR _unresolved
            PRE_EXCLUDE_REGEXES
                "^/usr/lib/.*"
                "^/System/.*"
            POST_EXCLUDE_REGEXES
                ".*/libSystem\\..*"
        )

        file(MAKE_DIRECTORY "${_fw}")
        foreach(_dep ${_deps})
            # FOLLOW_SYMLINK_CHAIN：依赖给出来的往往是软链
            # （libfreetype.6.dylib -> libfreetype.6.20.1.dylib），不加这个
            # 选项只拷软链、不拷目标，装出来是断链 —— 而且断得很难查。
            file(INSTALL "${_dep}" DESTINATION "${_fw}" FOLLOW_SYMLINK_CHAIN)
        endforeach()
        list(LENGTH _deps _n)
        message(STATUS "已捆绑 ${_n} 个 dylib 到 Contents/Frameworks/")
        if(_unresolved)
            message(WARNING "有未解析的依赖，可能是构建机的 RPATH 问题: ${_unresolved}")
        endif()

        # 把包内某段引用换成包内相对路径。
        # prefix: 主程序用 @executable_path/../Frameworks，库之间用 @loader_path
        function(_tg_rewrite target prefix)
            execute_process(COMMAND otool -L "${target}" OUTPUT_VARIABLE _out)
            string(REPLACE "\n" ";" _lines "${_out}")
            if(_lines)
                list(REMOVE_AT _lines 0)      # 第一行是文件自己的名字
            endif()
            foreach(_line ${_lines})
                string(REGEX MATCH "^[ \t]+([^ \t]+)" _m "${_line}")
                set(_ref "${CMAKE_MATCH_1}")
                get_filename_component(_refname "${_ref}" NAME)
                if(EXISTS "${_fw}/${_refname}")
                    execute_process(COMMAND install_name_tool -change
                                    "${_ref}" "${prefix}/${_refname}" "${target}")
                endif()
            endforeach()
        endfunction()

        # 先改库：库自己的 id + 库与库之间的引用
        file(GLOB _fw_libs "${_fw}/*.dylib")
        foreach(_lib ${_fw_libs})
            get_filename_component(_name "${_lib}" NAME)
            execute_process(COMMAND install_name_tool -id "@rpath/${_name}" "${_lib}")
            _tg_rewrite("${_lib}" "@loader_path")
        endforeach()

        # 再改主程序：指向 Frameworks 里那份
        _tg_rewrite("${_bin}" "@executable_path/../Frameworks")

        # 兜底自检：改写完不该再剩下任何指向系统之外的绝对路径。
        # 漏一个，用户机器上就是一句 "Library not loaded" 且很难查。
        foreach(_f "${_bin}" ${_fw_libs})
            execute_process(COMMAND otool -L "${_f}" OUTPUT_VARIABLE _o)
            string(REPLACE "\n" ";" _ls "${_o}")
            if(_ls)
                list(REMOVE_AT _ls 0)
            endif()
            foreach(_l ${_ls})
                string(REGEX MATCH "^[ \t]+([^ \t]+)" _m "${_l}")
                set(_r "${CMAKE_MATCH_1}")
                if(_r MATCHES "^/usr/lib/" OR _r MATCHES "^/System/")
                    continue()
                endif()
                if(_r MATCHES "^/")
                    message(FATAL_ERROR "install name 没改写干净: ${_f} -> ${_r}")
                endif()
            endforeach()
        endforeach()

        # 改写过的每一个 Mach-O 都要重签名，否则 arm64 上内核拒绝启动。
        #
        # ⚠️ 顺序要紧：**先签库，最后才签主程序**。
        #    在 bundle 里签主程序会把整个 bundle 盖章 —— codesign 会生成
        #    Contents/_CodeSignature/CodeResources，里面记着每个嵌套代码的哈希。
        #    要是反过来先签主程序，之后再签那 16 个 dylib，封条立刻对不上：
        #      nested code is modified or invalid
        #    实测就是这么挂的。
        foreach(_lib ${_fw_libs})
            execute_process(COMMAND codesign --force --sign - "${_lib}"
                            RESULT_VARIABLE _rc)
            if(NOT _rc EQUAL 0)
                message(FATAL_ERROR "重签名失败(${_rc}): ${_lib}")
            endif()
        endforeach()
        execute_process(COMMAND codesign --force --sign - "${_bin}"
                        RESULT_VARIABLE _rc)
        if(NOT _rc EQUAL 0)
            message(FATAL_ERROR "重签名失败(${_rc}): ${_bin}")
        endif()
        message(STATUS "已改写 install name 并 ad-hoc 重签名")
    ]])
endif()

# ============================================================
# Windows: 打包 SFML 及依赖 DLL
# ============================================================
if(WIN32)
    if(DEFINED VCPKG_INSTALLED_DIR)
        set(_vcpkg_bin "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    else()
        get_target_property(_sfml_loc SFML::Graphics IMPORTED_LOCATION_RELEASE)
        if(NOT _sfml_loc)
            get_target_property(_sfml_loc SFML::Graphics IMPORTED_LOCATION)
        endif()
        get_filename_component(_vcpkg_bin "${_sfml_loc}" DIRECTORY)
        get_filename_component(_vcpkg_bin "${_vcpkg_bin}" DIRECTORY)
        set(_vcpkg_bin "${_vcpkg_bin}/bin")
    endif()

    message(STATUS "vcpkg DLL 目录: ${_vcpkg_bin}")

    if(EXISTS "${_vcpkg_bin}")
        install(DIRECTORY "${_vcpkg_bin}/"
                DESTINATION .
                COMPONENT Runtime
                FILES_MATCHING
                PATTERN "sfml-*.dll"
                PATTERN "openal32.dll"
                PATTERN "*.dll")
    else()
        message(WARNING "找不到 vcpkg DLL 目录: ${_vcpkg_bin}")
    endif()
endif()

# ============================================================
# 资源文件
# ============================================================
#
# ⚠️ macOS 上资源必须装进 bundle 里，不能放 tar 根目录。
#    Paths 判定"打包模式"看的是**可执行文件旁边**有没有 assets/
#    （include/core/paths.h:20），而 macOS 的可执行文件在
#    <bundle>/Contents/MacOS/ 下。装到根目录等于找不到 —— 那样 Paths 会退回
#    "开发模式"，去用编译期的 PROJECT_ROOT（构建机的路径），用户机器上根本
#    不存在，字体和关卡都加载不了。
#
#    但这里**不能写死 bundle 目录名**：MACOSX_BUNDLE_BUNDLE_NAME 只影响
#    Info.plist，.app 的目录名来自 target 名（实际是 text_game.app）。
#    写死 TEXT-GAME.app 会凭空多出一个只有资源的空 bundle —— 真发生过。
#    所以 macOS 那段挪进下面的 install(CODE)，用 glob 出来的真实 bundle 路径。
if(NOT APPLE)
    install(DIRECTORY assets/    DESTINATION assets    COMPONENT Runtime)
    install(DIRECTORY wallpaper/ DESTINATION wallpaper COMPONENT Runtime)
endif()
install(FILES README.md LICENSE DESTINATION .      COMPONENT Runtime)

# ============================================================
# CPack
# ============================================================
set(CPACK_PACKAGE_NAME        "TEXT-GAME")
set(CPACK_PACKAGE_VENDOR      "ljm-233")
set(CPACK_PACKAGE_VERSION     "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "A 2D platformer built from scratch in C++20 + SFML 3")
set(CPACK_PACKAGE_CONTACT     "ljm-233")

if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "TEXT-GAME-${PROJECT_VERSION}-Windows")
    set(CPACK_GENERATOR "NSIS")

    set(CPACK_NSIS_DISPLAY_NAME "TEXT-GAME")

    if(EXISTS "${CMAKE_SOURCE_DIR}/packaging/icons/text-game.ico")
        # ⚠️ 这里**不能**设 CPACK_PACKAGE_ICON。
        #    NSIS 生成器把它当成 MUI_HEADERIMAGE_BITMAP 用，而那个宏只收 BMP。
        #    喂 .ico 进去会让 makensis 直接 abort：
        #      !define: "MUI_HEADERIMAGE_BITMAP"= ".../text-game.ico"
        #      File: ".../text-game.ico" -> no files found.
        #      Error in macro MUI_HEADERIMAGE_INIT ... aborting creation process
        #    安装程序的图标走 CPACK_NSIS_MUI_ICON（那个才是收 .ico 的），
        #    顶部横幅图留着 NSIS 默认的就行；真要换得先有张 BMP。
        set(CPACK_NSIS_MUI_ICON    "${CMAKE_SOURCE_DIR}/packaging/icons/text-game.ico")
    endif()

    # 开始菜单 / 桌面快捷方式
    set(CPACK_PACKAGE_EXECUTABLES "text_game" "TEXT-GAME")
    set(CPACK_CREATE_DESKTOP_LINKS "text_game")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "text_game.exe")
    set(CPACK_NSIS_SHORTCUTS "TEXT-GAME" "text_game.exe")
elseif(APPLE)
    set(CPACK_PACKAGE_FILE_NAME "TEXT-GAME-${PROJECT_VERSION}-macOS")
    set(CPACK_GENERATOR "TGZ;ZIP")
else()
    set(CPACK_PACKAGE_FILE_NAME "TEXT-GAME-${PROJECT_VERSION}-Linux")
    set(CPACK_GENERATOR "TGZ;ZIP")
endif()

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

# ⚠️ macOS 上绝不能 strip。
#    Apple Silicon 要求一切可执行代码都带有效签名（CMake/ld 会自动加 ad-hoc
#    签名），而 strip 改完文件签名就对不上了 —— 内核直接 SIGKILL
#    （"Code Signature Invalid"），不是报个错而是连启动都启动不了。
#    Homebrew 在 ARM 上同样不 strip，就是同一个原因。
#    代价是符号留着、包大几 MB，换的是"在真机上能跑"。
if(APPLE)
    set(CPACK_STRIP_FILES FALSE)
else()
    set(CPACK_STRIP_FILES TRUE)
endif()

include(CPack)

#========================================================
# 老设备支持
#========================================================
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        # 针对 Intel Atom (Bonnell) 架构优化
        target_compile_options(text_game PRIVATE
            -march=bonnell
            -mtune=bonnell
            -O2  # 从 -O3 降级到 -O2，减少代码膨胀
            -fno-omit-frame-pointer  # 方便 perf 分析
            -fno-plt
        )
        # 链接时优化
        target_link_options(text_game PRIVATE -flto=auto)
    endif()
endif()

# ============================================================
# 关卡验证工具（独立目标）
# ============================================================
# 以前这里手写了 3 个源文件 + 3 个 include 目录，是源码清单的第二份。
# 现在只链 text_game_core —— 清单只剩顶层那一份。
add_executable(validate_levels tools/validate_levels.cpp)
target_link_libraries(validate_levels PRIVATE text_game_core)

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
                COMMAND ${LCOV_EXE} --zerocounters --directory ${CMAKE_BINARY_DIR}
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${LCOV_EXE} --capture --directory ${CMAKE_BINARY_DIR}
                        --output-file coverage.info
                        --rc branch_coverage=1 --ignore-errors mismatch
                COMMAND ${LCOV_EXE} --remove coverage.info
                        '/usr/*' '*/tests/doctest.h' '*/tests/*' '*/_deps/*'
                        --output-file coverage_filtered.info
                        --rc branch_coverage=1 --ignore-errors unused
                COMMAND ${GENHTML_EXE} coverage_filtered.info
                        --output-directory coverage_html
                        --branch-coverage --legend
                COMMAND ${CMAKE_COMMAND} -E echo
                        "覆盖率报告: file://${CMAKE_BINARY_DIR}/coverage_html/index.html"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "生成覆盖率报告"
                USES_TERMINAL
            )

            message(STATUS "  运行: cmake --build ${CMAKE_BINARY_DIR} --target coverage")
        else()
            message(WARNING "未找到 lcov / genhtml，coverage target 未创建")
        endif()
    else()
        message(WARNING "覆盖率只在 GCC / Clang 上支持，已忽略")
    endif()
endif()

# ============================================================
# 单元测试（-DBUILD_TESTS=ON 开启）
# ============================================================
option(BUILD_TESTS "Build unit tests" OFF)

if(COVERAGE AND NOT BUILD_TESTS)
    message(WARNING "COVERAGE=ON 但 BUILD_TESTS=OFF，无法生成有意义覆盖率报告")
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "=== 单元测试已启用 ===")
endif()

message(STATUS "=== 构建配置完成 ===")