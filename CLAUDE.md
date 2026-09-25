# CLAUDE.md

给 AI 助手的项目说明。新会话请先读这个文件。

## 项目概览

`TEXT-GAME` —— 用 C++20 + SFML 3 从零手写的 2D 平台跳跃游戏。
不依赖游戏引擎，物理、UI、渲染、音频全部自研。

## 构建 / 运行 / 测试

```bash
# Debug 构建
cmake --preset debug
cmake --build --preset debug
./build/debug/text_game

# Release 构建（**跑游戏和编辑器请用 Release**）
cmake --preset release
cmake --build --preset release
./build/release/text_game

# 单元测试
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
include/ 和 src/ 一一对应：
  core/          Application / Game / SceneManager / Logger / Platform / Lang
                 Achievement / text_strings
  config/        BootstrapConfig / RuntimeConfig / Preferences（header-only）
  scene/         8 个场景类
  scene/tabs/    6 个设置 Tab（AudioTab / GraphicsTab / InterfaceTab /
                 DisplayTab / OtherTab / KeysTab）
  ui/            UI 组件（Button / Slider / TextInput / Window /
                 Upscaler / PostProcessor / Console / SoundManager /
                 Gamepad / GamepadVibration / Notification ...）
  game/          游戏本体（Player / Enemy / Level / GameWorld / Camera ...）
tests/           doctest 单元测试
tools/           validate_levels 命令行工具
assets/
  levels/*.txt   ASCII 关卡
  shaders/*.frag GLSL 着色器（5 个）
  lang/*.txt     翻译文件
```

## 分层架构

```
core      ← 场景/应用层
  ↑
scene     ← 具体场景
  ↑
ui / game ← UI 组件 + 游戏对象（互不依赖）
```

**重要**：`game/` 层不应该 `#include` `ui/` 层的东西。
`GameWorld` 通过 `EventBus` 发事件，`GameScene` 订阅处理音效/粒子/振动。

## 关键约定

1. **新增 `src/` 一级子目录时要改 `CMakeLists.txt` 的 GLOB 列表**，`.cpp` 文件本身会被自动扫描。`src/scene/tabs/*.cpp` 已在 GLOB 里。
2. **头文件用 `#pragma once`，`.cpp` 首行必须是 `#include`**。
3. **中文必须走 `toSf()`**（`include/ui/utf8.h`），否则 SFML 3 按 Latin-1 解释。
4. **字号用 `scaledFontSize()`**（`include/ui/ui_scale.h`），跟随 UI 缩放。
5. **颜色从 `getTheme()` 取**（`include/ui/theme.h`），不硬编码。
6. **配置读写走 `Config::set*/get*`**，写盘由 `flush()` 统一处理。
7. **物理常量放 `include/game/game_constants.h`**。
8. **每帧渲染用 `screenView`**，不用 `getDefaultView()`。
9. **UI 文字走 `Str::T(Str::Xxx)`**，字符串定义在 `include/core/text_strings.h`。
10. **游戏事件走 EventBus**，不要从 GameWorld 直接调 SoundManager / ParticleSystem / Gamepad。
11. **着色器放 `assets/shaders/`**，`.frag` 后缀，GLSL 330 core。
12. **手柄焦点导航**：每个 Scene 的 `render()` 末尾调 `FocusGroup::instance().setItems({...})`，或者由 Tab 的 `registerFocus()` 收集。

## 常见任务入口

| 想改什么 | 去哪 |
| :--- | :--- |
| 玩家手感（速度/重力/跳跃） | `include/game/game_constants.h` |
| 添加新游戏对象 | `include/game/` + `src/game/`，在 `GameWorld::spawnLevelObjects` 和 `checkCollisionsSafe` 注册 |
| 添加新场景 | `include/scene/` + `src/scene/`，在 `Game::createScene` 里加 case，同时在 `scene_id.h` 加枚举 |
| **添加新设置项** | 在对应 Tab（`include/scene/tabs/xxx_tab.h/cpp`）加成员 + 创建控件 + update 回调。**不用改 SettingsScene** |
| 添加新事件 | `include/game/event_bus.h` 加 struct + 加入 variant，然后 `GameWorld` emit + `GameScene` 订阅 |
| 添加关卡 | `assets/levels/levelN.txt`，参考已有格式；用 `validate_levels` 验证 |
| 添加主题 | `include/ui/theme.h` 加枚举 + `src/ui/theme.cpp` 加颜色组 |
| 添加成就 | `src/core/achievement.cpp` 的 `kAchievements` 加一行 + `text_strings.h` 加名称/描述 |
| 添加画面预设 | `src/scene/tabs/graphics_tab.cpp` 的 `kPresets` 加一行 + `presetRow_` 加按钮 |
| 添加后处理效果 | `assets/shaders/postprocess.frag` 加 uniform + `include/ui/postprocess.h` 加 setter + SettingsScene → GraphicsTab 加滑条 |

## 架构要点

### SceneManager：场景常驻

**场景只构造一次，之后复用**（避免 sf::Text 频繁析构触发 SFML 3.1.0 HarfBuzz 死锁）。

- `onEnter()`：首次进入 + 每次重新进入都调用
- `onResume()`：从 pop 恢复时调用
- `onPause()`：被上层覆盖时调用
- **`onExit()` 只在程序退出时调用**

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

`Gamepad::vibrate(low, high, duration)` 是入口，内部有正弦包络（0 → 1 → 0）。

## 已知陷阱

### SFML 3.1.0 相关

- ⚠️ **`sf::Text` 析构会死锁**（HarfBuzz + FreeType 清理时 pthread_mutex_lock）。
  - **任何频繁创建/销毁 `sf::Text` 的地方都是定时炸弹**。
  - 场景改成常驻、`LevelIntro` 改成复用、编辑器笔刷条和图标预渲染到 RT。
  - **新代码不要每帧构造 `sf::Text`**，一律做成成员变量。
- ⚠️ **SFML 3 没有振动 API**，自己通过 `GamepadVibration` 调系统。
- ⚠️ **SFML 3 移除了 `RenderWindow::capture()`**，用 `glReadPixels` 自己实现（需要 `find_package(OpenGL REQUIRED)` + `OpenGL::GL`）。

### 项目自身

- `Game::createScene` 里不要有副作用。`GameScene` 的 `takePendingSave()` 已挪到 `onEnter()`。
- `EventBus::subscribe` 的回调是同步的，`emit` 时会立刻执行。
- `save_manager::updateProgress` 的 `progress` 参数实际存的是金币数（命名误导）。
- `FocusGroup::setItems` 目前在 `render()` 里调用（反模式，但已稳定）。
- `PauseMenu::render` 会重设 `FocusGroup::setItems`，与 Scene 的焦点注册冲突。
- `SettingsScene::anySliderEditing()` 检查所有 Tab 的 `anyEditing()`，用于判断 ESC 是否应该退出场景。
- 场景常驻后 `SceneManager::pop` 不再销毁场景，`history_` 存指针。
- **程序退出时可能会卡 1~2 秒**（缓存场景统一析构），`main.cpp` 用了 `std::quick_exit(0)` 绕过。

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
| Enemy / Coin / Checkpoint / Door / Spike / Key / GameWorld / Scene | ❌ |
| SaveManager / Logger / UI 组件 | ❌（依赖运行环境） |

测试写法：`tests/test_*.cpp`，参考 `tests/test_player.cpp` 的"无 sprite 模式"技巧。

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