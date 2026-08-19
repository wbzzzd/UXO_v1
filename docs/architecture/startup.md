# CURRENT 启动与对象装配

> 本文从 [ARCHITECTURE.md](../ARCHITECTURE.md) 第 3 节迁出，是 CURRENT 实现细节子文档。描述当前启动时序、对象装配事实与页面栈路由。

```mermaid
sequenceDiagram
    participant Main as main()
    participant App as CURRENT Application生命周期类
    participant Window as MainWindow
    participant Ctrl as MosPlanningController
    participant Engine as DetectionEngine
    participant Demo as DemoScenarioProvider
    participant Flow as SimulationWorkflow
    participant Panels as UI Panels

    Main->>App: initialize()
    App->>App: 配置/日志/数据库/通信/模块初始化
    Note over App: 五个函数当前直接返回成功
    App->>Window: new MainWindow()
    Window->>Window: setupUi()
    Window->>Ctrl: new MosPlanningController(this)
    Note over Ctrl: 在 UI 构造完成前创建，<br/>controller 为 window 的 QObject 子对象
    Window->>Window: 菜单栏/主布局/引擎与模拟器 new/地图工具栏/状态栏/信号连接
    Window->>Engine: initialize(patchcore_512.onnx, yolov8_cls_224.onnx)
    alt 初始化或推理出错
        Engine-->>Window: error(QString)
        Window->>Panels: 状态栏告警 "[AI] 检测引擎错误: ..."（不阻断启动）
    end
    Window->>Demo: loadMockData() -> DemoScenarioProvider::create()
    Demo-->>Window: 2设备 + 1任务 + 无人机航线 + 检测数据 + 机场边界（空起步：0 目标）
    Window->>Flow: reset(空目标列表)
    Window->>Panels: 下发missions/devices/航线/检测数据/机场边界
    Window->>Ctrl: requestReplan(MosFixtureGenerator seed=42)
    Ctrl-->>Window: 首次 fixture/plan/tier1 快照就绪
    Window->>Panels: 下发首次 MOS 快照(DecisionView)
    App->>Window: show()
    Main->>App: run()
    App->>Window: 再次show同一窗口
```

装配事实：

- CURRENT `Application` 生命周期类创建 `MainWindow` 并负责显示。
- `MainWindow` 创建并按值拥有 `SimulationWorkflow`。
- `MosPlanningController` 在 `MainWindow` 构造早期、UI 创建前创建（`setupUi` 内、菜单栏与主布局之前），作为 `MainWindow` 的 `QObject` 子对象；它内部按值持有 plain Core `MosPlanningSession` 与同步 `MosReplanWorker`，不引入生产 `QThread`。
- 启动期 `loadMockData()` 触发 `requestReplan`，由 `MosFixtureGenerator` 以 seed=42 生成确定性 fixture 与 tier1 规划，快照同步 `DecisionView`；该 seed 只在 `MainWindow` 启动期使用，`MosGeneratorDialog` 中后续修改不持久化、不回写。
- `setupUi()` 内创建 `DetectionEngine`（PatchCore 异常检测 + YOLOv8-cls 分类双 ONNX 推理）与 `DroneTelemetrySimulator`；`DetectionEngine::initialize` 失败仅经 `error` 信号在状态栏告警，不阻断启动；模型路径经 `DETECTION_ASSETS_DIR` 以开发期绝对路径注入。
- `MainWindow` 构造按序执行 `setupUi()` 与 `loadMockData()`：启动为空起步（0 目标），探测结果由态势页地图工具栏 [开始]/[结束] 控制的抽帧分析产生。
- `DemoScenarioProvider` 只提供初始模拟数据，不参与运行时状态。
- CURRENT `Application` 生命周期类的五个初始化函数（配置/日志/数据库/通信/模块）当前直接返回成功。
- 配置文件和外部依赖没有进入装配流程。

页面栈路由事实（`pageStack` 为 `QStackedWidget`，索引 0 = 态势页 `m_situationPage`，索引 1 = `DetectionView` 探测页，索引 2 = `DecisionView` 决策页，启动时 `setCurrentIndex(0)`）：

- `NavigationWidget` 共 6 个导航项，索引 2 的 `key` 为 `decision`/`决策`。
- `onNavigationChanged(navIndex)` 路由：`navIndex == 1` -> `pageStack` 切到索引 1（探测页）；`navIndex == 2` -> 切到索引 2（决策页）；其他 nav 索引（0/3/4/5）-> 切回索引 0（态势页）。不隐藏顶部工具栏。
- `DecisionView` 不在 `pageStack` 之外的任何容器内重复实例化；它只持有 `MosPlanningSnapshot` 副本，不拥有会话状态、不发起规划、不联网。`DetectionView` 同样仅在 `pageStack` 索引 1 实例化一次。
