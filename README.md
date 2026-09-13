# TEXT-GAME

一个基于 C++20 和 SFML 3 的跨平台游戏项目。目前包含完整的**应用框架**：场景系统、依赖注入、配置管理、日志、存档、虚拟终端控制台、可扩展的设置页。

## 📁 项目结构

```text
TEXT-GAME/
├── assets/                # 资源（字体等）
│   └── font.otf
├── cache/                 # 可重建缓存（不提交内容）
├── config/                # 运行时配置（不提交内容）
│   ├── app.log            # 日志
│   ├── bootstrap.conf     # 引导配置
│   ├── preferences.conf   # 用户偏好
│   └── runtime.conf       # 运行时状态
├── include/               # 头文件
│   ├── application.h
│   ├── background.h
│   ├── bootstrap_config.h
│   ├── button.h
│   ├── calculator.h
│   ├── config.h
│   ├── confirm_dialog.h
│   ├── console.h
│   ├── console_scene.h
│   ├── font_holder.h
│   ├── game.h
│   ├── game_scene.h
│   ├── logging.h
│   ├── main_menu_scene.h
│   ├── paths.h
│   ├── preferences.h
│   ├── resolution.h
│   ├── runtime_config.h
│   ├── save_manager.h
│   ├── save_select_scene.h
│   ├── scene.h
│   ├── scene_id.h
│   ├── settings_scene.h
│   ├── slider.h
│   ├── theme.h
│   ├── time_utils.h
│   ├── ui_scale.h
│   ├── utf8.h
│   └── window.h
├── saves/                 # 存档（不提交内容）
├── src/                   # 源文件
│   ├── application.cpp
│   ├── background.cpp
│   ├── button.cpp
│   ├── calculator.cpp
│   ├── confirm_dialog.cpp
│   ├── console.cpp
│   ├── console_scene.cpp
│   ├── font_holder.cpp
│   ├── game.cpp
│   ├── game_scene.cpp
│   ├── main.cpp
│   ├── main_menu_scene.cpp
│   ├── save_manager.cpp
│   ├── save_select_scene.cpp
│   ├── settings_scene.cpp
│   ├── slider.cpp
│   ├── theme.cpp
│   ├── ui_scale.cpp
│   ├── utf8.cpp
│   └── window.cpp
├── temp/                  # 临时文件（不提交内容）
├── wallpaper/             # 壁纸
│   └── wallpaper.jpg
├── .gitignore
├── CMakeLists.txt
└── README.md
```

## 🧩 核心架构

| 模块 | 职责 |
| :--- | :--- |
| **DI 容器**（`application.h`） | 统一注册/解析所有依赖 |
| **场景系统**（`scene.h` / `scene_id.h`） | `MainMenu` / `SaveSelect` / `Game` / `Settings` / `Console`，支持 ESC 返回上一场景 |
| **配置分层** | `BootstrapConfig`（启动前）/ `RuntimeConfig`（运行时）/ `Preferences`（用户偏好） |
| **日志**（`logging.h`） | 彩色终端输出 + 文件记录 + 多级别过滤（Trace~Error） |
| **虚拟终端**（`console.h`） | 用 `streambuf` 重定向 `cin`/`cout`，在窗口内跑命令行程序 |
| **UI 组件** | `Button` / `Slider` / `ConfirmDialog`，全部走主题系统 |
| **主题**（`theme.h`） | 深色 / 蓝色 / 浅色，三套配色 |
| **UI 缩放**（`ui_scale.h`） | 0.8x ~ 1.5x，全局字号系数 |

## 🎮 功能一览

### 主菜单
- 启动游戏（选择/创建/删除存档）
- 计算器（在虚拟终端里跑）
- 设置
- 退出游戏

### 设置（左侧 Tab 分页）
- **显示**：分辨率、全屏、V-Sync、抗锯齿、日志级别
- **界面**：帧率显示、界面缩放、控制台遮罩、控制台字号、控制台历史行数、主题、壁纸切换
- **其他**：记住窗口大小、恢复默认设置

### 控制台
- 全键盘输入，回车提交，退格删除
- ↑/↓ 翻历史命令
- ANSI 颜色码自动过滤
- ESC 返回上一场景

## 🛠 技术栈

- **语言**：C++20
- **构建**：CMake ≥ 3.20 + Ninja
- **图形/窗口**：SFML 3
- **依赖注入**：自研简易 `Container`
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

### 准备资源

字体文件（必需，用于显示中文）：

```bash
# Arch Linux：从 Noto CJK 提取简体中文字体
python3 -c "
from fontTools.ttLib import TTCollection
ttc = TTCollection('/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc')
ttc.fonts[2].save('assets/font.otf')
"
```

壁纸（可选）：

```bash
cp ~/Pictures/你的图.jpg wallpaper/wallpaper.jpg
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

## 📥 克隆（下载）

```bash
cd ~/coding
git clone git@github.com:ljm-233/TEXT-GAME.git
cd TEXT-GAME
```

> **SSH 配置**：如果尚未配置 SSH 密钥，请先生成并添加到 GitHub 账户。
> 测试连接：`ssh -T git@github.com`

## 📤 推送（上传）

每次写完代码后：

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

## 🧭 开发约定

1. **新增 `.cpp` 文件后，记得加进 `CMakeLists.txt` 的 `add_executable` 列表。**
2. **头文件用 `#pragma once`，`.cpp` 开头 `#include` 对应的 `.h`。**
3. **新增类通过 DI 容器注册**，在 `Application::registerDependencies()` 里加一行。
4. **中文要经过 `toSf()` 转换**（`#include "utf8.h"`），否则 SFML 3 会按 Latin-1 解释。
5. **字号用 `scaledFontSize()`**（`#include "ui_scale.h"`），跟随全局 UI 缩放。
6. **颜色从 `getTheme()` 取**（`#include "theme.h"`），不要硬编码。
7. **配置读写走 `Config::set*` / `get*`**，不要直接操作文件；写盘由 `flush()` 统一处理。

---

**开发习惯**：开始写代码前 `git pull`，完成一个小功能就 `add → commit → push`，保持提交历史清晰。