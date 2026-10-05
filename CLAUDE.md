# CLAUDE.md

给 AI 助手的项目说明。新会话请先读这个文件。

## 项目概览

`TEXT-GAME` —— 用 C++20 + SFML 3 从零手写的 2D 平台跳跃游戏。
不依赖游戏引擎，物理、UI、渲染、音频全部自研。

## 构建 / 运行 / 测试

```bash
# 一键启动（增量构建 Release 后直接进游戏，日常就敲这个）
./s.sh
./s.sh --debug        # 要接调试器时

# 生成源码快照 project_dump.txt（给 AI 喂上下文用）
./dump.sh

# Debug 构建
cmake --preset debug
cmake --build --preset debug
./build/debug/text_game

# Release 构建（**跑游戏和编辑器请用 Release**）
cmake --preset release
cmake --build --preset release
./build/release/text_game

# 单元测试（scripts/test.sh 等价于下面三条）
cmake --preset tests
cmake --build --preset tests
ctest --preset tests

# 关卡可达性验证
./build/debug/validate_levels assets/levels

# Sanitizer
cmake --preset asan
cmake --build --preset asan
./build/asan/tests/unit_tests
```

> ⚠️ **Debug 构建下 SFML 的 `sf::Text` 和 `sf::Shape` 构造极慢**。编辑器帧率可能只有 20~30fps。**跑游戏和编辑器请用 Release**。

## 目录结构

```
include/ 和 src/ 一一对应，共 9 个层（从叶子到顶层）：

  utils/          零内部依赖的叶子：utf8 / lang / text_strings / vec2 /
                  animation / animator
  common/         共享基元：AppError / ContainerError
  infrastructure/ 设备层：Gamepad / GamepadVibration / KeyBindings
  config/         配置（header-only）+ keys.h（配置键的唯一权威来源）
                  + bootstrap（registerConfig）
  log/            日志三层：Logger / LogFormatter / LogHandler
                  + formatters/ + handlers/ + rotation + bootstrap
  core/           Container / Application / MainLoop / SceneRegistry /
                  SceneManager / Game / Paths / Platform / Achievement
  ui/             UI 组件与渲染（Button / Slider / TextInput / Window /
                  Upscaler / PostProcessor / Console / SoundManager /
                  Notification / Theme / ParticleSystem ...）
  game/           游戏本体（Player / Enemy / Level / GameWorld / Camera /
                  LevelValidator / LevelCodec / SaveManager ...）
  scene/          8 个场景 + console/（控制台命令）

  scene/tabs/     6 个设置 Tab（AudioTab / GraphicsTab / InterfaceTab /
                  DisplayTab / OtherTab / KeysTab）
tests/            doctest 单元测试（含分层架构测试）
tools/            validate_levels 命令行工具
assets/
  levels/*.txt    ASCII 关卡
  shaders/*.frag  GLSL 着色器（5 个）
  lang/*.txt      翻译文件
```

## 分层架构

```
  scene                 ← 具体场景
    ↑
  ui    game            ← 兄弟层，双向都不许依赖
    ↑
  core                  ← Container / Application / 场景契约
    ↑
  config   log
    ↑
  common  infrastructure  utils   ← 叶子层
```

**分层规则由测试守护，不是靠自觉**：`tests/test_layers.cpp` 会扫描全部
`include/` 与 `src/`，把 `#include` 解析成"哪一层指向哪一层"并比对禁止表。
违规会让 `ctest` 直接失败并报出 `文件:行号`。

- `ui` 与 `game` **双向**都不许互相 include（`GameWorld` 通过 `EventBus` 发事件，
  `GameScene` 订阅后处理音效/粒子/振动；粒子系统归 `GameScene` 所有，
  `GameWorld` 只管发事件）
- `utils` / `common` 是零内部依赖的叶子
- `infrastructure` 是设备层，不认识应用层与前端
- 新增一层时记得同步 `test_layers.cpp` 里的 `kLayers` 与 `kRules`，
  否则会有一条测试提醒你

### 装配与生命周期

`main()` 只做三件事：装配、跑生命周期、交退出码。

```cpp
app.boot([](Application& a) {
    registerConfig(a.container());  // 各层自带 bootstrap，底层在前
    registerLog(a.container());
    registerCore(a.container());
    registerScenes(a.container());
    wireCore(a);                    // 挂钩子，此处不执行
});
code = app.exec();
```

- **`Container`**：命名式注册表，`reg` / `get` / `require` / `tryGet` / `peek` /
  `touch`。只管注册与解析，**不认识生命周期**
- **`Application::exec()` 五阶段**：boot → setup → wire → run → teardown，
  teardown 用 RAII 守卫保证必达
- **四类钩子**：`onBoot` / `onLoop` / `onQuit` / `onFinal`，先注册先执行
- **`MainLoop`**：前端交出主循环的入口，`Game` 就是它的实现；没有前端时
  自动回退 `HeadlessMainLoop`
- **boot 阶段必须无副作用**（只放工厂）。偏好生效、子系统初始化都在
  `wireCore` 挂的 `onBoot` 钩子里做

把 `log_level` 调到 Debug 可以看到装配快照（容器、日志层、偏好、场景注册表）。

## 关键约定

1. **新增 `src/` 一级子目录时要改 `CMakeLists.txt` 的 GLOB 列表**，`.cpp` 文件本身会被自动扫描。`src/scene/tabs/*.cpp` 已在 GLOB 里。
2. **头文件用 `#pragma once`，`.cpp` 首行必须是 `#include`**。
3. **中文必须走 `toSf()`**（`include/utils/utf8.h`），否则 SFML 3 按 Latin-1 解释。
4. **字号用 `scaledFontSize()`**（`include/ui/ui_scale.h`），跟随 UI 缩放。
5. **颜色从 `getTheme()` 取**（`include/ui/theme.h`），不硬编码。
6. **配置读写走 `Config::set*/get*`**，写盘由 `flush()` 统一处理。
   **键名必须用 `include/config/keys.h` 的常量**，不要再写字面量 ——
   写错字只会静默回退默认值，不会报错。
7. **物理常量放 `include/game/game_constants.h`**（只放游戏自己的常量，
   设备属性不要塞进来）。
8. **每帧渲染用 `screenView`**，不用 `getDefaultView()`。
9. **UI 文字走 `Str::T(Str::Xxx)`**，字符串定义在 `include/utils/text_strings.h`。
10. **游戏事件走 EventBus**，不要从 GameWorld 直接调 SoundManager / ParticleSystem / Gamepad。
11. **着色器放 `assets/shaders/`**，`.frag` 后缀，GLSL 330 core。
12. **手柄焦点导航**：在**状态变化时**注册焦点（`onEnter` / `onResume` / `update` / 自己的 `syncFocus()`），
    **不要放在 `render()` 里** —— 渲染函数不该有副作用。设置 Tab 由 `registerFocus()` 收集。
13. **日志用 `log/logger.h` 的 `Logger`**（`logger->info(...)`），
    不要再用已删除的 `core/logging.h` 转发头。
14. **每层自带一个 bootstrap**（`registerXxx(Container&)`），入口的
    `boot()` 只负责按"底层在前"的顺序调用它们。
15. **跨层 include 用限定路径**（`utils/utf8.h`、`infrastructure/gamepad.h`）。
    `include/utils` 与 `include/infrastructure` **刻意不在 include 路径里**，
    这样平铺写法根本编译不过，跨层依赖在 include 行上直接可见。

## 常见任务入口

| 想改什么 | 去哪 |
| :--- | :--- |
| 玩家手感（速度/重力/跳跃） | `include/game/game_constants.h` |
| 添加新游戏对象 | `include/game/` + `src/game/`，在 `GameWorld::spawnLevelObjects` 和 `checkCollisionsSafe` 注册 |
| 添加新场景 | `include/scene/` + `src/scene/`，在 `scene_id.h` 加枚举 + 在 `registerScenes()`（`src/scene/bootstrap.cpp`）加一行工厂。**不用改 Game** |
| **添加新设置项** | 在对应 Tab（`include/scene/tabs/xxx_tab.h/cpp`）加成员 + 创建控件 + update 回调。**不用改 SettingsScene** |
| 添加新事件 | `include/game/event_bus.h` 加 struct + 加入 variant，然后 `GameWorld` emit + `GameScene` 订阅 |
| 添加关卡 | `assets/levels/levelN.txt`，参考已有格式；用 `validate_levels` 验证 |
| 添加主题 | `include/ui/theme.h` 加枚举 + `src/ui/theme.cpp` 加颜色组 |
| 添加成就 | `src/core/achievement.cpp` 的 `kAchievements` 加一行 + `include/utils/text_strings.h` 加名称/描述 |
| 添加画面预设 | `src/scene/tabs/graphics_tab.cpp` 的 `kPresets` 加一行 + `presetRow_` 加按钮 |
| 添加后处理效果 | `assets/shaders/postprocess.frag` 加 uniform + `include/ui/postprocess.h` 加 setter + SettingsScene → GraphicsTab 加滑条 |

## 架构要点

### SceneManager：场景常驻

**场景只构造一次，之后复用**（避免 sf::Text 频繁析构触发 SFML 3.1.0 HarfBuzz 死锁）。

- `onEnter()`：首次进入 + 每次重新进入都调用
- `onResume()`：从 pop 恢复时调用
- `onPause()`：**离开本场景**时调用（start 换掉当前 / push / pop / replace 都走它）
- **`onExit()` 只在程序退出时调用 —— 它不是"离开场景"**

⚠️ **离开场景的清理必须挂在 `onPause`**。`ConsoleScene` 踩过这个坑：它把
"恢复 cin/cout/cerr + 停 worker 线程"只写在 `onExit` 里，于是离开控制台后标准流
一直被劫持、线程一直活着，而且因为装配有 `if (!redirect_)` 判断，再进去看着还是
正常的 —— 完全静默。这条契约由 `tests/test_scene_manager.cpp` 守着。

场景状态必须能在 `onEnter` 里重置（如 `GameScene::onEnter` 会检查 pending save）。

### SettingsScene：Tab 架构

6 个 Tab 都是**自包含类**，各自持有：
- 状态变量
- 控件（unique_ptr）
- `handleEvent` / `update` / `render` / `refreshLabels` / `refreshSelection` / `registerFocus` / `anyEditing`

SettingsScene 只负责：
- Tab 切换（6 个 `tabButtons_`）
- 分发事件到当前 Tab
- 底部按钮（返回 / 关于 / 重置）
- 设计坐标系 View + 滚动

**加设置项只需在对应 Tab 改 1~2 处**。

### 渲染管线

```
Scene 绘制 ──> RenderTexture (rt_)
                    │
                    ├─ renderScale < 1.0 ──> [Upscaler: 双三次/FSR1]
                    ├─ renderScale > 1.0 ──> [超采样降采样]
                    └─ [PostProcessor: 色彩分级 + 泛光 + 色差 + ...]
                                    │
                                    v
                              window.display()
```

`Window` 通过 `needsRT()` 决定是否走中间 RT。
`Upscaler` / `PostProcessor` 只在需要时激活。

### 成就系统

`AchievementManager::instance()` 是**跨存档全局单例**，状态存 `config/achievements.conf`。
解锁时自动弹通知，数据是**只增不减**的。

### 手柄振动

SFML 3 移除了振动 API。项目通过 `GamepadVibration` 单例直接调底层：
- Linux：evdev `/dev/input/event*` + `EVIOCSFF`
- Windows：XInput `XInputSetState`
- 其它平台（macOS 等）：`gamepad_vibration_stub.cpp` 空实现，保证能编译链接

已在 `GameScene` 接在 8 个游戏事件上（跳跃/落地/金币/踩敌/受伤/弹跳板/…），
设置里有开关。**这个功能是完成的，README 里"待接入"的说法已过时。**

`Gamepad::vibrate(low, high, duration)` 是入口，内部有正弦包络（0 → 1 → 0）。

## 已知陷阱

### SFML 3.1.0 相关

- ⚠️ **`sf::Text` 析构会死锁**（HarfBuzz + FreeType 清理时 pthread_mutex_lock）。
  - **任何频繁创建/销毁 `sf::Text` 的地方都是定时炸弹**。
  - 场景改成常驻、`LevelIntro` 改成复用、编辑器笔刷条和图标预渲染到 RT。
  - **新代码不要每帧构造 `sf::Text`**，一律做成成员变量。
- ⚠️ **SFML 3 没有振动 API**，自己通过 `GamepadVibration` 调系统。
- ⚠️ **SFML 3 移除了 `RenderWindow::capture()`**，用 `glReadPixels` 自己实现（需要 `find_package(OpenGL REQUIRED)` + `OpenGL::GL`）。

### 跨平台（CI 会在 4 个平台构建）

CI 矩阵：Arch（系统包 SFML **3.1**）、Ubuntu（源码编译 SFML **3.0**）、
macOS（Homebrew SFML **3.0**）、Windows（vcpkg SFML **3.0**）。

`build.yml` 有 **6 个 job**（前 4 个是平台构建，后 2 个是质量门）：

| job | 干什么 |
| :--- | :--- |
| Arch Linux | 系统 SFML 3.1，构建 + 跑单测 |
| Ubuntu 24.04 | 从源码编 SFML 3.0，构建 + 跑单测 |
| macOS | Homebrew SFML 3.0，构建 + 跑单测 |
| Windows | vcpkg SFML 3.0（MSVC），构建 + 跑单测 |
| **Sanitizer** | Arch 容器 + `-DSANITIZE=ON`（ASan + UBSan + LSan） |
| **clang-tidy** | 只 configure 拿 `compile_commands.json`，再跑 `run-clang-tidy` |

质量门的原则：**闸门必须能过，过不了的闸门等于没有**。

- `.clang-tidy` 只开「能当 BUG 修」的检查，并设 `WarningsAsErrors: '*'`。
  带一堆假阳性的门禁会被无视，所以 `easily-swappable-parameters`、
  `signed-bitwise`、`narrowing-conversions`、`clang-analyzer-optin.*`
  这些"报得没错但改了没意义"的都排除了。
- **加检查之前先确认当前代码能过**。本机跑：
  `run-clang-tidy -p build/release -j $(nproc) src/`（先 configure 出
  `compile_commands.json`）。
- `format.yml` **只检查 PR 改动过的文件**。原因：仓库里 193 个源文件中有
  132 个不符合 `.clang-format`，全仓库扫的写法在任何 PR 上都不可能绿。
  想一次性清掉：
  `clang-format -i $(find src include -name '*.cpp' -o -name '*.h')`
  —— 约 7340 行纯机械改动，建议单独一个 commit。
- `.clang-format` 里 `SortIncludes: Never` 与 `ReflowComments: false` 是**故意**的：
  项目约定是「`.cpp` 首行必须是本文件对应的头」，而 clang-format 的排序会把它
  排到后面去，直接冲突；注释是手写换行（尤其中文，全角宽度算不准）。

⚠️ **本地 GCC 绿灯不代表没问题**。下面几条都是本地正常、CI 连续红了几周才发现的：

- **`sf::Event::getIf` 的非 const 重载是 SFML 3.1 才加的**。3.0.x 只有 const 版
  （返回 `const T*`），写 `ev.getIf<T>()->field = x` 会在 3.0 上编译失败。
  要改事件副本就用 `settings_scene.cpp` 里的 `eventIfMutable<T>()`。
  **brew 和 vcpkg 目前都还停在 3.0.2**，不能假设别人有 3.1。
- **`sf::Image` 别用单参数花括号构造**：`sf::Image img({w, h})` 在 clang/MSVC 上
  与其它重载歧义（GCC 接受）。写 `sf::Image img(sf::Vector2u{w, h})`。
  带 `sf::Color` 参数的写法没有这个问题。
- **doctest 断言里别直接比较智能指针**：MSVC 的 `<memory>` 给 `shared_ptr` 定义了
  `operator<<`，doctest 的 SFINAE 会选中它；指向的类型不可流输出就硬报错，
  而且错误指向 MSVC 自己的头文件。比较 `.get()` —— 走
  `operator<<(ostream&, const void*)`。
- **每个平台都要有 GamepadVibration 的实现**：Linux(evdev) / Windows(XInput) /
  其余平台兜底（`gamepad_vibration_stub.cpp`）。缺一个就是链接错误。
- **源码只在顶层 CMakeLists 的 GLOB 里列一次**（`text_game_core` 静态库），
  `text_game` / `validate_levels` / `unit_tests` 三个目标共用它。
  以前 `tests/CMakeLists.txt` 里还有一份手写清单，漏一个文件就是链接错误
  （macOS 漏掉 `gamepad_vibration_stub.cpp` 就是这么挂的）—— 已经不存在了。
  新增 `src/` 一级子目录时仍然要往 GLOB 里加一行。

改完这类东西**不要只看本地构建**：`git push` 之后用
`gh run watch` 看 `build.yml` 六个 job 的结果（Arch / Ubuntu / macOS /
Windows / Sanitizer / clang-tidy），这才是唯一能验证的地方。

### 项目自身

- **场景工厂里不要有副作用**（现在是 `registerScenes()` 注册的 lambda）。
  `GameScene` 的 `takePendingSave()` 已挪到 `onEnter()`。
- `EventBus::subscribe` 的回调是同步的，`emit` 时会立刻执行。
- `SettingsScene::anySliderEditing()` 检查所有 Tab 的 `anyEditing()`，用于判断 ESC 是否应该退出场景。
- 场景常驻后 `SceneManager::pop` 不再销毁场景，`history_` 存指针。
- **程序退出时可能会卡 1~2 秒**（缓存场景统一析构），`main.cpp` 用了
  `std::_Exit(code)` 直接终止进程绕过（跳过所有静态/单例析构）。
  **这样不会丢数据**，已经核查过：配置有 `Game::flushConfigs()`
  （初始化时 + 每 5 秒 + 退出时各一次），成就是解锁即 `save()`。
  往析构函数里塞持久化才是危险的 —— `_Exit` 不会执行它们。

### 打包

`BUNDLE_RUNTIME_DEPS` 在**本机 `release-package` preset 与 `release.yml` 两个 job
里都开着**。CI 产的包和本机打的包现在是同一套东西，不用再手工覆盖。

**Linux**：

- 用 `GET_RUNTIME_DEPENDENCIES` 把 SFML 及其依赖收进 `lib/`，并装一个 `TEXT-GAME`
  启动器。**必须用启动器而不是 `text_game`** —— 只设可执行文件的 RPATH 不够，
  间接依赖（SFML 依赖的 freetype / harfbuzz / X11）是用【那个库自己的 RUNPATH】
  解析的，会静默回退到系统库。
- `file(INSTALL)` 要用 `FOLLOW_SYMLINK_CHAIN`：依赖给出来的往往是软链
  （`libsfml-graphics.so.3.1 -> .3.1.0`），不加这个选项只拷软链，装出来是断链，
  开发机上一切正常、纯净机器上起不来。
- **GL 驱动库绝不打包**（`libGL` / `libGLX` / `libGLdispatch` / `libOpenGL` /
  `libEGL` / `libdrm` / `libgbm`），glibc 家族同理 —— 必须用系统那份。

**macOS**（同一条 `BUNDLE_RUNTIME_DEPS` 分支）：

- 依赖进 `text_game.app/Contents/Frameworks/`，**资源进 `Contents/Resources/`**
  （`Paths::resourceRootFor()` 就是为这个布局存在的）。
  **资源绝不能放 `Contents/MacOS/`** —— codesign 会把那里的目录当成嵌套代码
  逐个验签，直接报 `code object is not signed at all`，包都做不出来。
  资源还必须**在签名之前**装进 bundle，否则封条盖不住它们。
- install name 全部改写成 `@executable_path/../Frameworks`（主程序）与
  `@loader_path`（库之间），不改写就仍指向构建机的 `/opt/homebrew/...`。
- **改写完必须 `codesign --force --sign -` 重签名**，两者是一套的：arm64 要求
  一切可执行代码都有有效签名，`install_name_tool` 一写文件签名就失效，内核
  随后 SIGKILL（不是报错，是直接被杀）。
- **先签库、最后签主程序**。在 bundle 里签主程序会给整个 bundle 盖章
  （`Contents/_CodeSignature/CodeResources` 记着每个嵌套代码的哈希），顺序反了
  立刻变成 `nested code is modified or invalid`。
- **`CPACK_STRIP_FILES` 在 `APPLE` 上必须是 `FALSE`**：strip 同样会让签名失效。
  Homebrew 在 ARM 上不 strip 也是这个原因。代价是符号留着、包大几 MB。
- **bundle 目录名 = `<OUTPUT_NAME>.app`**，`MACOSX_BUNDLE_BUNDLE_NAME` 只写进
  Info.plist、不改目录名。所以实际是 `text_game.app` / `text_game`，不是
  `TEXT-GAME.app`。**永远不要在任何地方写死这个名字** —— 写死过一次，结果包里
  凭空多出一个只装资源的空 bundle，而 glob 按字典序正好让它挡住了真的那个。

**通用**：

- `release.yml` 现在有**三个平台 job**，一次出全：
  Linux（tar.gz / zip / deb / rpm / AppImage）、macOS（tar.gz）、
  Windows（NSIS 安装包）。`create-release` 把三个 artifact 目录全挂上去。
- 想在不发版的前提下验证打包：`gh workflow run release.yml`。
  `create-release` 有 `if: startsWith(github.ref, 'refs/tags/')`，手动触发只当演练。
- **每个 job 都有自检步骤，不达标就红**：
  - Linux：`lib/` 库数量 + 带 `LD_LIBRARY_PATH` 的 `ldd` 无 `not found`、
    zip 有内容、deb 的 `Architecture` 非空且不是 i386、rpm 的 `ARCH=x86_64`、
    AppImage 解包后 `usr/lib` 有库且 `ldd` 干净
  - macOS：`.app` 数量为 1、`Contents/Resources/assets` 在、每个 Mach-O
    `codesign --verify` 过、没有残留的绝对依赖路径
  - Windows：安装包存在、安装树里有 `sfml-*.dll` 与 `font.ttf`
  这些检查都是用真实的翻车现场换来的，别删。
- **deb 那一条特别容易踩**：Arch 容器里不装 `dpkg`，CPackDeb 就拿不到
  `dpkg --print-architecture`，打出来的包 `Architecture` 字段是**空的**，
  装不上。同理 rpm 需要 `rpm-tools`。
- **Windows 的 NSIS 有个坑**：`CPACK_PACKAGE_ICON` 会被当成
  `MUI_HEADERIMAGE_BITMAP` 用，而那个宏只收 **BMP**。喂 `.ico` 进去
  makensis 直接 abort（`Error in macro MUI_HEADERIMAGE_INIT`）。
  安装程序图标走 `CPACK_NSIS_MUI_ICON`（那个才是收 .ico 的）。
- **AppImage 脚本不自己维护库名清单**：它先 `cmake --install` 到 staging
  （`BUNDLE_RUNTIME_DEPS` 会把完整闭包放进 `lib/`），再从 staging 组装 AppDir。
  于是五种包共用同一套依赖闭包。CI 里要传 `TEXTGAME_BUILD_DIR=build`，
  因为脚本默认找 `build/release-package`（本机 preset 的目录名）。
- **发行版默认设置放 `assets/defaults/preferences.conf`**，由
  `config/bootstrap.cpp` 在用户没有配置文件时铺一次。**只放观感类设置**，
  个人与机器相关的键（`player_name` / `resolution_index` / `fullscreen` /
  `ui_scale` / `current_wallpaper` / `key_*`）一律不放。

### 编辑器

- 笔刷条和关卡图标预渲染到 `RenderTexture`，静止时每帧仅 3 个 draw call。
- 大关卡（>4096 像素）会自动回退到每帧绘制图标。

## 调试快捷键

游戏内：

| 键 | 功能 |
| :--- | :--- |
| F1 | 调试 HUD（关卡 / 坐标 / 速度 / 鼠标 tile） |
| F2 | 无敌（重生保留） |
| F3 | 清空敌人 |
| F4 | 重载当前关卡 |
| F5 / F6 | 上一关 / 下一关 |
| F7 | 慢动作 1x / 0.5x / 0.25x / 0.1x |
| F8 | 碰撞盒 |
| F9 | 截图到 `./screenshots/` |
| F10 | 性能面板 |

编辑器：

| 键 | 功能 |
| :--- | :--- |
| T | 可达性可视化 |
| G | 网格 |
| Shift + 左键拖动 | 矩形填充 |
| 左键拖动 | 连续绘制 |
| 右键拖动 | 连续擦除 |
| Ctrl+Z / Ctrl+S / Ctrl+N / Ctrl+E / Ctrl+I | 撤销 / 保存 / 切换文件 / 导出分享码 / 导入分享码 |

## 测试现状

| 模块 | 覆盖 |
| :--- | :--- |
| Vec2 / AABB / Config / Level / Camera / Animator / Player / MovingPlatform / JumpPad | ✅ |
| Container / Application 五阶段 / MainLoop / Logger / SceneRegistry | ✅ |
| SceneManager（常驻复用 + 钩子契约，含"离开场景走 onPause 不是 onExit"） | ✅ |
| **分层架构**（`test_layers.cpp` 会扫描真实源码树，违规即失败） | ✅ |
| Enemy / Coin / Checkpoint / Door / Spike / Key | ✅ |
| GameWorld（构造 / 金币 / 踩踏 / 尖刺 / 存档点 / 钥匙 / 终点 / 重生） | ✅ |
| LevelCodec（分享码往返 / 空白容错 / 非法输入 / RLE 边界） | ✅ |
| SaveManager（存档读写 / 只增不减 / PB 取最小 / 旧字段兼容 / 沙箱隔离） | ✅ |
| ScoreRules（星级 / 目标时间 / PB）—— 从 GameScene 抽出的纯逻辑 | ✅ |
| EditorTools（矩形与连线格子几何）—— 从 EditorScene 抽出的纯逻辑 | ✅ |
| FocusNav（手柄焦点导航的几何/线性移动）—— 从 FocusGroup 抽出的纯逻辑 | ✅ |
| Scene / 各设置 Tab / UI 组件本身 | ❌（构造必须有 `sf::Font`，而它是 `GlResource`） |

测试写法：`tests/test_*.cpp`。

**场景与 UI 组件本身测不了，就把纯逻辑抽出来测。** 场景构造必须有
`sf::Font`，而它是 `GlResource` —— 没有 GL 上下文连构造都做不到。所以
`ScoreRules`（星级/目标时间）与 `EditorTools`（编辑器格子几何）是从
`GameScene` / `EditorScene` 里**搬出来**的纯逻辑，搬完就能直接测。
以后再遇到"想测但构造不了"的逻辑，走同一条路：抽成不依赖字体与 GL 的模块。

**需要写文件的模块用 `Paths(root)` 开沙箱**：`Paths` 支持显式传根目录，
于是 `config` / `saves` / `cache` 全部落在临时目录下，测试不会碰到真实存档
（见 `tests/test_save_manager.cpp` 的 `Sandbox`）。

**无 sprite 模式**：`Player` / `Coin` / `Enemy` 的构造函数把贴图作为参数，
传 `nullptr` 就进入"无 sprite 模式"——逻辑与碰撞照常跑，只是画不出东西。
这既方便测试，也让贴图生成失败时自动降级而不是解引用空指针。

⚠️ **测试必须能在没有 DISPLAY 的环境下全绿**（现在就是）。
SFML 里凡是继承 `sf::GlResource` 的类型 —— `sf::Texture` / `sf::RenderTexture` /
`sf::Shader` / `sf::RenderTarget` —— **构造函数就会确保 GL 上下文存在**，
没有 DISPLAY 时直接 SIGABRT（不是返回错误码，拦不住）。所以：

- sprite factory 不能碰（它内部建 `RenderTexture`）→ `GameWorld::SpriteSheets`
  和实体的 sheet 参数就是为此存在的，传空即"无 sprite 模式"
- **这类类型不能作为按值成员**出现在要在无界面环境构造的类里。
  `GameWorld::shadowTex_` 踩过这个坑：一个按值的 `sf::Texture` 成员让
  "构造 GameWorld" 本身就依赖 GL，改成 `unique_ptr` 懒创建才好
- 测试里不要调 `render()`

加测试时请保持 `env -u DISPLAY ./build/tests/tests/unit_tests` 全绿。

## 关卡设计规范

基于 `game_constants.h` 的物理常量（重力 2200、跳跃初速 -720、移动速度 300、瓦片 32px）：

| 能力 | 数值 |
| :--- | :--- |
| 向上跳跃 | ≤ 3.5 格 |
| 水平跨距 | ≤ 5.5 格 |
| 土狼时间加成 | +0.9 格水平 |
| 跳跃缓冲加成 | +0.9 格水平 |
| 弹跳板 `J` | ≤ 10 格高 |

画完关卡后跑 `./build/debug/validate_levels assets/levels` 验证，或者在编辑器里按 T 看可达性。

## 依赖

- **SFML 3**（不是 2.x）
- **CMake ≥ 3.23 + Ninja**
- **doctest 2.5**（单头文件，已在 `tests/doctest.h`）
- **OpenGL**（用于截图 `glReadPixels`）

## License

MIT