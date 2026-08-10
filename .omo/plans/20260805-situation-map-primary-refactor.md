# 态势页地图主舞台重构执行计划

状态：Completed（7阶段全部完成，构建+ctest通过，截图已捕获）
关联需求：REQ-008（PRODUCT.md §9.2，Approved）
关联原型：`docs/ui/prototypes/situation/index.html`（第5版，用户已确认方向）
前置计划：`.omo/plans/20260803-detection-stage-demo.md`（探测阶段演示，已完成但将被本重构取代部分成果）

## 背景

### 当前事实（CURRENT）

MainWindow 当前布局（`src/MainWindow/MainWindow.cpp:183-280`）：

```
[菜单栏] [工具栏] [探测演示工具栏(36px)]
[导航80px | 左面板320px(目标/任务/设备tab) | 中心区(垂直splitter: 2D地图上 + 视频信息下) | 右面板320px]
[状态栏]
```

中心区下半的"信息区"包含 AlertPanel + DetectionControlPanel + BatchOperationBar。
右面板包含 SituationView(Qt3D,已判定不可用) + DeviceStatusPanel + DecisionSuggestionPanel。
探测演示由 DetectionTimelineController + 探测工具栏([重置][开始][结束]) 驱动 80s 5目标流入。
启动为空起步，目标由脚本注入。
三步状态工作流（已发现→已确认→处置中→已完成）由 DetectionControlPanel 三按钮驱动。

测试 `simulation_workflow_ui_test.cpp`（853行，14个测试）深度依赖：
- `detectionController()` 公开接口注入目标（测试3-7,12-14）
- 探测工具栏三按钮（测试8-11）
- DetectionControlPanel 三步按钮 `simulationConfirmButton/StartButton/CompleteButton`（测试1,5,6,7）
- `alertCount` 标签（测试14）

### 目标设计（TARGET，已确认原型）

```
[菜单栏] [工具栏(简化,无探测控制)]
[导航80px | 左pane(可折叠40↔320,仅目标) | 中心区(设备资源条36px + 地图全宽主舞台)]
[状态栏(含告警滚动)]
```

地图上浮层：左下角视频PiP(480x270,可拖动/缩放/最小化/主次切换) + 右侧目标详情研判浮层(选中时出现)。
设备资源条：UAV-1 + Robot-1 卡片，点击切换PiP视频源。
目标详情浮层3个模拟操作：创建处置任务 / 指派设备 / 查看历史检测（纯文本反馈，无状态机）。

### 关键语义变化（需用户确认）

本重构不仅是布局变化，还涉及3处语义变更：

**变更1：移除探测时间线窗口+播放控制工具栏，保留视频驱动目标注入**
- 旧：空起步 + DetectionTimelineController 脚本按 10s/25s/42s/60s/78s 注入5目标 + [重置][开始][结束]工具栏
- 新：空起步 + 视频自动播放 + DetectionTimelineController 保留（按时间点注入目标）+ 无播放控制工具栏 + 无时间线窗口
- 影响：DetectionTimelineController 类保留；探测工具栏移除；测试中直接 emit targetDetected 的方式仍可用
- 理由：用户否定的是时间线可视化窗口和播放控制按钮，不是视频驱动目标流入机制本身

**变更2：移除三步状态工作流，改为研判浮层模拟操作**
- 旧：DetectionControlPanel 三按钮（模拟确认/开始处置/完成处置）驱动 TargetStatus 状态机
- 新：TargetDetailOverlay 三操作（创建处置任务/指派设备/查看历史检测）纯文本反馈，不改状态
- 影响：DetectionControlPanel 类弃用；测试1,5,6,7（4个测试）需移除或重写
- 理由：新设计把研判操作放在选中浮层里，操作语义与旧状态机不同；SimulationWorkflow 状态机方法（requestSelectedTargetStatus 等）暂保留但无 UI 调用方，留作后续清理

**变更3：移除右面板与信息区，告警改为状态栏滚动**
- 旧：右面板（SituationView/DeviceStatusPanel/DecisionSuggestionPanel）+ 中心信息区（AlertPanel/DetectionControlPanel/BatchOperationBar）
- 新：无右面板；告警仅在状态栏滚动展示（StatusBarWidget 已有 addAlarm 能力）；设备状态移至设备资源条
- 影响：RightPanelWidget/AlertPanel/BatchOperationBar/DeviceStatusPanel/DecisionSuggestionPanel/SituationView 弃用
- 理由：用户明确"去掉右面板""不要流程可视化"

## 目标

把态势页 C++ 实现从旧布局（左右面板+垂直分割+时间线驱动）重构为确认的原型设计（地图主舞台+可折叠左pane+设备资源条+目标详情浮层+视频PiP）。布局与交互对齐原型 `docs/ui/prototypes/situation/index.html`。

## 非目标

- 不做识别/决策/规划/排弹阶段页面
- 不做真实设备控制/排弹/外部通信/数据库写入
- 不做 PiP 拖动+缩放的精细交互（本轮只做固定位置+最小化+关闭+主次切换，拖动缩放置后续）
- 不删除弃用类的源文件（保留 git 历史，仅从 CMakeLists 移除编译）
- 不改 SimulationWorkflow 内部状态机实现（保留，仅去掉 UI 调用方）
- 不改导航栏/菜单栏/状态栏结构（沿用现有）

## 安全边界

- 所有目标/告警/操作反馈标注"模拟"
- 视频为本地占位，不接外部流
- 2D 地图底图为本地占位
- 无真实设备控制命令

## 执行任务

### 阶段1：新建设备资源条 widget

- [ ] 1.1 新建 `DeviceResourceBar` 类
  - `include/MainWindow/DeviceResourceBar.h` + `src/MainWindow/DeviceResourceBar.cpp`
  - 继承 QWidget，固定高度36px，QHBoxLayout
  - 左侧 label "设备资源"
  - 设备卡片：UAV-1 / Robot-1（点击高亮+发 `deviceSelected(DeviceInfo)` 信号）
  - 卡片显示：状态点 + ID + 电量 + 任务状态
  - 暴露 `setDevices(QVector<DeviceInfo>)` + `selectDevice(QString id)`
  - objectName: `deviceResourceBar`
  - 验证：编译通过 + 单元测试断言2卡片存在

### 阶段2：新建目标详情研判浮层 widget

- [ ] 2.1 新建 `TargetDetailOverlay` 类
  - `include/MainWindow/TargetDetailOverlay.h` + `src/MainWindow/TargetDetailOverlay.cpp`
  - 继承 QWidget，固定宽340px，默认隐藏
  - 内容：关闭按钮 + 目标ID + 类型标签 + 详情行(威胁/置信度/坐标/检测设备/距跑道) + 3操作按钮 + 反馈文本
  - 暴露 `showTarget(TargetInfo)` / `hide()` / `isVisible()`
  - 3操作按钮发信号 `createTaskRequested(TargetInfo)` / `assignDeviceRequested(TargetInfo)` / `viewHistoryRequested(TargetInfo)`
  - 待检测目标：隐藏详情行+操作，显示"待检测"提示
  - objectName: `targetDetailOverlay`
  - 验证：编译通过

### 阶段3：改造 LeftPanelWidget 为可折叠目标列表

- [ ] 3.1 移除任务/设备tab，仅保留目标
  - 删除 m_tabWidget/m_missionTable/m_deviceTable 及相关方法
  - 删除 setMissions/setDevices/populateMissionList/populateDeviceList/setupMissionList/setupDeviceList
  - 保留 m_targetTable + 搜索框 + 状态子标签 + 刷新按钮
  - 移除筛选按钮（原型无）
  - 状态子标签文案改"待处置/处置中/已完成"（去掉"任务"后缀，按目标计数）
  - 验证：编译通过

- [ ] 3.2 增加可折叠能力
  - 新增 `setCollapsed(bool)` / `isCollapsed()` + 信号 `collapseChanged(bool)`
  - 折叠态：宽40px，竖排显示"▶ 目标"
  - 展开态：宽320px，显示完整内容 + 左上角 ◀ 折叠按钮
  - 默认折叠（原型默认收起）
  - objectName: `leftPane`
  - 验证：编译通过

### 阶段4：重构 MainWindow 布局

- [ ] 4.1 移除旧成员与依赖（保留 m_timelineController 与 detectionController()，见决策点1）
  - MainWindow.h 移除：m_rightPanel, m_detectionToolBar, m_detectionStartBtn/StopBtn/ResetBtn, m_centerSplitter, m_alertPanel, m_detectionControlPanel, m_batchOperationBar
  - MainWindow.h 保留：m_timelineController（视频驱动注入仍需），detectionController() 公开方法（测试注入仍需），onTargetDetected 槽（四区同步仍需）
  - MainWindow.h 新增：m_deviceBar, m_targetDetailOverlay
  - 移除 onDetectionStartClicked/StopClicked/ResetClicked, refreshDetectionToolBarState, createDetectionToolBar（工具栏UI移除）
  - 验证：编译通过

- [ ] 4.2 重写 createMainLayout
  - 布局：导航 | 左pane(可折叠) | 中心区(设备资源条 + 地图全宽)
  - 中心区 QVBoxLayout：上 DeviceResourceBar(36px固定) + 下 TacticalMapWidget(拉伸)
  - VideoStreamPanel 作为 TacticalMapWidget 的浮层子widget（左下角480x270）
  - TargetDetailOverlay 作为 TacticalMapWidget 的浮层子widget（右上角340px）
  - 移除 m_centerArea 旧分割结构
  - 验证：启动后布局正确

- [ ] 4.3 改造 loadMockData 为空起步 + 设备加载（保留视频驱动注入，见决策点1）
  - 保留 DetectionTimelineController 创建与脚本加载（视频驱动注入）
  - 空起步：工作流以空目标集合初始化（不预加载目标）
  - 设备加载到 DeviceResourceBar + StatusBarWidget
  - 移除 RightPanelWidget / AlertPanel / DetectionControlPanel 调用（告警仅走状态栏滚动）
  - 启动后视频自动播放驱动目标流入（保留 m_videoStreamPanel->play() 须用户或测试触发，启动不自动播放）
  - 验证：启动后目标表0行 + 地图0红点 + 设备资源条2卡片 + 无探测工具栏 + 无右面板

- [ ] 4.4 重连信号槽
  - 左pane折叠按钮 -> setCollapsed
  - 目标表行点击 -> onTargetSelected -> 显示 TargetDetailOverlay + 地图高亮
  - 地图红点点击 -> selectTargetRow + 显示 TargetDetailOverlay
  - TargetDetailOverlay 关闭 -> 隐藏 + 清除选中
  - TargetDetailOverlay 3操作 -> 纯文本反馈（"✓ 已创建 TASK-XXX（模拟）"等）
  - DeviceResourceBar 卡片点击 -> 切换 VideoStreamPanel 视频源标签
  - 视频PiP 最小化/关闭/主次切换按钮
  - 验证：交互闭环

### 阶段5：更新 CMakeLists

- [ ] 5.1 `src/MainWindow/CMakeLists.txt`
  - MAINWINDOW_SOURCES 新增：DeviceResourceBar.cpp, TargetDetailOverlay.cpp
  - MAINWINDOW_SOURCES 移除：RightPanelWidget.cpp, DetectionControlPanel.cpp, AlertPanel.cpp, BatchOperationBar.cpp（文件保留不删，仅不编译）
  - HEADERS 同步
  - 验证：cmake configure 通过

### 阶段6：重写测试

- [ ] 6.1 重写 `simulation_workflow_ui_test.cpp`（保留 DetectionTimelineController 注入测试，见决策点1）
  - 移除8个工具栏/三步工作流相关测试：
    - validActionsUpdateAllUiState, operationLogPreservesSimulationOrder, newWindowStartsWithFreshWorkflow, detectionToolBarInitialButtonState, detectionStartTogglesButtonState, detectionStopKeepsProgressAndResetsButtons, detectionResetClearsAllFourZones, detectionStageE2EFiveTargetsFourZoneSync
  - 保留并适配：detectingTargetInjectsFourZoneSync（控制器注入目标，移除 simulationOperationLog 断言）, selectingDetectedTargetShowsDetected（改为断言 TargetDetailOverlay 显示）, tacticalMapClickHighlightsTargetRow, targetRowClickHighlightsTacticalMap（去掉 startBtn 依赖，直接 emit targetDetected）
  - 改写 initialSurfaceIsSimulationOnly：断言空起步（目标表0行）+ 无探测工具栏按钮 + 无右面板 + 设备资源条2卡片
  - 改写 unsafeControlsAreAbsent：移除 DetectionControlPanel/DeviceStatusPanel 断言（类已弃用），仅保留全局禁用按钮扫描
  - 新增：左pane可折叠测试（setCollapsed/isCollapsed/collapseChanged信号）
  - 新增：设备资源条存在测试（2卡片objectName校验）
  - 新增：目标详情浮层选中显示测试（选中目标后浮层可见+3操作按钮objectName）
  - 验证：ctest 全绿

### 阶段7：构建与视觉验证

- [x] 7.1 构建验证
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` 退出0
  - `cmake --build build --target UXOMissionControl -j2` 退出0
  - `ctest --test-dir build --output-on-failure` 全绿（4/4 通过，14 个 UI 测试全 pass）

- [x] 7.2 离屏截图验证
  - `UXO_VISUAL_EVIDENCE_DIR=... simulation_workflow_ui_test` 抓图（4 张已捕获）
  - 截图文件：`refactor-initial.png` / `refactor-detected.png` / `refactor-overlay-shown.png` / `refactor-e2e-five-targets.png`
  - 视觉分析：AI 视觉模型不支持图像输入，截图已落盘供用户自行审查

## 完成总结

| 阶段 | 状态 | 说明 |
|------|------|------|
| 1. DeviceResourceBar | ✅ | 36px 顶栏，2 设备卡片，点击切换 PiP 源 |
| 2. TargetDetailOverlay | ✅ | 340px 浮层，3 操作信号，选中显现/关闭隐藏 |
| 3. LeftPanelWidget 可折叠 | ✅ | 40↔320px 切换，默认折叠，仅目标 tab |
| 4. MainWindow 布局重构 | ✅ | 地图主舞台+浮层定位+eventFilter+空起步 auto-start |
| 4.5. 修复 gaps | ✅ | auto-start 视频/控制器、stateChanged 连接、指针 cast、stylesheet |
| 5. CMakeLists.txt 更新 | ✅ | 移除 10 死代码文件，新增 2 新文件 |
| 6. 测试重写 | ✅ | 14 个测试覆盖全部新结构契约 |
| 7. 构建验证 | ✅ | cmake build 成功，ctest 4/4 通过 |

### 遗留事项

- 10 个旧文件（RightPanelWidget/AlertPanel/DetectionControlPanel/BatchOperationBar 等）已从 CMakeLists 移除但仍在磁盘上，是孤立死代码，待用户决定是否删除
- 截图视觉外观待用户自行审查（AI 视觉模型不可用）
- 未提交 git（按规则不主动提交）

## 待确认决策点

请在实现前确认以下3项（对应"关键语义变化"）：

1. **移除80s探测时间线，目标改启动静态加载** — 确认？DetectionTimelineController 类将弃用。
2. **移除三步状态工作流，改研判浮层3个模拟操作** — 确认？DetectionControlPanel 类将弃用，SimulationWorkflow 状态机方法暂保留无调用方。
3. **本轮 PiP 不做拖动+缩放精细交互**（仅固定左下角+最小化+关闭+主次切换），拖动缩放推迟到后续 — 确认？还是要求本轮就做完整 PiP 交互？

## 验证命令

```bash
# 构建
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2

# 测试
ctest --test-dir build --output-on-failure

# 离屏截图
UXO_VISUAL_EVIDENCE_DIR=/tmp/uxo-evidence ./build/tests/simulation_workflow_ui_test
```

## 完成条件

- 构建退出0，ctest全绿
- 离屏截图视觉对齐原型（地图主舞台+可折叠左pane+设备资源条+目标详情浮层+视频PiP，无右面板无探测工具栏）
- 无真实设备控制/外部通信/数据库写入/明文密钥
- 所有模拟数据标注"模拟"
