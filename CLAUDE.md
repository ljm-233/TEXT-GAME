# CLAUDE.md

给 AI 助手的项目说明。新会话请先读这个文件。

## 项目概览

`TEXT-GAME` —— 用 C++20 + SFML 3 从零手写的 2D 平台跳跃游戏。
不依赖游戏引擎，物理、UI、音频全部自研。

## 构建 / 运行 / 测试

```bash
# 构建
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 运行
./build/text_game

# 测试（需要 -DBUILD_TESTS=ON）
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j
./build/tests/unit_tests

# 关卡可达性验证
./build/validate_levels

目录结构
text
include/ 和 src/ 一一对应：
  core/    Application / Game / SceneManager / Logger / Platform
  config/  BootstrapConfig / RuntimeConfig / Preferences（header-only）
  scene/   6 个场景类
  ui/      UI 组件（Button / Slider / Console / SoundManager / Gamepad...）
  game/    游戏本体（Player / Enemy / Level / GameWorld / Camera...）
tests/     doctest 单元测试
tools/     validate_levels 命令行工具
assets/    levels/*.txt（ASCII 关卡）+ font.otf
分层架构
text
core      ← 场景/应用层
  ↑
scene     ← 具体场景
  ↑
ui / game ← UI 组件 + 游戏对象（互不依赖）
重要：game/ 层不应该 #include ui/ 层的东西。
GameWorld 通过 EventBus 发事件，GameScene 订阅处理音效/粒子。

关键约定
新增 src/ 一级子目录时要改 CMakeLists.txt 的 GLOB 列表，.cpp 会被自动扫描。

中文必须走 toSf()（include/ui/utf8.h），否则 SFML 3 按 Latin-1 解释。

字号用 scaledFontSize()（include/ui/ui_scale.h），跟随 UI 缩放。

颜色从 getTheme() 取（include/ui/theme.h），不硬编码。

配置读写走 Config::set*/get*，写盘由 flush() 统一处理。

物理常量放 include/game/game_constants.h。

新场景需要在 .h 里声明 onEnter/onResume，SceneManager 会自动调用。

每个 Scene 的 render 末尾要调 FocusGroup::instance().setItems({...})（手柄导航）。

常见任务入口
想改什么	去哪
玩家手感（速度/重力/跳跃）	include/game/game_constants.h
添加新游戏对象	include/game/ + src/game/，然后在 GameWorld::spawnLevelObjects 和 checkCollisionsSafe 里注册
添加新场景	include/scene/ + src/scene/，然后在 Game::createScene 里加 case
添加新设置项（新方式） | 参照 `include/scene/tabs/audio_tab.h`：新建 `xxx_tab.h/cpp`，SettingsScene 只转发
添加新设置项（旧方式） | SettingsScene 里改 6 处，参照 displayToggles_ / interfaceMultiRows_ 等
添加新事件	include/game/event_bus.h 加 struct + 加入 variant，然后 GameWorld emit + GameScene 订阅
添加主题	include/ui/theme.h 加枚举 + src/ui/theme.cpp 加颜色组
已知陷阱
Game::createScene 里不要有副作用。GameScene 的 takePendingSave() 已挪到 onEnter()。

SceneManager::pop() 不重建场景，history_ 里保存的是 unique_ptr<Scene>。

FocusGroup::setItems 目前在 render() 里调用（反模式，待重构）。

EventBus::subscribe 的回调是同步的，emit 时会立刻执行。

save_manager::updateProgress 的 progress 参数实际存的是金币数（命名误导）。

- SettingsScene 有 60+ 个 unique_ptr<Button> 成员，正在逐步重构。
- ⭐ **新架构**：Tab 自包含（见 `tabs/audio_tab.h`），SettingsScene 只转发。
- ⭐ 迁移中的 Tab：AudioTab 已完成，其他 Tab（Display/Interface/Graphics/Keys/Other）待迁移。

PauseMenu::render 会重设 FocusGroup::setItems，与 Scene 的焦点注册冲突。

测试现状
模块	覆盖
Vec2 / AABB / Config / Level / Camera / Animator / Player / MovingPlatform / JumpPad	✅
Enemy / Coin / Checkpoint / Door / Spike / Key / GameWorld / Scene	❌
SaveManager / Logger / UI 组件	❌（依赖运行环境）
测试写法：tests/test_*.cpp，参考 tests/test_player.cpp 的"无 sprite 模式"技巧。

关卡设计规范
玩家能力上限（基于当前物理常量）：

向上跳 ≤ 3.5 格

水平跨 ≤ 5.5 格

弹跳板 J ≤ 10 格高

画完关卡后跑 ./build/validate_levels 验证。

依赖
SFML 3（不是 2.x）

CMake ≥ 3.20 + Ninja

doctest 2.5（单头文件，已在 tests/doctest.h）

License
MIT

text

---

## 5B：替换 `.gitignore`

**操作**：打开 `.gitignore`，**全选删除**，粘贴：

```gitignore
# ===== 构建产物 =====
/build/
/build-*/
/out/
cmake-build-*/
compile_commands.json
CMakeCache.txt
CMakeFiles/

# ===== 运行时数据（不应进 git）=====
/config/*.conf
/config/*.log
/config/*.log.*
/saves/
/cache/
/temp/

# ===== 平台 =====
.DS_Store
Thumbs.db

# ===== 编辑器临时文件 =====
*.swp
*.swo
*~

# ===== 打包产物 =====
*.tar.gz
*.zip
_CPack_Packages/
注意：如果 config/ 或 saves/ 里已经有文件被 git track 了，加 .gitignore 不会自动移除。执行一次：

bash
git rm -r --cached config/ saves/ 2>/dev/null
git rm --cached config/*.conf config/*.log 2>/dev/null
（如果报"did not match any files"就忽略。不要加 -f，否则会删本地文件。）