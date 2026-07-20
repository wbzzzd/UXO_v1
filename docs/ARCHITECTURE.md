# 架构基线

最后更新：2026-07-20
状态：活文档（living document）。CURRENT 段落以源码为准，TARGET 段落尚未实现。

本文件是指挥席客户端的架构基线，供后续 agent 与开发者遵循。它不是 SRS/SDD 的复刻。出现冲突时，CURRENT 段落以源码和 `CMakeLists.txt` 为准，TARGET 段落以本文件为准；`.omo/plans/` 只负责执行，不定义长期架构。文档重整完成前，`docs/dev/` 保留为事实来源。

## 1. 架构目标与约束

目标：

- 单进程、单可执行文件 `UXOMissionControl`，基于 Qt 5 / CMake / C++17。
- 模块化单体（modular monolith），按业务能力拆模块，进程内直接调用，不引入进程间通信。
- 端口与适配器（ports and adapters）隔离外部世界：领域逻辑定义抽象端口，模拟适配器与未来真实适配器实现同一端口。
- 单一领域状态所有权：每类领域状态有且只有一个权威所有者，UI 只读回放，不持有可变副本。
- 模拟优先：MVP 阶段只允许本地模拟、只读分析和接口占位，遵守 `docs/dev/simulation-policy.md`。

约束：

- 不引入分布式部署、消息总线、微服务或独立数据库进程。MQTT、PostgreSQL、ZeroMQ 仅在 `CMakeLists.txt` 中以 `find_package(... QUIET)` 探测，当前未链接到任何目标，未在源码中使用。
- 不实现真实设备控制、排弹动作、真实 AI 推理或真实数据库写入。
- 不为尚未实现的模块预先创建空目录树。
- 任何外部接入必须经用户明确授权，并通过适配器隔离。

## 2. 当前视图（CURRENT，以源码为准）

### 2.1 构建产物与依赖

`CMakeLists.txt` 定义四个静态库子目录：`src/Common`、`src/Core`、`src/MainWindow`、`src/App`，最终链接为可执行文件 `UXOMissionControl`。

`src/Core` 当前纳入：`AirportData.cpp`、`3D/AirportSceneFactory.cpp`、`Simulation/DemoScenarioProvider.cpp`、`Simulation/SimulationWorkflow.cpp`。

`src/MainWindow` 纳入 15 个面板源文件，包括已编译但未接入主窗口的 `DecisionView.cpp` 和 `DeviceControlView.cpp`（见 2.4）。

`tests/` 下有 4 个 CTest 用例：`startup_visible`、`demo_scenario_provider`、`simulation_workflow`、`simulation_workflow_ui`。

Qt5 组件固定依赖：`Core/Widgets/Gui/3DCore/3DRender/3DInput/3DExtras/Network/Sql/Test`。ZeroMQ 与 PostgreSQL 为可选探测，不构成当前事实。

### 2.2 运行时调用链

实现在 `src/App/main.cpp`、`src/App/Application.cpp`、`src/MainWindow/MainWindow.cpp`：

```
main()
  -> Application::initialize()      // 配置/日志/数据库/通信/模块初始化
  -> Application::run()              // 显示 MainWindow
  -> app.exec()                      // Qt 事件循环
```

`Application::initialize()` 依次调用 `loadConfiguration()`、`initializeLogging()`、`initializeDatabase()`、`initializeCommunication()`、`initializeModules()`，五者当前全部为占位实现，直接 `return true`，不读取任何配置、不连接任何外部服务。`initialize()` 内部已 `new MainWindow()` 并 `show()`，随后 `run()` 再次 `show()` 同一实例。

`MainWindow` 构造函数调用 `setupUi()` 与 `loadMockData()`。`loadMockData()` 调用 `Core::Simulation::DemoScenarioProvider::create()` 取得 `DemoScenario`，把 `targets` 交给 `SimulationWorkflow::reset()`，把 `missions` 与 `devices` 拷贝到 `MainWindow` 自身成员，再分发给各面板。

### 2.3 状态所有权与复制现状

当前各类状态的权威所有者与副本分布：

| 状态 | 权威所有者 | 副本持有者 |
|------|-----------|-----------|
| 模拟目标列表 | `SimulationWorkflow` | `LeftPanelWidget`、`RightPanelWidget`、`SituationView`（位置标记）|
| 模拟任务列表 | `MainWindow::m_missions`（拷贝） | `LeftPanelWidget`、`DecisionSuggestionPanel` |
| 模拟设备列表 | `MainWindow::m_devices`（拷贝） | `LeftPanelWidget`、`RightPanelWidget`、`StatusBarWidget` |
| 当前选中目标 | `SimulationWorkflow::selectedTarget()` | `DetectionControlPanel`、`TargetDetailPanel`、`RightPanelWidget` |
| 操作日志 | `SimulationWorkflow::logEntries()` | `DetectionControlPanel` |

目标状态已收敛到 `SimulationWorkflow` 单一所有者，`MainWindow::onTargetSelected` 与 `refreshSelectedTarget` 已统一回读工作流权威副本。任务与设备仍以 `MainWindow` 成员为伪权威，通过 `setMissions(const QVector<...>&)`、`setDevices(const QVector<...>&)` 值传递给各面板，面板各自深拷贝。同一份设备数据在进程内有三到四份副本，刷新靠 `onRefreshSimulationRequested` 重新全量下发。

### 2.4 当前架构债务

以下为源码可验证的债务，迁移时应逐项处理：

- **导航路由为空操作**：`NavigationWidget` 发射 `navigationChanged(int)` 信号，`MainWindow::onNavigationChanged` 仅 `qDebug` 输出索引，不切换任何中心区域。六个导航按钮（态势/探测/决策/设备/统计/配置）除视觉选中态外无实际效果。
- **孤儿面板**：`DecisionView`、`DeviceControlView` 在 `src/MainWindow/CMakeLists.txt` 中编译，但 `MainWindow` 从未实例化或引用它们。`DecisionView` 构造函数内 `m_missionList`、`m_targetTable` 始终为 `nullptr`。`DeviceControlView` 内的 `DroneControlPanel`、`RobotControlPanel` 为空构造体。
- **`BatchOperationBar` 默认隐藏**：已加入布局但构造时 `hide()`，无任何路径触发 `setSelectedCount(>0)` 使其显示，信号 `assignTaskRequested`、`markIgnoreRequested` 无连接。
- **应用初始化全占位**：`Application` 的五个初始化函数均为 `return true`，配置路径 `m_configPath` 被赋值但未读取，`config/system.json`、`config/devices.json` 未被任何代码加载。
- **任务与设备状态散落**：任务和设备没有类似 `SimulationWorkflow` 的单一所有者，状态变更（若未来需要）将面临多副本同步问题。
- **`m_mainSplitter` 未使用**：`MainWindow.h` 声明 `m_mainSplitter`，`createMainLayout` 用 `QHBoxLayout` 直接布局，splitter 成员闲置。
- **窗口最小尺寸溢出**（P2，不阻塞）：1280×720 下 `DecisionSuggestionPanel` 底部约 5px 溢出，已在 `docs/dev/mvp-scope.md` 记录。

## 3. 目标视图（TARGET，尚未实现）

本节描述期望架构，作为后续迁移的参照。**所有命名、目录、接口均为设计占位，当前源码中不存在。** 实现前必须先在 `.omo/plans/` 下落地计划并经用户确认。

### 3.1 总体形态

单进程模块化单体，进程内分层：

```
App（进程入口、生命周期）
  -> ApplicationServices（应用服务编排，工作流边界）
    -> Domain Core（领域模型与纯逻辑，无 Qt UI 依赖）
       端口：IRecognitionService、IMOSPlanner、IDeviceAdapter、IStorage...
    -> Adapters（适配器实现）
       Mock* 适配器（当前）、Real* 适配器（未来，需授权）
  -> MainWindow（UI 组合，只读回放领域状态）
     各 UI Panel
```

依赖方向单向向内：`App` -> `ApplicationServices` -> `Domain Core` <- `Adapters`。`MainWindow` 依赖 `ApplicationServices` 暴露的只读视图与命令接口，不直接持有领域可变状态。

### 3.2 模块划分与依赖方向

| 模块 | 职责 | 依赖 |
|------|------|------|
| `Core/Data` | 稳定数据模型（`TargetInfo`、`MissionInfo`、`DeviceInfo`、`YieldEstimate` 等 POD） | Qt5::Core |
| `Core/Domain` | 领域逻辑（`SimulationWorkflow`、识别服务端口、MOS 规划端口） | `Core/Data` |
| 功能模块 | 获批功能所需的领域逻辑、端口和模拟适配器 | `Core/Domain`、`Core/Data` |
| `Adapters/Real*` | 未来真实设备/通信/存储适配器，需用户授权后引入 | `Core/Domain` 端口 |
| `MainWindow` | UI 组合、用户交互、展示状态 | `ApplicationServices` 只读视图 |
| `App` | 进程入口、依赖装配 | `MainWindow`、`ApplicationServices`、`Adapters` |

依赖方向铁律：`Core` 不依赖 `MainWindow`；`MainWindow` 不直接修改领域状态，只通过 `ApplicationServices` 发命令并接收回读。

### 3.3 单一领域状态所有权

每类领域状态有且只有一个所有者，其他模块持引用或只读快照：

- 模拟目标、选中目标、操作日志 -> `SimulationWorkflow`（已实现，保持）
- 模拟任务、模拟设备 -> 应收敛到一个 `ScenarioRepository` 或扩展后的 `SimulationWorkflow`，`MainWindow` 不再持有 `m_missions`、`m_devices` 副本
- 识别结果、MOS 规划结果 -> 由对应应用服务持有，UI 通过只读接口回读

UI 面板禁止缓存可变领域副本。需要本地展示缓存时，必须明确标注为只读快照，并在状态变更信号下统一刷新。

### 3.4 应用服务与工作流边界

`ApplicationServices` 层负责：

- 装配领域服务与适配器（构造函数注入，不使用服务定位器）
- 暴露给 UI 的只读查询接口（`targets()`、`selectedTarget()`、`logEntries()` 等）
- 暴露给 UI 的命令接口（`confirmTarget()`、`requestStatus()`、`overrideRecognition()` 等）
- 跨领域协调（如识别结果变更触发 MOS 重规划）

`SimulationWorkflow` 是当前唯一已实现的工作流，定位为领域内"目标状态机 + 日志"所有者。未来识别、MOS 等工作流各自独立，由 `ApplicationServices` 编排，不互相直接依赖。

### 3.5 UI 依赖规则

- `MainWindow` 与各 Panel 只依赖 `Core/Data` 的 POD 类型与 `ApplicationServices` 的抽象接口，不依赖具体适配器实现。
- Panel 之间不直接通信，信号一律上抛到 `MainWindow` 或 `ApplicationServices`，由其分发。
- 任何 Panel 都不应成为状态权威。当前 `LeftPanelWidget::setTargets` 等方法保留，但应改为接收只读引用或由工作流直接驱动模型。
- 新增 Panel 必须先有 `ApplicationServices` 接口或工作流支撑，不允许 Panel 自行持有领域状态。

### 3.6 模拟与真实适配器边界

- 外部交互仅在获批功能存在真实调用需求时定义最小端口；不提前创建 `IRecognitionService`、`IMOSPlanner`、`IDeviceAdapter`、`IStorage` 等空接口。
- MVP 阶段只实现 `Mock*` 适配器，类名含 `Mock`/`Simulation`/`Demo` 前缀，UI 文案含"模拟"字样。
- 真实适配器（`Real*`、`*Adapter`）只能在用户明确授权后引入，且必须与 Mock 实现物理隔离（独立目录、独立 CMake target）。
- 端口签名按同步调用 + POD 输入输出设计。未来若需异步或图像输入，通过扩展新端口而非修改旧端口实现。
- `config/` 下的 JSON 仅作为模拟数据输入，不写入真实数据库或外部系统。

## 4. 迁移序列

建议按以下顺序推进，每步落地前需在 `.omo/plans/` 建计划并经用户确认：

1. **完成文档重基线**：逐项评审 `PRODUCT.md`、本文档、`UI.md` 和 `DEVELOPMENT.md`，批准 NEXT 范围后再改代码。
2. **状态所有权收敛**：将 `m_missions`、`m_devices` 从 `MainWindow` 迁出，引入统一状态仓库或扩展 `SimulationWorkflow`，消除面板多副本。
3. **应用服务边界**：把业务命令和查询从 `MainWindow` 抽到应用服务，UI 只发送命令并回读状态。
4. **导航路由落地**：实现中心区域页面切换，明确接入或移除 `DecisionView`、`DeviceControlView` 等孤儿面板。
5. **可用模拟指挥环**：补齐目标选择、模拟设备指派、任务执行、状态联动和操作日志。
6. **功能增量评审**：分别评审 UXR、MOS 等功能草案；只有批准后才增加对应数据模型、端口、适配器和 UI。
7. **工程能力完善**：按 `DEVELOPMENT.md` 的评审结果处理配置加载、CI、发布物和未用控件。

## 5. 风险与技术债

| 编号 | 风险/债务 | 影响 | 缓解 |
|------|----------|------|------|
| R1 | 任务与设备状态多副本，刷新靠全量下发 | 中 | 步骤 3 收敛到单一所有者 |
| R2 | 导航信号空操作，6 个按钮无效果 | 高（演示可见） | 步骤 4 落地路由 |
| R3 | 孤儿面板 `DecisionView`、`DeviceControlView` 占编译资源 | 低 | 步骤 4 接入或移除 |
| R4 | `Application` 初始化全占位，配置文件未被读取 | 中 | 步骤 6 替换 |
| R5 | MOS 业务参数（机型尺寸、K 值、档位数等）未确认 | 高 | 见 PRD-INC-UXO-001 第 7 节，需业务方确认 |
| R6 | `SimulationWorkflow` 单线程内存状态，无持久化 | 低（MVP 内可接受） | P2 引入存储端口 |
| R7 | `MainWindow` 承担装配与 UI 组合双重职责 | 中 | 步骤 3 后逐步上抽 `ApplicationServices` |
| R8 | 端口签名可能与未来真实 AI/设备调用方式不匹配 | 中 | 端口按最小同步契约设计，扩展走新端口 |

## 6. 待确认事项

以下需用户或业务方明确，标记 `[待确认]`：

- `[待确认]` 目标机型最小起降尺寸（MOS-Q1，默认 460m×15m）
- `[待确认]` 殉爆安全系数 K（MOS-Q3，默认 1.5）
- `[待确认]` 弹坑结构损伤扩展系数（MOS-Q4，默认 1.5）
- `[待确认]` 递进档位数是否固定为 3（MOS-Q8，默认可配 2~5）
- `[待确认]` 任务与设备状态是否需要持久化，以及是否引入 `ScenarioRepository`
- `[待确认]` `DecisionView`、`DeviceControlView` 是接入还是移除
- `[待确认]` 真实设备/通信/存储适配器的引入时机与授权范围

## 7. 关联文档与决策占位

工程事实：
- `docs/dev/current-state.md` 当前代码事实
- `docs/dev/architecture-boundaries.md` 当前与未来边界
- `docs/dev/simulation-policy.md` 模拟策略与安全边界
- `docs/dev/build-and-run.md` 构建与运行
- `docs/dev/mvp-scope.md` MVP 范围与验收

设计资料（目标，非当前事实）：
- `SRS排弹抢修指挥系统_v1.0.md`
- `SDD排弹抢修指挥系统_v1.0.md`
- `docs/前端UI详细设计文档.md`
- `docs/dev/incremental-prd-uxo-recognition-and-mos.md`
- `docs/dev/product-design-mos.md`
- `docs/dev/architecture-uxo-recognition-and-mos.md`
- `docs/dev/mos-ui-design-brief.md`

决策记录（`docs/ddr/`）：
- DDR-001 项目现状评审
- DDR-002 硬件缺位下开发策略
- DDR-003 Phase1 执行计划
- DDR-004 多 Agent 并行架构调整
- DDR-005 DDR004 二次验证与修正
- DDR-006 产品视角架构评估
- DDR-007 代码架构之道
- DDR-008 当量识别技术可行性评估

本架构基线的重大变更应新增 ADR 或更新对应 DDR，不在本文件内追加长篇论述。
