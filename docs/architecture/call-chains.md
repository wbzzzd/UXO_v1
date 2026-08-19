# CURRENT 已验证操作调用链

> 本文从 [ARCHITECTURE.md](../ARCHITECTURE.md) 第 5 节迁出，是 CURRENT 实现细节子文档。描述经自动测试与 UI 契约测试验证的操作路径。

下图为当前验证的三条完整操作路径：目标三向选择联动链、AI 检测与人工校验链、MOS 重规划链。三向联动链经自动测试覆盖（`simulation_workflow_ui` 验证目标表与 2D 战术地图双向高亮，`stop_select_flicker_repro` 为事件风暴调试复现）；AI 检测与人工校验链暂无自动化测试，经实际运行冒烟验证（见 [features/detection-onnx-integration.md](../features/detection-onnx-integration.md)）；MOS 重规划链经自动测试与 UI 契约测试验证。

## 1. 目标三向选择联动链

```mermaid
sequenceDiagram
    actor User as 用户
    participant Left as LeftPanelWidget
    participant Map as TacticalMapWidget
    participant DV as DetectionView
    participant Window as MainWindow
    participant Flow as SimulationWorkflow
    participant Overlay as TargetDetailOverlay

    Note over User,Overlay: 三个等价入口：左表行点击 / 地图目标点点击 / 探测页结果行选择
    User->>Left: 点击目标行
    Left->>Window: targetSelected(target副本)
    User->>Map: 点击目标标点
    Map->>Window: targetClicked(targetId)
    User->>DV: 点击结果行
    DV->>Window: resultSelected(targetId)
    Window->>Window: onSelectTargetEverywhere(targetId)
    Window->>Left: blockSignals + selectTargetRow（防递归）
    Window->>Map: setSelectedTarget(targetId)
    Window->>Flow: selectTarget(targetId)
    Window->>Overlay: showTarget(selected)
    Window->>Overlay: setEvidence(冻结证据快照) 或 clearEvidence
```

事实补充：

- 探测页结果行选择同时驱动 `DetectionView::displayRecord` 更新证据查看器、分类 Top-3、详情字段、热力图与状态时间线；`DetectionView` 只作为联动入口发出 `resultSelected`，不接收联动链回写。
- 证据快照在 AI 分析产出目标时冻结于 `MainWindow::m_evidenceByTargetId`（AI 红框标注图，无则回退热力图叠加图）。
- 未编译基线面板（`DetectionControlPanel`、`DecisionSuggestionPanel` 等，见 `UI.md`）不在本链中。

## 2. AI 检测与人工校验链

```mermaid
sequenceDiagram
    actor User as 用户
    participant Window as MainWindow
    participant Video as VideoStreamPanel
    participant Sim as DroneTelemetrySimulator
    participant Engine as DetectionEngine
    participant DV as DetectionView
    participant Flow as SimulationWorkflow
    participant Sync as 左表/地图/状态栏

    User->>Window: 地图工具栏 [开始]
    Window->>Video: play()（抽帧定时器随播放启动）
    Window->>Sim: start()
    Video->>Engine: frameExtracted(frame)
    Engine->>Engine: PatchCore 异常检测 + YOLOv8-cls 分类（QtConcurrent 后台）
    Engine-->>Window: imageAnalyzed(result)
    alt 正常帧，或异常帧但分类未确认
        Window->>DV: onFrameAnalyzed(result, 空)（仅追加结果行）
    else 异常且分类确认
        Window->>Flow: addTarget(T-NNN)
        Window->>Sync: 左表插行 + 地图红点 + 状态栏告警
        Window->>DV: onFrameAnalyzed(result, targetId)
        Window->>Window: 冻结证据快照 m_evidenceByTargetId
    end
    User->>DV: 选中结果行，点击 [确认]/[拒绝]（仅待处理状态启用）
    alt 确认
        DV-->>Window: targetConfirmed(targetId)
        Window->>Flow: selectTarget + requestSelectedTargetStatus(Confirmed)
    else 拒绝
        DV-->>Window: targetRejected(targetId)
        Window->>Flow: markSelectedTargetFalseAlarm（标记误报）
    end
    Window->>DV: 结果行状态更新为 已确认/已拒绝
```

事实补充：

- 引擎在 `setupUi` 末尾 `initialize` 加载 ONNX 模型（`patchcore_512.onnx`/`yolov8_cls_224.onnx`/`patchcore_params.json`，路径经 `DETECTION_ASSETS_DIR` 注入）；失败经 `error` 信号在状态栏告警 `[AI] 检测引擎错误: ...`，不阻断启动。
- 地图工具栏 [结束] 停止视频与遥测并回 0s；[重置] 额外清空目标、航迹、探测页结果与证据快照。
- 目标处置状态机的处置中/完成（`Disposing`/`Disposed`）无 UI 入口，仅 `SimulationWorkflow` API 可达。
- 本链暂无自动化测试，冒烟验证记录见 [features/detection-onnx-integration.md](../features/detection-onnx-integration.md)。

## 3. MOS 重规划链

```mermaid
sequenceDiagram
    actor User as 用户
    participant DV as DecisionView
    participant Window as MainWindow
    participant Ctrl as MosPlanningController
    participant Worker as MosReplanWorker(同步)
    participant Planner as MosPlanner(Core)
    participant Session as MosPlanningSession(Core)

    User->>DV: 修改参数并请求重规划
    DV->>Window: replanRequested()
    Window->>Ctrl: requestReplan(obstacles, params)
    Ctrl->>Ctrl: 分配 revision 并替换 pending 请求
    Ctrl->>Worker: replan(request副本)
    Worker->>Planner: planProgressive(obstacles, params)
    Planner-->>Worker: MosProgressiveResult
    Worker-->>Ctrl: replanCompleted(revision, result)
    alt revision 与 pending 不匹配
        Ctrl-->>Window: IgnoredStale（无日志、无状态通知）
    else worker 返回拒绝结果
        Ctrl->>Session: rejectReplan(reason)
        Ctrl-->>Window: mosStateChanged()
    else worker 返回接受结果
        Ctrl->>Planner: 按 supplied tiers 重新规划
        alt supplied tiers 非法或完成结果不一致
            Ctrl->>Session: rejectReplan(具体原因或 CompletionMismatch)
        else 重算结果逐位一致
            Ctrl->>Session: commitReplan(request副本, result)
        end
        Ctrl-->>Window: mosStateChanged()
        Window->>Ctrl: snapshot()
        Ctrl-->>Window: MosPlanningSnapshot 副本
        Window->>DV: setSnapshot(snapshot副本)
    end
```

通知语义事实：

- 控制器接受提交或拒绝日志落盘后发出无载荷 `mosStateChanged()`；`MainWindow` 随后调用 `snapshot()` 拉取按值副本并交给 `DecisionView::setSnapshot`。陈旧或重复完成返回 `IgnoredStale`，不发状态通知。
- `MosPlanningSnapshot` 按值复制 fixture、params、result、selectedTier、committedRevision 与 sequencedLog；`DecisionView` 不持有控制器或会话指针，无法回写。
- revision guard 完全同步：controller 在请求时分配单调 revision 并保存 pending 请求；只有完成 revision 与 pending 相等才可进入重算/提交，陈旧或重复完成直接忽略。worker 不写 session，controller 对接受结果按 supplied tiers 重算并逐位比对后才提交。当前实现不引入生产 `QThread`；若未来线程化必须保留同一 guard 与重算边界。
- 合法无解是接受结果：复合结果仍提交，具体 tier 以 `rectangle.valid=false` 与 `NoFeasibleRectangle` 表示；它不是 `IgnoredStale`。
- 导出链为 `MosGeneratorDialog` 请求 -> `DecisionView::exportRequested` -> `MainWindow` -> `MosPlanningController::exportFixture`。仅 controller 使用 `QSaveFile` 写出当前已提交障碍物的 canonical bytes；导出不改业务状态、revision、日志或通知计数，也无导入、网络或数据库路径。

导航仅实现页面栈路由（见 [startup.md](./startup.md)）；任务选择、设备选择、批量操作与紧急停止没有形成等价的完整消费链。
