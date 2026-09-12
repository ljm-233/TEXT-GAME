# TEXT-GAME

一个基于 C++ 的文本游戏/工具项目，包含简易的计算器示例、日志模块、时间工具，以及一个用于解决依赖注入（DI）的简易容器。

## 📁 项目结构

```text
TEXT-GAME/
├── .vscode/               # VS Code 调试与任务配置
├── build/                 # CMake 构建产物（不提交至 Git）
├── include/               # 头文件目录
│   ├── application.h      # DI 容器 + Application 主类声明
│   ├── logging.h          # 日志模块（支持颜色、多级别、线程安全）
│   └── time_utils.h       # 时间工具接口
├── scripts/               # 辅助脚本（如构建、打包脚本）
│   └── tree.sh
├── src/                   # 源文件目录
│   ├── application.cpp    # Application 实现，负责注册依赖
│   └── main.cpp           # 程序入口（仅启动 Application）
├── .gitignore
├── app.log                # 运行时的日志文件 （实际上不在这里）
├── CMakeLists.txt
└── README.md
```

## 🧩 核心模块说明

| 文件/模块 | 职责 |
| :--- | :--- |
| `time_utils.h` | 提供时间同步与格式化接口 |
| `logging.h` | 日志模块，调用时间接口，支持彩色终端输出与文件记录 |
| `application.h` / `application.cpp` | 包含主类 `Application` 与简易 DI 容器 `Container` |
| `main.cpp` | 程序入口，创建 `Application` 并运行 |

## 🔧 依赖注入（DI）设计

项目使用一个简易的 `Container` 类来管理对象的创建与依赖关系。
- 在 `Application` 的构造函数中**注册类型**（如 `Logger`）。
- 在 `Application::run()` 中**解析并获取实例**。
- 新增类时，只需在 `Application::Application()` 中注册，无需修改 `main.cpp`。

## 🚀 构建与运行

**环境要求**：Linux / macOS / WSL，已安装 `g++`、`CMake` 和 `Ninja`。

```bash
# 进入项目根目录
cd TEXT-GAME

# 配置（使用 Ninja 作为生成器）
cmake .. 

# 编译
cmake --build . -j

# 运行
./text_game
```

> **提示**：如果在 VS Code（仍然建议VSCodium）中开发，可直接使用 `.vscode` 中预配置的 `tasks.json` 和 `launch.json`，按 `F5` 即可调试运行。

## 📥 克隆（下载）

建议先创建一个名为 `coding` 的文件夹用于存放项目。

```bash
# 进入你想存放项目的目录
cd ~/coding

# 克隆仓库（推荐 SSH，一次配置永久免密）
git clone git@github.com:ljm-233/TEXT-GAME.git

# 或者使用 HTTPS（需要输入 Token）
git clone https://github.com/ljm-233/TEXT-GAME.git

# 进入项目
cd TEXT-GAME
```

> **SSH 配置**：如果尚未配置 SSH 密钥，请先生成并添加到 GitHub 账户。
> 测试连接：`ssh -T git@github.com`

## 📤 推送（上传）

每次写完代码后，按以下步骤提交并推送：

```bash
# 查看改了哪些文件
git status

# 添加所有改动到暂存区
git add .

# 提交，并写清楚改了什么
git commit -m "改了什么写什么"

# 推送到远程 main 分支
git push
```

首次推送如果还没绑定远程分支：

```bash
git push -u origin main
```

## 👥 多人协作

推送前先拉取远程最新代码，避免冲突：

```bash
# 拉取远程最新代码并变基
git pull --rebase

# 如果有冲突，手动解决冲突文件后：
git add .
git rebase --continue

# 再次推送
git push
```

---

**开发习惯建议**：开始写代码前先 `git pull`，完成一个小功能就 `add → commit → push`，保持提交历史清晰。