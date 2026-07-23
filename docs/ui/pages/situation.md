# 态势页面设计

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](design-system.md)
应用壳：[docs/ui/application-shell.md](application-shell.md)

> 本文是态势页面（situation page）的完整设计契约。每个交互控件拥有稳定 `SIT-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据，控件 ID、区域结构和状态规则不得在实现阶段自行变更。所有视觉值取自 `design-system.md`。

CURRENT 来源：
- [`src/MainWindow/LeftPanelWidget.cpp`](../../../src/MainWindow/LeftPanelWidget.cpp)
- [`src/MainWindow/AlertPanel.cpp`](../../../src/MainWindow/AlertPanel.cpp)
- [`src/MainWindow/DetectionControlPanel.cpp`](../../../src/MainWindow/DetectionControlPanel.cpp)
- [`src/MainWindow/BatchOperationBar.cpp`](../../../src/MainWindow/BatchOperationBar.cpp)
- [`src/MainWindow/RightPanelWidget.cpp`](../../../src/MainWindow/RightPanelWidget.cpp)
- [`src/MainWindow/SituationView.cpp`](../../../src/MainWindow/SituationView.cpp)
- [`src/MainWindow/VideoStreamPanel.cpp`](../../../src/MainWindow/VideoStreamPanel.cpp)
- [`src/MainWindow/DeviceStatusPanel.cpp`](../../../src/MainWindow/DeviceStatusPanel.cpp)
- [`src/MainWindow/DecisionSuggestionPanel.cpp`](../../../src/MainWindow/DecisionSuggestionPanel.cpp)
- [`include/MainWindow/VideoStreamPanel.h`](../../../include/MainWindow/VideoStreamPanel.h)
- [`include/Core/Data/Types.h`](../../../include/Core/Data/Types.h)

## 1. 页面概述

态势页面是系统默认页面（导航 `SIT-NAV-01` 默认选中）。它一屏呈现：左侧目标/任务/设备列表，中心视频流与告警/操作日志，右侧三维态势地图、设备状态、决策建议。所有数据来自本地模拟 fixture（`DemoScenarioProvider`），所有操作仅修改内存中的 `SimulationWorkflow`，不连接真实设备、不写入数据库、不执行排爆动作。

页面整体布局由应用壳定义（见 `application-shell.md` 第 2 节）。本文档化页面内部的四个内容区域：

| 区域 | 位置 | 内部组件 | CURRENT 主控件 |
|------|------|----------|----------------|
| A | 左面板 | 搜索栏、状态子标签、三标签表格 | `LeftPanelWidget` |
| B | 中心上 | 视频流面板（模拟占位） | `VideoStreamPanel` |
| C | 中心下 | 告警面板、模拟流程与操作日志、批量操作条 | `AlertPanel` + `DetectionControlPanel` + `BatchOperationBar` |
| D | 右面板 | 三维态势地图、模拟设备状态、模拟决策建议 | `RightPanelWidget`（含 `SituationView`、`DeviceStatusPanel`、`DecisionSuggestionPanel`） |

## 2. 区域 A：左面板

宽 320px 固定，背景 `--color-panel`，外边距 8px，间距 8px。从上到下：搜索栏工具条 -> 状态子标签 -> 三标签页（目标/任务/设备）。

### 2.1 搜索栏工具条

背景 `--color-toolbar`，圆角 `--radius-control`，下边距 8px。内边距 `8px 6px`，间距 8px。包含搜索框、筛选按钮、刷新按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|-----------|---------|------|---------|---------------|------|
| `SIT-LP-SEARCH` | - | QLineEdit | 搜索栏左，弹性宽 | 按文字过滤目标表 | 高 28px；默认 `--color-bg` 背景、`--color-border` 边框；focus 边框 `--color-border-focus`；placeholder `搜索目标...`、`--color-text-disabled` | 输入文本实时过滤目标表（CURRENT `onSearchTextChanged` 遍历所有列做包含匹配，空文本显示全部） | Tab 聚焦，Esc 不清空（CURRENT 无 Esc 绑定） | 同 CURRENT；空文本时显示全部目标 | `LeftPanelWidget.cpp` `m_searchEdit`、`onSearchTextChanged` | 无 |
| `SIT-LP-FILTER` | 筛选 | QPushButton | 搜索框右，50x28 | 触发筛选（CURRENT 占位） | 默认 `--color-toolbar` 背景、`--color-text-secondary` 文本；hover 背景 `--color-border`；字号 `--font-size-caption` | 无效果（CURRENT `onFilterChanged` 空实现） | Tab 聚焦，Enter 触发 | **禁用并标注“占位”**，附 tooltip“筛选功能未实现” | `LeftPanelWidget.cpp` `onFilterChanged` | 无 |
| `SIT-LP-REFRESH` | 刷新 | QPushButton | 筛选右，50x28 | 请求工作流同步最新模拟数据 | 主要按钮变体：`--color-primary` 背景、`--color-text-primary` 文本；hover `--color-primary-hover`；字号 `--font-size-caption` | 发出 `refreshSimulationRequested`，`MainWindow` 重读工作流权威副本并回填左/右面板，不重置选择 | Tab 聚焦，Enter 触发 | 同 CURRENT；按钮显示加载态 200ms 后回填（仅视觉反馈，无网络请求） | `LeftPanelWidget.cpp` `onRefreshTargets` -> `MainWindow::onRefreshSimulationRequested` | 无设备通信，仅内存同步 |

> 注：CURRENT 源码注释提到“类型/威胁”筛选 combo（`m_typeFilterCombo`、`m_threatFilterCombo` 作为成员声明），但 `setupUi` 中并未实例化这两个 combo。本试点按实际渲染处理：不文档化未实例化的控件。如后续实现筛选，需先在本文登记新 `SIT-*` ID。

### 2.2 状态子标签

背景 `--color-toolbar`，圆角 `--radius-control`，下边距 8px。三个等宽按钮，内边距 `4px`，间距 0。每按钮高约 32px（`padding: 8px 2px`），字号 `--font-size-caption`。

默认态：透明背景、`--color-text-secondary` 文本、底部 2px 透明边框。hover：背景 `--color-border`。选中态：`--color-text-primary` 文本、底部 2px `--color-primary` 边框。

| ID | 标签 | 类型 | 位置 | 用途 | 默认值 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|---------|------|---------|---------------|------|
| `SIT-LP-TAB-PENDING` | 待处置任务 N | QPushButton | 状态子标签左，等宽 | 显示待处置任务计数 | `待处置任务 0`，加载后由 `updateStatusTabs` 填充 | CURRENT 无点击连接；原型切换为选中态并过滤任务表（Planned/PendingApproval/Approved） | Tab 聚焦，Enter 触发 | 默认选中；点击切换选中态并过滤任务表显示对应状态 | `LeftPanelWidget.cpp` `m_statusTabPending`、`updateStatusTabs` | 无 |
| `SIT-LP-TAB-EXECUTING` | 处置中任务 N | QPushButton | 中间，等宽 | 显示处置中任务计数 | `处置中任务 0` | 同上，过滤 Executing/Paused | 同上 | 同上 | `LeftPanelWidget.cpp` `m_statusTabExecuting` | 无 |
| `SIT-LP-TAB-COMPLETED` | 已完成任务 N | QPushButton | 右，等宽 | 显示已完成任务计数 | `已完成任务 0` | 同上，过滤 Completed/Failed/Cancelled | 同上 | 同上 | `LeftPanelWidget.cpp` `m_statusTabCompleted` | 无 |

> CURRENT 状态子标签默认 `SIT-LP-TAB-PENDING` 选中（`setProperty("selected", true)`），但无点击槽。原型补齐点击切换与任务表过滤逻辑，过滤为前端行为，不调用后端。

### 2.3 三标签页

`QTabWidget`（`m_tabWidget`），背景 `--color-panel`。标签栏样式见 `design-system.md` 第 5.5 节。三个标签：目标、任务、设备。默认选中“目标”标签。

#### 2.3.0 标签选择器

三个标签选择器为 `QTabBar` 内的标签项，CURRENT 通过 `addTab` 依次添加。每个标签选中后切换 `m_tabWidget` 的可见页面，无信号转发到 `MainWindow`（`LeftPanelWidget` 内部完成切换）。

| ID | 标签 | 位置 | 用途 | 默认态 | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|--------|--------|---------|------|---------|---------------|------|
| `SIT-LP-PAGE-TARGET` | 目标 | 标签栏左，index 0 | 切换到目标表页面 | 选中（`addTab` 顺序首位，默认 currentIndex 0） | 背景 `--color-bg`、主文本色、底部 2px `--color-primary` 边框 | `m_tabWidget` 切到目标表页面，显示 `SIT-LP-TARGET-TABLE` | Tab 聚焦到标签栏，Left/Right 或 Enter 切换 | 同 CURRENT | `LeftPanelWidget.cpp` `m_tabWidget->addTab(m_targetTable, "目标")`（第 189 行） | 无 |
| `SIT-LP-PAGE-MISSION` | 任务 | 标签栏中，index 1 | 切换到任务表页面 | 未选中 | 选中后同上样式 | 切到任务表页面，显示 `SIT-LP-MISSION-TABLE` | 同上 | 同 CURRENT | `LeftPanelWidget.cpp` `m_tabWidget->addTab(m_missionTable, "任务")`（第 190 行） | 无 |
| `SIT-LP-PAGE-DEVICE` | 设备 | 标签栏右，index 2 | 切换到设备表页面 | 未选中 | 选中后同上样式 | 切到设备表页面，显示 `SIT-LP-DEVICE-TABLE` | 同上 | 同 CURRENT | `LeftPanelWidget.cpp` `m_tabWidget->addTab(m_deviceTable, "设备")`（第 191 行） | 无 |

未选中态：背景 `--color-toolbar`、`--color-text-secondary` 文本、无边框。hover（未选中）：背景 `--color-border`、`--color-text-secondary`。disabled 态不适用（三个标签始终可切换）。内边距 `--space-tab-pad-y --space-tab-pad-x`，字号 `--font-size-body`。

#### 2.3.1 目标表 `SIT-LP-TARGET-TABLE`

5 列，单选行模式（`SingleSelection`、`SelectRows`），不可编辑，交替行色。表头背景 `--color-toolbar`、主文本色、内边距 4px。行高 40px。水平滚动条始终关闭。

| 列 | 表头 | 宽度 | 内容 | 文字色 |
|----|------|------|------|--------|
| 0 | （空） | 28px 固定 | 复选框，默认未选中 | - |
| 1 | 类型 | 52px 固定 | `typeName`，按威胁等级着色 | 高=`--color-threat-high`、中=`--color-threat-medium`、低=`--color-threat-low`、未知=`--color-text-disabled` |
| 2 | 置信度 | 48px 固定 | `XX%`（confidence*100，整数） | `--color-text-primary` |
| 3 | 位置 | 72px 固定 | `X:n Y:n`（position.x、position.z 取整） | `--color-text-primary` |
| 4 | 模拟状态 | 拉伸 | `[模拟] 已发现/已确认/处置中/已完成/状态未知` | `--color-text-primary` |

行样式：默认 `--color-panel`；hover `--color-row-hover`；选中 `--color-selection`。复选框样式见 `design-system.md` 第 5.6 节。

| 字段 | 值 |
|------|----|
| ID | `SIT-LP-TARGET-TABLE` |
| 类型 | QTableWidget |
| 位置 | 目标标签页内容区 |
| 用途 | 显示模拟目标列表，单选触发目标选中 |
| 点击结果 | 单击任一单元格 -> 发出 `targetSelected(m_targets[row])`；`MainWindow` 调 `SimulationWorkflow::selectTarget`，刷新右面板决策区与三维高亮 |
| 双击结果 | 发出 `targetDoubleClicked`，CURRENT 复用选择流程（`onTargetDoubleClicked` 调 `onTargetSelected`） |
| 复选框 | CURRENT 仅切换复选状态，无信号连接，不触发批量操作条显示 |
| 键盘 | Tab 聚焦表；Up/Down 移动选中行并触发 `targetSelected`；Space 切换当前行复选框 |
| 原型行为 | 单击选中行并同步右面板与三维视图高亮（同 CURRENT）；复选框仅本地状态，不触发批量条（CURRENT 亦无连接） |
| CURRENT 映射 | `LeftPanelWidget.cpp` `setupTargetList`、`populateTargetList`、`itemClicked`/`itemDoubleClicked` 连接 |
| 安全 | 无设备控制，仅内存选择 |

#### 2.3.2 任务表 `SIT-LP-MISSION-TABLE`

4 列，单选行，不可编辑，交替行色。行高 40px。

| 列 | 表头 | 内容 | 文字色 |
|----|------|------|--------|
| 0 | 优先级 | `P0`/`P1`/`P2` | P0=`--color-threat-high`、P1=`--color-threat-medium`、P2=`--color-threat-low` |
| 1 | 任务编号 | `mission.id` | `--color-text-primary` |
| 2 | 执行设备 | `mission.deviceId` | `--color-text-primary` |
| 3 | 状态 | `规划中/待审批/已批准/执行中/已完成/失败/未知` | `--color-text-primary` |

| 字段 | 值 |
|------|----|
| ID | `SIT-LP-MISSION-TABLE` |
| 类型 | QTableWidget |
| 位置 | 任务标签页内容区 |
| 用途 | 显示模拟任务列表 |
| 点击结果 | 发出 `missionSelected`（CURRENT `MainWindow` 未连接此信号，无后续动作） |
| 键盘 | Tab 聚焦；Up/Down 移动选中 |
| 原型行为 | 单击选中行；CURRENT 无后续效果，原型保持一致，仅在右面板决策区显示该任务详情（调用 `decisionPanel()->setMission`，与 `loadMockData` 中初始任务展示一致） |
| CURRENT 映射 | `LeftPanelWidget.cpp` `setupMissionList`、`populateMissionList`、`itemClicked` 连接 |
| 安全 | 无 |

#### 2.3.3 设备表 `SIT-LP-DEVICE-TABLE`

3 列，单选行，不可编辑，交替行色。行高 40px。

| 列 | 表头 | 内容 | 文字色 |
|----|------|------|--------|
| 0 | 设备名称 | `device.name` | `--color-text-primary` |
| 1 | 状态 | `在线/任务中/离线/故障/未知` | 在线=`--color-status-online`、任务中=`--color-status-busy`、离线=`--color-status-offline`、故障=`--color-status-error`、未知=`--color-text-disabled` |
| 2 | 电量 | `XX%` | >60%=`--color-status-online`、20–60%=`--color-status-busy`、<20%=`--color-status-error` |

| 字段 | 值 |
|------|----|
| ID | `SIT-LP-DEVICE-TABLE` |
| 类型 | QTableWidget |
| 位置 | 设备标签页内容区 |
| 用途 | 显示模拟设备列表（只读） |
| 点击结果 | 发出 `deviceSelected`（CURRENT `MainWindow` 未连接，无后续动作） |
| 键盘 | Tab 聚焦；Up/Down 移动 |
| 原型行为 | 单击选中行；无后续效果，保持只读 |
| CURRENT 映射 | `LeftPanelWidget.cpp` `setupDeviceList`、`populateDeviceList` |
| 安全 | 只读展示，无设备控制 |

## 3. 区域 B：视频流面板（模拟占位）

位于中心区上半（splitter stretch 3）。CURRENT 为 `VideoStreamPanel`，使用 `QStackedWidget` 在网格视图与单格全屏视图间切换。所有视频内容为本地模拟（`QTimer` 每秒刷新 `● REC N\nHH:mm:ss` 占位文字），不接入真实视频流。

### 3.1 容器 `SIT-VSP`

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP` |
| 类型 | VideoStreamPanel（QWidget） |
| 位置 | 中心区 splitter 上半 |
| 用途 | 模拟视频流多分屏显示与全屏切换 |
| 默认状态 | 4 分屏网格，4 路模拟流均可见，控制栏可见 |
| 全屏状态 | 切换为单格视图，控制栏隐藏，退出按钮可见 |
| 加载状态 | 不适用（CURRENT 无加载流程，`setupUi` 同步创建所有控件） |
| 空状态 | 不适用（`m_streamCount` 默认 4，`createVideoCells` 总是创建 4 格） |
| 错误状态 | 不适用（CURRENT 无错误处理路径） |
| 禁用状态 | 不适用（面板始终可交互） |
| 点击结果 | 单元格本身无点击响应（`streamClicked`/`streamDoubleClicked` 信号声明于头文件但 `.cpp` 从未 `emit`） |
| 键盘 | 控制栏按钮可 Tab 聚焦；单元格本身不可聚焦 |
| 原型行为 | 同 CURRENT；仅保留分屏切换与全屏占位，不补齐单元格点击 |
| CURRENT 映射 | `MainWindow.cpp` `m_videoStreamPanel = new VideoStreamPanel`；`VideoStreamPanel.cpp` `setupUi` |
| 安全 | 不接入真实视频流，仅本地模拟占位文字 |

CURRENT 模拟流名称与状态（`createVideoCells` 硬编码）：`UAV-1 侦察无人机`（在线\|信号 95%）、`UAV-2 排爆无人机`（在线\|信号 88%）、`Robot-1 排爆机器人`（在线\|信号 92%）、`GPR-1 探地雷达`（离线）。

### 3.2 控制栏

底部 32px 固定高，背景 `--color-toolbar`，顶部 1px `--color-border` 边框，内边距 `8px 2px`，间距 4px。从左到右：“分屏:”标签 + 4 个分屏按钮 + 弹性留白 + 全屏按钮。控制栏在全屏模式下隐藏。

#### 3.2.1 分屏标签（只读）

`分屏:`，辅助文本色，`--font-size-caption`。无 ID（只读标签）。

#### 3.2.2 分屏按钮 `SIT-VSP-LAYOUT-1` 至 `SIT-VSP-LAYOUT-4`

四个 `QPushButton`，各 28x24px，字号 `--font-size-caption`，文本为 `1`/`2`/`3`/`4`。`property("layoutCount")` 存对应分屏数。

| ID | 标签 | 位置 | 默认态 | hover | active 态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|--------|-------|-----------|---------|------|---------|---------------|------|
| `SIT-VSP-LAYOUT-1` | 1 | 控制栏左1 | 背景 `--color-bg`、主文本色、1px `--color-border` 边框、圆角 3px | 背景 `--color-border` | 背景 `--color-primary`、白字、1px 主色边框（`active=true` 属性） | 调 `onLayoutButtonClicked(1)`：设 `m_currentLayout=1`，若在全屏则先退出全屏，`updateLayout` 切换为单格（仅显示 cell 0） | Tab 聚焦，Enter 触发 | 同 CURRENT | `VideoStreamPanel.cpp` `m_layoutButtons[0]`、`onLayoutButtonClicked`、`updateLayout` | 无 |
| `SIT-VSP-LAYOUT-2` | 2 | 控制栏左2 | 同上 | 同上 | 同上 | 调 `onLayoutButtonClicked(2)`：切换为 1x2（cell 0、1 并排） | 同上 | 同 CURRENT | `m_layoutButtons[1]` | 无 |
| `SIT-VSP-LAYOUT-3` | 3 | 控制栏左3 | 同上 | 同上 | 同上 | 调 `onLayoutButtonClicked(3)`：切换为 3 格（cell 0 跨 1x2 顶部，cell 1、2 居底部） | 同上 | 同 CURRENT | `m_layoutButtons[2]` | 无 |
| `SIT-VSP-LAYOUT-4` | 4 | 控制栏左4 | 同上 | 同上 | 默认 active（`m_currentLayout=4` 初始值） | 调 `onLayoutButtonClicked(4)`：切换为 2x2 网格（4 格全显） | 同上 | 同 CURRENT | `m_layoutButtons[3]` | 无 |

active 态由 `updateLayout` 末段同步：遍历 `m_layoutButtons`，`layoutCount == m_currentLayout` 者设 `active=true` 并 `unpolish`/`polish` 刷新。disabled 态不适用（按钮始终可点击）。

#### 3.2.3 全屏按钮 `SIT-VSP-FULLSCREEN`

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP-FULLSCREEN` |
| 标签 | 全屏 |
| 类型 | QPushButton |
| 位置 | 控制栏右，48x24px |
| 用途 | 进入 cell 0 全屏视图 |
| 默认态 | 背景 `--color-bg`、主文本色、1px `--color-border` 边框、圆角 3px、`--font-size-caption` |
| hover | 背景 `--color-border` |
| active | 不适用（无 active 属性） |
| disabled | 不适用（始终可点击） |
| 点击结果 | 若 `m_fullscreenIndex < 0` 且 cells 非空，调 `setFullscreenIndex(0)`：cell 0 重设父为 `m_singleContainer`，退出按钮显示，`m_stackWidget` 切到单格视图，控制栏隐藏，`emit fullscreenRequested(0)` |
| 键盘 | Tab 聚焦，Enter 触发 |
| 原型行为 | 同 CURRENT；进入全屏后仅显示 cell 0 与退出按钮 |
| CURRENT 映射 | `VideoStreamPanel.cpp` `fullscreenBtn`（setupUi 第 116-129 行）、`setFullscreenIndex` |
| 安全 | 无 |

#### 3.2.4 退出全屏按钮 `SIT-VSP-EXIT`

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP-EXIT` |
| 标签 | 退出全屏 |
| 类型 | QPushButton |
| 位置 | 全屏单格视图右上角，80x28px |
| 用途 | 退出全屏，回到网格视图 |
| 默认态 | 隐藏（`m_fullscreenIndex < 0` 时）；显示时背景 `rgba(0,0,0,150)`、白字、无边框、圆角 4px、`--font-size-caption` |
| hover | 背景 `rgba(0,0,0,200)` |
| active | 不适用 |
| disabled | 不适用 |
| 显示条件 | `setFullscreenIndex(index >= 0)` 时 `show()`；退出时 `hide()` |
| 点击结果 | 调 `setFullscreenIndex(-1)`：所有 cells 重设父回 `m_gridContainer`，本按钮隐藏，`m_stackWidget` 切回网格视图，控制栏显示，`updateLayout` 重排 |
| 键盘 | Tab 聚焦（显示时），Enter 触发 |
| 原型行为 | 同 CURRENT |
| CURRENT 映射 | `VideoStreamPanel.cpp` `m_fullscreenExitBtn`（setupUi 第 60-70 行）、`setFullscreenIndex` |
| 安全 | 无 |

### 3.3 视频单元格 `SIT-VSP-CELL-0` 至 `SIT-VSP-CELL-3`

4 个单元格由 `createVideoCells` 创建，默认全部加入 `m_gridContainer`。每个单元格为 QWidget，背景 `#1A1A1A`（CURRENT 字面量），1px `--color-border` 边框，圆角 2px，内边距 0，间距 0。

单元格内部从上到下：视频占位标签（弹性）+ 信息栏（28px 固定高）。

#### 3.3.1 视频占位标签（只读）

`QLabel`，背景 `#0D0D0D`（CURRENT 字面量），文本 `#666`（CURRENT 字面量），`--font-size-title`（24px），居中对齐，最小高 60px。`QTimer` 每秒刷新文本为 `● REC {index+1}\n{HH:mm:ss}`。无 ID（只读占位）。

#### 3.3.2 信息栏（28px 固定高）

背景 `rgba(0,0,0,180)`（CURRENT 字面量），内边距 `8px 2px`，间距 8px。从左到右：流名称标签 + 弹性留白 + 状态标签 + 全屏按钮。

**流名称标签（只读）**：主文本色，`--font-size-caption`（11px），加粗。值取自 `createVideoCells` 硬编码列表。无 ID。

**状态标签（只读）**：在线=`--color-status-online`、离线=`--color-text-disabled`，10px。值取自硬编码列表。无 ID。

#### 3.3.3 单元格全屏按钮 `SIT-VSP-CELL-{0-3}-FULLSCREEN`

每个信息栏右侧一个 `QPushButton`，文本 `全`，tooltip `全屏查看`，20x20px。

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP-CELL-0-FULLSCREEN` 至 `SIT-VSP-CELL-3-FULLSCREEN` |
| 标签 | 全 |
| 类型 | QPushButton |
| 位置 | 对应单元格信息栏右侧，20x20px |
| 用途 | 进入该单元格全屏视图 |
| 默认态 | 透明背景、`#AAA` 文本、无边框、14px |
| hover | 白字 |
| active | 不适用 |
| disabled | 不适用（始终可点击） |
| 点击结果 | 调 `setFullscreenIndex(i)`：该 cell 重设父为 `m_singleContainer`，退出按钮显示，`m_stackWidget` 切到单格视图，控制栏隐藏，`emit fullscreenRequested(i)` |
| 键盘 | Tab 聚焦，Enter 触发 |
| 原型行为 | 同 CURRENT |
| CURRENT 映射 | `VideoStreamPanel.cpp` `cell.fullscreenBtn`（createVideoCells 第 184-193 行）、`setFullscreenIndex` |
| 安全 | 无 |

### 3.4 分屏布局规则

`updateLayout` 按 `m_currentLayout` 重排 `m_gridContainer` 内可见单元格：

| 分屏数 | 布局 | 可见单元格 |
|--------|------|-----------|
| 1 | 单格全屏（网格内） | cell 0 |
| 2 | 1x2 水平 | cell 0、1 |
| 3 | cell 0 跨 1x2 顶部，cell 1、2 居底部 | cell 0、1、2 |
| 4 | 2x2 网格 | cell 0、1、2、3 |

切分屏时若处于全屏状态，`onLayoutButtonClicked` 先调 `setFullscreenIndex(-1)` 退出全屏，再 `updateLayout`。

### 3.5 信号说明

`VideoStreamPanel.h` 声明三个信号：

| 信号 | CURRENT 是否 emit | 说明 |
|------|-------------------|------|
| `streamClicked(int)` | **从不 emit** | 头文件声明，`.cpp` 无任何 `emit streamClicked` 调用 |
| `streamDoubleClicked(int)` | **从不 emit** | 同上 |
| `fullscreenRequested(int)` | 是 | `setFullscreenIndex(index >= 0)` 时 emit |

`MainWindow` 未连接任何 `VideoStreamPanel` 信号（`createConnections` 无相关 `connect`）。原型保持一致，不补齐单元格点击联动。

## 4. 区域 C：信息区

位于中心区下半（splitter stretch 2）。信息面板头（28px，背景 `--color-toolbar`，标题“信息面板”）下方为水平 splitter：告警面板（左，stretch 1）+ 模拟流程与操作日志（右，stretch 1），底部为隐藏的批量操作条（48px）。

### 4.1 告警面板

背景 `--color-panel`，外边距 8px，间距 4px。头部 28px：标题“告警信息”+ 计数徽章（20x20，`--color-danger` 背景、白字、10px 加粗、圆角 10px）+ 弹性留白。

告警条目：高 36px，背景 `--color-toolbar`，圆角 3px，hover 背景 `#363636`，鼠标手型。内边距 `8px 4px`，间距 6px。结构：等级圆点（`●`，16px 宽，按等级着色，10px）+ 时间（40px 宽，`--color-text-disabled`，11px，格式 `HH:mm`）+ 消息（等级色，11px，不换行）。

等级颜色：Critical=`--color-danger`、Error=`--color-threat-high`、Warning=`--color-threat-medium`、Info/default=`--color-text-secondary`。

最多显示 5 条；超出显示“还有 N 条告警...”（`--color-text-disabled`，11px，居中）。无告警显示“暂无告警”（`--color-text-disabled`，12px，居中，内边距 16px）。

| 字段 | 值 |
|------|----|
| ID | `SIT-ALERT-LIST`（列表容器）；计数徽章 `SIT-ALERT-COUNT` |
| 类型 | QWidget 容器（只读） |
| 位置 | 信息区水平 splitter 左半 |
| 用途 | 展示模拟告警，最多 5 条 |
| 默认值 | 加载后含 3 条模拟告警（`loadMockData` 注入） |
| 点击结果 | CURRENT 注释中标注告警条目点击未实现（`mousePressEvent` 为 protected，无法直接 connect）；原型保持不可点击，仅展示 |
| 键盘 | 不可聚焦 |
| 原型行为 | 只读展示模拟告警；计数徽章实时反映告警总数；超出 5 条显示“还有 N 条告警...”；无告警显示“暂无告警” |
| 五态 | 正常：列表展示；加载：骨架条；空：`暂无告警`；错误：边框 `--color-status-error` + `告警加载失败`；禁用：不适用（始终只读） |
| CURRENT 映射 | `AlertPanel.cpp` `setupUi`、`refreshList`、`addAlert` |
| 安全 | 只读，无操作入口 |

### 4.2 模拟流程与操作日志

背景 `--color-panel`，外边距 8px，间距 8px。头部 28px：标题“模拟流程与操作日志”。状态行 40px：目标标签 + 状态标签（均 `--font-size-caption`）。操作行 32px：三个按钮（68x32，间距 8px）。操作日志：`QTextEdit` 只读，最小高 72px，背景 `--color-bg`，1px `--color-border` 边框，圆角 4px，字号 10px，内边距 4px。

#### 4.2.1 状态行

| ID | 默认值 | 用途 | 样式 | CURRENT 映射 |
|----|--------|------|------|---------------|
| `SIT-DC-TARGET` | `模拟目标：未选择` | 显示当前选中目标 ID | 主文本色，`--font-size-caption` | `DetectionControlPanel.cpp` `m_targetLabel`、`showNoSelection`、`setSelectedTarget` |
| `SIT-DC-STATUS` | `模拟状态：未选择` | 显示当前目标模拟状态 | 辅助文本色，`--font-size-caption` | `DetectionControlPanel.cpp` `m_statusLabel` |

选中目标后：`SIT-DC-TARGET` 显示 `模拟目标：{targetId}`；`SIT-DC-STATUS` 显示 `模拟状态：{simulationStatusText}`（已发现/已确认/处置中/已完成/状态未知）。

#### 4.2.2 操作按钮

三个按钮统一 68x32px，圆角 4px，字号 `--font-size-caption`，主要按钮变体（`--color-primary` 背景、`--color-text-primary` 文本、hover `--color-primary-hover`）。禁用态：`--color-border` 背景、`--color-text-disabled` 文本。

按钮可用性由当前目标状态决定（`updateActionAvailability`）：

| ID | 标签 | 启用条件 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|----------|---------|------|---------|---------------|------|
| `SIT-DC-CONFIRM` | 模拟确认 | 目标状态 = `Detected` | 发出 `confirmSimulationRequested` -> `MainWindow::onConfirmSimulationRequested` -> `requestSelectedTargetStatus(Confirmed)`。工作流将目标状态改为 `Confirmed`，更新左表状态列与右面板决策区 | Tab 聚焦，Enter 触发（启用时） | 同 CURRENT；按钮按下后短暂禁用 200ms 显示处理中，再回填状态 | `DetectionControlPanel.cpp` `m_confirmButton`、`confirmSimulationRequested` 信号；`MainWindow.cpp` `onConfirmSimulationRequested` | 仅修改内存模拟状态，无设备控制 |
| `SIT-DC-START` | 模拟处置 | 目标状态 = `Confirmed` | 发出 `startSimulationDisposalRequested` -> 状态改为 `Disposing` | 同上 | 同上 | `DetectionControlPanel.cpp` `m_startButton`、`startSimulationDisposalRequested`；`MainWindow.cpp` `onStartSimulationDisposalRequested` | 同上 |
| `SIT-DC-COMPLETE` | 模拟完成 | 目标状态 = `Disposing` | 发出 `completeSimulationDisposalRequested` -> 状态改为 `Disposed` | 同上 | 同上 | `DetectionControlPanel.cpp` `m_completeButton`、`completeSimulationDisposalRequested`；`MainWindow.cpp` `onCompleteSimulationDisposalRequested` | 同上 |

按钮按下后若工作流拒绝（如状态不匹配），`MainWindow` 仅刷新日志，按钮状态保持不变。

#### 4.2.3 操作日志 `SIT-DC-LOG`

只读 `QTextEdit`，最小高 72px，弹性填充剩余空间。背景 `--color-bg`，1px `--color-border` 边框，圆角 4px，字号 10px，内边距 4px，辅助文本色。

| 字段 | 值 |
|------|----|
| ID | `SIT-DC-LOG` |
| 类型 | QTextEdit（只读） |
| 位置 | 操作日志区，按钮下方 |
| 用途 | 显示模拟操作历史记录 |
| 默认值 | `暂无模拟操作记录（重启后清空）` |
| 内容格式 | 每行 `[HH:mm:ss] 消息`，按 `sequence` 稳定排序 |
| 点击结果 | 无（只读） |
| 键盘 | 只读，不可编辑；可滚动 |
| 五态 | 正常：日志列表；空：`暂无模拟操作记录（重启后清空）`；加载：骨架；错误：不适用（日志为本地内存）；禁用：不适用 |
| 原型行为 | 同 CURRENT；操作按钮触发后追加日志条目 |
| CURRENT 映射 | `DetectionControlPanel.cpp` `m_operationLog`、`setOperationLog` |
| 安全 | 本地内存，重启清空，无持久化 |

### 4.3 批量操作条 `SIT-BOB`

默认隐藏（`hide()` 调用）。高 48px，背景 `#333333`（CURRENT 字面量），顶部 1px `--color-border` 边框。内边距 `12px 6px`，间距 12px。结构：计数标签（`已选择: 0`，主文本色，12px）+ 弹性留白 + 分配任务按钮 + 标记忽略按钮。

CURRENT 中 `setSelectedCount` 是唯一显示入口，但目标表复选框无信号连接到 `BatchOperationBar`，因此实际上始终隐藏。原型保持此行为。

| 字段 | 值 |
|------|----|
| ID | `SIT-BOB`（容器）；`SIT-BOB-COUNT`（计数）；`SIT-BOB-ASSIGN`（分配任务）；`SIT-BOB-IGNORE`（标记忽略） |
| 类型 | QWidget 容器 + 2 QPushButton |
| 位置 | 信息区底部 |
| 用途 | 批量操作（CURRENT 未启用） |
| 显示条件 | `setSelectedCount > 0` 时显示，否则隐藏 |
| 按钮 | 分配任务：主要按钮变体，32px 高，内边距 `4px 16px`；标记忽略：透明背景、`--color-text-secondary` 文本、1px `--color-border` 边框 |
| 点击结果 | CURRENT 发出 `assignTaskRequested`/`markIgnoreRequested` 信号，但 `MainWindow` 未连接，无后续 |
| 键盘 | 按钮可聚焦，Enter 触发 |
| 原型行为 | 保持隐藏；如未来启用目标表复选框连接，需先在本文登记新 ID 与信号路径 |
| CURRENT 映射 | `BatchOperationBar.cpp` 全文 |
| 安全 | 无实际效果，信号未连接 |

## 5. 区域 D：右面板

宽 360–420px（CURRENT `setMinimumWidth(360)`/`setMaximumWidth(420)`），背景 `--color-panel`。垂直 splitter 三段，stretch 5/2/3，决策区最小高 280px。

### 5.1 三维态势地图

地图头 32px：背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `8px 0`。标题“三维态势地图”（主文本色、`--font-size-body`、加粗）+ 弹性留白 + 全屏按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 样式 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|------|---------|------|---------|---------------|------|
| `SIT-RP-FULLSCREEN` | 全 | QPushButton | 地图头右，24x24 | 全屏查看（CURRENT 占位） | 透明背景、`--color-text-secondary` 文本、16px；hover 白字 | 无连接（CURRENT 无槽） | Tab 聚焦，Enter 触发 | **禁用并标注“占位”**，附 tooltip“全屏功能未实现” | `RightPanelWidget.cpp` `mapFullscreenBtn` | 无 |
| `SIT-RP-MAP` | - | SituationView（Qt3DWindow） | 地图头下方，弹性 | 三维机场场景与目标标记显示 | 背景天空色 `#87CEEB`（CURRENT `QForwardRenderer::setClearColor`）；容器背景 `--color-bg` | 鼠标拖拽旋转相机（`QOrbitCameraController`）；CURRENT 3D 标记**无拾取路径**：`targetClicked`/`targetDoubleClicked` 信号在 `SituationView.h` 声明、`RightPanelWidget` 转发、`MainWindow` 连接为 `qDebug`，但 `SituationView.cpp` 从未 `emit`，点击标记无响应 | - | 同 CURRENT：3D 标记仅渲染，不可交互；原型如需点击选中左表目标，属 HTML-only 增量，CURRENT 缺失 | `SituationView.h` 第 46-47 行信号声明；`RightPanelWidget.cpp` 第 108-109 行转发；`MainWindow.cpp` 第 270-273 行连接（仅 `qDebug`）；`SituationView.cpp` 无 `emit` | 仅本地相机操作，无设备控制 |

目标标记为 `QCuboidMesh`，缩放 3.0。`TargetMarkerEntity` 构造时按 `target.threatLevel` 着色（高=`--color-threat-high`、中=`--color-threat-medium`、低=`--color-threat-low`、未知=`--color-text-disabled`），高亮时 specular 设为白色、shininess 128。**CURRENT 注意**：`SituationView::addTargetMarker`（`SituationView.cpp` 第 261-280 行）在创建 `TargetInfo` 时硬编码 `target.threatLevel = Core::ThreatLevel::Medium`，因此所有标记实际渲染为橙色，fixture 中的目标威胁等级不会传递到 3D 颜色。原型如需显示 fixture 真实威胁色，属 HTML-only 增量，CURRENT 缺失。

#### 5.1.1 三维视图工具栏

CURRENT 在 `SituationView` 右侧叠加一个 60px 宽的竖直工具栏，背景 `rgba(30,30,30,200)`，左边框 1px `#3D3D3D`。从上到下：标题“视角”+ 俯/侧/3D/复位 四按钮 + 缩放标签 + 位置标签 + 弹性留白。

| ID | 标签 | 尺寸 | 用途 | 样式 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|---------|------|---------|---------------|------|
| `SIT-RP-TOP` | 俯 | 40x28 | 切换俯视图 | 背景 `#0078D7`（CURRENT 蓝色字面量）、白字、12px；hover `#1984D8`；pressed `#005A9E` | 相机移至 `(target.x, 1500, target.z)`，viewCenter 设为 target | Tab 聚焦，Enter 触发 | 同 CURRENT | `SituationView.cpp` `btnTop`、`setCameraView("top")` | 仅本地相机 |
| `SIT-RP-SIDE` | 侧 | 40x28 | 切换侧视图 | 背景 `#4A4A4A`、`#DDDDDD` 文本、1px `#5A5A5A` 边框；hover `#5A5A5A` | 相机移至 `(100, 200, target.z)` | 同上 | 同 CURRENT | `SituationView.cpp` `btnSide`、`setCameraView("side")` | 同上 |
| `SIT-RP-3D` | 3D | 40x28 | 切换 3D 视角 | 同侧视样式 | 相机移至 `(target.x+500, 500, target.z+700)` | 同上 | 同 CURRENT | `SituationView.cpp` `btn3D`、`setCameraView("3d")` | 同上 |
| `SIT-RP-RESET` | 复位 | 40x28 | 复位相机到默认 | 背景 `#D9534F`（CURRENT 红色字面量）、白字、11px；hover `#E74C3C` | 调 `m_sceneFactory->setupCamera` 复位 | 同上 | 同 CURRENT；与工具栏 `SIT-TB-RESET` 等价 | `SituationView.cpp` `btnReset`、`resetCameraView` | 同上 |
| `SIT-RP-ZOOM` | 缩放: 100% | QLabel | 只读，20px 高 | `--color-text-disabled`，10px | 无 | 不可聚焦 | 同 CURRENT；相机移动时更新 | `SituationView.cpp` `m_zoomLabel`、`updateZoomLabel` | 无 |
| `SIT-RP-POS` | X:n Y:n Z:n | QLabel | 只读，40px 高 | `#666666`（CURRENT 字面量），9px，自动换行 | 无 | 不可聚焦 | 同 CURRENT；显示相机坐标 | `SituationView.cpp` `m_positionLabel` | 无 |

### 5.2 模拟设备状态（只读）

背景 `--color-panel`，外边距 8px，间距 6px。头部 28px：标题“模拟设备状态（只读）”（主文本色、`--font-size-title`、加粗）。下方为设备卡片列表，间距 4px。

每张卡片高 40px，背景 `--color-toolbar`，圆角 4px，hover `#363636`，鼠标手型。内边距 `8px 4px`，间距 8px。结构：状态圆点（`●`，14px 宽，按状态着色，10px）+ 名称（80px 宽，主文本色，12px）+ 状态文本（按状态着色，11px）+ 弹性留白 + 电量标签（`⚡XX%`，40px 宽，按电量着色，11px）。

状态文本与颜色：在线/Idle=`模拟在线`/`--color-status-online`；Busy=`模拟任务中`/`--color-status-busy`；Offline=`模拟离线`/`--color-status-offline`；Error=`模拟故障`/`--color-status-error`；未知=`模拟未知`/`--color-text-disabled`。

电量颜色：>60% `--color-status-online`；20–60% `--color-status-busy`；<20% `--color-status-error`。

| 字段 | 值 |
|------|----|
| ID | `SIT-DS-LIST`（列表容器）；每张卡片 `SIT-DS-CARD-{deviceId}` |
| 类型 | QWidget 容器（只读） |
| 位置 | 右面板中段 |
| 用途 | 展示模拟设备状态（只读） |
| 默认值 | 加载后含 `loadMockData` 注入的设备列表 |
| 点击结果 | CURRENT 注释标注卡片点击未实现（`mousePressEvent` 为 protected）；原型保持不可点击 |
| 键盘 | 不可聚焦 |
| 五态 | 正常：卡片列表；加载：骨架卡；空：`暂无设备`；错误：`--color-status-error` 边框 + `设备状态加载失败`；禁用：不适用 |
| 原型行为 | 同 CURRENT；只读展示模拟设备状态与电量 |
| CURRENT 映射 | `DeviceStatusPanel.cpp` `setupUi`、`refreshList`、`updateDeviceStatus` |
| 安全 | 只读，无设备控制入口 |

### 5.3 模拟决策建议

背景 `--color-panel`，外边距 8px，间距 8px。头部 28px：标题“模拟决策建议”（主文本色、`--font-size-title`、加粗）。内容卡片背景 `--color-toolbar`，圆角 4px，内边距 12px，间距 10px。

内容自上而下：
1. 模拟状态标签：`[模拟] 目标状态：{状态文本}`，`--color-text-secondary`，`--font-size-caption`。
2. 方案标题：`建议方案`，`--color-text-secondary`，`--font-size-caption`。
3. 方案值：`聚能引爆/转移处置/人工排除/待评估`，`--color-primary`，`--font-size-body`，加粗。
4. 风险行：`风险等级:` + `● {高/中/低/未知}`，按风险着色（高=`--color-threat-high`、中=`--color-threat-medium`、低=`--color-threat-low`、未知=`--color-text-disabled`），12px，加粗。
5. 置信度行：`置信度:` + 进度条（8px 高，chunk `--color-primary`，背景 `--color-border`，圆角 4px，无文字）+ 百分比标签（36px 宽，主文本色，12px）。
6. 详情标签：多行文本，`--color-text-secondary`，`--font-size-caption`，行高 1.4，自动换行。

方案与风险由目标威胁等级决定（`setTarget`）：High -> 聚能引爆/高；Medium -> 转移处置/中；Low -> 人工排除/低；未知 -> 待评估/未知。

详情标签在选中任务时显示任务信息：
```
模拟任务编号: {mission.id}
任务状态: {规划中/待审批/已批准/执行中/已完成/未知}
执行单位: {mission.assignee}
指派设备: {mission.deviceId}
（模拟数据，不连接真实设备）
```
未选择时显示：`[模拟模式] 请选择目标以获取决策建议。\n（当前数据为模拟，不连接真实设备）`

| 字段 | 值 |
|------|----|
| ID | `SIT-DEC-PANEL`（容器）；`SIT-DEC-STATUS`（模拟状态标签）；`SIT-DEC-METHOD`（方案值）；`SIT-DEC-RISK`（风险值）；`SIT-DEC-CONF-BAR`（置信度条）；`SIT-DEC-CONF-TEXT`（置信度文字）；`SIT-DEC-DETAIL`（详情标签） |
| 类型 | QWidget 容器（只读） |
| 位置 | 右面板下段，最小高 280px |
| 用途 | 展示模拟决策建议（只读，无操作入口） |
| 默认值 | 未选择目标时显示空状态；加载后默认显示首个任务的详情 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：决策信息；空：`请选择目标以获取决策建议`；加载：骨架；错误：不适用（数据为本地模拟）；禁用：不适用 |
| 原型行为 | 同 CURRENT；选中左表目标后实时更新方案、风险、置信度与详情；未选择时显示空状态 |
| CURRENT 映射 | `DecisionSuggestionPanel.cpp` `setupUi`、`setTarget`、`setMission`、`setSuggestion`、`clear` |
| 安全 | 只读，无操作入口，无设备控制 |

## 6. 状态规则汇总

每个区域必须定义五态。下表汇总各区域的五态实现：

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 左面板目标表 | 列表展示 | 骨架行 | `暂无目标`（CURRENT 无空态，原型补齐） | 边框 `--color-status-error` + `目标加载失败` | 不适用（始终可交互） |
| 左面板任务表 | 列表展示 | 骨架行 | `暂无任务`（原型补齐） | `任务加载失败` | 不适用 |
| 左面板设备表 | 列表展示 | 骨架行 | `暂无设备`（原型补齐） | `设备加载失败` | 不适用 |
| 视频流面板 | 4 分屏模拟占位 | 不适用（`setupUi` 同步创建） | 不适用（`m_streamCount=4` 固定） | 不适用（无错误路径） | 不适用（始终可交互） |
| 告警面板 | 列表展示 | 骨架条 | `暂无告警`（CURRENT 已实现） | `告警加载失败` | 不适用（只读） |
| 操作日志 | 日志列表 | 不适用（本地内存） | `暂无模拟操作记录（重启后清空）`（CURRENT 已实现） | 不适用 | 不适用 |
| 批量操作条 | 隐藏（默认） | 不适用 | 不适用 | 不适用 | 不适用 |
| 三维地图 | 场景渲染 | 骨架 + `场景加载中…` | `暂无场景数据` | `场景加载失败` | 不适用 |
| 设备状态面板 | 卡片列表 | 骨架卡 | `暂无设备`（原型补齐） | `设备状态加载失败` | 不适用（只读） |
| 决策建议面板 | 决策信息 | 骨架 | `请选择目标以获取决策建议`（CURRENT 已实现） | 不适用 | 不适用（只读） |

CURRENT 已实现的空态：告警面板、操作日志、决策建议。其余空态在原型中补齐，文字必须同时给出（颜色不作为唯一信息）。

## 7. 交互流程

### 7.1 目标选择流程

1. 用户在 `SIT-LP-TARGET-TABLE` 单击目标行。
2. `LeftPanelWidget` 发出 `targetSelected`。
3. `MainWindow::onTargetSelected` 调 `SimulationWorkflow::selectTarget(targetId)`。
4. 工作流选中目标后，`refreshSelectedTarget` 同步：
   - 左表选中行高亮（CURRENT 通过表选择，未显式同步；原型补齐）。
   - 右面板 `setTarget`：决策区更新方案/风险/置信度/状态；三维视图 `highlightTarget` 高亮标记、`focusOnTarget` 相机聚焦。
   - 探测控制区 `setSelectedTarget`：更新 `SIT-DC-TARGET`、`SIT-DC-STATUS` 与按钮可用性。

### 7.2 模拟处置流程

目标状态机：`Detected -> Confirmed -> Disposing -> Disposed`。每步对应一个按钮：

1. 目标 `Detected`：`SIT-DC-CONFIRM` 启用，`SIT-DC-START`/`SIT-DC-COMPLETE` 禁用。
2. 点击 `SIT-DC-CONFIRM`：状态变为 `Confirmed`，`SIT-DC-START` 启用。
3. 点击 `SIT-DC-START`：状态变为 `Disposing`，`SIT-DC-COMPLETE` 启用。
4. 点击 `SIT-DC-COMPLETE`：状态变为 `Disposed`，三个按钮全部禁用。

每步操作在 `SIT-DC-LOG` 追加一条 `[HH:mm:ss] 消息`，并更新左表第 4 列模拟状态文字与右面板决策区状态标签。所有变更仅影响内存 `SimulationWorkflow`，重启后清空。

### 7.3 刷新流程

点击 `SIT-LP-REFRESH`：
1. 发出 `refreshSimulationRequested`。
2. `MainWindow::onRefreshSimulationRequested` 重读工作流权威副本，回填左面板三表、右面板设备、决策区。
3. 不重置选择、不重载场景、不调用 `loadMockData`。
4. 操作日志保持不变。

## 8. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280x720 | 左面板 320px 固定，目标表前 4 列共 200px + 状态列拉伸；右面板 360px（360–420px 弹性范围的最小值，CURRENT `setMinimumWidth(360)`），决策区最小 280px（`decisionSection->setMinimumHeight(280)`）；信息区告警与探测控制按 1:1 并排，探测控制三个按钮（3x68px + 间距）需在半宽内完整显示；批量操作条隐藏不占位 |
| 1920x1080 | 默认尺寸；中心视频流与信息区按 3:2 分配；右面板三段按 5:2:3；所有控件按 token 展示 |
| 3840x2160 | 固定区域不变；中心区与右面板弹性区按比例放大；右面板可至 420px；字号与控件尺寸保持固定 px |

CURRENT 在 1280x720 下决策面板末两行（指派设备、模拟声明）可能被截断。`RightPanelWidget` 已通过 `setMinimumHeight(280)` 与 stretch 5/2/3 调整缓解。原型需验证末两行完整可见。

## 9. 安全清单

本页面所有控件必须遵守以下安全约束：

| 控件 | 约束 |
|------|------|
| 所有模拟操作按钮（`SIT-DC-CONFIRM`/`SIT-DC-START`/`SIT-DC-COMPLETE`） | 仅修改内存 `SimulationWorkflow`，不发送设备命令、不执行排爆动作 |
| 设备状态面板 `SIT-DS-LIST` | 只读，无控制入口 |
| 决策建议面板 `SIT-DEC-PANEL` | 只读，无操作入口 |
| 三维视图 `SIT-RP-MAP` 与视角按钮 | 仅本地相机操作，不控制真实设备视角 |
| 全屏按钮 `SIT-RP-FULLSCREEN` | 禁用占位 |
| 刷新按钮 `SIT-LP-REFRESH` | 仅内存同步，无网络请求 |
| 批量操作条 | 信号未连接，无实际效果 |
| 紧急停止 `SIT-SB-EMERGENCY` | 见 `application-shell.md` 第 6.3 节，禁用占位 |
| 设备菜单/工具栏设备控制台 | 省略（见 `application-shell.md` 第 7 节） |

所有模拟操作与结果必须带“模拟”或“演示”字样。涉及设备状态、决策建议、告警的内容若来自本地 fixture，必须在区域标题或控件旁标注“模拟数据（只读）”。本页面不实现登录、角色切换、外部通信、持久化、UXR、MOS。

## 10. CURRENT 映射总结

| 页面元素 | CURRENT 源码位置 |
|---------|------------------|
| 左面板构造 | `LeftPanelWidget.cpp` `setupUi`（第 65-194 行） |
| 目标表填充 | `LeftPanelWidget.cpp` `populateTargetList`（第 314-348 行） |
| 任务表填充 | `LeftPanelWidget.cpp` `populateMissionList`、`updateStatusTabs`（第 350-417 行） |
| 设备表填充 | `LeftPanelWidget.cpp` `populateDeviceList`（第 419-454 行） |
| 搜索过滤 | `LeftPanelWidget.cpp` `onSearchTextChanged`（第 501-514 行） |
| 刷新请求 | `LeftPanelWidget.cpp` `onRefreshTargets`（第 492-495 行） |
| 告警面板 | `AlertPanel.cpp` 全文 |
| 探测控制面板 | `DetectionControlPanel.cpp` 全文 |
| 批量操作条 | `BatchOperationBar.cpp` 全文 |
| 右面板构造 | `RightPanelWidget.cpp` `setupUi`（第 26-110 行） |
| 三维视图 | `SituationView.cpp` 全文 |
| 设备状态面板 | `DeviceStatusPanel.cpp` 全文 |
| 决策建议面板 | `DecisionSuggestionPanel.cpp` 全文 |
| 模拟数据注入 | `MainWindow.cpp` `loadMockData`（第 290-347 行） |
| 目标选择槽 | `MainWindow.cpp` `onTargetSelected`（第 354-364 行） |
| 模拟处置槽 | `MainWindow.cpp` `requestSelectedTargetStatus`（第 398-412 行） |
| 数据类型枚举 | `Types.h`（TargetType/ThreatLevel/TargetStatus/MissionStatus/DeviceStatus/AlarmLevel） |

## 11. SIT-* ID 索引

下表列出本文档化的全部 `SIT-*` ID，供 HTML 原型与 Playwright 测试对齐：

| ID | 控件 | 区域 |
|----|------|------|
| `SIT-LP-SEARCH` | 搜索框 | 左面板 |
| `SIT-LP-FILTER` | 筛选按钮（禁用占位） | 左面板 |
| `SIT-LP-REFRESH` | 刷新按钮 | 左面板 |
| `SIT-LP-TAB-PENDING` | 待处置任务子标签 | 左面板 |
| `SIT-LP-TAB-EXECUTING` | 处置中任务子标签 | 左面板 |
| `SIT-LP-TAB-COMPLETED` | 已完成任务子标签 | 左面板 |
| `SIT-LP-PAGE-TARGET` | 目标标签选择器 | 左面板 |
| `SIT-LP-PAGE-MISSION` | 任务标签选择器 | 左面板 |
| `SIT-LP-PAGE-DEVICE` | 设备标签选择器 | 左面板 |
| `SIT-LP-TARGET-TABLE` | 目标表 | 左面板 |
| `SIT-LP-MISSION-TABLE` | 任务表 | 左面板 |
| `SIT-LP-DEVICE-TABLE` | 设备表 | 左面板 |
| `SIT-VSP` | 视频流面板容器 | 中心上 |
| `SIT-VSP-LAYOUT-1` | 分屏按钮 1 | 中心上 |
| `SIT-VSP-LAYOUT-2` | 分屏按钮 2 | 中心上 |
| `SIT-VSP-LAYOUT-3` | 分屏按钮 3 | 中心上 |
| `SIT-VSP-LAYOUT-4` | 分屏按钮 4 | 中心上 |
| `SIT-VSP-FULLSCREEN` | 全屏按钮（cell 0） | 中心上 |
| `SIT-VSP-EXIT` | 退出全屏按钮 | 中心上 |
| `SIT-VSP-CELL-0-FULLSCREEN` | cell 0 单元格全屏按钮 | 中心上 |
| `SIT-VSP-CELL-1-FULLSCREEN` | cell 1 单元格全屏按钮 | 中心上 |
| `SIT-VSP-CELL-2-FULLSCREEN` | cell 2 单元格全屏按钮 | 中心上 |
| `SIT-VSP-CELL-3-FULLSCREEN` | cell 3 单元格全屏按钮 | 中心上 |
| `SIT-ALERT-LIST` | 告警列表 | 中心下 |
| `SIT-ALERT-COUNT` | 告警计数徽章 | 中心下 |
| `SIT-DC-TARGET` | 模拟目标标签 | 中心下 |
| `SIT-DC-STATUS` | 模拟状态标签 | 中心下 |
| `SIT-DC-CONFIRM` | 模拟确认按钮 | 中心下 |
| `SIT-DC-START` | 模拟处置按钮 | 中心下 |
| `SIT-DC-COMPLETE` | 模拟完成按钮 | 中心下 |
| `SIT-DC-LOG` | 操作日志 | 中心下 |
| `SIT-BOB` | 批量操作条容器 | 中心下 |
| `SIT-BOB-COUNT` | 已选择计数 | 中心下 |
| `SIT-BOB-ASSIGN` | 分配任务按钮 | 中心下 |
| `SIT-BOB-IGNORE` | 标记忽略按钮 | 中心下 |
| `SIT-RP-FULLSCREEN` | 全屏按钮（禁用占位） | 右面板 |
| `SIT-RP-MAP` | 三维态势地图 | 右面板 |
| `SIT-RP-TOP` | 俯视按钮 | 右面板 |
| `SIT-RP-SIDE` | 侧视按钮 | 右面板 |
| `SIT-RP-3D` | 3D 视角按钮 | 右面板 |
| `SIT-RP-RESET` | 复位按钮 | 右面板 |
| `SIT-RP-ZOOM` | 缩放标签 | 右面板 |
| `SIT-RP-POS` | 位置标签 | 右面板 |
| `SIT-DS-LIST` | 设备状态列表 | 右面板 |
| `SIT-DS-CARD-{deviceId}` | 设备卡片（每张） | 右面板 |
| `SIT-DEC-PANEL` | 决策建议面板 | 右面板 |
| `SIT-DEC-STATUS` | 模拟状态标签 | 右面板 |
| `SIT-DEC-METHOD` | 方案值 | 右面板 |
| `SIT-DEC-RISK` | 风险值 | 右面板 |
| `SIT-DEC-CONF-BAR` | 置信度进度条 | 右面板 |
| `SIT-DEC-CONF-TEXT` | 置信度文字 | 右面板 |
| `SIT-DEC-DETAIL` | 详情标签 | 右面板 |

导航栏、菜单栏、工具栏、状态栏的 `SIT-*` ID 见 `application-shell.md` 第 3 节（导航栏）、第 4 节（菜单栏）、第 5 节（工具栏）、第 6 节（状态栏）。
