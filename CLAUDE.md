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

# 冒烟测试（**都需要 DISPLAY，手动跑**）
./build/release/wallpaper_smoke   # 壁纸：淡入淡出 / 窗口适配 / 缩略图
./build/release/settings_smoke    # 设置：点某一页的控件只改这一页的键
./build/release/scene_smoke       # 场景：构造 + 渲染几帧（抓构造期/排版期崩溃）

# Sanitizer
cmake --preset asan
cmake --build --preset asan
./build/asan/tests/unit_tests
```

> ⚠️ **Debug 构建下 SFML 的 `sf::Text` 和 `sf::Shape` 构造极慢**。编辑器帧率可能只有 20~30fps。**跑游戏和编辑器请用 Release**。

## 目录结构

```
include/ 和 src/ 一一对应，共 10 个层（从叶子到顶层）：

  utils/          零内部依赖的叶子：utf8 / lang / text_strings / vec2 /
                  animation / animator
  common/         共享基元：AppError / ContainerError
  infrastructure/ 设备层：Gamepad / GamepadVibration / KeyBindings
  wallpaper/      壁纸：WallpaperLibrary（扫盘/匹配，纯逻辑，可测）
                  + WallpaperLoader（后台线程预解码到 sf::Image，不需要 GL）
  config/         配置（header-only）+ keys.h（配置键的唯一权威来源）
                  + bootstrap（registerConfig）
  log/            日志三层：Logger / LogFormatter / LogHandler
                  + formatters/ + handlers/ + rotation + bootstrap
  core/           Container / Application / MainLoop / SceneRegistry /
                  SceneManager / Game / Paths / ResourceManager /
                  Platform / Achievement
  ui/             UI 组件与渲染（Button / Slider / TextInput / Window /
                  Upscaler / PostProcessor / Console / SoundManager /
                  Notification / Theme / ParticleSystem ...）
  game/           游戏本体（Player / Enemy / Level / GameWorld / Camera /
                  LevelValidator / LevelCodec / SaveManager ...）
  scene/          9 个场景 + console/（控制台命令）
                  （MainMenu / SaveSelect / LevelSelect / Game / Settings /
                   Console / Editor / Achievements / Stats）

  scene/tabs/     9 个设置 Tab（DisplayTab / InterfaceTab / WallpaperTab /
                  GraphicsTab / AudioTab / ControlsTab / GameTab /
                  ConsoleTab / AdvancedTab）+ settings_tab_id（归属表）
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
  config   log          ← 可以依赖 core 的 Container 契约
    ↑
  wallpaper             ← 静态资源内容（扫盘/匹配/预解码），不认识任何上层
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
- `wallpaper` 只解决"静态资源的内容"（哪个目录、哪张图、怎么解码），
  不认识 config / core / ui 及以上任何一层 —— 它被 `ui::Background` 用，
  但不反过来依赖 ui
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
   `Config` **只管键值**：以前它还兼职转发 `Paths` 的那几个目录方法
   （`assetFile` / `saveFile` / `assetsDir` …），同一个路径有两套入口，
   而且为此让 config 层反向依赖 core。0.3.7 起这些转发全删了。
7. **资源路径一律走 `ResourceManager` 的别名**，不要自己拼：
   ```cpp
   resources->get("levels", "level1.txt")   // ✅
   resources->dir("shaders")                // ✅
   paths->assetsDir() / "levels" / "x.txt"  // ❌ 散落、无防护
   ```
   别名表（`assets` / `levels` / `shaders` / `lang` / `defaults` / `wallpaper`）
   是"某个资源在包里的哪一层"的**唯一**定义处。加新资源目录 =
   在 `ResourceManager` 构造里加一行 `add(...)`。
   `get()` 带路径穿越防护（归一化后必须仍在别名子树内）；
   配置值里的 `${path:别名}` 由 `Config::get()` 自动展开
   （`Config` 用裸路径构造时没有资源表，原样返回）。
   **用户可写目录（config / saves / cache / temp）不走别名** ——
   那些跟机器绑定、落在 XDG 下，仍然由 `Paths` 提供。
   两者的分工可以这样记：`ResourceManager` 解决**路径**问题
   （"某个目录是哪一层"），`wallpaper::WallpaperLibrary` 解决**内容**问题
   （"这个目录下有几张图、哪张是用户选的"）。
8. **物理常量放 `include/game/game_constants.h`**（只放游戏自己的常量，
   设备属性不要塞进来）。
9. **每帧渲染用 `screenView`**，不用 `getDefaultView()`。
10. **UI 文字走 `Str::T(Str::Xxx)`**，字符串定义在 `include/utils/text_strings.h`。
11. **游戏事件走 EventBus**，不要从 GameWorld 直接调 SoundManager / ParticleSystem / Gamepad。
12. **着色器放 `assets/shaders/`**，`.frag` 后缀，GLSL 330 core。
13. **手柄焦点导航**：在**状态变化时**注册焦点（`onEnter` / `onResume` / `update` / 自己的 `syncFocus()`），
    **不要放在 `render()` 里** —— 渲染函数不该有副作用。设置 Tab 由 `registerFocus()` 收集。
14. **日志用 `log/logger.h` 的 `Logger`**（`logger->info(...)`），
    不要再用已删除的 `core/logging.h` 转发头。
15. **每层自带一个 bootstrap**（`registerXxx(Container&)`），入口的
    `boot()` 只负责按"底层在前"的顺序调用它们。
16. **新玩家第一反应是方向键**。默认键位是 `A` / `D` / `空格`（见
    `keybindings.cpp` 的 `kDefaults`），但方向键是**固定兜底**
    （`KeyBindings::fallbackKey()`：← → ↑ 对应左/右/跳），由
    `Player::handleEvent` 在校验可配置键位之外一并接受。
    加新的"会移动的角色"时照这个来 —— 只改默认值没用（键位是可配置的），
    把方向键塞进可配置键位更不行（一个动作只有一格，会覆盖玩家的设置）。
    菜单那边同理：`FocusGroup` 现在方向键/WASD 移动、**Enter** 激活
    （不用 Space —— 游戏里 Space 是跳跃，暂停菜单开着时会双触发）。
17. **跨层 include 用限定路径**（`utils/utf8.h`、`infrastructure/gamepad.h`）。
    `include/utils` 与 `include/infrastructure` **刻意不在 include 路径里**，
    这样平铺写法根本编译不过，跨层依赖在 include 行上直接可见。

### 语言文件的格式与它的三个坑

格式极简：一行一条 `中文原文=译文`，解析时按**第一个 `=`** 切开。
简单是有代价的，下面三条都"不报错，只是静默显示中文"：

1. **中文原文里不能有 `=`**。`Str::EditorHint` 曾经写成 `Shift+拖动=矩形`，
   于是它永远查不到译文（`=== TEXT-GAME 控制台 ===` 同理，它**以** `=` 开头）。
   把原文里的 `=` 换成空格即可；**译文里的 `=` 无所谓** —— 切分只看第一个。
2. **中文原文里不能有换行**。一行就是一条，`\n` 表示不了（`Str::CalcWhatWant`
   就是这种，只能原样用中文）。需要多行的文案请拆成多条。
3. **改中文原文 = 让旧译文静默失效**。键就是原文本身，改一个字，那条译文就再也
   匹配不上，界面退回中文而且没有任何提示。改文案时**必须**在 4 个语言文件里
   同步补一条新键（旧的那条留着变成死数据 —— 按约定不删已有行）。

查有没有漏翻（或原文改过而译文没跟上），跑这段就够：

```bash
python3 -c "
import re
h = open('include/utils/text_strings.h', encoding='utf-8').read()
consts = dict(re.findall(r'constexpr const char\* (\w+)\s*=\s*\"([^\"]*)\";', h))
for lang in ['en', 'ja', 'ko', 'zh-TW']:
    keys = {l.split('=', 1)[0] for l in open('assets/lang/' + lang + '.txt', encoding='utf-8')
            if '=' in l and not l.startswith(('#', '//'))}
    miss = [n for n, zh in consts.items() if zh and zh not in keys]
    print(lang, '缺', len(miss), miss)
"
```

目前只剩两条查不出来，都是上面第 1、2 条的已知无解项：`ConsoleTitle`
（以 `=` 开头，且它本来就是原样使用的）与 `CalcWhatWant`（值里有换行）。

## 常见任务入口

| 想改什么 | 去哪 |
| :--- | :--- |
| 玩家手感（速度/重力/跳跃） | `include/game/game_constants.h` |
| 添加新游戏对象 | `include/game/` + `src/game/`，在 `GameWorld::spawnLevelObjects` 和 `checkCollisionsSafe` 注册 |
| 添加新场景 | `include/scene/` + `src/scene/`，在 `scene_id.h` 加枚举 + 在 `registerScenes()`（`src/scene/bootstrap.cpp`）加一行工厂。**不用改 Game** |
| **添加新设置项** | 键加进 `include/config/keys.h`（含 `src/config/keys.cpp` 的 `allKeys()`）→ 在对应 Tab（`include/scene/tabs/xxx_tab.h/cpp`）加控件 → **在 `src/scene/settings_tab_id.cpp` 的归属表里登记**。不用改 SettingsScene |
| **加/删一个设置页** | 枚举与标签在 `include/scene/settings_tab_id.h`，归属表在同名 .cpp；`SettingsScene` 里补 4 处分发（焦点/事件/更新/渲染）+ 一个 Tab 类。忘了补会收获 `-Wswitch` 警告 |
| 加/换语言文案 | 中文原文写进 `include/utils/text_strings.h`，再往 `assets/lang/{en,ja,ko,zh-TW}.txt` 各补一行「中文=译文」。**三条坑见「语言文件的格式与它的三个坑」** |
| 添加新事件 | `include/game/event_bus.h` 加 struct + 加入 variant，然后 `GameWorld` emit + `GameScene` 订阅 |
| 添加关卡 | `assets/levels/levelN.txt`，参考已有格式；用 `validate_levels` 验证 |
| 加/换壁纸素材 | 直接丢进 `wallpaper/`（`.jpg` / `.jpeg` / `.png`），会被自动扫到。**不用改代码**；顺手补 `wallpaper/CREDITS.md` |
| 改壁纸行为（排序/过滤/过渡） | 扫盘与匹配在 `wallpaper/wallpaper_library.cpp`；过渡与适配在 `src/ui/background.cpp`；设置页在 `src/scene/tabs/wallpaper_tab.cpp` |
| 改壁纸缩略图大小/画质 | `kThumbnailMaxDim`（`wallpaper_thumbnail.h`）；行内显示尺寸在 `wallpaper_tab_layout` |
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

9 个 Tab 都是**自包含类**，各自持有状态变量、控件（unique_ptr）、
`handleEvent` / `update` / `render` / `refreshLabels` / `refreshSelection` /
`registerFocus` / `anyEditing`，外加一个 `reapply()`（见下）。

SettingsScene 只负责 Tab 切换、把事件/更新/渲染分发给当前页、
底部按钮（返回 / 恢复本页默认 / 高级页上的关于与全部重置）、
设计坐标系 View + 滚动。

**加设置项只需在对应 Tab 改 1~2 处** —— 但新键**必须**在
`src/scene/settings_tab_id.cpp` 的归属表里登记，否则 `test_settings_tabs.cpp`
会报"有键没有归属的设置页"。

### 设置页的身份与归属：settings_tab_id.h

`SettingsTab` 枚举 + `keysForTab(t)` + `unownedKeys()` 是**三处共用的唯一来源**：

1. `SettingsScene`：建 Tab 按钮（`settingsTabLabel()`）、分发
2. **「恢复本页默认」**：`resetKeys(keysForTab(t))` —— 把键从配置里**删掉**，
   于是各处读取点自然回落到自己的兜底值。默认值因此永远只有一处定义，
   不会出现"默认值表和应用点两边打架"（CLAUDE.md 里 vsync/fps_limit 那次翻车
   就是这么来的）。删完再调该页的 `reapply()`
3. `tools/settings_smoke.cpp`：逐页逐个控件点过去，检查**点这一页的控件
   只改了这一页的键**（见下）

⚠️ **`SettingsTab` 里刻意没有 `Count`**。放进去的话每个 switch 都得补一个
`case Count:` 才能过 `-Wswitch`，而那会让"加了新 Tab 却忘了处理"**不再报警**。
现在是独立常量 `kSettingsTabCount` + 穷尽 switch：加一页就会收获四条
`-Wswitch` 警告，指路明明白白。

⚠️ **各页的控件一律用具名指针，不要用 `toggles_[7]` / `multiRows_[12]` 这种数字
下标**。这个文件群以前全是下标，搬一行就要重排全部下标，而排错了不报错 ——
只是"点了这个改了那个"。0.3.8 重做时全部换成具名指针（`rowPseudo3D_` 等），
并用下面那个冒烟工具把结果钉住。

（顺带一个容易吓人的点：`addToggle()` / `addMulti()` 返回的是**堆上对象**的地址，
缓存下来是安全的 —— vector 扩容搬的是 `unique_ptr`，指向的对象不动。）

### 编辑器「试玩」（F5）

编辑器按 F5 把**当前编辑中（未保存）**的关卡直接丢进游戏跑，退出后回到编辑器继续改。
三步：

1. `PlaytestRequest`（`include/scene/playtest_request.h`，header-only）是个交接槽：
   编辑器 `request(lines_, 文件名)`，`GameScene::onEnter` 里 `take()` 取走
2. `GameScene` 走"从内存建关卡"这条路（`buildWorld()`），而不是 `loadLevel()` 读磁盘
3. 退出走 `SceneId::Back` —— 而 `nextScene_ = SceneId::X` 是 **push**
   （`Game::switchScene` 里 `push` / `Back→pop`），所以编辑器本来就在栈下面，
   "返回"天然回到编辑器。这一点不用额外写代码，但**依赖它**，改场景切换时要记得

几条刻意的设计：

- **`take()` 是一次性的**。不清空的话，"试玩 → 退出 → 从选关页进正式关卡"
  会让人又玩到那份旧草稿，看起来像"关卡加载错了"
- **试玩不写任何存档/PB/成就**（`playtestMode_` 挡着四处写入点）。试玩是
  "看看改得怎么样"，污染真实进度是最让人恼火的那种 bug
- **正式关卡进场景时会把 `playtestMode_` 清掉**。不清的话，上一条会反向生效：
  正式关卡也不写存档了，而且完全不报错
- **F5 不落盘**。顺手存一下看着贴心，实际会在用户没按 Ctrl+S 时覆盖磁盘上的关卡
- 没有玩家出生点（`P`）时按 F5 只给提示、不进游戏 —— 那种关卡能构造出来，
  但玩家会被摆在 (0,0) 卡在边界墙里

⚠️ 两边格式对不上是这功能最可能坏的方式（F5 能按、场景也切了，一进去弹
"关卡解析失败"）。`tests/test_playtest_request.cpp` 里**原样复刻**了
`EditorScene::loadFile()` 的读盘变换（去 `\r`、补到等宽）与 `startPlaytest()`
的拼接，再对 `assets/levels/` 里的**真实文件**跑一遍解析 —— 改任一边的格式都会红。

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

### 成绩 / 统计页

`SceneId::Stats`，从主菜单进。**一个字都不自己算** —— 数据全部来自
`Stats::summarize(SaveInfo)`（`include/game/stats.h`，纯函数、有单测）。

为什么抽成纯函数：成绩页是 Scene，构造要 `sf::Font`（GlResource），无头环境连
构造都做不到。而"某关没打过时那一行显示什么""存档数组长度不齐怎么办"恰恰最容易
写错。搬出来之后这些都能直接测。

页面本身只负责排版，几条刻意的做法：

- **关卡数不写死**：一屏放不下就按可用高度分栏，栏数又被可用宽度夹住
- **纵向布局按实测文字高度往下推**，不写死 y —— 字号走 `scaledFontSize()`，
  写死的话大字号下总计会和进度条叠在一起
- 每行的 `sf::Text` 是**成员**、按关数只增不减地扩容，**绝不在 render 里构造**
  （SFML 3.1 的 `sf::Text` 析构会死锁，CLAUDE.md 上面写了）
- 存档默认选**最近玩的那个**（`listSaves()` 已按 `lastPlayed` 降序），
  多个存档时给上一个/下一个按钮

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

### 壁纸系统

0.3.7 重做。三块职责分开，各自只干一件事：

| 组件 | 层 | 干什么 |
| :--- | :--- | :--- |
| `WallpaperLibrary` | wallpaper | 扫盘、排序、把 `current_wallpaper` 里的名字解析成下标（纯逻辑，可测） |
| `WallpaperLoader` | wallpaper | 后台线程把图**解码成 `sf::Image`** |
| `Background` | ui | 上传 `sf::Texture`、窗口适配、淡入淡出 |
| `WallpaperTab` | scene | 设置里的「壁纸」页：列出全部壁纸（缩略图 + 文件名），点一下就切 |

**为什么要拆**：`Background` 里有 `sf::Texture`（GlResource），无界面环境连
构造都做不到，所以它整个没法测。把"扫盘 + 匹配"这段纯逻辑挪到 wallpaper 层
之后就能直接测了（`tests/test_wallpaper_library.cpp`，11 个用例），
剩下的 GL 部分交给冒烟工具。

**`sf::Image` 不是 GlResource** —— 解码不需要 GL 上下文。这正是能把解码整个
扔进 worker 线程的原因；`sf::Texture` 的上传必须回主线程做。

**启动策略（"启动别卡"）**：构造 `Background` 时**只同步解码用户选中的那一张**
（实测 ~70ms），第一帧就能画；其余壁纸交给 loader 后台慢慢预解码，切图时
基本都已经 ready。启动时同步解码全部 5 张最坏要 1 秒多，那才是要避免的。

**切图**：`next()` / `loadByName()` 把新图放到 `back_` 图层并开始 0.5s 的
crossfade（旧图 alpha 1→0，新图 0→1），结束后 `front_ = std::move(back_)`。

**设置里的「壁纸」页**（0.3.7 从 InterfaceTab 里独立出来）：一行一张，
`[缩略图 160x90] [Button: 文件名]`。用 `Button` 当行是为了白拿点击、悬停、
选中态与手柄焦点导航（`FocusGroup` 收的就是 `vector<Button*>`，按位置做几何导航）。

⚠️ **缩略图必须先缩到 256 再上传纹理**（`wallpaper_thumbnail.h`）。壁纸原图
最长边到 3840，一张解码就是 33MB；5 张全尺寸纹理直接吃 160MB+ 显存，而那些
像素一个都不会真的显示出来。缩到 256 之后一张只有 147KB。缩放用**盒式平均**
而不是最近邻 —— 15 倍降采样下最近邻的锯齿会直接影响"这张图大概什么样"的判断。

`makeThumbnail()` 是纯函数、不碰 GL（`sf::Image` 不是 `GlResource`），所以
它有正常的单元测试（`tests/test_wallpaper_thumbnail.cpp`）。缩略图由
`WallpaperLoader` 在**解码那一刻顺带产出**（一次解码两个产物，不重复解），
UI 在 `update()` 里逐个上传成小纹理。

⚠️ **改这段代码时踩过两个只有运行时才能发现的坑**，都记在
`tools/wallpaper_smoke.cpp` 顶部：

1. **纹理不能按值持有**。`sf::Sprite` 内部存的是 `const sf::Texture*` —— 记的是
   **地址**。淡入结束时要把 `back_` 搬到 `front_`，纹理若按值成员就会换地址，
   sprite 便指向 moved-from 的空纹理。表现是**每次淡入结束壁纸直接消失**
   （实测画出来是纯白）。用 `unique_ptr` 持有，搬的只是指针就没事。
2. **新建的 `back_` 必须立刻 `fitToWindow`**。`render()` 里的重适配只在
   `lw != lastW_`（尺寸**变化**）时触发，而切图时尺寸没变 —— 漏了这一步，
   新图会以原生像素尺寸（3840×2160）从左上角画出来，淡入结束也没人再修。
3. **"用户选中的"和"画面上显示的"是两个概念**，`currentIndex()` /
   `currentFile()` 必须返回**选中的**那个（`selected_`），
   `displayedIndex()` 才是画面上的那张。淡入要 0.5s，这期间画面还是旧图，
   但设置里的 "n/m" 标签和 `current_wallpaper` 都要**立刻**反映用户的选择 ——
   两者合并成一个的话，点完"下一张"存进配置的还是旧名字，重启打回上一张，
   而且完全静默。同理 `next()` 要从 `selected_` 往前推而不是 `front_.index`，
   否则淡入期间连点两下只会原地打转。

验证方式是 `./build/release/wallpaper_smoke`（**需要 DISPLAY**，所以进不了
`tests/`）：它把背景渲染进 `RenderTexture` 取中心像素，和"同一张图不走淡入"
的基准渲染对比。**只判"不是纯黑"是抓不住上面第 1 个坑的** —— 那个 bug 画出来
是纯白。两个坑都用变异测试确认过会被抓到。

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

### 存档格式（`save_manager.cpp`）

`config/saves/*.conf` 是扁平的 `key=value`，**整体重写**：每个 setter 都
`loadSave` → 改一个字段 → 把**全部**字段重新写一遍。

⚠️ **每加一个字段，要改五个地方**：`createSave` / `updateProgress` /
`setLevelStar` / `setLevelBestTime` / 各自的 setter —— 漏掉任何一个，
用另一个 setter 存一次就会把新字段悄悄抹掉。
（`tests/test_save_manager.cpp` 里有一条专门盯它：存完金币再用另外两个 setter
各存一次，三个字段必须都还在。真要收拾就把写盘收成一个 `writeSave(info)`。）

新增字段本身是**向后兼容**的：老存档没有那一行 → 解析端回落默认值。
但默认值要么在 `loadSave` 开头铺好（`levelBestCoins` 就是 9 个 0），
要么在解析分支里补到 9 项 —— 给成空数组的话 setter 会因"关卡序号越界"直接失败。

### 场景为什么需要冒烟工具

场景类**写不了单元测试**：构造要 `sf::Font` / `sf::RenderTexture`，而它们是
`sf::GlResource`，没有 GL 上下文时**构造就 SIGABRT**；而且场景只在进入时才由
注册表工厂创建，所以"跑一下游戏"也抓不到**构造期**崩溃。

`tools/scene_smoke.cpp` 补这一块：把场景真的构造出来、`onEnter()`、渲染几帧、
再喂一个事件。它一跑起来就抓到了一个真 bug（见下），所以别把它当成形式主义。

⚠️ 这个工具的 DISPLAY 判断**必须用 `DISPLAY`（X11）**，`WAYLAND_DISPLAY` 不算数：
这份 SFML 是 X11 后端，只有 `WAYLAND_DISPLAY` 而没有 `DISPLAY` 时构造窗口照样
SIGABRT（实测 exit=134 而不是工具自己的 exit=2）。判断错的话，"没有 DISPLAY 时
优雅退出"这条保护就形同虚设。

### ⚠️ 加载失败时 `GameScene` 的 `world_` 是空的

`GameScene::onEnter()` 里 `loadLevel()` 失败时**只记日志 + 发通知**，`world_`
保持 null。而 `render()` / `update()` 里有十来处 `world_->…` ——
`scene_smoke` 第一次跑起来就是在这里**每帧 SIGSEGV**：用户永远看不到那句
"关卡加载失败"，只看到程序崩了。

现在的做法是三道一起上：`onEnter` 失败即 `nextScene_ = SceneId::Back`（离开场景）、
`update()` / `render()` 开头各有 `if (!world_) return;` 挡住。

**真实可达路径**：存档里的 `currentLevel` 超出现有关卡文件数（关卡文件被删过、
或存档来自关卡更多的版本）、关卡文件缺失或损坏。

教训：**"出错时继续往下走"的代码，必须检查那条路上每个使用者都容得下空状态**。
这里 `onEnter` 很体贴地发了通知，然后 `render` 就把通知连同窗口一起崩掉了。

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
- **发版说明走附注 tag，但有两个坑**（0.3.7 时全踩了，正文只剩一行自动 changelog）：

  1. **打 tag 必须加 `--cleanup=verbatim`**：
     ```bash
     git tag -a --cleanup=verbatim -F /tmp/notes.md v0.3.7
     ```
     默认的 `cleanup=strip` 会把 **`#` 开头的行当注释吃掉** ——
     说明里的 `## 0.3.7 xxx` 标题就这么没了，而且 git 不会给任何提示。
  2. **`action-gh-release` 不会自动拿 tag 说明当正文**。只设
     `generate_release_notes: true` 的话，正文是一行自动生成的
     `**Full Changelog**: ...`，手写的说明根本不出现。
     现在 `release.yml` 里加了一步 `git tag -l --format='%(contents)'`，
     用 `body_path` 显式喂进去 —— 改的是 workflow，不用每次手工编辑 release。

  ⚠️ 别被历史发布迷惑：v0.3.6 的正文看着是手写说明，**那是发布后手工编辑的**，
  不代表 workflow 会写进去。要看正文对不对，发完用
  `gh release view v0.3.7 --json body --jq .body` 核一遍。
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
- **开发模式找源码树是运行时做的，不要用编译期宏**。
  `Paths::findDevRoot()` 从可执行文件往上找「同时有 `CMakeLists.txt` 与 `src/`」
  的那一层，最远 6 层，覆盖 `build/release`、`build/tests/tests`、
  `build/asan/tests` 这些深度。
  以前是 `-DPROJECT_ROOT=<构建机路径>`，那会把开发者的家目录编进**每一个**
  发出去的二进制里 —— `makepkg` 会直接报「软件包含有对 $srcdir 的引用」，
  `namcap` 也会 flag，而那个路径在用户机器上根本不存在。
  验证方法：`strings <任意构建的 text_game> | grep -c "$PWD"` 应为 **0**
  （开发构建、`release-package`、以及不开 `BUNDLE_RUNTIME_DEPS` 的发行构建都要为 0）。
  测试自己要读源码树（分层测试扫 `src/` `include/`、关卡测试读 `assets/levels`），
  那一份 `PROJECT_ROOT` 由 `tests/CMakeLists.txt` 单独定义 —— 测试不对外分发。
- **`Paths::createAll()` 只建用户数据目录**（config/cache/temp/saves），
  不建 `assets/` 与 `wallpaper/` —— 判定「打包模式」看的就是 assets/ 在不在，
  凭空造一个空目录会让下次启动误判成打包模式，然后一路加载失败。
- **发行版默认设置放 `assets/defaults/preferences.conf`**，由
  `config/bootstrap.cpp` 在用户没有配置文件时铺一次。**只放观感类设置**。

  ⚠️ **跟机器/显卡绑定的性能键一个都不能放**，否则每个新用户都继承开发机的状态：

  | 键 | 为什么不能放 | 不放时走哪 |
  | :--- | :--- | :--- |
  | `player_name` | 个人名字 | 空 |
  | `resolution_index` | 机器分辨率 | 代码兜底 |
  | `window_mode` / `fullscreen` | 机器窗口习惯 | 代码兜底 |
  | `ui_scale` / `font_scale` | 按开发机屏幕调的 | 1.0 |
  | `current_wallpaper` | 个人口味 | 列表第一张 |
  | `remember_window_size` | 与 `runtime.conf` 联动 | 代码兜底 |
  | `key_*` | 个人键位 | 代码兜底 |
  | **`vsync` / `fps_limit` / `anti_aliasing`** | **跟显卡与显示器绑定** | `bootstrap.cpp` / `display_tab.cpp` |

  最后一行是 0.3.4 才修的：0.3.1~0.3.3 把开发机的
  `vsync=false` + `fps_limit=0` + `anti_aliasing=0` 原样发了出去，
  而 `fps_limit=0` 在 SFML 里就是**不限帧** —— 新用户默认不锁帧、不垂直同步、
  不抗锯齿，显卡满载空转。**同一份默认值出现在两个地方且互相打架，本身就是 bug**：
  要么只留代码里的兜底，要么只留这个文件，别两边都写。

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
| ResourceManager（别名寻址 / 路径穿越拒绝 / `${path:别名}` 展开） | ✅ |
| WallpaperLibrary（扩展名过滤 / 全名与主名匹配 / 中文名 / 越界 / 空目录） | ✅ |
| WallpaperThumbnail（等比缩放 / 盒式平均 / 不放大 / 极端宽高比 / 内存预算） | ✅ |
| SettingsTab 归属（每个键恰好属一页 / 表里无错名 / `tabForKey` 互逆 / 可携带白名单） | ✅ |
| SettingsCodec（分享码往返 / 数字不被 RLE 吞 / 非法与截断输入 / 内存放大上限） | ✅ |
| SettingsShare（端到端往返 / **机器绑定的键不被带走** / 恶意文本只认白名单） | ✅ |
| ParticleSystem 密度（半分/两倍/零/负数夹取/容量上限） | ✅ |
| ShakeIntensity（0 = 不抖 / 2 倍偏移恰为 1 倍的两倍 / 关掉开关后强度无效） | ✅ |
| KeyBindings（默认键位固定成测试 / resetToDefaults 回到默认 / 每个动作都有键且不重复） | ✅ |
| Platform::openDirectory（目录不存在 / 传文件 / 空路径 → false，不去执行外部命令） | ✅ |
| PlaytestRequest（取走即清空 / 覆盖 / clear / 无出生点识别 / **内存路径与磁盘路径逐格等价**) | ✅ |
| KeyBindings 方向键兜底（**改键后兜底仍生效** / 兜底键不撞任何默认键位） | ✅ |
| FocusNav::Repeater（按下沿立刻触发 / 未到延迟不重复 / 按节拍重复 / 松开清状态） | ✅ |
| EnemyKind（四种敌人各自的 AI / **现有 E 的行为逐字未变**（停顿帧数 == 0）） | ✅ |
| Stats 聚合（数组长度不齐 / 星级越界夹取 / 只累加有记录的 / 空存档不除零） | ✅ |
| SaveManager 每关最佳金币（只增不减 / **五个写盘点都不抹掉它** / 老存档缺字段可读） | ✅ |
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

**设置页的归属由两条腿守着**：`tests/test_settings_tabs.cpp` 保证"每个配置键都
恰好属于一页"（加了键却忘了归类会直接报出来），`tools/settings_smoke.cpp`
则真的逐页逐个控件点过去，比对点击前后配置键的差集，保证"点这一页的控件
只改这一页的键"。工具有一个坑值得记住：`registerFocus()` 是**追加**语义，
收两次列表就会翻倍，于是"跳过最后 4 个动作按钮"跟着错位 ——
结果是**真的去打开文件管理器**（踩过一次）。

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

关卡里的敌人有四种：`E` 基础（遇墙/悬崖立刻掉头）、`W` 巡逻（掉头后停顿
0.35s）、`F` 飞行（不受重力、正弦浮动）、`B` 跳跃（贴地待机、周期起跳）。
**四种都在编辑器的笔刷条里**，不用手写关卡文件。

⚠️ `E` 与 `W` 的差别**只有"掉头后停一下"**：边缘掉头是两者共有的
（`cliffAhead()` 一直在转向条件里），别以为 `W` 才是会看边缘的那个 ——
`tests/test_enemy.cpp` 里有一条专门断言 `E` 的停顿帧数 `== 0`。

画完关卡后跑 `./build/debug/validate_levels assets/levels` 验证，或者在编辑器里按 T 看可达性。

## 依赖

- **SFML 3**（不是 2.x）
- **CMake ≥ 3.23 + Ninja**
- **doctest 2.5**（单头文件，已在 `tests/doctest.h`）
- **OpenGL**（用于截图 `glReadPixels`）

## License

MIT