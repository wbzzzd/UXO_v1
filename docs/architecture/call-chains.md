# CURRENT 已验证操作调用链

> 本文从 [ARCHITECTURE.md](../ARCHITECTURE.md) 第 5 节迁出，是 CURRENT 实现细节子文档。描述经自动测试与 UI 契约测试验证的操作路径。

下图为当前经自动测试与 UI 契约测试验证的两条完整操作路径：目标处置链（原有）与 MOS 重规划链（新增）。

## 1. 目标处置链

```mermaid
sequenceDiagram
    actor User as 用户
    participant Left as LeftPanelWidget
    participant Window as MainWindow
    participant Flow as SimulationWorkflow
    participant Control as DetectionControlPanel
    participant Decision as DecisionSuggestionPanel

    User->>Left: 点击目标行
    Left->>Window: targetSelected(target副本)
    Window->>Flow: selectTarget(target.id)
    Window->>Control: setSelectedTarget()
    Window->>Decision: setTarget()
    User->>Control: 模拟确认/处置/完成
    Control->>Window: 对应requested信号
    Window->>Flow: requestSelectedTargetStatus()
    Window->>Left: updateTargetStatus()
    Window->>Control: 刷新状态和日志
    Window->>Decision: 刷新目标状态
```

## 2. MOS 重规划链

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

导航、任务选择、设备选择、批量操作、紧急停止和 3D 目标点击没有形成等价的完整消费链。
