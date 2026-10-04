# AlbusWall 架构报告（面向 C++ 复刻）

- 分析对象：`/home/link/albuswall/src/albuswall/`（174 个 `.py`，≈25,300 行；不含 `.venv`）
- 分析方式：全量 AST import 图统计 + 逐文件精读（实际打开 60+ 个源文件）
- 约束：本报告为只读分析的产物，未修改 albuswall 任何文件
- 证据格式：`相对路径:行号`（相对 `src/albuswall/`）

---

## 0. 一张图看懂分层

```
                       ┌──────────────────────────────────────────────┐
   最外层（可替换）     │  plugins/            ui/builtin/**            │
                       │  （插件适配层）       （内置 PySide6 前端）      │
                       └───────────┬──────────────────┬───────────────┘
                                   │ @declare/@ui_loader│ implements UIProtocol
                       ┌───────────▼──────────────────▼───────────────┐
   前端契约层           │  plugin/ (declare/discovery/manager)          │
                       │  ui/ (protocol.UIProtocol, bootstrap, common) │
                       └───────────┬──────────────────────────────────┘
                                   │ register/get by string key
                       ┌───────────▼──────────────────────────────────┐
   编排层               │  core/  Container · Application · MainLoop    │
                       │         Runtime（异常→退出码）                 │
                       └───────────┬──────────────────────────────────┘
         ┌─────────────────────────┼───────────────────────────┐
         │                         │                           │
 ┌───────▼────────┐      ┌─────────▼──────────┐      ┌─────────▼────────┐
 │ services/      │      │ repositories/      │      │ configue/        │
 │ 业务用例编排    │─────▶│ SQL ↔ DTO          │      │ 配置引擎(声明/解析)│
 └───────┬────────┘      └─────────┬──────────┘      └─────────┬────────┘
         │                         │                           │
         └───────────┬─────────────┴───────────────┬───────────┘
                     │                             │
            ┌────────▼─────────┐          ┌────────▼────────┐
   基础设施  │ infrastructure/  │          │ log/  common/    │
            │ db · task · trig │          │ utils/ resources/│
            └──────────────────┘          └─────────────────┘
                     ▲
            ┌────────┴─────────┐
   纯数据    │ dto/  (+ui/vo/)  │   叶子：无业务依赖、可跨进程 pickle / 纯展示
            └──────────────────┘
```

依赖方向（实测，仅列运行时 import，非 `TYPE_CHECKING`）：

```
__main__  → configue, core, infrastructure, log, plugin, repositories, services, ui
core      → log, plugin, utils
plugin    → (无运行时依赖；core 仅 TYPE_CHECKING)
ui        → configue, core, plugin
ui.builtin→ configue, core, dto, log, plugin, resources, ui.vo, __about__
configue  → log
log       → configue, core
services  → configue, core, dto, infrastructure, log, repositories, utils
repositories → common, dto, infrastructure, log, utils
infrastructure → common, configue, core, dto, log, resources
dto       → common, utils
utils     → （无 albuswall 内部依赖）
resources → （无）
ui.vo     → （无）
```

---

## 1. 顶层包划分与职责

| 包 | 文件数 | 行数 | 一句话职责 |
|---|---:|---:|---|
| `common/` | 8 | 176 | 全项目共享的**无依赖叶子**：领域枚举、异常基类、Future 工具 |
| `configue/` | 15 | 3 091 | **配置引擎**：声明式字段 + 惰性 Resolver + 只读静态配置 + 可持久化动态状态 |
| `core/` | 5 | 765 | **生命周期编排**：DI 容器、Application 五阶段、主循环抽象、退出码映射 |
| `dto/` | 8 | 1 399 | **数据传输对象**：DB 行 ↔ 业务对象，跨层/跨进程边界的纯数据 |
| `infrastructure/` | 16 | 2 148 | **技术设施**：SQLite 连接器、任务服务（线程/进程池+背压）、触发器、原子文件写 |
| `log/` | 9 | 234 | 日志门面：`TRACE` 级别、`Logger` 协议、启动期内存缓冲与重放 |
| `plugin/` | 4 | 384 | **插件机制**：声明（`@declare`/`@ui_loader`）、发现、注册表与激活 |
| `plugins/` | 2 | 5 | **内置插件包**（发现器扫描的唯一内置目录），当前仅 `builtin_ui` |
| `repositories/` | 10 | 2 725 | **数据访问**：SQL 唯一出口，行 → DTO，写串行化与事务 |
| `resources/` | 2(+资产) | 108 | **静态资源寻址**：别名 → 打包内路径，含 SQL schema / 主题 / 图标 / 默认 ini |
| `services/` | 11 | 2 937 | **业务用例**：源扫描、导入、缩略图、视图组合、触发器同步 |
| `ui/` | 70 | 10 380 | **前端契约 + 内置 PySide6 实现**（`ui/` 只有契约，`ui/builtin/` 才是 Qt） |
| `utils/` | 11 | 774 | **通用无状态工具**：信号、可观察属性、注册表样板消除、路径/JSON/图片 |

### 1.1 各包目录清单（关键文件 + 一句话）

**`common/`**
```
common/__init__.py                 空包标记
common/exceptions.py               BackpressureError（任务背压，携带 reason/queue 尺寸）
common/futures.py                  wait_with_timeout：规避 as_completed 卡死
common/enums/__init__.py           枚举统一出口（UpdateMode / FileTypeCheckMode / CandidateStatus / ThumbSpec）
common/enums/trigger.py            UpdateMode（MANUAL / SCHEDULED_TIME / INTERVAL_TIME / DEVICE_TRIGGER）
common/enums/source.py             CandidateStatus
common/enums/import_.py            FileTypeCheckMode
common/enums/thumbnail.py          ThumbSpec
```

**`configue/`**
```
configue/__init__.py               对外门面 + 惰性 __getattr__ 导出（切断循环 import）
configue/configue.py               Configue 单例：static(只读视图) + dynamic(可挂载子树) + resolver
configue/resolver.py               Resolver：${sec:key} 展开 + 类型转换 + 记忆化 + 环检测 + 反向失效
configue/declaration.py            ConfigDeclaration / ConfigField 描述符（声明式配置的核心）
configue/dynamic_declaration.py    @dynamic_state：把 StatefulNamespace 登记为运行时可挂载子树
configue/state.py                  ObservableContext/StateContext、StateField、Observable/StatefulNamespace
configue/bootstrap.py              setup_static_config / setup_dynamic_config 两阶段引导
configue/exceptions.py             ConfigError（带 source/key/value/cause）
configue/utils/namespace.py        Namespace / FrozenNamespace（点号访问）
configue/utils/ini_parser.py       RawINIParser / TypedConfigParser / DictInterpolation / _convert
configue/utils/ini_single_section_parser.py  parse_section_file（只读一个 section）
configue/utils/ini_transform.py    INI 值变换（# ; 转义等）
configue/utils/deep_merge.py       deep_merge_dicts
configue/utils/platform_dir.py     get_user_config_dir / get_user_data_dir / get_cache_dir / get_temp_dir
```

**`core/`**
```
core/__init__.py                   Container / Application / MainLoop / HeadlessMainLoop / Runtime
core/container.py                  name→factory→instance 最小注册表（不认识生命周期）
core/application.py                五阶段编排 + 四类钩子 + 异常钩子 + 单例门面
core/main_loop.py                  MainLoop(ABC) + HeadlessMainLoop（Event.wait + 信号）
core/runtime.py                    顶层异常→退出码、SIGINT 优雅退出、日志 handler 收尾
```

**`dto/`**
```
dto/album.py                       Album / AssetDTO（+ AssetDTO.from_row）
dto/source.py                      IngestSource / Create / Update / FormData / SourceScanFinished（600 行，最大 DTO）
dto/import_.py                     AssetCandidateCacheDTO / AssetCreateDTO（跨进程任务载荷）
dto/thumbnail.py                   ThumbSpec / ThumbnailPaths / ThumbnailTaskInput / Stats / Result
dto/task.py                        ExecutorType / BackpressureEvent / BackpressureReason / TaskServiceStats
dto/trigger.py                     TriggerConfig（is_void / scheduled / device_trigger）
dto/sentinel.py                    UNSET / PatchField：PATCH 语义的三态标记
dto/__init__.py                    空（有意不聚合，避免隐式重依赖）
```

**`infrastructure/`**
```
infrastructure/fs.py               atomic_write / atomic_write_bytes（同目录 tmp + os.replace）
infrastructure/database/connector.py   Connector：threading.local 每线程连接、PRAGMA、schema 版本与迁移
infrastructure/database/bootstrap.py   register_database + keep_alive/release 生命周期钩子
infrastructure/task/protocol.py        TaskServiceProtocol（对外最小契约：submit/shutdown/get_stats/set_backpressure_callback）
infrastructure/task/service.py         TaskService：优先级队列 + 背压 + psutil 动态并发 + 线程/进程混合
infrastructure/task/_semaphore.py      DynamicSemaphore：可动态调上限的信号量
infrastructure/task/_task.py           _Task：可排序任务项（priority, seq）
infrastructure/task/bootstrap.py       register_task_service + on_final shutdown
infrastructure/task/__main__.py        `python -m ...task` 演示入口
infrastructure/trigger/__init__.py     register_trigger_service（facade + scheduler + on_final stop）
infrastructure/trigger/trigger.py      TriggerFacadeService：多后端门面，纯推送式（不读仓储）
infrastructure/trigger/scheduler.py    SchedulerService（APScheduler：Cron/Interval，幂等 start/stop/upsert）
infrastructure/trigger/protocols.py    TriggerHandler / DeviceTriggerHandler 协议
```

**`log/`**
```
log/__init__.py                    导出 TRACE/Logger/getLogger；setup_log 走 __getattr__ 惰性导入
log/common.py                      TRACE = 5
log/type.py                        Logger Protocol（trace/debug/.../addHandler）
log/logger.py                      getLogger：给标准 logger 挂 .trace 方法
log/bootstrap.py                   setup_log：fileConfig + 移除内存缓冲 + 重放启动期日志
log/handlers/memory_chache_handler.py  MemoryCacheHandler（BufferingHandler，flush/close 故意 no-op）
log/formatters/multiline_formatter.py  MultilineFormatter（多行日志每行带头部）
```

**`plugin/`**
```
plugin/declaration.py              @declare / @ui_loader
plugin/discovery.py                discover_package / discover_entry_points / discover_directory / discover_all
plugin/manager.py                  PluginManager 单例：declare / declare_ui / _module_owner / activate / activate_ui
```

**`plugins/`**
```
plugins/builtin_ui.py              from albuswall.ui.builtin.factory import *（唯一的“内置插件包”文件）
```

**`repositories/`**
```
repositories/__init__.py           Repositories TypedDict（类型表=唯一事实来源）+ registry_repository
repositories/base.py               BaseRepository：_execute/_fetchone/_fetchall/_transaction + 全局写锁
repositories/view.py               读侧仓储（虚拟相册 scope、相册、邻居定位、磁盘路径）
repositories/asset.py              写侧仓储（收藏/软删/恢复/硬删/相簿成员）——与 view 严格分工
repositories/source.py             IngestSource 仓储（含 TriggerConfig 映射）
repositories/import_.py            候选缓存 / 资产入库
repositories/thumbnail.py          缩略图列读写（get_paths / get_task_input / clear_and_snapshot）
repositories/utils/exif.py         EXIF 解析纯函数
repositories/utils/sql_helpers.py  SQL 片段辅助
```

**`resources/`**
```
resources/core.py                  CURRENT_PATH + PATH_ALIASES + get(alias)（含路径穿越防护）
resources/__init__.py              __getattr__ 别名访问（theme/qss/schema/..._dir）
resources/sql/media_library_schema.sql  建表 SQL（Connector 启动时 executescript）
resources/themes/default/main_window.qss + icons/*.svg
resources/config/conf.ini          默认主配置（含 ${path:config} 插值示例）
resources/config/log_config.ini    默认 logging.config 文件
```

**`services/`**
```
services/__init__.py               Services TypedDict + _ARG_MAP + register_service（注册 + 全部信号接线 + 生命周期）
services/source.py                 SourceService：扫描源→候选缓存，状态收敛到 SourceServiceState
services/import_.py                ImportService：候选→进程池→落库；纯函数层可 pickle
services/thumbnail/service.py      缩略图编排：提交任务/去重/失败分类/冷却/回填/purge
services/thumbnail/config.py, paths.py, renderer.py, storage.py  配置 / 目录布局 / 纯渲染 / 磁盘写
services/view.py                   ViewService：读组合 + 虚拟相册（All/Trash）分派
services/trigger_sync.py           源配置信号 → TriggerFacadeService（唯一 import 仓储的触发器组件）
```

**`ui/`**
```
ui/protocol.py                     UIProtocol（name/main_loop/setup/teardown）+ _REGISTRY + register_ui/get_ui
ui/bootstrap.py                    registry_ui：按配置激活 ui_loader → 取类 → 注册 "ui" 工厂 + teardown 钩子
ui/common.py                       DEFAULT_THEME="default" / DEFAULT_UI="builtin"
ui/vo/album.py                     TitleBarVO（纯展示 VO，无 Qt 依赖）
ui/builtin/factory.py              @declare("builtin_ui") + @ui_loader("builtin")
ui/builtin/application.py          @register_ui("builtin") 的 QApplication 子类 + MainLoop 适配 + setup/teardown
ui/builtin/presenter/*             PresenterManager + 6 个 presenter（view 层组合根）
ui/builtin/model/*                 QAbstractListModel（只持有数据）
ui/builtin/delegate/*              绘制层
ui/builtin/widgets/**              UI 组件库（虚拟滚动、图片异步加载、模糊标签、网格…）
ui/builtin/window/**               主窗口与各功能面板（titlebar/album/content/menu/source/detail）
ui/builtin/setting/**              设置页（Tab 栈）
ui/builtin/config/**               UI 侧配置声明（static / preference / window state）
ui/builtin/anims/**, platform/**, utils/**  动画、无边框窗口缩放、Qt objectName 工具
```

**`utils/`**
```
utils/registry.py                  build_getters / register_all：把「类型表」变成容器注册（本模块不依赖 core）
utils/signal.py                    Signal / ProcessSignal / SignalDescriptor
utils/observable_property.py       ObservableProperty：带变更回调的 property
utils/path.py                      is_valid_mount_point / iter_files_depth_first / join_path
utils/decode_json.py               json_loads_dict / json_loads_list
utils/image.py                     open_normalized（EXIF 方向归一）
utils/mount.py                     auto_mount（挂载点处理）
utils/format.py                    factory_repr（容器 __str__ 用）
utils/sql_log.py                   SQL 日志格式化
utils/time.py                      时间工具
```

---

## 2. 依赖方向规则（含实际证据）

### 2.1 规则表

| 层 | 允许 import | **绝对禁止** | 实测结论 |
|---|---|---|---|
| `utils/`, `resources/`, `ui/vo/` | 仅标准库 | 任何 albuswall 包 | ✅ 成立（AST 图中零内部边） |
| `common/` | 标准库 | 任何 albuswall 包 | ✅ 成立（`common/futures.py:5` 仅 stdlib） |
| `dto/` | `common/`, `utils/` | `core/services/repositories/ui/infrastructure` | ✅ 成立（`dto/source.py:28-30`、`dto/thumbnail.py:7-8`） |
| `log/` | stdlib + `configue`（仅 bootstrap） | 业务/UI | ⚠️ `log/bootstrap.py:12` import `configue.ConfigField` |
| `infrastructure/` | `common/configue/core/dto/log/resources` | `services/repositories/ui` | ✅ 静态成立；⚠️ 通过容器字符串键反向依赖 services（见 2.3-V4） |
| `repositories/` | `infrastructure/dto/common/log/utils` | `services/ui` | ✅ 成立（唯一入口 `repositories/base.py:9`） |
| `services/` | `repositories/infrastructure/dto/configue/common/log/utils` | `ui` | ✅ 成立（`services/` 无任何 ui import） |
| `ui/`（契约层） | `core/configue/plugin` | PySide6、services、repositories | ✅ 成立（`ui/protocol.py:8`、`ui/bootstrap.py:6-12`） |
| `ui/builtin/` | 以上 + PySide6 + services/repositories(dto) | 不得被任何下层 import | ✅ 成立（下层无 ui import） |
| `plugin/` | 无运行时依赖 | `core`（运行时） | ✅ 成立（`plugin/manager.py:7-8` 仅 TYPE_CHECKING） |
| `core/` | `log/plugin/utils` + 惰性 `configue` | PySide6、sqlite3、services、repositories、ui | ⚠️ 依赖 `plugin`（见 2.3-V1） |

**关键证据**：`services/`、`repositories/`、`dto/`、`infrastructure/` 全域 **0 处** PySide6 引用（`grep -rn "PySide6" | grep -v "^./ui/"` 为空），PySide6 只出现在 `ui/builtin/**`（44 个文件）。

### 2.2 上游 → 下游的实际 import 证据

```python
# repositories 只依赖 infrastructure/dto/log/utils
repositories/base.py:9        from albuswall.infrastructure.database import Connector
repositories/base.py:10-11    from albuswall.log import getLogger / utils.sql_log
repositories/view.py:8        from albuswall.dto.album import Album, AssetDTO

# services 依赖 repositories + dto + configue + utils（不 import ui）
services/source.py:46-49      from albuswall.repositories import IngestSourceRepository, ImportRepository
services/source.py:50-54      from albuswall.configue.state import ObservableNamespace, StateField, ObservableContext
services/source.py:55-63      from albuswall.dto.task / dto.import_ / dto.source
services/__init__.py:6-8      from albuswall.core import Container, Application; utils.registry; utils.signal

# infrastructure 依赖 core/configue/dto/resources/log
infrastructure/database/bootstrap.py:4-5   from albuswall.core import Container, Application; configue ConfigField
infrastructure/database/connector.py:29    from albuswall.resources import schema
infrastructure/task/service.py:38-45       from albuswall.dto.task import ...; common.exceptions BackpressureError

# ui 契约层只依赖 core/configue/plugin
ui/protocol.py:8              from albuswall.core.main_loop import MainLoop
ui/bootstrap.py:7-9           from albuswall.core import Application, Container; configue; plugin.PluginManager

# core 只依赖 plugin/utils/log（+ 惰性 configue）
core/application.py:47-49     from .container import Container; .main_loop; albuswall.plugin.manager.PluginManager
core/container.py:10          from albuswall.utils.format import factory_repr
core/application.py:86        from albuswall.configue import Configue   # 函数内惰性，避免循环
```

### 2.3 分层违规与结构异味（逐条带证据）

**V1｜`core → plugin` 运行时反向依赖（环形依赖靠 TYPE_CHECKING 打断）**
- `core/application.py:49` `from albuswall.plugin.manager import PluginManager`（模块级）
- 反向：`plugin/manager.py:7-8`、`plugin/declaration.py:8-10` 只在 `TYPE_CHECKING` 下 import `core`
- 后果：`core` 不是依赖图的最底层，无法把 `core` 编译成「不需要 plugin 的库」；C++ 移植时若用真实 `#include` 会立刻形成环。建议把 `PluginManager` 下沉为独立 `plugin::Registry`（零依赖），或让 `Application` 通过 `std::function` 注入插件激活器。

**V2｜`log ↔ configue` 双向依赖**
- `log/bootstrap.py:12` `from albuswall.configue import ConfigField`
- `configue/bootstrap.py:13` `from albuswall.log import getLogger`
- 之所以没炸，是因为 `log/__init__.py:20-24` 用模块级 `__getattr__` 把 `setup_log` 做成惰性属性，且 `configue/__init__.py:57-68` 同样对 `bootstrap/declaration` 惰性导出。**这是"靠 import 时机续命"的脆弱平衡**，C++ 里对应「静态初始化顺序」。

**V3｜`configue → core` 上溯**
- `configue/declaration.py:26-29` `get_config()` 内部 `Application.instance().configure`
- `configue/bootstrap.py:30,83` 函数内 `from albuswall.core import Application`
- 配置引擎本应低于 core，却通过 `Application.instance()` 拿全局单例。属于「服务定位器式的隐式上溯」，可用「显式传入 Configue」消除。

**V4｜`infrastructure → services` 的字符串键耦合（静态分析看不见）**
- `infrastructure/trigger/__init__.py:46` `container.get("trigger_refresh_signal").emit(key)`，注释自承「延迟解析：注册时信号可能还没注册」
- 该键**唯一**注册点是 `services/__init__.py:62`
- 结论：infrastructure 在**命名契约**上依赖服务层；若只启动基础设施而不注册该键，触发器回调会 `KeyError`。

**V5｜`ui/builtin → repositories` 绕过 services**
- `ui/builtin/presenter/manager.py:99` `thumb_repo=c.require("thumbnail_repo")`
- `ui/builtin/presenter/manager.py:100` `thumb_service=c.require("thumbnail_service")`（同一 presenter 同时拿仓储与服务）
- `ui/builtin/presenter/viewer_presenter.py:23` TYPE_CHECKING `from albuswall.repositories import AssetRepository`
- 因为走容器字符串键，`ui.builtin` 的运行时 import 图里**看不到** repositories 边（AST 实测只出现在 TYPE_CHECKING）——即 **DI 把分层违规隐藏了**。C++ 复刻时若想守住分层，必须额外做「键→所属层」的白名单校验。

**V6｜import 期副作用（最危险的一处）**
- `ui/builtin/presenter/window_presenter.py:30` `WINDOW_STATE_FILENAME = Application.instance().configure.static.files.window_state`（import 即读配置）
- `ui/builtin/presenter/window_presenter.py:46-50` 模块级直接调用 `setup_dynamic_config(Application.instance().container, modules=[...], preload=["window"])`
- 该模块由 `ui/builtin/presenter/manager.py:79` 在 **PresenterManager 构造时**（即 `Application._phase_setup` 期间）才惰性导入，因此其副作用刚好发生在「静态配置已就绪」之后——**依赖了隐式的时序巧合**。

**V7｜注释与实现不符（两处 on_final 顺序说明）**
- `ui/bootstrap.py:77` 注释「用 insert(0) 保证 teardown 早于其它 final 收尾动作」，实际 `ui/bootstrap.py:78` 是 `Application.on_final(_teardown_ui)`（追加，最后执行）；全仓库无任何 `insert(0)`（grep 仅有此注释）。
- `infrastructure/task/bootstrap.py:70` 注释「插到最前：先停任务，再让下游收尾」；实现同样是追加，只因为它在 `__main__.py:93` 早于 services 注册（`services/__init__.py:109-112`）才事实上靠前。

**V8｜启动期 DB 锚点先于所有服务释放（顺序风险）**
- 注册顺序：`__main__.py:92` `register_database`（其 `on_final` 在 `infrastructure/database/bootstrap.py:33`）→ `:93` task → `:94` trigger → `:98` services → `:101` ui
- 执行语义：`core/application.py:330-334` **按注册顺序**执行 `on_final`
- 结果：`release_keep_alive()`（关闭主线程连接、销毁 shared-cache 内存库锚点）**最早**执行，而 `thumbnail_service.stop` / `import_service.stop_worker` 等收尾还在后面。对文件库影响小，对内存库（`db_path=None`）会导致后续查询落到已销毁的库上。**移植时建议把 on_final 改为「逆注册序」执行，或显式分组成对拆卸。**

**V9｜DTO 依赖 `utils`**
- `dto/source.py:29-30`（`is_valid_mount_point`、`json_loads_*`）、`dto/thumbnail.py:8`（`join_path`）
- 边界可接受（`utils` 是叶子），但要注意 **DTO 必须保持可 pickle**：`services/import_.py:105-108` 的 `process_candidate(AssetCandidateCacheDTO, str)` 会被提交到 `ProcessPoolExecutor`，任何 import 期重量级依赖都会拖累子进程。

**V10｜services 对 repositories.utils 的软依赖**
- `services/import_.py:32-42` 用 `try: from albuswall.repositories.utils.exif import ...  except ImportError: HAS_EXIF=False` 做能力降级。属于有意的可选依赖，但破坏了「import 必成功」的静态可判定性。

---

## 3. DI 容器机制（`core/container.py`）

### 3.1 数据结构与语义

```python
core/container.py:18   self._factories: Dict[str, Tuple[Callable[[], Any], bool, Any]]  # name -> (factory, singleton, returns)
core/container.py:19   self._instances: Dict[str, Any]                                  # 已构造单例缓存
```

| API | 行号 | 语义 | 触发构造 | 未注册时 |
|---|---|---|---|---|
| `register(name, factory, *, singleton=True, returns=None)` | 22-33 | 登记工厂；`name` 重复直接 `KeyError` | 否 | — |
| `reg`（别名） | 103 | 同 register | 否 | — |
| `annotate(name, returns)` | 35-40 | 只补/改返回类型元数据 | 否 | `KeyError` |
| `get(name, default=_Missing)` | 43-55 | 单例则缓存并复用；`singleton=False` 每次新建 | 是 | 有 default 返回 default，否则 `KeyError` |
| `require(name)` | 65-71 | 与 `get(name)` 等价，仅**语义声明「必须有」**（无逃生口） | 是 | `KeyError` |
| `try_get(name, default=None)` | 58-63 | **永不抛异常**，连工厂内部异常也吞掉 | 是 | `default` |
| `peek(name, default=_Missing)` | 73-79 | 只看已缓存实例，**不触发构造** | 否 | default / `KeyError` |
| `has_instance(name)` | 81-82 | 是否已实例化 | 否 | `False` |
| `__contains__` | 85-86 | 判断**是否已注册**（不是是否已实例化） | 否 | `False` |
| `__str__` | 88-100 | 打印 `factory_repr(func, returns)` + 实例类型，`factory_repr` 见 `utils/format.py:8-33` | 否 | — |

设计要点（`core/container.py:1-6` 文档字符串）：**容器只负责注册与按需解析，不认识生命周期**——什么时候构造、什么时候拆、按什么顺序，全部由 `Application` 编排。

### 3.2 谁往容器里注册（全量，17 个键）

| 键 | 注册点 | 工厂 | `returns` |
|---|---|---|---|
| `app` | `__main__.py:121` | `lambda: app` | `Application` |
| `config`, `configue` | `configue/bootstrap.py:35-36` | `lambda: config`（同一个 Configue 单例） | `Configue` |
| `db` | `infrastructure/database/bootstrap.py:24-27` | `Connector(data_dir/db_file, must_exist=…)` | `Connector` |
| `task_service` | `infrastructure/task/bootstrap.py:42-61` | `TaskService(**15 个配置项)` | `TaskServiceProtocol` |
| `trigger_scheduler` | `infrastructure/trigger/__init__.py:60` | `SchedulerService(_fire, logger)` | `TriggerHandler` |
| `trigger_facade` | `infrastructure/trigger/__init__.py:61` | `TriggerFacadeService(_fire, scheduler=container.get("trigger_scheduler"))` | `TriggerFacadeService` |
| `trigger_refresh_signal` | `services/__init__.py:62-66` | `Signal(name="TriggerRefreshRequested")` | `Signal` |
| `ingest_source_repo` `import_repo` `thumbnail_repo` `view_repo` `asset_repo` | `repositories/__init__.py:33-37` → `utils/registry.py:150-164` | 默认 `type_(container.get("db"))`（`utils/registry.py:92-94`） | 各自仓储类 |
| `source_service` `trigger_sync_service` `thumbnail_service` `view_service` `import_service` | `services/__init__.py:68-76` + `_ARG_MAP` (`:28-53`) | 各自显式装配（如 `cls(repo, repo, task_service)`） | 各自服务类 |
| `ui` | `ui/bootstrap.py:63` | `create_ui()`：`ui_cls()` + `setup(container)` | `ui_cls`（UIProtocol 实现） |

**"类型表即唯一事实来源"模式**：`repositories/__init__.py:18-23`（`Repositories(TypedDict)`）与 `services/__init__.py:18-23`（`Services(TypedDict)`）用 TypedDict 的注解当表，`utils/registry.py:101-143` 遍历它生成 `Getter`，再由 `register_all` 交给 `container.reg`。`_ARG_MAP` 只为「需要额外参数」的项提供自定义装配（`services/__init__.py:28-53`），默认工厂是「拿 `db` 构造」（`utils/registry.py:92-94`）。

### 3.3 谁从容器里取

- **core**：`core/application.py:301` `container.get("ui", None)`（可选 UI）；`__main__.py:121` 注册 `app` 自身。
- **bootstrap 层**：`configue/bootstrap.py:35`、`log/bootstrap.py:36`、`infrastructure/*/bootstrap.py`、`ui/bootstrap.py:51`。
- **装配闭包**：`services/__init__.py:30-51`（`_ARG_MAP` 内 `c.get(...)`）、`infrastructure/trigger/__init__.py:46,56,69`、`infrastructure/database/bootstrap.py:31,35`。
- **UI 层（最多）**：`ui/builtin/presenter/manager.py:88,99-100,111,122-124`（`require` 表示硬依赖，`get(k, None)` 表示可选）、`ui/builtin/presenter/manager.py:143,168,274,289`、`ui/builtin/application.py:72`、`ui/builtin/factory.py:24`、`ui/builtin/presenter/source_presenter.py:133`、`ui/builtin/presenter/window_presenter.py:69`。
- **观测/保障**：`infrastructure/task/bootstrap.py:65` 与 `infrastructure/trigger/__init__.py:64` 用 `has_instance()` 实现「没被实例化过就不必关」的收尾。

### 3.4 键名命名约定（实测）

1. **角色后缀**：仓储统一 `*_repo`（`ingest_source_repo` / `import_repo` / `thumbnail_repo` / `view_repo` / `asset_repo`，见 `repositories/__init__.py:19-23`），服务统一 `*_service`（`services/__init__.py:19-23`）。
2. **基础设施用「资源名」而非类名**：`db`、`ui`、`app`、`config`（`configue/bootstrap.py:35`）——短名 = 全局唯一能力。
3. **`trigger_facade` / `trigger_scheduler`**：`<领域>_<角色>`，角色词是抽象概念（facade/scheduler）而非实现类名（`TriggerFacadeService`/`SchedulerService`），因此可以换实现不改键（`infrastructure/trigger/__init__.py:60-61` 的 `returns=` 直接声明协议类型 `TriggerHandler`）。
4. **信号也是容器公民**：`trigger_refresh_signal`（`services/__init__.py:62`），`<事件名>_signal`。
5. **历史遗留**：`config` 与 `configue` 双别名并存（`configue/bootstrap.py:35-36`）；`plugin/declaration.py:54` 的示例代码仍引用已改名的 `"_config"` 键（`ui/builtin/application.py:72` 的注释记录了这次改名）——**键名是靠约定维持的弱契约，没有常量表**。

---

## 4. bootstrap 模式：5 个 bootstrap 各自做什么

| bootstrap | 入口函数 | 职责 | 被谁调用 | 时序位置 |
|---|---|---|---|---|
| `configue/bootstrap.py` | `setup_static_config(container, path)` (`:29`) | 注册 `config`/`configue` 键；注入 5 个内建字段（`path.config/data/cache/temp` + `files`，`:42-50`）；读主 INI → `resolver.inject_raw`（`:53-55`）；`ConfigDeclaration.load(resolver)` 补类型与默认值（`:58`） | `__main__.py:75` | 第 1 个（插件激活之后） |
| `configue/bootstrap.py` | `setup_dynamic_config(container, *, modules, preload)` (`:71`) | 消费 `DYNAMIC_REGISTRY`，把每个子树 `dynamic.mount_lazy(name, loader)`（`:100-102`）；`preload` 强制触发加载（`:105-106`） | `__main__.py:89`；**另一次**由 `ui/builtin/presenter/window_presenter.py:46` 在 import 期调用 | 第 4 个（在 infrastructure 之前） |
| `log/bootstrap.py` | `setup_log(container)` (`:35`) | 从 `config.static` 取日志级别与 `log_config.ini` 路径；用**类型**而非下标找到内存 handler（`:52-56`）；`logging.config.fileConfig(..., disable_existing_loggers=False)`（`:64`）；**重放启动期缓冲日志并按级别过滤**（`:79-86`） | `__main__.py:81` | 第 2 个 |
| `infrastructure/database/bootstrap.py` | `register_database(container)` (`:21`) | 注册 `db` 工厂（路径 = `configue.static.path.data / files.db`）；把 `keep_alive()` 挂 `on_boot`（`:29-31`）、`release_keep_alive()` 挂 `on_final`（`:33-35`） | `__main__.py:92` | 第 5 组第 1 个 |
| `infrastructure/task/bootstrap.py` | `register_task_service(container)` (`:39`) | 用 15 个 `ConfigField` 装配 `TaskService`，`returns=TaskServiceProtocol`；`on_final` 里**先 `has_instance` 再 shutdown(wait=True)**（`:63-68`） | `__main__.py:93` | 第 5 组第 2 个 |
| `infrastructure/trigger/__init__.py` | `register_trigger_service(container, *, scheduler_handler, device_handler)` (`:32`) | 注册两个后端；`update_callback` 固定为 emit `trigger_refresh_signal`（延迟解析，`:44-46`）；`on_final` 停止 facade（`:63-71`） | `__main__.py:94` | 第 5 组第 3 个 |
| `ui/bootstrap.py` | `registry_ui(container)` (`:26`) | ① `PluginManager.activate_ui(container, active)`（`:33`）② `get_ui(name)` 从 `_REGISTRY` 取类（`:41`）③ 注册 `ui` 工厂（`:57-63`）④ `on_final(_teardown_ui)`（`:66-78`） | `__main__.py:101` | 第 7 个（最后） |
| `repositories/__init__.py` / `services/__init__.py` | `registry_repository` / `register_service` | 同上表；`register_service` 还承担**全部跨服务信号接线与 on_boot/on_final 注册**（`services/__init__.py:79-112`） | `__main__.py:95,98` | 第 5 组第 4 个 / 第 6 个 |

**共同模式（4 步）**：
1. 在**模块顶层**用 `ConfigField` 声明配置（`infrastructure/task/bootstrap.py:15-33`）——import 即完成声明；
2. 在 `register_xxx(container)` 里只做 **`container.register`**，工厂是 `lambda`（**不构造**，`:42-61`）；
3. 需要参与生命周期的，顺手把 `on_boot` / `on_final` 挂到 `Application`（`:29-35`)；
4. 真正的构造推迟到「有人 `get` 它」或 `_phase_setup`（`core/application.py:290-306`）。

---

## 5. 启动与关闭时序

### 5.1 入口链

```
python -m albuswall
  └─ __main__.py:131  exit(main())
       └─ __main__.py:127-128  main() → runtime.run(_main)
            └─ core/runtime.py:89-118  Runtime.run(body)：捕获 SystemExit/NotImplementedError/KeyboardInterrupt/BaseException → 退出码
                 └─ __main__.py:116-124 _main()
                      ├─ :117 _logger.addHandler(memory_handler)     # 启动期日志先进内存
                      ├─ :120 app = Application()
                      ├─ :121 app.container.register("app", …)
                      ├─ :122 runtime.install()                      # 替换 sys.excepthook / threading.excepthook / SIGINT
                      ├─ :123 app.boot(_boot)                        # 只记录 boot 回调
                      └─ :124 app.exec() → core/application.py:273-280
```

`Application` 是**单例门面**：`core/application.py:347-357` 在类体里就直接构造 `_Application()`（import 即安装异常兜底钩子 `:16-41`），`__new__` 保证单例（`:359-362`），`__getattr__/__setattr__` 把一切委托给 `_object`（`:364-373`）。

### 5.2 五阶段 `_Application.exec()`（`core/application.py:273-280`）

| 阶段 | 方法 | 行号 | 动作 |
|---|---|---|---|
| 1 boot | `_phase_boot` | 283-287 | 调 `_boot_fn(self)` → `__main__._boot` 完成**全部注册**（此时容器里只有工厂，没有实例） |
| 2 setup | `_phase_setup` | 290-306 | ① 强制访问 `self.configure`（触发 `LazyConfigue`，`:73-88`）② 建数据目录 `config.static.path.data`（`:296-297`）③ `container.get("ui", None)`，拿到实例后**直接写 `self._main_loop`**，绕过 setter 的「只能设一次」约束（`:299-306`） |
| 3 wire | `_phase_wire` | 309-315 | 顺序执行 `_boot_hooks`；**单个钩子抛异常不阻断其余**（try/except + traceback） |
| 4 run | `_phase_run` | 318-325 | 执行 `_loop_hooks(loop)` → `loop.run()`（默认 `HeadlessMainLoop`，`:201-208`） |
| 5 teardown | `_phase_teardown` | 328-334 | 顺序执行 `_final_hooks`；同样单点隔离异常 |

`exec()` 用 `try/finally` 包住 2-4 阶段（`:273-280`），**保证任何异常都会走到 teardown**。

### 5.3 `__main__._boot` 的完整注册顺序（`__main__.py:47-113`）

```
phase cold   (:50-72)
   discover_all(builtin_package="albuswall.plugins", user_dir=DATA/plugins)   # 见 §6
   parse_section_file(config.ini, "plugin_disables")                         # 插件黑名单
   app.plugins.activate(container, disabled=plugin_disables)                  # 执行 @declare 回调
phase1 static config  (:74-78)  setup_static_config(container, CONFIG/config.ini)
phase2 log            (:80-82)  setup_log(container); app.log_enable = True
phase3 gc             (:84-86)  gc.collect(); gc.freeze()                     # 冻结长期对象，降低 GC 抖动
phase4 dynamic config (:88-89)  setup_dynamic_config(container)
phase5 infrastructure (:91-95)  register_database → register_task_service → register_trigger_service → registry_repository
phase6 services       (:97-98)  register_service(container)     # 同时注册 5 个 on_boot + 4 个 on_final
phase7 ui             (:100-101) registry_ui(container)        # 激活 @ui_loader → 注册 "ui"
```

> ⚠️ 关键约束：**phase cold 早于 static config**。因此 `@declare` 回调里**不能读配置**——`ui/builtin/factory.py:10-14` 的 `setup()` 只做 `from .config import WindowPresenterConfs`（声明字段），完全符合该约束；而 `plugin/declaration.py:48-56` 的示例代码却在 `setup` 里 `container.get("_config")` 读配置，属于**过期示例**。

### 5.4 运行期 on_boot 顺序（wire 阶段实际执行顺序）

```
services/__init__.py:98   _wire_scan_to_import   : source_service.scan_finished → import_service.trigger()
services/__init__.py:99   _wire_import_to_thumb  : import_service.assets_imported → thumbnail_service.scan_and_submit()
services/__init__.py:100  : trigger_refresh_signal.connect(source_service.update_source)   # 触发器 → 重扫
services/__init__.py:103  : trigger_sync_service.start()    # 把源配置喂给 facade
services/__init__.py:104  : source_service.start()
services/__init__.py:105  : import_service.start()
services/__init__.py:106  : thumbnail_service.start()
infrastructure/database/bootstrap.py:29  _keep_db_alive : db.keep_alive()    # 注册更早 → 先于上面全部执行
```

注意 `Application.on_boot` 是**先注册先执行**（`core/application.py:225-228`），而 DB 的 `on_boot` 在 phase5 就注册了，因此实际顺序是 **DB 锚点 → 信号接线 → trigger_sync → source → import → thumbnail**。`infrastructure/database/bootstrap.py` 用 `@app.on_boot` 装饰器（`:29`），services 用 `Application.on_boot(...)`（`:98-106`），两种写法等价（`core/application.py:416-433` 同时支持 `@X` 与 `@X()`）。

### 5.5 teardown 顺序（`_final_hooks`，按注册顺序）

| # | 钩子 | 注册点 | 内容 |
|---|---|---|---|
| 1 | `_release_db_alive` | `infrastructure/database/bootstrap.py:33` | `db.release_keep_alive()` |
| 2 | `_shutdown_task_service` | `infrastructure/task/bootstrap.py:71` | `has_instance` 检查后 `shutdown(wait=True)` |
| 3 | `_shutdown_trigger` | `infrastructure/trigger/__init__.py:73` | `trigger_facade.stop()` |
| 4 | `thumbnail_service.stop(wait=True)` | `services/__init__.py:109` | |
| 5 | `import_service.stop_worker()` | `services/__init__.py:110` | |
| 6 | `trigger_sync_service.stop()` | `services/__init__.py:111` | 只解信号，不 stop facade（`:89-90` 明确注释：facade 归容器所有） |
| 7 | `source_service.shutdown()` | `services/__init__.py:112` | |
| 8 | `_teardown_ui` | `ui/bootstrap.py:78` | 仅当 UI 实例真被创建过才 `teardown()`（闭包判空，`:55,66-75`） |

服务内部拆卸顺序 = 注册的**逆序**（启动正序 source→import→thumbnail，拆卸 thumbnail→import→source），符合依赖倒置拆卸原则；UI 的 `teardown`（`ui/builtin/application.py:109-116`）会 `presenters.teardown()`，而 `PresenterManager.teardown()` 又**逆序**拆 presenter 并断开 import_service 信号（`ui/builtin/presenter/manager.py:313-330`）。

**退出机制**：
- `Runtime._on_exception`（`core/runtime.py:42-54`）：`KeyboardInterrupt→130`、`SystemExit→其 code`、其它→记 critical 并 `request_quit(1)`；返回 `True` 表示「已处理」。
- `Runtime._request_quit`（`:62-75`）：**用 `_main_loop` 私有属性，不触发惰性创建**，写 `loop.code` 后 `loop.quit()`。
- `Application.request_quit`（`core/application.py:252-264`）：跑 `_quit_hooks` 再 `loop.quit()`。
- `HeadlessMainLoop`（`core/main_loop.py:27-61`）：`threading.Event.wait()` 零 CPU；信号处理器把退出码设为 **`128+signum`**（`:49-50`，Unix 约定）；只在主线程注册（`:38-40`）。
- UI 侧 `Application.MainLoop` 直接把 Qt 事件循环包成 `MainLoop`（`ui/builtin/application.py:49-57`），并在 `exec()` 里启动 100ms 空转 timer 以让 Python 信号有机会被处理（`:119-124`）。

---

## 6. 插件系统

### 6.1 注册表结构（`plugin/manager.py`）

```python
:16  declare:     Dict[str, Callable[[Container], Any]]                  # 插件名 → setup 回调（执行顺序 = 声明顺序）
:18  declare_ui:  Dict[str, Tuple[Callable[[Container], Any], str]]      # UI 名 → (loader, 拥有它的插件名)
:21  _module_owner: Dict[str, str]                                       # 模块名 → 该模块声明的插件名（用于 @ui_loader 归属校验）
:24  _enabled:    set[str] | None                                        # 激活过滤结果，None = 全开
:25  _activated:  bool                                                   # 激活后禁止再声明
:15/:27-30 单例（`__new__`）
```

### 6.2 声明：`@declare` / `@ui_loader`

```python
plugin/declaration.py:26-37  declare(name_or_func)
    - 传字符串 → 返回装饰器，注册名 = 显式名字（:28-33）
    - 直接当装饰器用 → 注册名 = 函数名（:34-36）
    - 其余 → ValueError
plugin/declaration.py:40-66  ui_loader(name) → 注册到 declare_ui（:63-65）
```

**归属校验**（关键设计）：`plugin/manager.py:43-49` 在 `registry()` 时记录 `call_.__module__ → name`；`registry_ui()` 时（`:61-68`）要求该模块**必须先声明过插件**，否则 `RuntimeError`——保证「UI 一定属于某个插件」，于是启用/禁用插件时 UI 能被连带控制（`:144-147`）。同时要求声明对象必须有 `__module__`（`:44-48`，即必须是模块级函数）。

### 6.3 发现：`plugin/discovery.py`

| 来源 | 函数 | 行为 |
|---|---|---|
| 内置包 | `discover_package(package)` (`:18-33`) | 遍历包内子模块并 import（跳过 `_` 前缀）；`@declare` 在 import 期生效。包不存在时仅 debug 日志 |
| 第三方 pip | `discover_entry_points(group="albuswall.plugins")` (`:37-74`) | 读 `importlib.metadata.entry_points`；目标若是**模块**→import 即声明；若是**可调用**→显式 `target()`（`:64-72`） |
| 用户脚本 | `discover_directory(path, prefix="_albuswall_user_plugin")` (`:77-90`) | `sorted(glob("*.py"))` → `spec_from_file_location` 动态加载（`:116-128`），失败时从 `sys.modules` 回滚 |
| 聚合 | `discover_all(builtin_package, user_dir, entry_group)` (`:93-101`) | **固定顺序：内置 → 入口点 → 用户目录**（即覆盖优先级 = 声明顺序；注意 `registry()` 对重名会 `ValueError`，所以后加载者不能重名） |

所有分支都「失败即跳过 + 记日志」，单个插件坏掉不影响启动（`:107-112`、`:126-128`）。

### 6.4 激活：三种互斥模式（`plugin/manager.py:73-129`）

```python
activate(container, enabled=None, disabled=None)
  :85-88  已激活 → RuntimeError；两者同时给 → ValueError
  :90     allow = _resolve_allow(enabled, disabled)
          enabled  → 白名单；列表里出现未声明名字 → KeyError（:112-118）
          disabled → 黑名单；未知名字只 log（:120-127）
          都不给   → None（全开）
  :94-103 按声明顺序执行 factory(container)；单个插件异常只记日志，不中断
```

### 6.5 UI 激活（`activate_ui`，`:133-150`）

```
declare_ui 里没有该名字        → KeyError（ui/bootstrap.py:34-37 捕获并 warn + 返回 None）
拥有它的插件被 enabled 过滤掉   → 返回 None（debug 日志，:144-147）
正常                          → loader(container)，**返回值原样返回**（通常是 UI 类）
辅助：list_ui()（:152-154）、__str__（:161-176，打印 enable 集合 + 两张注册表）
```

### 6.6 内置 UI 如何作为插件接入

```
plugins/builtin_ui.py:5          from albuswall.ui.builtin.factory import *
   └─ ui/builtin/factory.py:10-14   @declare("builtin_ui") def setup(container):
                                        from .config import WindowPresenterConfs   # 只声明配置字段，不读值
   └─ ui/builtin/factory.py:17-28   @ui_loader("builtin") def load(container):
                                        if container.get("config").static.ui.no_builtin: return None
                                        from .application import Application          # ← import 触发注册
                                        return Application
                                        └─ ui/builtin/application.py:43  @register_ui("builtin")
                                             → ui/protocol.py:38-46 写入 _REGISTRY，并 cls.name = "builtin"
```

`albuswall.ui.builtin.__init__` 有意**不** import `application`（`ui/builtin/__init__.py:4` 被注释掉），从而把「UI 实现」的 import 推迟到 `ui_loader` 回调里——这正是「核心不硬依赖 PySide6」的物理保证。

---

## 7. UI 抽象：为什么核心不依赖 PySide6

### 7.1 契约（`ui/protocol.py`）

```python
ui/protocol.py:11-12   @runtime_checkable class UIProtocol(Protocol)
ui/protocol.py:15-16       name: str;  main_loop: MainLoop
ui/protocol.py:18-20       setup(self, container) -> None      # 构建窗口/主题/信号，不阻塞
ui/protocol.py:22-24       teardown(self) -> None              # loop.run() 返回后调用
ui/protocol.py:35          _REGISTRY: Dict[str, Type[UIProtocol]]
ui/protocol.py:38-46       register_ui(name, override=False) → 装饰器（重复注册抛 RuntimeError，可 override）
ui/protocol.py:49-57       get_ui(name) / list_uis()
```

注意 `UIProtocol` 是**结构化协议**（`Protocol`），不要求继承任何基类；内置实现甚至直接继承 `QApplication`（`ui/builtin/application.py:44`）——**鸭子类型让 Qt 类不需要知道 UIProtocol 的存在**。`main_loop` 也只要求是 `core/main_loop.py:14-23` 的 `MainLoop`（`run()`/`quit()`）。

### 7.2 `registry_ui` / `activate_ui` 流程（`ui/bootstrap.py:26-88`）

```
registry_ui(container)                                   # 由 __main__.py:101 在最后调用
 ├─ confs = UIConfig()                                   # :27  声明在 :18-23
 │     theme_path : ConfigField("path","theme")          # 兼容旧键
 │     theme      : ConfigField("ui","theme", default=DEFAULT_THEME="default")
 │     active     : ConfigField("ui","ui",    default=DEFAULT_UI="builtin")
 │     no_builtin : ConfigField("ui","no_builtin", default=False)
 ├─ PluginManager.activate_ui(container, active.lower()) # :33  ← KeyError → warn + return None（:34-37）
 ├─ ui_cls = get_ui(active.lower())                      # :41  ← RuntimeError → warn + return None（:42-49）
 ├─ if container.get("config").static.debug: log(ui_cls) # :51-53
 ├─ container.reg("ui", create_ui, returns=ui_cls)       # :63
 │     create_ui(): _ui_instance = ui_cls(); _ui_instance.setup(container); return _ui_instance   # :57-61
 └─ Application.on_final(_teardown_ui)                   # :78  # 仅当 _ui_instance 非 None 才 teardown（:66-75）
```

- 「实例只在闭包里持有」（`:55` 注释）→ 未被 `get("ui")` 触发过就绝不会白造一个 UI，teardown 也不会凭空创建。
- 谁真正触发构造？`core/application.py:301` `self._ui = self.container.get("ui", None)`——**setup 阶段**。构造后立刻抓 `main_loop`（`:302-306`），这就是「UI 交出主循环」的注入点。
- `_ui_instance.setup(container)` 内部即 `ui/builtin/application.py:71-107`：建 `Window` → `PresenterManager(...).build()`（各自 try/except 隔离）→ 加载 UI 配置 → 逐个 presenter `setup()`。

### 7.3 「核心不硬依赖 PySide6」的四道保证

1. **物理隔离**：`grep PySide6` 只命中 `ui/builtin/**`（44 个文件），`core/`、`services/`、`repositories/`、`dto/`、`infrastructure/` **零命中**。
2. **契约隔离**：`ui/protocol.py`、`ui/bootstrap.py`、`ui/common.py`、`ui/vo/` 都不 import Qt；`ui/__init__.py:4` 只导出 `registry_ui`。
3. **加载期隔离**：UI 实现由 `@ui_loader` 回调**按需 import**（`ui/builtin/factory.py:27`），未启用 UI 时 `plugins/builtin_ui.py` 的 star-import 只带入 `factory.py` 本身。
4. **可降级运行**：`core/application.py:201-208` 在无 UI 时惰性回退 `HeadlessMainLoop`；`_phase_setup:301-306` 对 `ui` 键用 `get(..., None)` 而非 `require`。因此「无 PySide6 的核心 / 无头自动化」是真实可运行的路径（README 亦如此声明）。

---

## 8. repositories / services / dto / ui.vo 四者分工

| 角色 | 位置 | 职责 | 依赖 | 关键约束 |
|---|---|---|---|---|
| Repository | `repositories/` | **唯一写 SQL 的地方**；行 → DTO | `infrastructure.database.Connector` + `dto` | 写串行（`repositories/base.py:18` 模块级 `RLock`）；`_transaction` 内禁止 `_execute`（`:59-63`） |
| Service | `services/` | 用例编排：组合多个仓储、信号、任务、触发 | repositories + dto + configue | 不含 SQL、不含 Qt |
| DTO | `dto/` | **数据库形状的业务数据**；跨层/跨进程边界 | `common` + `utils` | 可 pickle（要进进程池）；不依赖 Qt |
| VO | `ui/vo/` | **展示意图**：只描述"要显示成什么样" | 仅标准库 | 三态语义 `None/""/值`；不持有控件、不查库 |

### 8.1 真实数据流（专辑 → 标题栏），带行号

```
① SQLite  → sqlite3.Row
   repositories/view.py:127-145   _fetchone("SELECT a.id, a.uuid, a.title, … FROM albums … LEFT JOIN assets AS cover …")

② Row → DTO（唯一的转换点）
   repositories/view.py:148-157   Album(id=…, uuid=UUID(row["uuid"]), cover=UUID(row["cover_uuid"]) if … else None, …)
   dto/album.py:12-21             @dataclass class Album: id/uuid/title/description/cover/type/created_at/modified_at

③ DTO + 领域规则 → Service（虚拟相册分派）
   services/view.py:133-147       spec = self._spec(uuid)
                                  if spec is None: return self._repo.get_album_by_uuid(...)      # 物理相册透传
                                  cover = self._repo.get_cover_asset_by_scope(spec.scope)          # All/Trash 合成
                                  return Album(id=spec.sentinel_id, title=spec.title, …)
   services/view.py:26-28         VIRTUAL_ALBUM_ALL_UUID / TRASH_UUID 固定 uuid；:57-70 虚拟相册表

④ DTO + 用户偏好 → VO
   ui/builtin/presenter/album_presenter.py:378-391  _build_vo(album) -> TitleBarVO(
                                       window_title=…, cover=self._view_service.get_cover_full_path(album),
                                       title=album.title, description=album.description,
                                       album_icon=…, search_expanded=…)
   ui/vo/album.py:17-65           @dataclass(slots=True) TitleBarVO（字段三态：None=不改，""=清空，值=设置）

⑤ VO → 控件（渲染层，单向、无回读）
   ui/builtin/presenter/album_presenter.py:251-256  set_album(): _current_album = album; title_bar.set_vo(...); _sync_selection(...)
   ui/builtin/window/titlebar.py:182-190            set_vo(vo)：None→空 VO；dict→TitleBarVO.from_dict；然后 _apply_vo
   ui/builtin/window/titlebar.py:193-196            _apply_vo：`if vo.window_title is not None: setText(...)`
```

**DTO 与 VO 的实际差别（不是命名游戏）**：

| 维度 | DTO（`dto/album.py`） | VO（`ui/vo/album.py`） |
|---|---|---|
| 数据来源 | DB 行（`AssetDTO.from_row`，`dto/album.py:78-115`） | DTO + 配置 + 服务查询结果的**拼装** |
| 字段语义 | 客观事实（uuid/id/时间/尺寸/EXIF） | 显示意图（图标路径、是否展开、要显示的文案） |
| 空值语义 | `None` 就是「无」（`cover=None` 表示无封面） | **三态**：`None`=不处理、`""`=显式清空、值=设置（`ui/vo/album.py:22-27` 明文约定） |
| 依赖 | 无 Qt，但依赖 `utils`（可 pickle） | 纯标准库，甚至不 import albuswall |
| 消费方 | service / repository / 子进程任务载荷 | presenter → widget |
| 生命周期 | 一次查询一次构造 | 每次 `refresh()` 重新生成（`album_presenter.py:245-249`），**无状态地推给 view** |

**第二组例子（证明 DTO 的"跨进程"职责）**：
- `services/import_.py:105-108` `process_candidate(candidate: AssetCandidateCacheDTO, source_path: str) -> Optional[AssetCreateDTO]`——该函数在 **`ProcessPoolExecutor`（spawn）**里执行（`infrastructure/task/service.py:68-71` 约束），所以两个 DTO 必须可 pickle；DTO 因此**不能**持有 DB 连接、Qt 对象或 lambda。
- **第三组例子（DTO 与"路径口径"）**：`dto/thumbnail.py:51-82` `ThumbnailPaths`（base + rel）与 `resolve(spec)` 把「缩略图完整路径」的拼接规则收敛成一个类型；`repositories/thumbnail.py:55-73` 明确「仓储层不做文件系统 IO」，`services/thumbnail/storage.py:16-18` 才落盘。这是「DTO 定义不变量、仓储定义访问、服务定义副作用」的典型切分。

### 8.2 层间边界（读侧/写侧分离）

- `repositories/view.py:13-19` 注释：「本仓储只做读查询，收藏/软删/恢复/硬删应放 `AssetRepository`」；`repositories/asset.py:1-30` 对称声明「写操作专属，不提供任何查询接口」。**同一张表被拆成两个仓储，按读写职责而不是按表拆分**。
- 排序契约单点化：`repositories/view.py:26-29` `_ASSET_ORDER_SQL`（active/deleted 两套 ORDER BY）被 `list_assets` 与 `get_asset_neighbours*` 共用（`:231,256`），避免「网格顺序」与「上/下一张」分叉（`:325-335` 有完整契约说明）。
- Service 只做组合，不做 SQL：`services/view.py:43-53` 声明「只做读组合」；`services/trigger_sync.py:14-15` 声明「唯一允许 import IngestSourceRepository 的触发器组件」。

---

## 9. configue 配置系统

### 9.1 五个角色的关系

```
ConfigField(描述符)  ──__set_name__──▶  ConfigDeclaration._registry（key = module.qualname.attr）
      │  __get__/__set__                          │
      │                                           │ ConfigDeclaration.load(resolver)
      ▼                                           ▼
   Resolver  ◀──── register_type / inject_raw ────┘
      ▲   （${sec:key} 展开 + 类型转换 + 记忆化 + 环检测 + 反向失效）
      │
   Configue 单例 ──┬─ static : StaticConfig    → 只读、按 section 惰性生成 _SectionView
                   └─ dynamic: DynamicConfig   → 可 mount / mount_lazy / unmount 的 State 子树根
                                                     ▲
                              @dynamic_state(...) ───┘  （登记到 DYNAMIC_REGISTRY）
```

### 9.2 声明式定义机制（从"写一个字段"到"能读到值"）

```python
# ① 业务侧只写这一行（模块顶层）
class ImportConfig:
    batch_size: int = ConfigField("import", "batch_size", default=500)     # services/import_.py:71

# ② ConfigField.__set_name__ 在类创建时把声明登记进 registry
configue/declaration.py:179-182   annotation = owner.__annotations__.get(name)
                                  decl_cls.register(owner, name, self, annotation)
configue/declaration.py:111-122   key = f"{owner.__module__}.{owner.__qualname__}.{attr_name}"
                                  _registry[key] = {"path": path, "default": default, "type": annotation}

# ③ 启动时（__main__.py:75）一次性把 registry 灌进 Resolver
configue/declaration.py:96-109    for key, meta in registry: 
                                      section, field = path (长度 2) 或 ("__default__", path[0])
                                      resolver.register_type(section, field, converter)   # 注解字符串用 eval 解析，失败回退 str（:33-58）
                                      resolver.inject_raw(...)  # 缺失才注入默认值（:108-109）

# ④ 读取时惰性 + 记忆化
configue/declaration.py:145-165   __get__ → resolver.get_typed(section, key)；KeyError 且有 default → 返回 default（:157-160）
configue/declaration.py:167-172   __set__ → resolver.set(...) + 清本地缓存
```

内建字段由 bootstrap 直接注入（`configue/declaration.py:82-94` + `configue/bootstrap.py:42-50`）：`path.config/data/cache/temp`、`files`。

### 9.3 Resolver（`configue/resolver.py`）——所有访问路径的唯一中枢

| 能力 | 行号 | 说明 |
|---|---|---|
| `${section:key}` 字符串展开 | 29-60 | 正则 `REF_RE = \${([^}:]+)(?::([^}]+))?}`（`:11`）；**支持 `${key}`（同 section）**；非字符串原样返回（`:36-37`） |
| 环检测 | 41-47 | 维护 `_stack`，命中即抛 `InterpolationError` 并打印完整链 |
| 反向依赖记录 | 54 | `_rdeps[(ref_sec, ref_key)].add(node)` —— 谁引用了我 |
| 类型化 + 记忆化 | 63-78 | `_typed_cache`；`converter is None or raw 非 str` → 直接采用 |
| 写 + 反向失效 | 81-104 | `set/inject_raw` 后 BFS 沿 `_rdeps` 清掉所有传递依赖的缓存（`:92-104`） |
| 注册/查询 | 123-150 | `register_type` / `register_custom_converters` / `sections()` / `keys_of` / `schema()` / `check_all()` |

两个缓存（`_str_cache`/`_typed_cache`）+ 一张反向依赖图，构成「惰性 + 记忆化 + 精确失效」的完整实现。

### 9.4 静态配置 vs 动态状态

| | StaticConfig（`configue/configue.py:155-182`） | DynamicConfig（`:214-348`） |
|---|---|---|
| 数据源 | 主 INI（`configue/bootstrap.py:125-140`）+ Declaration 默认值 | JSON 文件（每个子树一个文件） |
| 访问 | `config.static.<section>.<key>`；section 首次访问生成 `_SectionView`（`:98-153` 代理到 resolver） | `config.dynamic.<name>`；首次访问触发 `mount_lazy` 的工厂（`:286-295`） |
| 可变性 | **只读**（`__setattr__` 直接抛，`:181-182`；`_SectionView.__setattr__` 也抛，`:148-149`） | 可变，写入即脏标记，`autosave=True` 时立刻落盘（`configue/state.py:74-77`） |
| 默认值 | `_SectionView.ensure(key, type, default)` 可现场补默认（`:111-130`） | `StateField(default=...)`（`configue/state.py:83-143`） |
| 显式登记 | `ConfigDeclaration` + `ConfigField` | `@dynamic_state(name, base_key, filename, autosave=…)`（`configue/dynamic_declaration.py:40-85`） |

动态子树映射：`DynamicMountSpec.resolve_path(static)` 把 `base_key`（如 `("path","cache")`）沿 StaticConfig 走到底再拼文件名（`configue/dynamic_declaration.py:29-33`），由 `setup_dynamic_config` 统一 `mount_lazy`（`configue/bootstrap.py:92-102`）。

唯一生产用例：`ui/builtin/presenter/window_presenter.py:33-44` 的 `@dynamic_state("window", ("path","cache"), files.window_state, autosave=True)` + `:80-88` teardown 时写入窗口几何。

### 9.5 Namespace / State 家族（`configue/state.py`）

```
Namespace（configue/utils/namespace.py:16-…）      点号访问 + dict 自动提升
 ├─ FrozenNamespace                                不可变变体
 ├─ StaticConfig                                   只读视图（proxy 到 Resolver）
 └─ ObservableNamespace                     :149-298 脏标记 + 观察者 + 快照/回滚，无持久化
      └─ StatefulNamespace                  :304-350 + JSON 落盘（tmp→replace 原子替换 :333-341）
StateField（描述符）                        :83-143  类型校验 + 默认值 + nested 推断（:104-110）
ObservableContext                           :37-61   dirty/observers/RLock/root
 └─ StateContext                            :64-77   + path/autosave；mark_dirty 后自动 save（:74-77）
```

`StateField` 的 `nested` 推断（`configue/state.py:104-110`）允许 `StateField(SomeNamespaceSubclass)` 把 dict 自动升级为共享同一 `_ctx` 的子树（`:192-197`），这是「一棵树共享脏标记」的实现基础。

---

## 10. infrastructure 各模块职责

### 10.1 database：`connector.py` + `bootstrap.py`

- **连接模型**：`threading.local` 每线程一条连接（`infrastructure/database/connector.py:83,131-147`）；`check_same_thread` **保持默认 True**（`:159` 注释），跨线程误用立刻 `ProgrammingError` 而非静默竞争；**不提供 `close_all()`**（`:8-11` 说明理由）。
- **内存库**：`db_path=None` 时用每实例唯一 shared-cache URI（`:87-89`，用 `uuid4` 而非 `id(self)` 防 id 复用），并提供 `keep_alive()/release_keep_alive()` 作为「库锚点」（`:269-282`）。
- **PRAGMA**：`foreign_keys=ON`、`journal_mode=WAL`（仅文件库）、`busy_timeout=5000`、`recursive_triggers=OFF`（`:35-38, 162-170`）。
- **初始化与迁移**：进程级 `_initialized_dbs: Set[(resolved_path, SCHEMA_VERSION)]` + 锁（`:65-66,181-197`）；**key 不含 schema 内容**，改结构必须 `SCHEMA_VERSION += 1`（`:41, 186-188` 有完整契约说明）；`_run_migrations` 用 `PRAGMA user_version` 逐级补齐（`:199-240`），schema 文件始终表示最新版（`:204-207`）。
- **bootstrap**：注册 `db` 键 + 生命周期锚点（`infrastructure/database/bootstrap.py:21-35`）；配置项 3 个（`db.must_exist`、`files.db`）。

### 10.2 task：`protocol.py` / `service.py` / `_task.py` / `_semaphore.py`

- **契约先行**：`infrastructure/task/protocol.py:38-99` 只声明 4 个方法（`submit`/`shutdown`/`get_stats`/`set_backpressure_callback`）；文档明确列出**刻意不放进契约**的东西（调度循环、DynamicSemaphore、内部方法、队列水位）（`:13-21`）。
- **背压语义**（`:62-80` 双通道约定）：① `submit()` **同步抛** `BackpressureError(QUEUE_FULL)`；② 任务已入队但被 `put_back` 失败时，错误**经 Future 传递**（`PUT_BACK_FAILED`）。调用方必须同时处理，否则漏错。异常定义在 `common/exceptions.py:4-17`。
- **实现**：优先级队列 + 全局 `DynamicSemaphore`（`infrastructure/task/service.py:126-130`）+ 线程/进程混合池 + `psutil` 动态并发（高负载立即减半、低负载需连续 N 次确认才 +1，`:59-62`）；进程任务拒绝 lambda/局部函数/绑定方法（`:70-71`）。
- **`_task.py:16-29`**：`@dataclass(order=True)`，只有 `priority`/`seq` 参与比较，payload 全部 `compare=False` → 携带任意不可比较对象。
- **`_semaphore.py:18-116`**：动态上限信号量；`decrease_max` 后 `_current` 可暂时大于 `_max`（`available` 返回 0、acquire 阻塞）——**明确声明这是刻意行为而非 bug**（`:26-29`）。
- **bootstrap**：15 个配置项（`infrastructure/task/bootstrap.py:15-33`），装配 `TaskService` 并声明 `returns=TaskServiceProtocol`（`:60`）；`on_final` 用 `has_instance` 保证「没造过就不关」（`:63-68`）。
- `infrastructure/task/__main__.py`：`python -m albuswall.infrastructure.task` 的独立演示入口（83 行），说明该子系统可脱离主程序运行。

### 10.3 trigger：`scheduler.py` / `trigger.py` / `protocols.py`

- **协议**（`infrastructure/trigger/protocols.py:23-57`）：`TriggerHandler[K]`（`start/stop/upsert/remove`，全部幂等，`:26-32` 有契约声明）；`DeviceTriggerHandler` 是**空扩展点**（`:51-57`，仓库不提供实现）。`K` 是触发器主体标识类型（int/str/UUID 均可），门面与 handler **都不解释它**（`:9-13`）。
- **门面**（`infrastructure/trigger/trigger.py:26-231`）：纯推送式，**不 import 任何 repository**（`:36-41` 明文承诺）；`start/reload/upsert/remove` 维护内存快照并做 diff（`:146-161`）；按类型分派子集给不同后端（`_filter_scheduler_configs:212-220` / `_filter_device_configs:222-231`）；`TriggerService = TriggerFacadeService` 别名（`:235`）。
- **调度**（`infrastructure/trigger/scheduler.py:56-339`）：APScheduler `BackgroundScheduler` + `CronTrigger`/`IntervalTrigger`；`start/stop/upsert/remove` **全部幂等**（`:59-70` 契约）；`upsert` 未 start 会自动 start（`:179-187`）；时区在构造时定格一次（`:50-53, 72-81`）。
- **bootstrap**（`infrastructure/trigger/__init__.py:32-73`）：注册 `trigger_scheduler`/`trigger_facade`；`update_callback` 固定为 emit `trigger_refresh_signal`（延迟解析，`:44-46`）；`on_final` **只在 `has_instance` 时** stop（`:63-71`）。文件头明确「应用层自己负责 start/reload/upsert/remove」（`:9-12`）——基础设施不读仓储，配置由应用层喂入。

### 10.4 fs：`infrastructure/fs.py`

与领域无关的字节/路径原语：`atomic_write(dest, writer)`（`:19-51`）——同目录 tmp（带 pid+tid 防并发互踩，`:37-39`）→ `os.replace` 原子替换；失败尽力清理 tmp（`:46-51`）；`atomic_write_bytes` 为便利封装（`:54-61`）。使用者示例：`services/thumbnail/storage.py:17`。

---

## 11. 横切关注点如何贯穿各层

### 11.1 日志（`log/`）

```
log/common.py:6        TRACE = 5                              # 自定义级别
log/type.py:8-19       Logger(Protocol)                       # 结构化契约（含 trace，不含 setLevel）
log/logger.py:11-14    getLogger(name) → 标准 Logger + 动态挂 .trace 方法
log/__init__.py:20-24  __getattr__("setup_log") → 惰性 import bootstrap  # 切断 log↔configue 循环
log/bootstrap.py:21    logging.addLevelName(TRACE, "TRACE")
log/bootstrap.py:35-91 setup_log：
                         :52-56  按**类型**（isinstance MemoryCacheHandler）查 handler，而非 handlers[0]
                         :58-68  fileConfig(disable_existing_loggers=False)
                         :79-86  重放缓冲：逐条 isEnabledFor 过滤后用 source_logger.handle(record)
log/handlers/memory_chache_handler.py:8-29   BufferingHandler 子类：flush/close 故意 no-op（logging.shutdown 会先调它们），emit 溢出丢最旧
log/formatters/multiline_formatter.py:6-43   MultilineFormatter：多行消息/异常堆栈每行都补日志头
```

**贯穿方式**：全项目统一用 `albuswall.log.getLogger`（`services/source.py:45`、`repositories/base.py:10`、`infrastructure/fs.py:14`、`infrastructure/database/connector.py:28`、`ui/builtin/application.py:15`），只有少数底层模块用标准 `logging.getLogger`（`core/main_loop.py:11`、`core/runtime.py:13`、`plugin/manager.py:10`）。启动期（静态配置尚未就绪）日志先进 `MemoryCacheHandler`，配置就绪后由 `setup_log` 重放并做级别过滤——**这是"日志早于配置"难题的实现**。

### 11.2 异常（`common/exceptions.py` + 各层自有异常）

- `common/exceptions.py:4-17` `BackpressureError(RuntimeError)`：携带 `reason/queue_size/max_queue_size`，**让调用方能按背压原因降速**（退避重试 / 拒绝上游 / 丢低优先级）。生产于 `infrastructure/task/service.py:45`，消费于 `infrastructure/task/__main__.py:10`。
- `configue/exceptions.py:7-46` `ConfigError` + `ConfigueErrorSource(ENV/ARG/FILE)`：统一携带来源、键、原值、根因，并提供 `from_env/from_arg/from_file` 构造器。
- 约定式错误：容器用 `KeyError`（未注册/未实例化，`core/container.py:31,47,78`）；插件重复声明用 `ValueError`（`plugin/manager.py:40,59`）、激活后声明用 `RuntimeError`（`:37,56`）；配置字段缺失用 `ValueError`（`configue/declaration.py:194`）。
- **顶层兜底**：`core/application.py:16-41` 在**任何可能失败的 import 之前**安装 sys/threading 兜底钩子（写到 stderr）；`:169-196` 再装稳定 wrapper（每次从 `self` 读最新 handler，替换 handler 无需重装）；`core/runtime.py:33-36` 把 handler 换成退出码映射器。三层设计保证「从 import 那一刻起异常绝不静默」。

### 11.3 信号与可观察属性

| 机制 | 位置 | 语义 |
|---|---|---|
| `Signal` | `utils/signal.py:20-48` | 线程安全（RLock + 回调快照后执行）；`connect/disconnect/send`，`emit = send`（`:48`）；回调异常只打印不传播（`:44-45`） |
| `ProcessSignal` | `utils/signal.py:51-116` | 本地回调立即执行 + 经 `multiprocessing.Manager().Queue()` 广播；跨进程只传 `repr(sender)`（`:106`）；惰性启动监听线程（`:67-75`） |
| `SignalDescriptor` | `utils/signal.py:119-145` | 类属性式声明信号；按 owner 缓存（`:133-141`），`cross_process=True` 自动起监听 |
| `ObservableProperty` | `utils/observable_property.py:18-85` | 与内置 `property` 同用法 + `.call(cb)` 注册 `cb(instance, new, old)`；`setter/getter` 派生时**共享回调列表**（`:75-79`）；`UNSET` 哨兵（`:4-14`） |

**贯穿方式**：
- 服务间解耦：`services/source.py:64` 定义 `scan_finished/source_added/...` 信号；`services/__init__.py:84-95` 在 on_boot 里接线（scan→import→thumbnail 流水线**不通过直接调用**）；`services/trigger_sync.py:74-76` 订阅源变更同步触发器。
- 跨层通信：`trigger_refresh_signal` 作为容器公民（`services/__init__.py:62`）被基础设施 `emit`（`infrastructure/trigger/__init__.py:46`）、被服务层 `connect`（`services/__init__.py:100-102`）。
- UI 侧桥接：Qt `Signal`（`ui/builtin/presenter/album_presenter.py:202`）与 Python `Signal` 并存——**Qt 信号只在 UI 层使用**，Python `Signal` 只在非 UI 层使用，交界处由 presenter 转换（如 `ui/builtin/presenter/manager.py:173` 订阅 `import_service.assets_imported`）。
- 状态可观察：`configue/state.py:53-61` 的 `mark_dirty` 会通知 observers 并**吞掉观察者异常**（写路径不被 UI 拖垮）；`services/source.py:88-144` 用 `ObservableNamespace`（而非可持久化的 `StatefulNamespace`）承载含运行时对象的可变状态，并说明原因（`:94-96`）。

---

## 12. 总结

### 12.1 核心设计原则（9 条）

1. **容器只做注册表，生命周期归编排者**——`Container` 不认识 start/stop（`core/container.py:1-6`），顺序完全由 `Application` 的五阶段 + 四类钩子决定（`core/application.py:92-105,273-334`）。这让"什么时候构造"成为可推理的显式数据，而不是 import 顺序的副作用。
2. **注册与构造分离（lazy everything）**——`register()` 只存 `lambda` 工厂（`infrastructure/task/bootstrap.py:42-61`），真正构造发生在被 `get` 或 setup 阶段；`peek/has_instance` 专门用于"不确定是否造过"的收尾（`infrastructure/task/bootstrap.py:63-68`）。
3. **契约前置、实现后置（Protocol 优先）**——`TaskServiceProtocol`（`infrastructure/task/protocol.py:38-99`）、`TriggerHandler`（`infrastructure/trigger/protocols.py:23-49`）、`UIProtocol`（`ui/protocol.py:11-32`）都**先定义最小接口并明文写出"什么不进契约"**；容器 `returns=` 记录的就是协议类型而非实现类（`infrastructure/trigger/__init__.py:60`）。
4. **依赖倒置 + 前端可替换**——核心通过 `ui` 容器键与 `MainLoop` 抽象拿主循环（`core/application.py:201-208,301-306`），PySide6 只存在于 `ui/builtin/**`；无 UI 时自动 headless。
5. **声明式注册 + 类型表即唯一事实来源**——配置用 `ConfigField` 描述符（`configue/declaration.py:129-182`），仓储/服务用 `TypedDict` 注解表 + `build_getters`（`utils/registry.py:101-143`），插件用 `@declare`（`plugin/declaration.py:26-37`）。新增一项 = 改一张表/加一个字段，不需要改引导代码。
6. **读写分离与契约单点化**——同一张表拆成读仓储/写仓储（`repositories/view.py:13-19` vs `repositories/asset.py:1-30`）；排序契约集中在 `_ASSET_ORDER_SQL`（`repositories/view.py:26-29`）；缩略图路径契约集中在 `ThumbnailPaths`（`dto/thumbnail.py:51-82`）。
7. **DTO 是跨边界通货，VO 是展示意图**——DTO 可 pickle、无 Qt、`from_row` 单点转换；VO 纯标准库、三态语义、单向推给控件（`ui/vo/album.py:22-27`）。
8. **失败隔离优先于失败传播**——钩子逐个 try/except（`core/application.py:311-315,330-334`）、插件激活逐个隔离（`plugin/manager.py:98-103`）、presenter 逐个隔离（`ui/builtin/presenter/manager.py:130-151`）、插件/用户脚本发现失败即跳过（`plugin/discovery.py:107-112`）。**主窗口/主进程必须能起来**。
9. **启动期可观测性**——内存日志缓冲 + 配置就绪后重放（`log/handlers/memory_chache_handler.py:8-29` + `log/bootstrap.py:79-86`）；异常钩子在第一个 import 前安装（`core/application.py:16-41`）；`Container.__str__` 提供注册表快照（`core/container.py:88-100`，`__main__.py:112-113` 在 debug 下打印）。

### 12.2 移植到 C++ 的结构对应表

| Python 机制 | C++ 对应 | 注意事项 |
|---|---|---|
| `albuswall.<pkg>` 包 | `namespace albuswall::<pkg>` + 一个 CMake 静态库 target | 用 `target_link_libraries` **只允许向下链接**来固化第 2 节的方向规则；Python 侧无任何 import-linter（`pyproject.toml` 只有依赖，无 lint/test 配置），C++ 应补上 |
| `Container._factories: Dict[str, (factory, singleton, returns)]` | `std::unordered_map<std::string, Entry{std::function<std::any()>, bool singleton, std::type_index}>` + `std::unordered_map<std::string, std::any> _instances` | 建议 `get<T>(name)` 用 `std::type_index` 校验 `returns`，避免 `std::any_cast` 运行期爆炸；单例缓存用 `std::shared_ptr<void>` + 类型擦除 deleter |
| `get/require/peek/try_get/has_instance` | 同名方法：`std::optional<T> try_get`、`T& require`（抛 `KeyError` 等价物 `std::out_of_range`） | `try_get` 吞掉工厂异常这一语义要显式保留（`core/container.py:58-63`），否则启动期行为不等价 |
| `LazyConfigue` 描述符（`core/application.py:73-88`） | 函数内 `static` 局部变量（Meyers singleton）+ `std::call_once` | 直接对应「用惰性初始化打断 import 环」；不要用命名空间作用域全局对象 |
| `@Application.on_boot/on_final` 装饰器 | `std::vector<std::function<void()>>` + `REGISTER_BOOT(fn)` 宏（生成匿名 namespace 的静态注册对象） | 静态初始化顺序不定的坑与 V2/V6 相同；更稳的做法是**显式注册函数表**（由 `main` 顺序调用），而非静态自注册 |
| 五阶段 `exec()` | `enum class Phase { Boot, Setup, Wire, Run, Teardown }` + `int exec()` 里顺序调用 | 用 `try/finally` 的等价物：RAII scope guard 保证 teardown 必达（`core/application.py:273-280`） |
| `MainLoop(ABC)` / `HeadlessMainLoop` | `class IMainLoop { virtual int run()=0; virtual void quit()=0; };` + `HeadlessLoop`（`std::condition_variable` + `sigaction`） | 退出码 `128+signum`（`core/main_loop.py:49-50`）要照搬，便于脚本/CI 判读 |
| `UIProtocol`（结构化 Protocol） | C++20 `concept Ui`（`requires(U u, Container& c){ u.setup(c); u.teardown(); u.main_loop(); u.name; }`）或抽象基类 | 概念更贴近 Python 的鸭子类型——`QApplication` 子类不需要继承框架基类（`ui/builtin/application.py:44`） |
| `@declare` / `@ui_loader` / `@register_ui` | 三个注册函数 + 宏（`DECLARE_PLUGIN("builtin_ui", setup)`），或显式表 | 需要保留「UI 归属插件」的校验（`plugin/manager.py:61-68`）与 `enabled/disabled` 三态激活（`:110-129`） |
| `ConfigField` + `__set_name__` 自动登记 | 宏：`CONFIG_FIELD(int, "import", "batch_size", 500)` → 展开为「描述符 + 静态登记对象」 | C++ 无属性反射，宏是唯一等价物；登记 key 建议显式给（Python 用 `module.qualname.attr`，`configue/declaration.py:113-117`） |
| `Resolver`（`${sec:key}` + 记忆化 + 环检测 + 反向失效） | `std::unordered_map<Key, Value>` + `std::unordered_map<Key, std::unordered_set<Key>> rdeps` + 显式栈 DFS | 算法可 1:1 抄（`configue/resolver.py:29-104`）；值用 `std::variant<std::string,int64_t,double,bool>` 承载类型化结果 |
| `StaticConfig` 只读 + `_SectionView` 惰性 | `const` 视图对象 + 返回代理 `SectionView`，写操作 `throw std::logic_error` | 「section 首次访问才建视图」是性能关键（`configue/configue.py:163-178`） |
| `ObservableNamespace` / `StatefulNamespace` / `StateField` | CRTP `ObservableState<Derived>` + `Signal<void(std::string_view)>` 观察者列表 + `std::shared_mutex`；持久化用 `nlohmann::json` | `autosave` 语义 = 每次脏写立刻落盘（`configue/state.py:74-77`）；原子写照抄 `tmp → rename`（`:333-341`、`infrastructure/fs.py:19-51`） |
| `Signal` / `ProcessSignal` | `template<class... A> class Signal`（mutex + emit 前拷贝回调表）；跨进程换 UDS/pipe + 序列化 | 回调异常被吞（`utils/signal.py:42-45`）这一容错语义要有意保留 |
| `Connector`（thread-local 连接） | `thread_local std::unique_ptr<sqlite3>`，per-Connector 实例；PRAGMA 同序设置 | schema 版本键 `(resolved_path, version)` 与迁移策略照抄（`infrastructure/database/connector.py:181-240`） |
| `BaseRepository`（全局写锁 + 事务） | `std::mutex` + RAII `Transaction`（析构时若未 commit 则 rollback） | 保留「事务内再 `_execute` 直接报错」的保护（`repositories/base.py:59-63`）——这是最容易被 C++ 复刻者漏掉的隐性契约 |
| `TaskService`（优先级队列 + 背压 + 动态并发） | `std::priority_queue<Task>` + `DynamicSemaphore`（自行用 `mutex+condition_variable` 实现，因为 `std::counting_semaphore` 上限编译期固定）+ 线程池 | 「进程池」在 C++ 里成本高，建议首版只保留线程池；`BackpressureError` 的**双通道语义**（同步抛 + Future 携带）必须复刻，否则调用方漏错（`infrastructure/task/protocol.py:62-80`） |
| `SchedulerService`（APScheduler） | 自研/引入 cron 库；关键是**幂等 start/stop/upsert/remove** | 幂等是最重要的契约（`infrastructure/trigger/scheduler.py:59-70`） |
| `atomic_write` | 同目录临时文件 + `std::filesystem::rename` + 失败清理（RAII） | `fsync` 语义 Python 版没做，C++ 可补 |
| `MemoryCacheHandler` + TRACE | 自研 ring-buffer sink + 自定义 level（spdlog `level::trace` 语义相近但值不同） | 「配置就绪后重放并做级别过滤」是启动期可观测性的关键（`log/bootstrap.py:79-86`） |
| `dto/`（可 pickle） | 纯 POD/`struct` + 序列化（必要时） | DTO 不要引入 Qt/sqlite3 头文件，否则跨进程/单测成本上升 |

### 12.3 移植时的三处**不要照抄**

1. **`core → plugin` 的运行时依赖**（V1）：C++ 里会变成真环。把插件注册表下沉成零依赖模块，或用注入的回调解耦。
2. **`log ↔ configue` 的惰性 import 平衡**（V2）+ **`window_presenter` 的 import 期副作用**（V6）：Python 靠 `__getattr__` / import 时机续命；C++ 的静态初始化顺序不可控，应改为「显式初始化函数表 + `main` 里顺序调用」。
3. **`on_final` 按注册顺序执行 + DB 锚点最先释放**（V8）：Python 版靠开发者手工排列注册顺序来凑出正确的拆卸序；C++ 建议直接实现为**逆注册序**（栈式拆卸），或把钩子分组（`infrastructure_group` / `service_group` / `ui_group`）并按组逆序。

---

### 附：本次分析使用的主要证据命令

```bash
# 包级 import 图（区分运行时 / TYPE_CHECKING / 函数内惰性）—— AST 解析，只读
python3 - <<'EOF'  # ast.walk + 记录 Import/ImportFrom，按 if TYPE_CHECKING 分支与函数作用域分类
EOF

grep -rn --include="*.py" -E "^\s*(from|import)\s+albuswall" .            # 内部 import 全量
grep -rn --include="*.py" "PySide6" . | grep -v "^./ui/"                  # 结果为空 → 核心零 Qt 依赖
grep -rn --include="*.py" -E "\.(register|reg|annotate)\(" .              # 全部注册点
grep -rn --include="*.py" -E "(container|c)\.(get|require|peek|try_get|has_instance)\(" .   # 全部解析点
grep -rn --include="*.py" -E "@(declare|ui_loader|register_ui)|@dynamic_state|Application\.(on_boot|on_final)" .
grep -rn "insert(0)" --include="*.py" .                                   # 仅命中一条注释 → ui/bootstrap.py:77 与实现不符
```
