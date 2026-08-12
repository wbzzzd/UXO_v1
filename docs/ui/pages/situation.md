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
- [`src/MainWindow/DeviceResourceBar.cpp`](../../../src/MainWindow/DeviceResourceBar.cpp)
- [`src/MainWindow/TargetDetailOverlay.cpp`](../../../src/MainWindow/TargetDetailOverlay.cpp)
- [`src/MainWindow/DeviceStatusPanel.cpp`](../../../src/MainWindow/DeviceStatusPanel.cpp)
- [`src/MainWindow/DecisionSuggestionPanel.cpp`](../../../src/MainWindow/DecisionSuggestionPanel.cpp)
- [`include/MainWindow/VideoStreamPanel.h`](../../../include/MainWindow/VideoStreamPanel.h)
- [`include/MainWindow/DeviceResourceBar.h`](../../../include/MainWindow/DeviceResourceBar.h)
- [`include/MainWindow/TargetDetailOverlay.h`](../../../include/MainWindow/TargetDetailOverlay.h)
- [`include/Core/Data/Types.h`](../../../include/Core/Data/Types.h)

## 1. 页面概述

态势页面是系统默认页面（导航 `SIT-NAV-01` 默认选中）。它一屏呈现：左侧目标/任务/设备列表，中心顶部设备资源条 + 战术地图（含视频 PiP 浮层与目标详情浮层）与告警/操作日志，右侧三维态势地图、设备状态、决策建议。所有数据来自本地模拟 fixture（`DemoScenarioProvider`），所有操作仅修改内存中的 `SimulationWorkflow`，不连接真实设备、不写入数据库、不执行排爆动作。

页面整体布局由应用壳定义（见 `application-shell.md` 第 2 节）。本文档化页面内部的六个内容区域：

| 区域 | 位置 | 内部组件 | CURRENT 主控件 |
|------|------|----------|----------------|
| A | 左面板 | 搜索栏、状态子标签、三标签表格 | `LeftPanelWidget` |
| B | 地图容器内浮层 | 视频 PiP 浮层（本地演示视频 + HUD-only） | `VideoStreamPanel` |
| C | 中心下 | 告警面板、模拟流程与操作日志、批量操作条 | `AlertPanel` + `DetectionControlPanel` + `BatchOperationBar` |
| D | 右面板 | 三维态势地图、模拟设备状态、模拟决策建议 | `RightPanelWidget`（含 `SituationView`、`DeviceStatusPanel`、`DecisionSuggestionPanel`） |
| E | 中心顶部 | 设备资源卡片条（状态点 + ID + 电量 + 任务状态） | `DeviceResourceBar` |
| F | 地图容器内浮层 | 目标详情浮层（冻结标注证据 + 研判操作） | `TargetDetailOverlay` |

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

4 列，单选行模式（`SingleSelection`、`SelectRows`、`NoEditTriggers`），不可编辑，交替行色。表头背景 `--color-toolbar`、主文本色、内边距 4px。行高 40px。水平滚动条始终关闭。

| 列 | 表头 | 宽度 | 内容 | 文字色 |
|----|------|------|------|--------|
| 0 | 类型 | 52px 固定 | `typeName`，按威胁等级着色 | 高=`--color-threat-high`、中=`--color-threat-medium`、低=`--color-threat-low`、未知=`--color-text-disabled` |
| 1 | 置信度 | 48px 固定 | `XX%`（confidence*100，整数） | `--color-text-primary` |
| 2 | 位置 | 72px 固定 | `X:n Y:n`（position.x、position.z 取整） | `--color-text-primary` |
| 3 | 模拟状态 | 拉伸 | `[模拟] 已发现/已确认/处置中/已完成/状态未知` | `--color-text-primary` |

行样式：默认 `--color-panel`；hover `--color-row-hover`；选中 `--color-selection`。

| 字段 | 值 |
|------|----|
| ID | `SIT-LP-TARGET-TABLE` |
| 类型 | QTableWidget |
| 位置 | 目标标签页内容区 |
| 用途 | 显示模拟目标列表，单选触发目标选中 |
| 点击结果 | 单击任一单元格 -> 发出 `targetSelected(m_targets[row])`；`MainWindow::onSelectTargetEverywhere` 驱动地图标点高亮与详情浮层冻结证据显示，刷新右面板决策区 |
| 键盘 | Tab 聚焦表；Up/Down 移动选中行并触发 `targetSelected` |
| 原型行为 | 单击选中行并同步地图标点高亮、详情浮层冻结证据与右面板（同 CURRENT） |
| CURRENT 映射 | `LeftPanelWidget.cpp` `setupTargetList`、`populateTargetList`、`itemClicked` 连接（`SingleSelection`，无双击连接，无勾选列） |
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

## 2.5 区域 E：设备资源条

36px 固定高横条，位于中心区顶部、地图容器上方。背景 `--color-toolbar`，承载 UAV/机器人等设备卡片。设计语义：设备是"资源"而非研判对象，点击卡片仅切换视频 PiP 源，不进入研判流程。

### 2.5.1 容器 `SIT-DRB`

| 字段 | 值 |
|------|----|
| ID | `SIT-DRB` |
| 类型 | DeviceResourceBar（QWidget） |
| 位置 | 中心区顶部，36px 高，填满宽度 |
| 用途 | 显示设备资源卡片（状态点 + ID + 电量 + 任务状态），点击切换视频 PiP 源 |
| 默认状态 | `loadMockData` 后显示所有设备卡片，首张卡片选中 |
| 点击结果 | emit `deviceSelected` -> `MainWindow` 更新视频 PiP 设备名称 |
| 键盘 | Tab 聚焦 |
| 原型行为 | 同 CURRENT；点击卡片切换 PiP 源 |
| CURRENT 映射 | `MainWindow.cpp` `m_deviceResourceBar = new DeviceResourceBar(...)`；`DeviceResourceBar.cpp` `setupUi`、`setDevices`、`selectDevice` |
| 安全 | 只读展示，点击仅切换模拟视频源，无设备控制 |

设备卡片由 `setDevices()` 加载，每张卡片含：状态点（在线绿/忙碌橙/离线灰）+ 设备 ID + 电量 + 任务状态文案。`selectDevice()` 可程序化选中指定设备（高亮其卡片，不重复发信号）。

## 3. 区域 B：视频 PiP 浮层

位于地图主舞台容器（`m_mapContainer`）内，作为画中画（PiP）浮层叠加在战术地图上。CURRENT 为 `VideoStreamPanel`，加载本地演示视频文件（`kVideoPath = "/home/lin/uxo-assets/video/perth_airport_drone_edit.mp4"`），通过 `QMediaPlayer` 输出到 `QVideoWidget` 渲染。`VideoOverlayWidget` 作为透明叠加层覆盖在视频画面上，仅持久显示 HUD（十字准星、REC、遥测 LAT/LON/ALT/HDG、时间码），**不绘制检测框**。冻结标注证据仅在 `TargetDetailOverlay` 中按目标选中显示（见 §3.5）。不接入真实视频流。

默认布局：地图为主视图（填满 `m_mapContainer`），视频 PiP 浮于左下角（480×294 = 480 宽 × (270 视频高 + 24 标题栏高)）。主次可切换、可最小化、可关闭。

### 3.1 容器 `SIT-VSP`

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP` |
| 类型 | VideoStreamPanel（QWidget） |
| 位置 | `m_mapContainer` 内浮层；默认 PiP 模式位于左下角（margin 12px），可切换为全屏主视图 |
| 用途 | 本地演示视频播放 + HUD-only 叠加层 |
| 默认状态 | PiP 模式（地图为主视图），480×294，视频区可见 |
| 全屏状态 | 视频为主视图（填满 `m_mapContainer`），地图缩为 PiP |
| 最小化状态 | 仅显示标题栏（24px），视频区隐藏 |
| 关闭状态 | PiP 完全隐藏 |
| 加载状态 | `loadMockData` 中 `loadVideo(kVideoPath)` 同步加载本地 mp4；不自动播放，等待用户点击 [开始] |
| 空状态 | 不适用（`loadMockData` 同步加载视频文件） |
| 错误状态 | 不适用（CURRENT 无错误处理路径） |
| 禁用状态 | 不适用（面板始终可交互） |
| 点击结果 | 视频区本身无点击响应（`VideoOverlayWidget` 鼠标事件透传给下层视频控件） |
| 键盘 | 标题栏按钮可 Tab 聚焦 |
| 原型行为 | 同 CURRENT；PiP 浮层 + HUD-only，不补齐视频区点击 |
| CURRENT 映射 | `MainWindow.cpp` `m_videoPiP = new VideoStreamPanel(m_mapContainer)`、`repositionFloatingWidgets`；`VideoStreamPanel.cpp` `setupUi` |
| 安全 | 不接入真实视频流，仅本地演示视频文件 |

PiP 尺寸常量（`MainWindow.cpp`）：`kPipWidth = 480`、`kPipVideoHeight = 270`、`kPipTitleBarHeight = 24`、`kPipHeight = 294`、`kPipMargin = 12`。定位由 `repositionFloatingWidgets()` 管理：主视图填满 `m_mapContainer`，PiP 位于 `(kPipMargin, ch - kPipHeight - kPipMargin)`。

### 3.2 标题栏（24px 固定高）

背景 `--color-toolbar`，objectName `pipTitleBar`，内边距 `8px 0 4px 8px`，间距 6px。从左到右：绿色状态点 + 设备名称 + 弹性留白 + 主次切换按钮 + 最小化按钮 + 关闭按钮。

**状态点（只读）**：`QLabel`，8×8px，`--color-status-online` 背景，圆角 4px。无 ID。

**设备名称（只读）**：`QLabel`，默认文本 `UAV-1 侦察无人机`，主文本色，11px。`setDeviceTitle()` 可更新（设备资源条选中设备时触发）。无 ID。

#### 3.2.1 主次切换按钮 `SIT-VSP-SWAP`

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP-SWAP` |
| 标签 | ⇄ |
| 类型 | QPushButton |
| 位置 | 标题栏右1，24×20px |
| 用途 | 切换视频与地图的主/PiP 角色 |
| 默认态 | 透明背景、主文本色、无边框、12px |
| hover | 背景 `--color-border` |
| active | 不适用 |
| disabled | 不适用（始终可点击） |
| 点击结果 | emit `swapRequested` -> `MainWindow::onPipSwapClicked` 翻转 `m_videoIsMain`，`repositionFloatingWidgets()` 重排 |
| 键盘 | Tab 聚焦，Enter 触发 |
| 原型行为 | 同 CURRENT |
| CURRENT 映射 | `VideoStreamPanel.cpp` `m_swapBtn`（`createTitleBar`）、`swapRequested` 信号；`MainWindow.cpp` `onPipSwapClicked` |
| 安全 | 无 |

#### 3.2.2 最小化按钮 `SIT-VSP-MINIMIZE`

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP-MINIMIZE` |
| 标签 | - |
| 类型 | QPushButton |
| 位置 | 标题栏右2，24×20px |
| 用途 | 折叠/展开 PiP 视频区（仅保留标题栏） |
| 默认态 | 透明背景、主文本色、无边框、12px |
| hover | 背景 `--color-border` |
| active | 不适用 |
| disabled | 不适用 |
| 点击结果 | emit `minimizeRequested` -> `MainWindow::onPipMinimizeClicked` 翻转 `m_pipMinimized`；最小化时 `setMinimized(true)` 隐藏视频区，PiP 高度降为 24px |
| 键盘 | Tab 聚焦，Enter 触发 |
| 原型行为 | 同 CURRENT |
| CURRENT 映射 | `VideoStreamPanel.cpp` `m_minimizeBtn`、`minimizeRequested` 信号、`setMinimized`；`MainWindow.cpp` `onPipMinimizeClicked` |
| 安全 | 无 |

#### 3.2.3 关闭按钮 `SIT-VSP-CLOSE`

| 字段 | 值 |
|------|----|
| ID | `SIT-VSP-CLOSE` |
| 标签 | ✕ |
| 类型 | QPushButton |
| 位置 | 标题栏右3，24×20px |
| 用途 | 隐藏 PiP 浮层 |
| 默认态 | 透明背景、主文本色、无边框、12px |
| hover | 背景 `--color-danger`、主文本色 |
| active | 不适用 |
| disabled | 不适用 |
| 点击结果 | emit `closeRequested` -> `MainWindow::onPipCloseClicked` 设 `m_pipVisible = false`，`repositionFloatingWidgets()` 隐藏 PiP |
| 键盘 | Tab 聚焦，Enter 触发 |
| 原型行为 | 同 CURRENT |
| CURRENT 映射 | `VideoStreamPanel.cpp` `m_closeBtn`、`closeRequested` 信号；`MainWindow.cpp` `onPipCloseClicked` |
| 安全 | 无 |

### 3.3 视频区（QVideoWidget + VideoOverlayWidget）

标题栏下方为视频区（`m_videoArea`），使用 `QVBoxLayout` 排列。`QVideoWidget` 作为 `QMediaPlayer` 的视频输出控件填满视频区，`VideoOverlayWidget` 作为子 widget 叠加在上方。

**QVideoWidget**（底层）：Qt 标准视频渲染控件，接收 `QMediaPlayer` 视频帧并渲染。`VideoStreamPanel` 通过 `QVideoProbe` 拦截视频帧（`onFrameProbed`），提供 `currentFrameSnapshot()` 返回当前帧的 detached `QImage`（用于证据捕获），`hasFrame()` 判断是否有帧。视频路径由 `loadVideo(kVideoPath)` 加载，不自动播放，等待 [开始] 触发 `play()`。

**VideoOverlayWidget**（上层，透明）：HUD 叠加层，仅绘制准星与遥测文本，不绘制检测框。鼠标事件透传给下层（`setAttribute(Qt::WA_TransparentForMouseEvents)` 不设置，但 `mousePressEvent`/`mouseReleaseEvent` 等不处理，事件自然传播）。

### 3.4 HUD 叠加层 `SIT-VSP-HUD`

`VideoOverlayWidget`，覆盖整个视频区，背景透明。`onHudTick`（500ms `QTimer`）触发 `update()` 重绘。绘制内容：

| 元素 | 位置 | 内容 | 数据源 |
|------|------|------|--------|
| 十字准星 | 视频中心 | 绿色十字线 + 中心圆点 | 固定（`drawCrosshair`） |
| REC 指示 | 左上 | 红色圆点 + `REC` 文本 | `drawRecIndicator`，视频播放时显示 |
| 遥测文本 | 右上 | `LAT: xx.xxxxxx  LON: xx.xxxxxx  ALT: xxxm  HDG: xxx°` | `setTelemetry()`，由 `MainWindow` 从模拟数据更新 |
| 时间码 | 底部居中 | `HH:mm:ss` | `onHudTick` 每秒刷新 |
| 设备信息 | 左下 | 设备名称 | `setDeviceInfo()`，由 `setDeviceTitle` 同步 |

**不绘制检测框**。冻结标注证据（含检测框）仅在 `TargetDetailOverlay` 中按目标选中显示（见 §3.5）。

`clear()` 清除遥测与设备信息文本，HUD 准星与 REC 指示不受影响。

### 3.5 冻结证据生命周期

证据捕获与显示流程（`MainWindow.cpp`）：

| 阶段 | 触发 | 行为 | 证据存储 |
|------|------|------|---------|
| 捕获 | `onDetectionOccurred` | `m_videoPiP->currentFrameSnapshot()` 获取当前视频帧 -> `annotateEvidenceImage()` 在帧上绘制检测框与标注 -> 存入 `m_evidenceByTargetId` | `QMap<QString, DetectionEvidence> m_evidenceByTargetId` |
| 显示 | `onSelectTargetEverywhere`（侧边栏/地图选中目标） | 查找 `m_evidenceByTargetId[targetId]`：找到则 `m_targetDetailOverlay->setEvidence(image, timestamp, frameIndex, targetId)` 显示冻结标注帧；未找到则 `clearEvidence()` | 只读查询 |
| 结束检测 | `onStopDetection` | 停止视频播放 + `seek(0)` + 停止模拟器；**保留所有证据**，已捕获的冻结帧仍可通过选中目标查看 | 保留 |
| 重置 | `onResetDetection` | 停止视频 + 重置模拟器 + 清空目标/地图/侧边栏 + `m_evidenceByTargetId.clear()` + `m_targetDetailOverlay->reset()` | 清空 |

`TargetDetailOverlay` 位于 `m_mapContainer` 右上角，340px 宽不透明面板，显示冻结标注帧时覆盖在地图之上；无目标时隐藏。面板包含目标详情行（ID、类型、威胁等级、置信度、坐标等）、证据视口（316×180）和 3 个模拟研判操作按钮（`createTaskRequested`/`assignDeviceRequested`/`viewHistoryRequested`），点击后仅给出纯文本反馈，不改状态机。证据图像为本地演示视频的冻结帧 + 模拟标注，不包含真实检测数据。

### 3.6 信号说明

`VideoStreamPanel.h` 声明七个信号，`MainWindow` 全部连接：

| 信号 | CURRENT 是否 emit | 说明 |
|------|-------------------|------|
| `positionChanged(qint64)` | 是 | 视频位置变化时 emit |
| `durationChanged(qint64)` | 是 | 视频时长变化时 emit |
| `stateChanged(QMediaPlayer::State)` | 是 | 播放/暂停/停止状态变化时 emit |
| `videoEnded()` | 是 | 视频播放结束时 emit |
| `swapRequested()` | 是 | 标题栏 [⇄] 按钮点击时 emit |
| `minimizeRequested()` | 是 | 标题栏 [-] 按钮点击时 emit |
| `closeRequested()` | 是 | 标题栏 [✕] 按钮点击时 emit |

`MainWindow` 连接全部信号：`onPipSwapClicked`、`onPipMinimizeClicked`、`onPipCloseClicked` 处理浮层操作；`positionChanged`/`durationChanged`/`stateChanged`/`videoEnded` 处理播放状态同步。原型保持一致。

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

CURRENT 中 `setSelectedCount` 是唯一显示入口，但目标表已移除复选框（单选行模式），`setSelectedCount` 始终为 0，因此实际上始终隐藏。原型保持此行为。

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
| 原型行为 | 保持隐藏；如未来启用批量选择功能，需先在本文登记新 ID 与信号路径 |
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

## 6. 应用壳控件

本节文档化态势页复用的应用壳控件（导航栏、菜单栏、工具栏、状态栏），为其分配 `SIT-*` ID。详细规格（尺寸、间距、跨页一致性规则）见 `application-shell.md`。

### 6.1 导航栏

宽 80px 固定（`--size-nav-width`），背景 `--color-bg`，右边框 1px `--color-border`。垂直布局：Logo + 间距 + 6 个导航项 + 弹性留白。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|--------|---------|------|---------|---------------|------|
| `SIT-NAV-LOGO` | UXO | div | 导航栏顶，40px 高 | 品牌标识 | `--color-primary` 色，18px，加粗，字间距 2px，居中 | 不适用 | 不适用 | 无 | 不可聚焦 | 同 CURRENT | 见 `application-shell.md` 第 3 节 | 无 |
| `SIT-NAV-01` | 态势 | div | 导航项 1，56px 高 | 切换到态势页（当前页） | 选中 | 背景 `--color-row-hover`、文本 `--color-text-primary` | 背景 `--color-selection`、左边框 `--color-primary`、文本 `--color-text-primary`、加粗 | 移除其他项 selected，当前加 selected（无实际页面跳转） | div 无 tabindex，不可键盘聚焦 | 默认选中；点击仅保持选中 | 见 `application-shell.md` 第 3 节 | 无 |
| `SIT-NAV-02` | 探测 | div | 导航项 2，56px 高 | 切换到探测页（占位） | 未选中，tooltip `未实现页面（占位）` | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `SIT-NAV-03` | 决策 | div | 导航项 3，56px 高 | 切换到决策页（占位） | 未选中，tooltip `未实现页面（占位）` | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `SIT-NAV-04` | 设备 | div | 导航项 4，56px 高 | 切换到设备页（占位） | 未选中，tooltip `未实现页面（占位）` | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `SIT-NAV-05` | 统计 | div | 导航项 5，56px 高 | 切换到统计页（占位） | 未选中，tooltip `未实现页面（占位）` | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `SIT-NAV-06` | 配置 | div | 导航项 6，56px 高 | 切换到配置页（占位） | 未选中，tooltip `未实现页面（占位）` | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |

导航项图标统一为 `◎`（18px）。导航项内边距由 flex 居中控制，字号 `--font-size-caption`，间距 4px。

> 注：原型中导航点击仅切换 selected 类，不执行实际页面跳转（单页原型）。CURRENT Qt 客户端中导航切换通过 `QStackedWidget` 实现，见 `application-shell.md`。

### 6.2 菜单栏

高 30px（`--size-menu-bar-height`），背景 `--color-menu`，底部 1px `--color-border` 边框，内边距 `0 4px`。5 个菜单项为 `<button>` 元素，内边距 `6px 12px`，`--font-size-body`。

默认态：透明背景、`--color-text-primary` 文本。hover：背景 `--color-border`。禁用态（`data-disabled="true"`）：`--color-text-disabled` 文本，hover 不变背景。省略态（`data-omitted="true"`）：`display:none`，不渲染。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled/omitted | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|------------------|---------|------|---------|---------------|------|
| `SIT-MENU-FILE` | 文件(&F) | button | 菜单栏左 1 | 文件菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无（无 JS 事件绑定） | Tab 聚焦，Enter 触发 | 点击无效果，不展开下拉菜单 | 见 `application-shell.md` 第 4 节 | 无 |
| `SIT-MENU-VIEW` | 视图(&V) | button | 菜单栏左 2 | 视图菜单（占位） | 同上 | 同上 | 不适用 | 无 | 同上 | 同上 | 见 `application-shell.md` 第 4 节 | 无 |
| `SIT-MENU-TOOLS` | 工具(&T) | button | 菜单栏左 3 | 工具菜单（禁用占位） | `--color-text-disabled` 文本 | 不变背景 | `data-disabled="true"`，tooltip `占位控件，未实现` | 无（disabled） | 不可聚焦 | **禁用并标注"占位"**，不响应点击 | 见 `application-shell.md` 第 4 节 | 无 |
| `SIT-MENU-DEVICE` | 设备(&D) | button | 菜单栏左 4 | 设备菜单（省略占位） | 不适用 | 不适用 | `data-omitted="true"`，`display:none` | 无（不可见） | 不可聚焦 | **省略不渲染**；对应 CURRENT 中连接空 lambda 的"打开设备控制台"占位菜单项 | 见 `application-shell.md` 第 7 节 | 无 |
| `SIT-MENU-HELP` | 帮助(&H) | button | 菜单栏左 5 | 帮助菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无 | Tab 聚焦，Enter 触发 | 点击无效果 | 见 `application-shell.md` 第 4 节 | 无 |

### 6.3 工具栏

高 32px（`--size-toolbar-height`），背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`，间距 8px。从左到右：图层控制（禁用占位）+ 测量工具（禁用占位）+ 坐标拾取（禁用占位）+ 视角复位按钮 + 同步状态（省略占位）+ 书签（省略占位）+ 设备控制台（省略占位）。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled/omitted | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|------------------|---------|------|---------|---------------|------|
| `SIT-TB-LAYER` | 图层控制 | span | 工具栏左 1 | 图层控制（禁用占位） | `--color-text-disabled`，`--font-size-caption` | 不变（disabled） | `data-disabled="true"`，tooltip `占位控件，未实现` | 无 | 不可聚焦 | **禁用并标注"占位"** | 见 `application-shell.md` 第 5 节 | 无 |
| `SIT-TB-MEASURE` | 测量工具 | span | 工具栏左 2 | 测量工具（禁用占位） | 同上 | 同上 | 同上 | 无 | 不可聚焦 | 同上 | 见 `application-shell.md` 第 5 节 | 无 |
| `SIT-TB-PICK` | 坐标拾取 | span | 工具栏左 3 | 坐标拾取（禁用占位） | 同上 | 同上 | 同上 | 无 | 不可聚焦 | 同上 | 见 `application-shell.md` 第 5 节 | 无 |
| `SIT-TB-RESET` | 视角复位 | button | 工具栏左 4 | 复位三维相机视角 | `--color-text-secondary` 文本、透明背景、1px `--color-border` 边框、圆角 `--radius-control`、`--font-size-caption`；内边距 `4px 8px` | 背景 `--color-border`、文本 `--color-text-primary` | 不适用 | 无（JS 未绑定 click 事件） | Tab 聚焦，Enter 触发 | 点击无效果；与右面板 `SIT-RP-RESET` 等价但工具栏按钮无绑定 | 见 `application-shell.md` 第 5 节 | 无 |
| `SIT-TB-SYNC` | 同步状态 | span | 工具栏左 5 | 同步状态（省略占位） | 不适用 | 不适用 | `data-omitted="true"`，`display:none` | 无（不可见） | 不可聚焦 | **省略不渲染** | 见 `application-shell.md` 第 5 节 | 无 |
| `SIT-TB-BOOKMARK` | 书签 | span | 工具栏左 6 | 书签（省略占位） | 不适用 | 不适用 | `data-omitted="true"`，`display:none` | 无（不可见） | 不可聚焦 | **省略不渲染** | 见 `application-shell.md` 第 5 节 | 无 |
| `SIT-TB-CONSOLE` | 设备控制台 | span | 工具栏左 7 | 设备控制台（省略占位） | 不适用 | 不适用 | `data-omitted="true"`，`display:none` | 无（不可见） | 不可聚焦 | **省略不渲染**；对应 CURRENT 中连接空 lambda 的"打开设备控制台"占位 | 见 `application-shell.md` 第 7 节 | 无 |

### 6.4 状态栏

高 28px，背景 `--color-bg`，顶部 1px `--color-border` 边框，内边距 `0 16px`，间距 16px。从左到右：设备状态标签 + 分隔线 + 最低电量标签 + 分隔线 + 模拟模式标签 + 分隔线 + 告警区（弹性）+ 紧急停止按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `SIT-SB-DEVICE` | 设备: 2/2 在线 | span | 状态栏左 1 | 显示模拟设备在线状态 | `--color-text-primary`，`--font-size-caption` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `设备: 2/2 在线`，不随操作变化 | 见 `application-shell.md` 第 6 节 | 模拟数据 |
| `SIT-SB-BATTERY` | 最低电量: 74% | span | 状态栏左 2，分隔线后 | 显示模拟设备最低电量 | `status-battery high` 类（`--color-status-online` 色），`--font-size-caption` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `最低电量: 74%`；电量阈值变色（high/mid/low） | 见 `application-shell.md` 第 6 节 | 模拟数据 |
| `SIT-SB-SIM` | [模拟模式] | span | 状态栏左 3，分隔线后 | 标注当前为模拟模式 | `--color-status-busy`，`--font-size-caption`，加粗 | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `[模拟模式]` | 见 `application-shell.md` 第 6 节 | 模拟标注 |
| `SIT-SB-ALARM` | - | div 容器 | 状态栏中，弹性宽 | 展示模拟告警滚动条目 | 条目 `--color-status-busy` 色、`--font-size-caption`、`--color-toolbar` 背景、内边距 `2px 8px`、圆角 `--radius-control` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 1 条告警 `模拟告警 1: 模拟设备电量偏低` | 见 `application-shell.md` 第 6 节 | 模拟告警 |
| `SIT-SB-EMERGENCY` | 紧急停止 | button | 状态栏右，80x20 | 紧急停止所有设备（禁用占位） | **始终禁用**：`--color-border` 背景、`--color-text-disabled` 文本、11px、加粗、圆角 3px；tooltip `危险占位：CURRENT 仅弹确认框，无设备停止效果，本试点禁用` | 不适用 | `disabled` + `data-disabled="true"` | 无（disabled，不响应点击） | 不可聚焦 | **模拟占位，无实际效果**；原型中禁用并标注"危险占位" | 见 `application-shell.md` 第 6 节 | 模拟占位，无设备停止效果 |

状态栏分隔线为 1px 宽、18px 高的 `--color-border` 竖线。

## 7. 状态规则汇总

每个区域必须定义五态。下表汇总各区域的五态实现：

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 左面板目标表 | 列表展示 | 骨架行 | `暂无目标`（CURRENT 无空态，原型补齐） | 边框 `--color-status-error` + `目标加载失败` | 不适用（始终可交互） |
| 左面板任务表 | 列表展示 | 骨架行 | `暂无任务`（原型补齐） | `任务加载失败` | 不适用 |
| 左面板设备表 | 列表展示 | 骨架行 | `暂无设备`（原型补齐） | `设备加载失败` | 不适用 |
| 视频 PiP 浮层 | 本地演示视频 + HUD 显示 | 不适用（`loadVideo` 同步加载） | 不适用（本地演示视频文件） | 不适用（CURRENT 无错误路径） | 不适用（始终可交互） |
| 告警面板 | 列表展示 | 骨架条 | `暂无告警`（CURRENT 已实现） | `告警加载失败` | 不适用（只读） |
| 操作日志 | 日志列表 | 不适用（本地内存） | `暂无模拟操作记录（重启后清空）`（CURRENT 已实现） | 不适用 | 不适用 |
| 批量操作条 | 隐藏（默认） | 不适用 | 不适用 | 不适用 | 不适用 |
| 三维地图 | 场景渲染 | 骨架 + `场景加载中…` | `暂无场景数据` | `场景加载失败` | 不适用 |
| 设备状态面板 | 卡片列表 | 骨架卡 | `暂无设备`（原型补齐） | `设备状态加载失败` | 不适用（只读） |
| 决策建议面板 | 决策信息 | 骨架 | `请选择目标以获取决策建议`（CURRENT 已实现） | 不适用 | 不适用（只读） |

CURRENT 已实现的空态：告警面板、操作日志、决策建议。其余空态在原型中补齐，文字必须同时给出（颜色不作为唯一信息）。

## 8. 交互流程

### 8.1 目标选择流程

1. 用户在 `SIT-LP-TARGET-TABLE` 单击目标行。
2. `LeftPanelWidget` 发出 `targetSelected`。
3. `MainWindow::onTargetSelected` 调 `SimulationWorkflow::selectTarget(targetId)`。
4. 工作流选中目标后，`refreshSelectedTarget` 同步：
   - 左表选中行高亮（CURRENT 通过表选择，未显式同步；原型补齐）。
   - 右面板 `setTarget`：决策区更新方案/风险/置信度/状态；三维视图 `highlightTarget` 高亮标记、`focusOnTarget` 相机聚焦。
   - 探测控制区 `setSelectedTarget`：更新 `SIT-DC-TARGET`、`SIT-DC-STATUS` 与按钮可用性。

### 8.2 模拟处置流程

目标状态机：`Detected -> Confirmed -> Disposing -> Disposed`。每步对应一个按钮：

1. 目标 `Detected`：`SIT-DC-CONFIRM` 启用，`SIT-DC-START`/`SIT-DC-COMPLETE` 禁用。
2. 点击 `SIT-DC-CONFIRM`：状态变为 `Confirmed`，`SIT-DC-START` 启用。
3. 点击 `SIT-DC-START`：状态变为 `Disposing`，`SIT-DC-COMPLETE` 启用。
4. 点击 `SIT-DC-COMPLETE`：状态变为 `Disposed`，三个按钮全部禁用。

每步操作在 `SIT-DC-LOG` 追加一条 `[HH:mm:ss] 消息`，并更新左表第 4 列模拟状态文字与右面板决策区状态标签。所有变更仅影响内存 `SimulationWorkflow`，重启后清空。

### 8.3 刷新流程

点击 `SIT-LP-REFRESH`：
1. 发出 `refreshSimulationRequested`。
2. `MainWindow::onRefreshSimulationRequested` 重读工作流权威副本，回填左面板三表、右面板设备、决策区。
3. 不重置选择、不重载场景、不调用 `loadMockData`。
4. 操作日志保持不变。

## 9. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280x720 | 左面板 320px 固定，目标表前 4 列共 200px + 状态列拉伸；右面板 360px（360–420px 弹性范围的最小值，CURRENT `setMinimumWidth(360)`），决策区最小 280px（`decisionSection->setMinimumHeight(280)`）；信息区告警与探测控制按 1:1 并排，探测控制三个按钮（3x68px + 间距）需在半宽内完整显示；批量操作条隐藏不占位 |
| 1920x1080 | 默认尺寸；中心地图区与信息区按 3:2 分配；右面板三段按 5:2:3；所有控件按 token 展示 |
| 3840x2160 | 固定区域不变；中心区与右面板弹性区按比例放大；右面板可至 420px；字号与控件尺寸保持固定 px |

CURRENT 在 1280x720 下决策面板末两行（指派设备、模拟声明）可能被截断。`RightPanelWidget` 已通过 `setMinimumHeight(280)` 与 stretch 5/2/3 调整缓解。原型需验证末两行完整可见。

## 10. 安全清单

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

## 11. CURRENT 映射总结

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

## 12. SIT-* ID 索引

下表列出本文档化的全部 `SIT-*` ID，供 HTML 原型与 Playwright 测试对齐：

| ID | 控件 | 区域 |
|----|------|------|
| `SIT-NAV-LOGO` | 导航栏 Logo | 应用壳 |
| `SIT-NAV-01` | 导航项：态势（选中） | 应用壳 |
| `SIT-NAV-02` | 导航项：探测（占位） | 应用壳 |
| `SIT-NAV-03` | 导航项：决策（占位） | 应用壳 |
| `SIT-NAV-04` | 导航项：设备（占位） | 应用壳 |
| `SIT-NAV-05` | 导航项：统计（占位） | 应用壳 |
| `SIT-NAV-06` | 导航项：配置（占位） | 应用壳 |
| `SIT-MENU-FILE` | 菜单：文件 | 应用壳 |
| `SIT-MENU-VIEW` | 菜单：视图 | 应用壳 |
| `SIT-MENU-TOOLS` | 菜单：工具（禁用占位） | 应用壳 |
| `SIT-MENU-DEVICE` | 菜单：设备（省略占位） | 应用壳 |
| `SIT-MENU-HELP` | 菜单：帮助 | 应用壳 |
| `SIT-TB-LAYER` | 工具栏：图层控制（禁用占位） | 应用壳 |
| `SIT-TB-MEASURE` | 工具栏：测量工具（禁用占位） | 应用壳 |
| `SIT-TB-PICK` | 工具栏：坐标拾取（禁用占位） | 应用壳 |
| `SIT-TB-RESET` | 工具栏：视角复位 | 应用壳 |
| `SIT-TB-SYNC` | 工具栏：同步状态（省略占位） | 应用壳 |
| `SIT-TB-BOOKMARK` | 工具栏：书签（省略占位） | 应用壳 |
| `SIT-TB-CONSOLE` | 工具栏：设备控制台（省略占位） | 应用壳 |
| `SIT-SB-DEVICE` | 设备状态 | 状态栏 |
| `SIT-SB-BATTERY` | 最低电量 | 状态栏 |
| `SIT-SB-SIM` | 模拟模式标签 | 状态栏 |
| `SIT-SB-ALARM` | 告警滚动区 | 状态栏 |
| `SIT-SB-EMERGENCY` | 紧急停止按钮（禁用占位） | 状态栏 |
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
| `SIT-VSP` | 视频 PiP 浮层容器 | 地图容器浮层 |
| `SIT-VSP-SWAP` | 主次切换按钮 | 地图容器浮层 |
| `SIT-VSP-MINIMIZE` | 最小化按钮 | 地图容器浮层 |
| `SIT-VSP-CLOSE` | 关闭按钮 | 地图容器浮层 |
| `SIT-VSP-HUD` | HUD 叠加层（准星/REC/遥测/时间码） | 地图容器浮层 |
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

应用壳控件（导航栏、菜单栏、工具栏、状态栏）的 `SIT-*` ID 见本文档第 6 节；跨页一致的尺寸、间距与状态规格见 `application-shell.md` 第 3 节（导航栏）、第 4 节（菜单栏）、第 5 节（工具栏）、第 6 节（状态栏）。`SIT-DS-CARD-{deviceId}` 为模式 ID，原型中以 fixture 设备实例 `SIT-DS-CARD-device-demo-drone-001`、`SIT-DS-CARD-device-demo-robot-001` 体现。
