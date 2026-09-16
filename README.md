# TEXT-GAME

[![Build & Test](https://github.com/ljm-233/TEXT-GAME/actions/workflows/build.yml/badge.svg)](https://github.com/ljm-233/TEXT-GAME/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

一个用 C++20 和 SFML 3 **从零构建**的 2D 平台跳跃游戏。

不依赖任何游戏引擎，从物理系统到 UI 组件全部手写。包含完整的引擎层（依赖注入、事件总线、场景管理、配置系统、日志、UI 组件、动画、通知、音效、多语言）和游戏本体（自写 AABB 物理、ASCII 关卡、玩家控制、敌人、金币、移动平台、弹跳板、存档点、关卡编辑器）。

## 🚧 当前状态

> 最后更新：2026-09

| 模块 | 状态 | 说明 |
| :--- | :--- | :--- |
| 引擎层 | ✅ 完成 | DI 容器 / 事件总线 / 场景管理 / 配置 / 日志 / UI 组件 / 动画 / 通知 / 音效 |
| 场景流程 | ✅ 完成 | 主菜单 → 存档选择 → 选关 → 游戏 → 结算，支持场景栈返回 |
| 玩家控制 | ✅ 完成 | 土狼时间 / 跳跃缓冲 / 长按跳更高 / 手柄支持 / 键位重映射 |
| 物理系统 | ✅ 完成 | AABB 瓦片扫描 / 固定时间步长 |
| 摄像机 | ✅ 完成 | 前瞻 / 死区 / 屏幕震动 |
| 存档系统 | ✅ 完成 | 创建 / 删除 / 星级 / 进度 |
| 设置系统 | ✅ 完成 | 6 个 Tab / 50+ 项 |
| 控制台 | ✅ 完成 | streambuf 重定向 / 命令系统 / 计算器 |
| 关卡编辑器 | ✅ 完成 | 绘制 / 撤销 / 缩放 / 尺寸调整 / 文件切换 / 元素统计 |
| **多语言** | ✅ 完成 | 中文 / 繁體中文 / English / 日本語 / 한국어 |
| **关卡内容** | ⚠️ **进行中** | 5 个第一版关卡，1~4 关存在可达性问题，待重做 |
| **测试覆盖** | ⚠️ **部分** | 62 个单元测试用例，纯逻辑覆盖较好，游戏对象部分覆盖 |
| 开发工具 | ✅ 完成 | Sanitizer / clang-tidy / CMake Presets / 覆盖率 / 关卡验证器 / 硬编码检测 |

### 已知问题

- `level1` / `level3` 存在平台垂直距离超过玩家跳跃上限的问题，可能导致不可通过
- 部分 UI 控件（SettingsScene / EditorScene）的回归测试靠手动

### 短期路线

1. 重做 `level1` 作为教学关，验证设计流程
2. 依次重做 `level2~5`

## 🎮 游戏玩法

### 操作

| 键盘 | 手柄 | 功能 |
| :--- | :--- | :--- |
| **A / ←** | 左摇杆 / 十字键左 | 向左移动 |
| **D / →** | 左摇杆 / 十字键右 | 向右移动 |
| **Space / W / ↑** | A 键 / 十字键上 | 跳跃（长按跳更高） |
| **R** | — | 重生（回到存档点） |
| **ESC** | B 键 | 暂停 / 返回 |
| **Enter** | Start 键 | 确认 / 下一关 |

> 以上键位可在 **设置 → 按键** 中自定义。

### 平台跳跃特性

- **土狼时间**：走出平台边缘 0.1 秒内还能跳
- **跳跃缓冲**：落地前 0.12 秒按跳，落地瞬间自动起跳
- **长按跳更高**：松手立刻给上升速度减半
- **固定时间步长物理**：1/120 秒为单位更新，任何 FPS 下手感一致
- **摄像机前瞻**：跑动时镜头朝移动方向偏移，提前露出前方
- **摄像机死区**：小幅移动时镜头不动，减少眩晕

### 游戏元素

| 字符 | 元素 | 说明 |
| :--- | :--- | :--- |
| `#` | 地面 / 平台 | 实体瓦片 |
| `P` | 玩家出生点 | 蓝色脉动圆环标记 |
| `E` | 敌人 | 左右巡逻，会掉头 |
| `C` | 金币 | 上下浮动，收集计数 |
| `J` | 弹跳板 | 碰到就弹飞，有冷却 |
| `S` | 存档点 | 激活后掉图回到这里 |
| `M` | 水平移动平台 | 玩家站上去会被带着走 |
| `V` | 垂直移动平台 | 上下移动 |
| `K` | 钥匙 | 收集后解锁所有门 `L` |
| `L` | 门 | 关着时是实体，解锁后消失 |
| `^` | 尖刺 | 碰到受伤 |
| `G` | 终点 | 红色旗帜，到达通关 |

### 关卡元数据

关卡文件顶部可选加元数据（不占瓦片）：

```
# name: 关卡名
# author: 作者名
########################################
#  ...
```

## 🗺 关卡设计规范

### 玩家能力上限

基于 `game_constants.h` 中的物理常量（重力 2200、跳跃初速 -720、移动速度 300、瓦片 32px），玩家在标准状态下的能力上限：

| 能力 | 数值 | 说明 |
| :--- | :--- | :--- |
| 向上跳跃 | **≤ 3.5 格** | 理论 3.68 格，留 0.18 格容错 |
| 水平跨距 | **≤ 5.5 格** | 理论 6.1 格，留 0.6 格容错 |
| 土狼时间加成 | +0.9 格水平 | 走出边缘后 0.1 秒内仍可跳 |
| 跳跃缓冲加成 | +0.9 格水平 | 落地前 0.12 秒按跳仍生效 |
| 弹跳板 `J` | **≤ 10 格高** | 突破普通跳跃限制，`kLaunchSpeed = -1200` |

### 画关卡时的检查清单

- [ ] 出生点周围 3 格内是平地（玩家不会一出生就掉图）
- [ ] 相邻平台垂直差 ≤ 3 格（保守值）
- [ ] 相邻平台水平差 ≤ 5 格（保守值）
- [ ] 需要跨越的坑宽度 ≤ 5 格
- [ ] 尖刺 `^` 前有至少 3 格反应距离
- [ ] 终点 `G` 站在实体瓦片上（不是悬空）
- [ ] 钥匙 `K` 和门 `L` 之间有绕路，不是直线
- [ ] 存档点 `S` 放在关卡中段，且玩家会自然路过

### 验证关卡

```bash
# 用关卡验证器检查所有关卡
./build/debug/validate_levels assets/levels
```

输出示例：

```
[ OK ] level1.txt: OK (5/5 platforms reachable)
[FAIL] level3.txt: FAIL | goal unreachable | platforms 2/8 reachable
  3 unreachable items:
    - coin @ (42,6)
    - enemy @ (55,8)
    - goal @ (98,4)

Total: 5 levels, 1 failed, 0 unloadable
```

## 📁 项目结构

```text
TEXT-GAME/
├── assets/
│   ├── font.otf              # 字体（需自己提取，见下文）
│   ├── lang/                 # 翻译文件
│   │   ├── en.txt
│   │   ├── ja.txt
│   │   ├── ko.txt
│   │   └── zh-TW.txt
│   └── levels/               # ASCII 关卡文件
│       ├── level1.txt
│       ├── level2.txt
│       ├── level3.txt
│       ├── level4.txt
│       └── level5.txt
├── include/
│   ├── config/               # 配置类（Bootstrap / Runtime / Preferences）
│   ├── core/                 # 核心（Application、Game、SceneManager、Logger、Lang、EventBus）
│   ├── game/                 # 游戏本体
│   │   ├── vec2.h            # 二维向量
│   │   ├── aabb.h            # 碰撞盒
│   │   ├── game_constants.h  # 物理常量
│   │   ├── game_object.h     # 对象基类
│   │   ├── event_bus.h       # 事件总线
│   │   ├── level.h           # ASCII 关卡
│   │   ├── level_validator.h # 关卡可达性验证
│   │   ├── camera.h          # 摄像机
│   │   ├── player.h          # 玩家
│   │   ├── enemy.h           # 敌人
│   │   ├── coin.h            # 金币
│   │   ├── jump_pad.h        # 弹跳板
│   │   ├── checkpoint.h      # 存档点
│   │   ├── moving_platform.h # 移动平台
│   │   ├── key.h / door.h    # 钥匙 / 门
│   │   ├── spike.h           # 尖刺
│   │   ├── parallax.h        # 视差背景
│   │   ├── level_intro.h     # 关卡开场文字
│   │   └── game_world.h      # 游戏世界
│   ├── scene/                # 场景
│   └── ui/                   # UI 组件
├── scripts/
│   ├── check_hardcoded.py    # 中文硬编码检测
│   ├── test.sh               # 一键跑单元测试
│   └── gen_levels.py         # 关卡生成器
├── src/                      # 对应 include 的实现
├── tests/                    # 单元测试（doctest）
├── tools/
│   └── validate_levels.cpp   # 关卡验证器（命令行）
├── wallpaper/                # 壁纸资源
├── .clang-format
├── .clang-tidy               # 静态分析配置
├── .editorconfig
├── .gitignore
├── CMakeLists.txt
├── CMakePresets.json         # 构建预设
├── LICENSE
├── README.md
└── CLAUDE.md                 # 给 AI 助手的项目说明
```

## 🧩 核心架构

### 引擎层

| 模块 | 职责 |
| :--- | :--- |
| **DI 容器** | 统一注册/解析所有依赖，自动缓存单例 |
| **事件总线** | GameWorld 发出游戏事件，GameScene 订阅处理音效/粒子，解耦 game 层和 ui 层 |
| **场景系统** | 主菜单 / 存档选择 / 选关 / 游戏 / 设置 / 控制台 / 编辑器，支持返回栈（保留场景状态） |
| **配置分层** | Bootstrap / Runtime / Preferences，延迟落盘 |
| **日志** | 彩色终端 + 文件 + 多级别 + 轮转 + 保留份数 |
| **多语言** | 用中文原文作 key，运行时查表；支持中/繁中/英/日/韩 |
| **虚拟终端** | 用 `streambuf` 重定向 `cin`/`cout`，支持命令系统 |
| **UI 组件** | Button / Slider / TextInput / ConfirmDialog / PauseMenu |
| **主题** | 深色 / 蓝色 / 浅色，所有组件自动跟随 |
| **动画** | 颜色平滑过渡，指数逼近，支持开关和速度 |
| **通知** | 屏幕角落消息，4 类型 × 4 位置 |
| **音效** | 程序化生成（正弦扫频 + 琶音），零外部依赖 |
| **手柄** | 焦点导航（方向键切按钮，A 键触发） |
| **粒子** | 跳跃 / 落地 / 金币 / 踩敌人 / 受伤 |
| **打包** | CPack 一键生成 `.tar.gz` / `.zip` |

### 游戏层

| 模块 | 说明 |
| :--- | :--- |
| **物理** | 自写 AABB 瓦片扫描，先水平后垂直 |
| **关卡** | ASCII 加载 + 视锥裁剪 + 顶点批处理 |
| **关卡验证** | BFS 从出生点出发，报告孤立平台 / 不可达元素 |
| **伪 3D** | 瓦片顶面高光 + 侧面阴影 + 对象投影 |
| **玩家** | 苦力怕精灵动画 + 弹性变形 + 无敌闪烁 + 受击闪白 |
| **摄像机** | 前瞻 + 死区 + 屏幕震动 + 受击停顿 |

## 🛠 技术栈

- **语言**：C++20
- **构建**：CMake ≥ 3.23 + Ninja + CMake Presets
- **图形/音频**：SFML 3
- **依赖注入**：自研简易 `Container`
- **物理**：自写 AABB（不依赖 Box2D）
- **测试**：doctest（单头文件，62 个用例）
- **静态分析**：clang-tidy
- **内存检测**：AddressSanitizer + UndefinedBehaviorSanitizer
- **覆盖率**：gcov + lcov
- **CI**：GitHub Actions（Arch / Ubuntu / macOS / Windows / 关卡验证）
- **跨平台**：Linux / Windows / macOS

## 🚀 构建与运行

### 环境要求

**Arch Linux**：
```bash
sudo pacman -S base-devel cmake ninja sfml python-fonttools lcov clang
```

**Ubuntu / Debian**：
```bash
sudo apt install build-essential cmake ninja-build libsfml-dev lcov clang-tidy
```
> ⚠️ Ubuntu 24.04 的 `libsfml-dev` 可能是 SFML 2.6。需要从源码编译 SFML 3，见 CI 配置。

**macOS**：
```bash
brew install cmake ninja sfml lcov
```

**Windows**：Visual Studio 2022 + vcpkg
```powershell
vcpkg install sfml:x64-windows
```

### 准备字体

项目需要中文字体文件 `assets/font.otf`：

```bash
# 从 Noto CJK 提取简体中文字体
python3 -c "
from fontTools.ttLib import TTCollection
ttc = TTCollection('/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc')
ttc.fonts[2].save('assets/font.otf')
"
```

或者从 [Google Fonts](https://fonts.google.com/noto/specimen/Noto+Sans+SC) 下载 `NotoSansSC-Regular.otf` 重命名为 `font.otf`。

### 编译运行（推荐用 Preset）

```bash
git clone git@github.com:ljm-233/TEXT-GAME.git
cd TEXT-GAME

# 查看所有可用预设
cmake --list-presets

# Debug 构建
cmake --preset debug
cmake --build --preset debug
./build/debug/text_game

# Release 构建
cmake --preset release
cmake --build --preset release
./build/release/text_game
```

**可用的 Preset**：

| Preset | 用途 |
| :--- | :--- |
| `debug` | 标准 Debug 构建 |
| `release` | 发布构建 |
| `tests` | Debug + 单元测试 |
| `asan` | Debug + 测试 + Sanitizer |
| `coverage` | Debug + 测试 + 覆盖率 |
| `clang-tidy` | Debug + 静态分析 |
| `release-package` | 用于 cpack 的发布构建 |

### 打包发布

```bash
cmake --preset release-package
cmake --build --preset release-package
cd build/release-package
cpack
# 生成 TEXT-GAME-0.1.0-Linux.tar.gz 和 .zip
```

### 单元测试

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

### 内存检测

```bash
cmake --preset asan
cmake --build --preset asan
./build/asan/tests/unit_tests
./build/asan/text_game
```

ASan 会在内存越界、悬垂指针、UB 时立即报错，精确到文件和行号。

### 代码覆盖率

```bash
cmake --preset coverage
cmake --build --preset coverage
cmake --build build/coverage --target coverage
# 报告生成在 build/coverage/coverage_html/index.html
```

### 静态分析

```bash
cmake --preset clang-tidy
cmake --build --preset clang-tidy -j
```

### 关卡验证

```bash
./build/debug/validate_levels assets/levels
```

检查每关的出生点、终点可达性、金币/敌人是否在可达平台上。

### 检查硬编码中文

```bash
python3 scripts/check_hardcoded.py
```

扫描所有 C++ 源文件，报告未经过 `Str::T()` 的中文字符串字面量。

## 🎛 功能一览

### 主菜单

- **启动游戏**：选择 / 创建 / 删除存档
- **选关**：直接跳到已解锁的关卡
- **关卡编辑器**：可视化编辑 ASCII 关卡
- **控制台**：内嵌虚拟终端，支持命令和计算器
- **设置**：6 个 Tab，50+ 项
- **退出游戏**

### 设置

**显示**：分辨率 / 全屏 / 垂直同步 / 抗锯齿 / 日志级别 / 帧率上限

**界面**：FPS 显示 / 界面缩放 / 主题 / 语言 / 壁纸 / 时钟 / 控制台遮罩 / 字号 / 历史 / 行高 / 自动滚动 / 光标闪烁 / 提示符

**画面**：动画 / 伪3D / 视差 / 玩家动画 / 关卡开场 / 粒子 / 屏幕震动 / 通知 / 按钮圆角 / 按钮边框 / 显示碰撞盒

**音频**：总音量 / 音效开关 / 音效音量 / BGM 开关 / BGM 音量 / 手柄支持

**按键**：左移 / 右移 / 跳跃 / 暂停 / 重开（可重映射）

**其他**：记住窗口大小 / 失焦自动暂停 / 日志轮转 / 保留份数 / 玩家名 / 关于 / 恢复默认

### 关卡编辑器

- 底部笔刷面板（13 种元素，图标预览）
- 左键画 / 右键擦
- `Ctrl+Z` 撤销（100 步栈）
- `Ctrl+S` 保存
- `Ctrl+N` 切换文件
- `Ctrl+0` 重置缩放
- `Ctrl+滚轮` 缩放（以鼠标为中心）
- `WASD` / 方向键移动摄像机
- `[` `]` `-` `=` 调整关卡尺寸
- `G` 开关网格
- 顶部 HUD：文件名 / 笔刷 / 尺寸 / 缩放 / 网格状态 / 撤销栈深度
- 第二行：元素统计（玩家 / 敌人 / 金币 / ...）

### 控制台命令

```
help              显示帮助
clear             清空屏幕
echo <text>       回显文本
version           显示版本
calc              启动计算器
scene <name>      切换场景 (main/save/settings/quit)
log <level>       设置日志级别
theme <name>      切换主题 (dark/blue/light)
save list         列出所有存档
exit              关闭控制台
```

## 🌏 多语言

支持 5 种语言，运行时在 **设置 → 界面 → 语言** 中切换：

| 语言 | 文件 |
| :--- | :--- |
| 中文 | `assets/lang/zh.txt`（隐含，key 即原文） |
| 繁體中文 | `assets/lang/zh-TW.txt` |
| English | `assets/lang/en.txt` |
| 日本語 | `assets/lang/ja.txt` |
| 한국어 | `assets/lang/ko.txt` |

### 添加新语言

1. 复制 `assets/lang/en.txt` 为 `assets/lang/xx.txt`（xx 是语言代码）
2. 逐行翻译（key 是中文原文，一字不差）
3. 在 `settings_scene.cpp` 的 `refreshLabels` 里给语言按钮加显示名
4. 重新编译，运行

## 🧭 开发约定

1. **新增 `src/` 一级子目录时，需要在 `CMakeLists.txt` 的 GLOB 列表里加一行**（`.cpp` 文件本身会被自动扫描）。
2. **头文件用 `#pragma once`，`.cpp` 首行必须是 `#include`**。
3. **新增类通过 DI 容器注册**，在 `Application::registerDependencies()` 里加一行。
4. **中文要经过 `toSf()` 转换**，否则 SFML 3 会按 Latin-1 解释。
5. **字号用 `scaledFontSize()`**，跟随全局 UI 缩放。
6. **颜色从 `getTheme()` 取**，不要硬编码。
7. **配置读写走 `Config::set*` / `get*`**，写盘由 `flush()` 统一处理。
8. **物理常量放 `game_constants.h`**。
9. **渲染用世界坐标**，平移交给 `sf::View`。
10. **每帧渲染用 `screenView`**，不用 `getDefaultView()`。
11. **新场景加 `FocusGroup::instance().setItems({...})`** 以支持手柄。
12. **UI 文字走 `Str::T(Str::Xxx)`**，字符串定义在 `include/core/text_strings.h`。
13. **游戏事件走 EventBus**，不要从 GameWorld 直接调 SoundManager / ParticleSystem。

### 代码格式化

```bash
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

## 📥 克隆

```bash
cd ~/coding
git clone git@github.com:ljm-233/TEXT-GAME.git
cd TEXT-GAME
```

## 📜 License

[MIT](LICENSE)