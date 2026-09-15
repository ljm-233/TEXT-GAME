# TEXT-GAME

[![Build & Test](https://github.com/ljm-233/TEXT-GAME/actions/workflows/build.yml/badge.svg)](https://github.com/ljm-233/TEXT-GAME/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

一个用 C++20 和 SFML 3 **从零构建**的 2D 平台跳跃游戏。

不依赖任何游戏引擎，从物理系统到 UI 组件全部手写。包含完整的引擎层（依赖注入、场景管理、配置系统、日志、UI 组件、动画、通知、音效）和游戏本体（自写 AABB 物理、ASCII 关卡、玩家控制、敌人、金币、移动平台、弹跳板、存档点）。

## 🚧 当前状态

> 最后更新：2026-09

| 模块 | 状态 | 说明 |
| :--- | :--- | :--- |
| 引擎层 | ✅ 完成 | DI 容器 / 场景管理 / 配置 / 日志 / UI 组件 / 动画 / 通知 / 音效 |
| 场景流程 | ✅ 完成 | 主菜单 → 存档选择 → 选关 → 游戏 → 结算 |
| 玩家控制 | ✅ 完成 | 土狼时间 / 跳跃缓冲 / 长按跳更高 / 手柄支持 |
| 物理系统 | ✅ 完成 | AABB 瓦片扫描 / 固定时间步长 |
| 存档系统 | ✅ 完成 | 创建 / 删除 / 星级 / 进度 |
| 设置系统 | ✅ 完成 | 5 个 Tab / 40+ 项 |
| 控制台 | ✅ 完成 | streambuf 重定向 / 命令系统 / 计算器 |
| **关卡内容** | ⚠️ **进行中** | 5 个第一版关卡，1~4 关存在可达性问题，待重做 |
| **关卡编辑器** | ❌ 未开始 | 目前手工编辑 ASCII 文件 |
| **测试覆盖** | ⚠️ **部分** | 纯逻辑模块覆盖较好，游戏对象（Player / Enemy）无测试 |
| **音频资源** | ⚠️ **程序化** | 音效和 BGM 全部代码生成，无外部资源 |

### 已知问题

- `level1` / `level3` 存在平台垂直距离超过玩家跳跃上限的问题，可能导致不可通过
- `Player` 未做单元测试，因为 `player.cpp` 依赖 sprite factory 无法独立链接
- 部分 UI 控件（PauseMenu / SettingsScene）的回归测试靠手动

### 短期路线

1. 重做 `level1` 作为教学关，验证设计流程
2. 写关卡可达性检查器（BFS 从出生点出发，报告孤立平台）
3. 让 `Player` 可被单元测试（无 sprite 模式）
4. 依次重做 `level2~5`

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

### 平台跳跃特性

- **土狼时间**：走出平台边缘 0.1 秒内还能跳
- **跳跃缓冲**：落地前 0.12 秒按跳，落地瞬间自动起跳
- **长按跳更高**：松手立刻给上升速度减半
- **固定时间步长物理**：1/120 秒为单位更新，任何 FPS 下手感一致
- **摄像机跟随**：玩家走到边缘时镜头自动跟随，边界锁定

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

### 常见错误

- ❌ 出生点周围没平台 → 玩家直接掉图
- ❌ 平台垂直差 4 格 → 跳不上去
- ❌ 平台水平差 6 格 → 跨不过去
- ❌ 终点悬空 → 到不了
- ❌ 尖刺放在玩家必经且无预警处 → 不公平
- ❌ 一行放 10 个敌人 → 不是设计，是堆砌

### 设计原则

1. **教学优先**：新机制第一次出现时，给玩家安全的空间练习
2. **递进节奏**：机制 A → 机制 A 应用 → 机制 A + B 组合 → 高潮
3. **最少元素**：能用 3 种元素表达的设计，不要用 5 种
4. **可预览**：玩家在跳跃之前，能看到目标平台的位置

## 📁 项目结构

```text
TEXT-GAME/
├── assets/
│   ├── font.otf              # 字体（需自己提取，见下文）
│   └── levels/               # ASCII 关卡文件
│       ├── level1.txt
│       ├── level2.txt
│       ├── level3.txt
│       ├── level4.txt
│       └── level5.txt
├── include/
│   ├── config/               # 配置类（Bootstrap / Runtime / Preferences）
│   ├── core/                 # 核心（Application、Game、SceneManager、Logger、Platform）
│   ├── game/                 # 游戏本体
│   │   ├── vec2.h            # 二维向量
│   │   ├── aabb.h            # 碰撞盒
│   │   ├── game_constants.h  # 物理常量
│   │   ├── game_object.h     # 对象基类
│   │   ├── level.h           # ASCII 关卡
│   │   ├── camera.h          # 摄像机 + 屏幕震动
│   │   ├── player.h          # 玩家
│   │   ├── enemy.h           # 敌人
│   │   ├── coin.h            # 金币
│   │   ├── jump_pad.h        # 弹跳板
│   │   ├── checkpoint.h      # 存档点
│   │   ├── moving_platform.h # 移动平台
│   │   ├── parallax.h        # 视差背景
│   │   ├── level_intro.h     # 关卡开场文字
│   │   └── game_world.h      # 游戏世界
│   ├── scene/                # 场景
│   └── ui/                   # UI 组件
├── scripts/
│   ├── test.sh               # 一键跑单元测试
│   └── gen_levels.py         # 关卡生成器
├── tests/                    # 单元测试（doctest）
├── wallpaper/                # 壁纸资源
├── .clang-format
├── .editorconfig
├── .gitignore
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## 🧩 核心架构

### 引擎层

| 模块 | 职责 |
| :--- | :--- |
| **DI 容器** | 统一注册/解析所有依赖，自动缓存单例 |
| **场景系统** | 主菜单 / 存档选择 / 选关 / 游戏 / 设置 / 控制台，支持返回栈 |
| **配置分层** | Bootstrap / Runtime / Preferences，延迟落盘 |
| **日志** | 彩色终端 + 文件 + 多级别 + 轮转 + 保留份数 |
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
| **伪 3D** | 瓦片顶面高光 + 侧面阴影 + 对象投影 |
| **玩家** | 苦力怕精灵动画 + 弹性变形 + 无敌闪烁 |

## 🛠 技术栈

- **语言**：C++20
- **构建**：CMake ≥ 3.20 + Ninja
- **图形/音频**：SFML 3
- **依赖注入**：自研简易 `Container`
- **物理**：自写 AABB（不依赖 Box2D）
- **测试**：doctest（单头文件）
- **CI**：GitHub Actions（Arch / Ubuntu / macOS / Windows）
- **跨平台**：Linux / Windows / macOS

## 🚀 构建与运行

### 环境要求

**Arch Linux**：
```bash
sudo pacman -S base-devel cmake ninja sfml python-fonttools
```

**Ubuntu / Debian**：
```bash
sudo apt install build-essential cmake ninja-build libsfml-dev
```
> ⚠️ Ubuntu 24.04 的 `libsfml-dev` 可能是 SFML 2.6。需要从源码编译 SFML 3，见 CI 配置。

**macOS**：
```bash
brew install cmake ninja sfml
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

### 编译运行

```bash
git clone git@github.com:ljm-233/TEXT-GAME.git
cd TEXT-GAME

mkdir -p build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./text_game
```

### 打包发布

```bash
cd build
cpack
# 生成 TEXT-GAME-0.1.0-Linux.tar.gz 和 .zip
```

### 跑单元测试

```bash
./scripts/test.sh
```

或手动：

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j
./build/tests/unit_tests
```

## 🎛 功能一览

### 主菜单

- **启动游戏**：选择 / 创建 / 删除存档
- **选关**：直接跳到已解锁的关卡
- **控制台**：内嵌虚拟终端，支持命令和计算器
- **设置**：5 个 Tab，40+ 项
- **退出游戏**

### 设置

**显示**：分辨率 / 全屏 / 垂直同步 / 抗锯齿 / 日志级别 / 帧率上限

**界面**：FPS 显示 / 界面缩放 / 主题 / 壁纸 / 时钟 / 控制台遮罩 / 字号 / 历史 / 行高 / 自动滚动 / 光标闪烁 / 提示符

**画面**：动画 / 伪3D / 视差 / 玩家动画 / 关卡开场 / 粒子 / 屏幕震动 / 通知 / 按钮圆角 / 按钮边框 / 显示碰撞盒

**音频**：音效开关 / 音效音量 / BGM 开关 / BGM 音量 / 手柄支持

**其他**：记住窗口大小 / 失焦自动暂停 / 日志轮转 / 保留份数 / 玩家名 / 关于 / 恢复默认

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

### 代码格式化

```bash
# 装 clang-format
sudo pacman -S clang

# 格式化所有源码
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

## 📥 克隆

```bash
cd ~/coding
git clone git@github.com:ljm-233/TEXT-GAME.git
cd TEXT-GAME
```

## 📤 推送

```bash
git add .
git commit -m "描述改动"
git push
```

## 👥 多人协作

```bash
# 推送前先拉取
git pull --rebase

# 有冲突就手动解决后
git add .
git rebase --continue

# 再推送
git push
```

## 📜 License

[MIT](LICENSE)