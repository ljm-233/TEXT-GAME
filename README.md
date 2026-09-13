# TEXT-GAME 文本由AI生成

一个基于 C++20 和 SFML 3 的跨平台 2D 平台跳跃游戏项目。包含完整引擎层（场景系统、依赖注入、配置管理、日志、UI 组件、动画、通知）和游戏本体（物理、关卡、摄像机、玩家控制）。

## 📁 项目结构

```text
TEXT-GAME/
├── assets/                 # 资源（字体等）
│   └── font.otf
├── cache/                  # 可重建缓存（不提交内容）
├── config/                 # 运行时配置（不提交内容）
│   ├── app.log
│   ├── bootstrap.conf
│   ├── preferences.conf
│   └── runtime.conf
├── include/
│   ├── config/             # 配置类
│   │   ├── bootstrap_config.h
│   │   ├── config.h
│   │   ├── preferences.h
│   │   └── runtime_config.h
│   ├── core/               # 核心
│   │   ├── application.h
│   │   ├── game.h
│   │   ├── logging.h
│   │   ├── paths.h
│   │   ├── scene.h
│   │   ├── scene_id.h
│   │   ├── scene_manager.h
│   │   └── strings.h
│   ├── game/               # 游戏本体
│   │   ├── aabb.h
│   │   ├── camera.h
│   │   ├── calculator.h
│   │   ├── game_world.h
│   │   ├── level.h
│   │   ├── player.h
│   │   ├── resolution.h
│   │   ├── save_manager.h
│   │   ├── time_utils.h
│   │   └── vec2.h
│   ├── scene/              # 场景
│   │   ├── console_scene.h
│   │   ├── game_scene.h
│   │   ├── main_menu_scene.h
│   │   ├── save_select_scene.h
│   │   └── settings_scene.h
│   └── ui/                 # UI 组件
│       ├── animation.h
│       ├── background.h
│       ├── button.h
│       ├── button_style.h
│       ├── confirm_dialog.h
│       ├── console.h
│       ├── font_holder.h
│       ├── new_save_dialog.h
│       ├── notification.h
│       ├── slider.h
│       ├── text_input.h
│       ├── theme.h
│       ├── ui_scale.h
│       ├── utf8.h
│       └── window.h
├── saves/                  # 存档（不提交内容）
├── src/                    # 对应源文件
├── temp/                   # 临时文件
├── wallpaper/              # 壁纸
│   └── wallpaper.png
├── .gitignore
├── CMakeLists.txt
└── README.md
```

## 🎮 游戏玩法

### 操作

| 按键 | 功能 |
| :--- | :--- |
| **A / ←** | 向左移动 |
| **D / →** | 向右移动 |
| **Space / W / ↑** | 跳跃（长按跳更高） |
| **R** | 重生（回到出生点） |
| **ESC** | 返回存档选择 |

### 平台跳跃特性

- **土狼时间**：走出平台边缘 0.1 秒内还能跳
- **跳跃缓冲**：落地前 0.12 秒按跳，落地瞬间自动起跳
- **长按跳更高**：松手立刻给上升速度减半
- **固定时间步长物理**：1/120 秒为单位更新，任何 FPS 下手感一致
- **摄像机跟随**：玩家走到边缘时镜头自动跟随，边界锁定

### 关卡格式

ASCII 文本地图，可直接用文本编辑器画：

```
################################################
#                                              #
#    P                                         #
#          #####                               #
#                         ########             #
#      ####                                    #
#                ##########                    #
#       ####               ###                 #
#                 #####                        #
################################################
```

| 字符 | 含义 |
| :--- | :--- |
| `#` | 地面 / 平台 |
| `P` | 玩家出生点 |
| `E` | 敌人（暂未实现） |
| `C` | 金币（暂未实现） |
| `G` | 终点（暂未实现） |
| ` ` | 空气 |

## 🧩 核心架构

| 模块 | 职责 |
| :--- | :--- |
| **DI 容器**（`application.h`） | 统一注册/解析所有依赖 |
| **场景系统**（`scene.h` / `scene_manager.h`） | 主菜单 / 存档 / 游戏 / 设置 / 控制台，支持 ESC 返回上一场景 |
| **配置分层** | `BootstrapConfig` / `RuntimeConfig` / `Preferences` |
| **日志**（`logging.h`） | 彩色终端输出 + 文件记录 + 多级别过滤 + 日志轮转 |
| **虚拟终端**（`console.h`） | 用 `streambuf` 重定向 `cin`/`cout`，支持命令系统 |
| **UI 组件** | `Button` / `Slider` / `TextInput` / `ConfirmDialog` / `NewSaveDialog` |
| **主题**（`theme.h`） | 深色 / 蓝色 / 浅色三套配色 |
| **UI 缩放**（`ui_scale.h`） | 0.8x ~ 1.5x 全局字号系数 |
| **动画**（`animation.h`） | 颜色平滑过渡，支持开关和速度 |
| **通知**（`notification.h`） | 屏幕角落消息提示，4 种类型 × 4 个位置 |
| **游戏世界**（`game_world.h`） | 关卡 + 玩家 + 摄像机 |
| **玩家物理**（`player.h`） | 自写 AABB 碰撞 + 平台跳跃手感 |
| **关卡**（`level.h`） | ASCII 加载 + 瓦片查询 + 可见性裁剪渲染 |

## 🛠 技术栈

- **语言**：C++20
- **构建**：CMake ≥ 3.20 + Ninja
- **图形/窗口**：SFML 3
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

### 准备资源

字体文件（必需，用于显示中文）：

```bash
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

## 🎛 功能一览

### 主菜单
- 启动游戏（选择/创建/删除存档）
- 计算器（在虚拟终端里跑）
- 设置
- 退出游戏

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

## 🧭 开发约定

1. **新增 `.cpp` 文件后，记得加进 `CMakeLists.txt` 的 `add_executable` 列表**。
2. **头文件用 `#pragma once`，`.cpp` 开头 `#include` 对应的 `.h`**。
3. **新增类通过 DI 容器注册**，在 `Application::registerDependencies()` 里加一行。
4. **中文要经过 `toSf()` 转换**（`#include "utf8.h"`），否则 SFML 3 会按 Latin-1 解释。
5. **字号用 `scaledFontSize()`**（`#include "ui_scale.h"`），跟随全局 UI 缩放。
6. **颜色从 `getTheme()` 取**（`#include "theme.h"`），不要硬编码。
7. **配置读写走 `Config::set*` / `get*`**，写盘由 `flush()` 统一处理。
8. **物理更新用固定时间步长**（见 `GameWorld::update`），不要直接用帧 dt。
9. **渲染用世界坐标**，平移交给 SFML 的 `sf::View`，不要自己减摄像机偏移。

---

**开发习惯**：开始写代码前 `git pull`，完成一个小功能就 `add → commit → push`，保持提交历史清晰。