# CURRENT 状态所有权

> 本文从 [ARCHITECTURE.md](../ARCHITECTURE.md) 第 4 节迁出，是 CURRENT 实现细节子文档。描述当前状态树与权威位置。

```text
MainWindow
├── m_missions（任务，静态）
├── m_devices（设备，静态）
├── SimulationWorkflow（按值持有）
│   ├── targets（目标，空起步：探测阶段由 DetectionEngine 检出异常动态注入）
│   ├── 当前选择
│   └── logEntries（操作日志）
├── DroneTelemetrySimulator（无人机遥测模拟器）
├── DetectionEngine（ONNX 检测引擎，视频每 3 秒抽帧驱动 analyzeFrame，输出 ImageDetectionResult；DetectionSimulator 保留未接线）
├── m_evidenceByTargetId（冻结标注证据，内存 QMap<QString, DetectionEvidence>）
├── m_tacticalMap（2D 战术地图目标列表）
├── VideoStreamPanel（视频 PiP 面板，QMediaPlayer + QVideoWidget + QVideoProbe，播放本地视频文件）
├── VideoOverlayWidget（HUD 叠加层，VideoStreamPanel 子 widget，无业务状态）
├── DeviceResourceBar（设备资源条，36px，显示设备在线状态，无业务状态）
├── TargetDetailOverlay（目标详情浮层，340px 不透明面板，显示选中目标的冻结标注证据）
└── MosPlanningController（QObject 子对象）
    ├── MosPlanningSession（按值持有，plain Core）
    │   ├── committedFixture / params / result / selectedTier（按值）
    │   ├── committedRevision（int）
    │   └── sequencedLog（按值快照）
    ├── MosReplanWorker（按值持有，同步）
    │   └── 内部持有 controller 指针，调用 MosPlanner::planProgressive
    └── pendingRevision（同步 revision guard）

AlertPanel（自身告警展示数据）
StatusBarWidget（自身告警展示数据）
DecisionView（持有 MosPlanningSnapshot 副本，不拥有会话状态）
```

| 状态 | 当前权威位置 | 复制/展示位置 | 问题 |
|------|--------------|--------------|------|
| 目标、当前选择、操作日志 | `SimulationWorkflow` | LeftPanel、DecisionPanel、TacticalMap、DetectionControlPanel 日志 | 需 MainWindow 手工同步到各 UI |
| 探测阶段遥测与 AI 检测 | `DroneTelemetrySimulator`、`DetectionEngine`（`DetectionSimulator` 保留未接线） | 遥测/检测结果 -> MainWindow 四区同步；2D 地图为 aspect-fit 卫星图共享 WGS84 叠加矩形，无人机沿跑道轴向本地模拟巡航，目标偏移按 UAV 航向旋转后转 WGS84 | 遥测仍为本地模拟，非真实 GIS/飞控；检测为真实 AI 推理但抽帧间隔/判定阈值硬编码 |
| 视频管线 | `VideoStreamPanel`（QMediaPlayer + QVideoWidget + QVideoProbe） | 播放本地视频文件，每 3 秒抽帧送 `DetectionEngine` 推理；证据图像取引擎热力图叠加图 | 本地文件回放，非真实视频流；AI 推理为真实算法（本地 ONNX） |
| 冻结标注证据 | `MainWindow::m_evidenceByTargetId`（内存 QMap）+ `TargetDetailOverlay` 显示 | 检测时捕获引擎热力图叠加图（`heatmapOverlay`），选中目标时在详情浮层显示 | 由 MainWindow 在检测事件中捕获、选择事件中下发到详情浮层 |
| 2D 地图目标列表 | `TacticalMapWidget` 内部 m_items | 与目标表保持同步 | 副本，由 MainWindow 手工同步 |
| 任务 | `MainWindow::m_missions` | LeftPanel、DecisionPanel | 静态，不随目标处置变化 |
| 设备 | `MainWindow::m_devices` | LeftPanel、DeviceStatusPanel、StatusBar | 静态，不随任务变化 |
| 告警展示数据 | `AlertPanel` 与 `StatusBarWidget` 各自保存 | 两套 UI | 启动时注入，无统一告警状态 |
| MOS 会话（fixture/params/result/tier/revision/log） | `MosPlanningController` 内 `MosPlanningSession` | `DecisionView` 持有 `MosPlanningSnapshot` 副本 | 控制器单点所有权，快照按值下发；UI 无回写路径 |
| MOS pendingRevision（同步防陈旧） | `MosPlanningController` | worker 启动时读取 | 仅同步语义，无跨线程竞态；若未来线程化需保留 revision guard |
