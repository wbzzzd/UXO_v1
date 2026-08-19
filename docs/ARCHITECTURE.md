# 指挥席客户端软件架构

版本：V0.4 草稿
状态：待评审

## 1. 文档作用与阅读导航

本文档同时承载三层架构内容：

- **TARGET**：长期产品方向，来源于 `PRODUCT.md` 第 5 节的产品意图和第 11 节已确认决策；除明确确认的产品边界外，其余内容仍待评审，不代表已实现或已批准合同。
- **CURRENT**：当前实现事实，来源于源码、CMake 和测试，描述系统现在如何构建与运行。
- **NEXT**：已确认产品边界但尚未批准实施的候选架构草案，来源于 `docs/requirements/` 中状态为 `Draft` 的 REQ-001 至 REQ-006；需通过架构与 UI 设计评审并获用户批准后才能进入实现计划。

CURRENT 与 TARGET 不一致只表示架构缺口，不能通过修改需求迁就代码；只有关联需求和功能设计均为 `Approved` 后，缺口才能进入整改计划。NEXT 草案在批准前不得直接指导实现。

阅读导航：CURRENT 构建与边界见第 2-3 节；CURRENT 启动装配、状态所有权与已验证调用链的细节见 [`architecture/`](./architecture/) 子目录（[启动装配](./architecture/startup.md)、[状态所有权](./architecture/state-ownership.md)、[调用链](./architecture/call-chains.md)）；长期 TARGET 客户端上下文与构建模块见第 4-5 节；NEXT 候选架构（Draft）见 [candidate-architecture.md](./architecture/candidate-architecture.md)；演进映射与架构差距见第 6 节。UI 控件与交互行为见 [UI.md](./UI.md)；构建与质量门禁见 [DEVELOPMENT.md](./DEVELOPMENT.md)；产品边界与需求 ID 见 [PRODUCT.md](./PRODUCT.md)；安全边界见 [AGENTS.md](../AGENTS.md)。

## 2. CURRENT 工程概览与构建目标

当前是单进程 Qt 桌面应用：`MainWindow` 组合界面并承担大部分协调；`Core` 提供本地模拟、目标状态机和部分 3D 支持；所有状态在内存中，无外部集成或持久化。

下图的箭头表示 CMake 链接依赖（A -> B 表示 A 链接 B）。Qt 是外部依赖，测试目标、配置和文件不是生产模块，均不画入。

```mermaid
flowchart LR
    App["UXOMissionControl<br/>(可执行)"] --> MainWindow["MainWindow<br/>(静态库)"]
    MainWindow --> Core["Core<br/>(静态库)"]
    MainWindow --> Common["Common<br/>(静态库)"]
    MainWindow --> Detection["Detection<br/>(静态库)"]
```

| 构建目标 | 源码位置 | 当前责任 | 直接项目依赖 |
|----------|----------|----------|--------------|
| `UXOMissionControl` | `src/App/`、`include/App/` | `main.cpp`、CURRENT `Application` 生命周期类与窗口创建 | `MainWindow` |
| `MainWindow` | `src/MainWindow/`、`include/MainWindow/` | 22 个已编译 UI 源文件：检测阶段控件（MainWindow、LeftPanelWidget、StatusBarWidget、NavigationWidget、VideoStreamPanel、VideoOverlayWidget、TacticalMapWidget、DeviceResourceBar、TargetDetailOverlay、DetectionView）+ MOS 决策模块（DecisionView 及其 Layout/Snapshot/Tier 辅助、MosPlanningController、MosRunwayWidget 及其 Interaction、MosParamsPanel、MosGeneratorDialog、TargetCardWidget、PlanCardWidget、AlertPanel）；承担 UI 组合与大部分协调 | `Common`、`Core`、`Detection` |
| `Core` | `src/Core/`、`include/Core/` | AirportData、AirportSceneFactory、DemoScenarioProvider、SimulationWorkflow、DroneTelemetrySimulator、DetectionSimulator（无当前调用方，保留备用回退）；MOS 合成数据模型（MosTypes）、输入包络校验（MosValidation）、确定性合成 fixture 生成器（MosFixtureGenerator）、合成修复估算器（MosEstimator）、合成规划器（MosPlanner/MosPlannerProgressive）、规划会话（MosPlanningSession） | 无项目依赖 |
| `Detection` | `src/Detection/`、`include/Detection/` | DetectionEngine、PatchCoreDetector、YoloClassifier：ONNX 双阶段 UXO 检测推理（PatchCore 异常检测 + YOLOv8-cls 分类），模型文件位于 `assets/models/` | 无项目依赖（链接 `onnxruntime` 与 `Qt5::Core/Gui/Concurrent`） |
| `Common` | `src/Common/`、`include/Common/` | GlobalStyle（UI 样式）、MockDataGenerator（无当前调用方） | 无项目依赖（注：`Core` 不链接 `Common`；`Common` 通过公共头文件目录引用 `Core/Data/Types.h`，构成隐藏依赖） |

外部依赖：根 CMake 查找 `Qt5::Network` 与 `Qt5::Sql`，但无生产目标链接使用；ZeroMQ、PostgreSQL 只做可选探测；当前没有 MQTT 依赖；ONNX Runtime 头文件随源码入库，预编译动态库 `libonnxruntime.so.1.23.2` 经 `.gitignore` 不入库（`third_party/onnxruntime/`），需本地部署后才能构建，由 `Detection` 库链接，模型资产路径经 `DETECTION_ASSETS_DIR` 以开发期绝对路径注入。根 CMake 另定义 17 个测试目标（6 个检测/通用 + 11 个 MOS），测试范围见 [DEVELOPMENT.md](./DEVELOPMENT.md) 第 4 节。

`MainWindow` 库内 `MosPlanningController` 拥有同步 `MosReplanWorker`（值持有，`Qt::DirectConnection` 直连），不引入生产 `QThread`；worker 调用 plain Core `MosPlanner::planProgressive` 后同步返回完成结果。`DecisionView` 只持有 `MosPlanningSnapshot` 副本，不拥有会话状态、不发起规划、不联网。`MosRunwayWidget` 的 P0 渲染只绘制并命中当前选中档位（`m_selectedTier`），在单一共享坐标系下使用各向同性 `pxPerM`（X/Y 共用同一比例），障碍物影响圆像素半径 = `influenceRadius × pxPerM`，绘制与命中测试共用同一公式且无钳制或系数。

> CURRENT 启动装配、状态所有权与已验证调用链的细节见：
> - [startup.md](./architecture/startup.md)：启动时序与对象装配
> - [state-ownership.md](./architecture/state-ownership.md)：状态树与权威位置
> - [call-chains.md](./architecture/call-chains.md)：目标三向选择联动链、AI 检测与人工校验链与 MOS 重规划链

## 3. CURRENT 结构问题

| 结构问题 | 影响 |
|----------|------|
| `MainWindow` 混合 UI 与应用编排并持有任务/设备状态副本 | 状态分散，UI 与编排耦合 |
| `Core` 混合领域模型、模拟场景与 Qt3D 渲染 | 领域与渲染耦合，难以独立演进 |
| `Common` 混合 UI 样式与未使用的模拟数据生成，且对 `Core/Data/Types.h` 存在隐藏头文件依赖 | 责任混杂，依赖不透明 |
| `UXOMissionControl` 是启动外壳，初始化为占位 | 无真实配置、日志、装配 |
| 状态权威分散在 `SimulationWorkflow`、`MainWindow` 与 UI 面板 | 跨面板不一致，手工同步易错 |
| 根 CMake 查找 `Qt5::Network`、`Qt5::Sql`、ZeroMQ、PostgreSQL 但无生产目标使用 | 误导存在外部集成 |
| ONNX Runtime 头文件随源码入库、预编译动态库 `libonnxruntime.so.1.23.2` 经 `.gitignore` 不入库并由 `Detection` 链接；模型资产路径为开发期绝对路径注入 | 新增真实外部依赖；新克隆仓库需本地部署动态库后才能构建；部署形态（安装相对路径/Qt 资源）未定，二进制可移植性受限 |
| 外部数据边界未开始 | 无网络、存储、设备或服务适配器 |

各问题的长期归属集中见第 6.1 节，避免在两处维护同一迁移决定。

## 4. 长期 TARGET 客户端上下文

长期 TARGET 描述客户端与外部环境之间的责任边界，不指定协议、端口、数据库、厂商或部署形态。箭头表示概念性的信息、结果或任务意图流向；虚线表示责任仍未确定。真实校验与执行始终位于客户端之外。本节内容属于长期方向，不是当前实现，也不是已批准接口合同。

```mermaid
flowchart LR
    User["指挥席用户<br/>(TARGET 角色)"]
    Client["指挥席客户端<br/>UXOMissionControl"]

    ExtDetect["外部探测/目标信息<br/>(传感器、无人机等外部来源)"]
    ExtRecogn["外部识别/分析输出<br/>(分类、融合、威胁评估等外部结果)"]
    ExtSituation["外部态势信息<br/>(责任未确定,后续评审)"]
    ExtExec["外部任务执行/安全责任<br/>(排爆机器人、安全执行链等外部环境)"]

    ExtDetect -->|"目标/探测信息"| Client
    ExtRecogn -->|"识别与分析结果"| Client
    ExtSituation -.->|"态势信息(责任未定)"| Client
    Client -->|"任务意图/资源指派"| ExtExec
    ExtExec -->|"执行状态与结果反馈"| Client

    User -->|"查询/操作/确认"| Client
    Client -->|"展示/反馈"| User
```

边界说明（依据 `PRODUCT.md` 第 5 节）：

- 外部探测、识别和分析结果进入客户端，客户端负责呈现和业务编排。
- 客户端只向外部安全执行责任提交任务意图，并接收执行状态与结果；不直接控制设备。
- 外部态势、历史/审计和持久化责任尚未确定，留待后续需求与设计评审。

上述关系不构成已批准接口合同。客户端不执行真实排爆动作、不写入真实数据库或外部系统，未来任何外部接入须经过单独评审和授权（见 [PRODUCT.md](./PRODUCT.md) §5.5、§5.6 与 [AGENTS.md](../AGENTS.md)）。

## 5. TARGET 项目模块与构建依赖

本节是长期责任架构，描述客户端的逻辑分层与构建依赖方向，不是 CURRENT 实现，也不是已批准的接口设计。箭头语义：实线箭头表示编译/链接依赖（A -> B 表示 A 依赖 B）；虚线箭头表示未来或未批准的外部集成。本图只表达编译/链接依赖，不表达状态、数据或通知流。

```mermaid
flowchart TB
    App["UXOMissionControl<br/>(可执行)"]
    Presentation["Presentation<br/>(库)"]
    Visualization["Visualization<br/>(库)"]
    Application["Application<br/>(TARGET 库)"]
    Domain["Domain<br/>(库)"]
    Simulation["Simulation<br/>(库)"]
    Integration["Integration<br/>(未来库)"]

    App --> Presentation
    App --> Application
    App --> Simulation
    App -.-> Integration
    Presentation --> Application
    Presentation --> Visualization
    Visualization --> Application
    Application --> Domain
    Simulation --> Application
    Simulation --> Domain
    Integration -.-> Application
    Integration -.-> Domain
```

| 架构构件 | 架构职责 | 依赖约束 |
|----------|----------|----------|
| `UXOMissionControl` 可执行 | 组合根、进程生命周期、配置与日志初始化 | 不含业务规则；依赖 Presentation、Application、Simulation 与未来 Integration |
| `Presentation` 库 | Qt Widgets 外壳、页面、面板 | 不含权威业务状态；不直接访问外部系统；依赖 Application 与 Visualization |
| `Visualization` 库 | Qt3D、视频、地图渲染 | 不含业务状态；不访问外部系统；依赖 Application |
| TARGET `Application` 库 | 用例、命令/查询、会话与工作流编排、适配接口 | 只依赖 Domain；不含 Qt Widgets、Qt3D、Network、Sql 或外部 SDK |
| `Domain` 库 | 目标/任务/设备模型、状态机、业务规则 | 不依赖 Qt Widgets、Qt3D、Network、Sql、外部 SDK 或任何上层 |
| `Simulation` 库 | 本地场景与 TARGET Application 适配接口的实现 | 依赖 Application 与 Domain；可使用 Domain 类型，但不得绕过 Application 修改权威状态；永不接入真实外部系统 |
| `Integration` 库（未来） | 外部信息/识别/执行反馈/历史的适配 | 依赖 Application 与 Domain；可使用 Domain 类型，但命令和状态修改必须经过 Application；无 UI、无直接硬件/设备控制；仅在批准需求后引入 |

模块按需增量引入：仅当批准的工作需要时才创建对应目标与目录，不预先创建空目标或空目录以匹配本图。不新增通用 Repository、Store、服务容器或事件总线。

> NEXT 候选架构（Draft）见 [candidate-architecture.md](./architecture/candidate-architecture.md)。

## 6. 演进关系与架构差距

### 6.1 CURRENT 到 TARGET 的模块映射

| CURRENT 模块 | 长期 TARGET 归属 | 演进说明 |
|--------------|------------------|----------|
| `UXOMissionControl` | `UXOMissionControl` 可执行 | 保留为组合根，但接收真实装配责任（配置/日志/初始化） |
| `MainWindow` | `Presentation` + TARGET `Application` 编排 | UI 部分归 Presentation；应用编排与状态副本归 TARGET Application |
| `Core` | `Domain` + TARGET `Application` + `Simulation` + `Visualization` | 领域模型归 Domain；工作流编排归 TARGET Application；场景提供器归 Simulation；Qt3D 渲染归 Visualization |
| `Common` | 消除为通用目标 | `GlobalStyle` 归 Presentation；`MockDataGenerator` 归 Simulation 或在无用时移除 |

### 6.2 NEXT 组件到长期 TARGET 的归属

| NEXT 候选组件 | 长期 TARGET 归属 | 说明 |
|---------------|------------------|------|
| CURRENT `Application` 生命周期类 | `UXOMissionControl` 可执行/组合根 | 保留生命周期与装配责任，不等同于 TARGET Application 库 |
| `SimulationWorkflow` | TARGET `Application` + `Domain` | 当前候选实现同时承担编排、状态与规则，是过渡组件，不等同于最终模块 |
| `DemoScenarioProvider` | `Simulation` | 只提供本地初始场景和模拟数据 |
| `MainWindow`、UI Panels | `Presentation`；渲染相关部分归 `Visualization` | UI 组合与渲染边界分离，业务编排不留在 UI |

### 6.3 CURRENT 到 NEXT 的差距

| 差距 | CURRENT | NEXT 候选（Draft） |
|------|---------|--------|
| Workflow 所有权 | MainWindow 按值持有 | CURRENT `Application` 生命周期类创建并注入 |
| 会话状态 | 目标在 Workflow；任务/设备在 MainWindow | 全部在 SimulationWorkflow |
| UI 更新 | MainWindow 分别调用多个 setter | 统一 change 通知后回查刷新 |
| 业务命令 | MainWindow 拼接调用 | Workflow 原子处理并返回结果 |
| 任务/设备状态机 | 不存在 | 支持指派、执行、完成和拒绝路径 |
| 页面导航 | 只改变高亮 | 页面路由行为由 UI 设计确定并实现 |

### 6.4 NEXT 到长期 TARGET 的差距

下表只列出 `PRODUCT.md` 已支持的长期缺口；每项均需后续需求或设计评审，不属于已承诺工作。

| 长期 TARGET 缺口 | NEXT 草案范围 | 后续归属 |
|------------------|----------------|----------|
| 外部信息接入 | 不包含；NEXT 仅本地模拟数据 | 需求与设计评审，外部接入须单独授权 |
| 长期决策支持 | 不包含；NEXT 仅维护状态一致性 | 后续需求与算法/规则设计评审 |
| 外部执行反馈边界 | 不包含；NEXT 不接外部执行 | 外部安全执行评审，须单独授权 |
| 历史/持久化边界 | 不包含；NEXT 仅内存日志 | 持久化与回放设计评审，责任未定 |
| 用户/权限模型 | 不包含；NEXT 单一指挥席用户 | 角色与权限设计评审 |

实现步骤属于 `.omo/plans/`，不写入本架构基线。
