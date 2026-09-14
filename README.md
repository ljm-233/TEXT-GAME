# TEXT-GAME

一个用 C++20 和 SFML 3 从零构建的 **2D 平台跳跃游戏**。包含自研的引擎层（DI 容器、场景系统、配置管理、日志、UI 组件、动画、通知、音效）和完整的游戏本体（物理、关卡、玩家控制、敌人、金币、多关卡）。

## 🎮 游戏玩法

### 操作

| 按键 | 功能 |
| :--- | :--- |
| **A / ←** | 向左移动 |
| **D / →** | 向右移动 |
| **Space / W / ↑** | 跳跃（长按跳更高） |
| **R** | 重生（回到关卡起点） |
| **ESC** | 暂停 / 返回 |

### 目标

- 收集金币
- 踩敌人头顶消灭它们（从上方落下）
- 到达关底的 **G** 位置通关
- 有 **3 条命**，掉出地图或撞到敌人扣 1 条

### 平台跳跃特性

- **土狼时间**：走出平台边缘 0.1 秒内还能跳
- **跳跃缓冲**：落地前 0.12 秒按跳，落地瞬间自动起跳
- **长按跳更高**：松手立刻给上升速度减半
- **固定时间步长物理**：1/120 秒为单位更新，任何 FPS 下手感一致

## 🗺 关卡

关卡使用 **ASCII 文本地图**，可直接用文本编辑器设计。

| 字符 | 含义 |
| :--- | :--- |
| `#` | 地面 / 平台 |
| `P` | 玩家出生点 |
| `E` | 敌人（左右巡逻） |
| `C` | 金币 |
| `G` | 终点 |
| ` ` | 空气 |

示例：

```
################################################
#                                              #
#    P                    C     C              #
#   ####                #########              #
#            E                                 #
#         ######                               #
#                       G                      #
################################################
```

**多关卡**：`assets/levels/level1.txt` / `level2.txt` / `level3.txt`，通关后按 Enter 进下一关。

## 📁 项目结构

```text
TEXT-GAME/
├── assets/
│   ├── font.otf              # 字体（需自己提取）
│   └── levels/               # ASCII 关卡文件
│       ├── level1.txt
│       ├── level2.txt
│       └── level3.txt
├── cache/                    # 运行时缓存（不提交）
├── config/                   # 运行时配置（不提交）
├── include/
│   ├── config/               # 配置类
│   ├── core/                 # 核心（Application、Game、SceneManager、Logger）
│   ├── game/                 # 游戏本体
│   │   ├── aabb.h            # 碰撞盒
│   │   ├── vec2.h            # 二维向量
│   │   ├── game_constants.h  # 物理常量
│   │   ├── game_object.h     # 对象基类
│   │   ├── level.h           # ASCII 关卡
│   │   ├── camera.h          # 摄像机
│   │   ├── player.h          # 玩家
│   │   ├── enemy.h           # 敌人
│   │   ├── coin.h            # 金币
│   │   └── game_world.h      # 游戏世界
│   ├── scene/                # 场景
│   └── ui/                   # UI 组件
├── saves/                    # 存档（不提交）
├── src/                      # 对应源文件
├── temp/                     # 临时文件
├── wallpaper/                # 壁纸
├── .clang-format
├── .editorconfig
├── .gitignore
├── CMakeLists.txt
└── README.md
```

## 🧩 核心架构

| 模块 | 职责 |
| :--- | :--- |
| **DI 容器**（`application.h`） | 统一注册/解析所有依赖 |
| **场景系统**（`scene.h` / `scene_manager.h`） | 主菜单 / 存档 / 游戏 / 设置 / 控制台，支持返回栈 |
| **配置分层** | `BootstrapConfig` / `RuntimeConfig` / `Preferences` |
| **日志**（`logging.h`） | 彩色终端 + 文件 + 多级别 + 轮转 |
| **虚拟终端**（`console.h`） | 用 `streambuf` 重定向 `cin`/`cout` |
| **UI 组件** | `Button` / `Slider` / `TextInput` / `ConfirmDialog` / `PauseMenu` |
| **主题** | 深色 / 蓝色 / 浅色三套配色 |
| **动画** | 颜色平滑过渡，开关 + 速度 |
| **通知** | 屏幕角落消息，4 类型 × 4 位置 |
| **音效**（`sound_manager.h`） | 代码生成的音效，无外部文件 |
| **物理** | 自写 AABB 碰撞 + 固定时间步长 |
| **关卡** | ASCII 加载 + 视锥裁剪渲染 |
| **对象系统**（`game_object.h`） | `GameObject` 基类，所有实体统一 update/render |

## 🛠 技术栈

- **语言**：C++20
- **构建**：CMake ≥ 3.20 + Ninja
- **图形/音频**：SFML 3
- **依赖注入**：自研简易 `Container`
- **物理**：自写简化 AABB（不依赖 Box2D）
- **跨平台**：Linux / Windows / macOS

## 🚀 构建与运行

### 环境要求

- **Arch Linux**：
  ```bash
  sudo pacman -S base-devel cmake ninja sfml python-fonttools
  ```
- **Ubuntu / Debian**：
  ```bash
  sudo apt install build-essential cmake ninja-build libsfml-dev
  ```
- **Windows**：Visual Studio 2022 + vcpkg
  ```powershell
  vcpkg install sfml:x64-windows
  ```

### 准备字体

```bash
python3 -c "
from fontTools.ttLib import TTCollection
ttc = TTCollection('/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc')
ttc.fonts[2].save('assets/font.otf')
"
```

### 编译运行

```bash
git clone git@github.com:ljm-233/TEXT-GAME.git
cd TEXT-GAME

mkdir -p build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j
./text_game
```

### VS Code

`.vscode/` 里预配置了 `tasks.json` 和 `launch.json`，按 `F5` 即可调试运行。

## 🎛 功能一览

### 主菜单
- 启动游戏（选择 / 创建 / 删除存档）
- 计算器（在虚拟终端里跑）
- 设置
- 退出游戏

### 暂停菜单（游戏中按 ESC）
- 回到游戏
- 设置（主题 / 动画 / 通知）
- 保存并退出游戏

### 设置（左侧 Tab 分页）

**显示**
- 分辨率 / 全屏 / 垂直同步 / 抗锯齿
- 日志级别 / 帧率上限
- 动画开关 + 速度
- 屏幕通知开关 + 位置

**界面**
- 帧率显示 + 位置
- 界面缩放
- 控制台遮罩 / 面板透明度 / 字号 / 历史行数 / 行高
- 控制台自动滚动 / 光标闪烁
- 主题 / 壁纸切换
- 时间显示 + 位置

**其他**
- 记住窗口大小
- 日志轮转 / 保留份数
- 按钮圆角 / 边框
- 玩家名
- 关于 / 恢复默认设置

### 控制台命令

```
help              显示帮助
clear             清空屏幕
echo <text>       回显文本
version           显示版本
calc              启动计算器
scene <name>      切换场景 (main/save/settings/quit)
log <level>       设置日志级别 (trace/debug/info/warn/error)
theme <name>      切换主题 (dark/blue/light)
save list         列出所有存档
exit              关闭控制台
```

## 🧭 开发约定

1. **新增 `.cpp` 文件后，记得加进 `CMakeLists.txt` 的 `add_executable` 列表**。
2. **头文件用 `#pragma once`，`.cpp` 开头 `#include` 对应的 `.h`**。
3. **新增类通过 DI 容器注册**，在 `Application::registerDependencies()` 里加一行。
4. **中文要经过 `toSf()` 转换**（`#include "utf8.h"`），否则 SFML 3 会按 Latin-1 解释。
5. **字号用 `scaledFontSize()`**（`#include "ui_scale.h"`），跟随全局 UI 缩放。
6. **颜色从 `getTheme()` 取**（`#include "theme.h"`），不要硬编码。
7. **配置读写走 `Config::set*` / `get*`**，写盘由 `flush()` 统一处理。
8. **物理常量放 `game_constants.h`**，不要散落在各 `.cpp`。
9. **渲染用世界坐标**，平移交给 SFML 的 `sf::View`，不要自己减摄像机偏移。
10. **每帧渲染用 `screenView`**（尺寸 = 当前窗口尺寸），不要用 `getDefaultView()`，否则窗口拉伸后内容会变形。

### 代码格式化

```bash
# 格式化所有源码
clang-format -i src/**/*.cpp include/**/*.h

# 或只格式化当前文件（VS Code 按 Shift+Alt+F）
```

## 📥 克隆（下载）

```bash
cd ~/coding
git clone git@github.com:ljm-233/TEXT-GAME.git
cd TEXT-GAME
```

> **SSH 配置**：如果尚未配置 SSH 密钥，请先生成并添加到 GitHub 账户。
> 测试连接：`ssh -T git@github.com`

## 📤 推送（上传）

```bash
git status
git add .
git commit -m "描述这次改了什么"
git push
```

首次推送如果还没绑定远程分支：

```bash
git push -u origin main
```

## 👥 多人协作

推送前先拉取远程最新代码：

```bash
git pull --rebase

# 如果有冲突，手动解决冲突文件后：
git add .
git rebase --continue
git push
```

---

**开发习惯**：开始写代码前 `git pull`，完成一个小功能就 `add → commit → push`，保持提交历史清晰。