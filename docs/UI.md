# UI 规范与扩展契约

最后更新：2026-07-20
状态：初始草稿，待逐节评审。`TARGET` 为长期结构，`CURRENT` 为源码事实，`NEXT` 尚未批准实施。

本文档定义 UXO_v1 桌面客户端的 UI 基线：稳定的 shell、扩展契约、当前页面对应表与状态规范。
目标设计资料（SRS/SDD/2024 UI 详设/PRD）作为需求输入，不当作已实现事实。
所有"模拟/占位"措辞遵循 `docs/dev/simulation-policy.md`。

---

## 1. 角色

| 角色 | 主要任务 | 主要交互面 |
|------|---------|-----------|
| 指挥员 | 态势研判、方案选定、下令 | 态势图、决策面板、方案对比 |
| 操作员 | 目标确认、识别修正、状态推进 | 左侧目标列表、检测控制面板 |
| 维护员 | 设备/场景配置、模拟数据准备 | 配置页、JSON 配置 |

---

## 2. TARGET · 目标信息架构

目标 IA（导航栏顺序）：

| # | 入口 | 主要内容 |
|---|------|---------|
| 0 | 态势 | 3D 态势图全屏 + 目标图层 |
| 1 | 探测 | 视频流 + 信息区（告警/检测控制） |
| 2 | 决策 | 决策建议 + MOS 起降带规划工作台 |
| 3 | 设备 | 设备状态 + 控制台 |
| 4 | 统计 | 任务/处置统计（未实现） |
| 5 | 配置 | 系统/场景参数（未实现） |

应用 Shell（所有页面共享，不应随单页变化）：

```
MainWindow
├── MenuBar / ToolBar（图层/视角/同步等工具入口）
├── NavigationWidget (80px 固定宽，6 项)
├── 主交互区（按导航 index 切换 page，仅"探测"页已实现）
└── StatusBar (28px，设备数/电量/告警/模拟模式)
```

Shell 约束：
- 导航栏、菜单栏、工具栏、状态栏在所有页面常驻。
- 左侧目标列表（`LeftPanelWidget`）在探测/决策/设备页常驻。
- 中心区与右侧区按导航切换。
- 默认窗口 1920×1080，最小 1280×720（已知 P2 缺陷：1280×720 下 `DecisionSuggestionPanel` 底部约 5px 溢出）。

---

## 3. CURRENT · 当前实现事实

来源：`src/MainWindow/MainWindow.cpp`、`src/MainWindow/NavigationWidget.cpp`。

### 3.1 当前 shell

`MainWindow::createMainLayout()` 当前硬编码布局：

```
NavigationWidget (80px) | LeftPanelWidget (320px) | CenterArea (弹性) | RightPanelWidget (360~420px)
```

`CenterArea` 是固定的 `VideoStreamPanel`（上 3 份）+ `InfoArea`（下 2 份，含 `AlertPanel` + `DetectionControlPanel` + `BatchOperationBar`）。

**关键事实**：当前不存在 `QStackedWidget`，导航点击只切换按钮高亮，不切换中心区内容。

### 3.2 导航信号现状

`NavigationWidget` 发射 `navigationChanged(int index)` 信号；
`MainWindow::onNavigationChanged(int)` 已连接该信号，但函数体仅 `qDebug() << "Navigation changed to:" << index;`。

即：**导航按钮发射信号但当前不切换页面**，是预留的扩展点。

### 3.3 模拟工作流位置

- 状态所有者：`Core::Simulation::SimulationWorkflow`（进程内，无持久化）。
  - 维护目标列表、当前选中目标、有序操作日志。
  - 状态流转：`Detected -> Confirmed -> Disposing -> Disposed`。
- UI 唯一交互入口：`DetectionControlPanel`（中心区下方）。
  - 信号：`confirmSimulationRequested` / `startSimulationDisposalRequested` / `completeSimulationDisposalRequested`。
  - 经 `MainWindow` 转发到 `SimulationWorkflow`，回写后刷新 `LeftPanelWidget` 与日志区。
- 只读展示：`DecisionSuggestionPanel`（决策建议）、`DeviceStatusPanel`（设备状态）不发起真实控制。
- 数据源：`Core::Simulation::DemoScenarioProvider::create()`，启动时由 `MainWindow::loadMockData()` 一次性加载。

### 3.4 已编译但未接入 shell 的页

`TargetDetailPanel`、`DecisionView`、`DeviceControlView` 已在 `src/MainWindow/CMakeLists.txt` 编译，但 `MainWindow.cpp` 未实例化，留作未来页切换的占位实现，**当前不算"已实现"**。

---

## 4. NEXT · 下一阶段方向

来源：`docs/PRODUCT.md` 的 NEXT 草稿需求。

- 建立真正可切换的页面容器和导航路由，但不一次实现所有目标页面。
- 将目标、任务、设备和日志从统一状态源投影到 UI，消除各面板独立维护可变业务副本。
- 补齐模拟设备指派与任务执行反馈，使态势、设备、决策和日志随同一操作联动。
- 删除或隐藏没有真实行为的装饰性入口，避免 UI 暗示尚不存在的能力。

UXR、MOS 的 UI 资料是后续功能设计输入，不属于当前 NEXT；其布局和入口需在对应功能获批后单独评审。

---

## 5. 稳定 Shell 与扩展契约

### 5.1 页面接入契约

新页面接入"决策/设备/统计/配置"等导航项时，遵循：

1. 在 `MainWindow` 引入 `QStackedWidget` 作为 `CenterArea` 容器；现有"探测"内容迁为 page 0。
2. `onNavigationChanged(int)` 实现：`m_stack->setCurrentIndex(mapNavToPage(index))`。
3. 新页以独立 `QWidget` 子类形式实现，不侵入 `MainWindow` 既有信号槽。
4. 模拟状态统一经 `SimulationWorkflow` 或 `MainWindow` 持有的服务，不在页控件内自建副本。
5. 新页标题或图例必须包含"模拟"字样（依据 `simulation-policy.md`）。

### 5.2 数据流契约

- 页面控件只持有 UI 状态（高亮、选中索引、折叠），不持有业务状态。
- CURRENT 期间目标/日志来自 `SimulationWorkflow`，missions/devices 暂存于 `MainWindow`；NEXT 必须把四类状态收敛到统一所有者。
- TARGET 中跨页操作通过应用服务处理，不直接跨页引用，也不把 `MainWindow` 作为长期业务状态仓库。

### 5.3 不应做的事

- 不在 MVP 阶段引入 3D 渲染做 MOS 分析（纯 2D QPainter）。
- 不在 shell 之外另开独立窗口承载业务（除非用户明确授权）。
- 不为尚未实现的页（统计/配置/真实设备控制台）创建大而空的目录树。

---

## 6. 状态规范

所有业务面板需覆盖以下状态，不在每页单独约定：

| 状态 | 触发 | UI 表现 |
|------|------|---------|
| Empty | 无数据加载/无选中目标 | 居中提示 + 引导按钮（"前往探测页"/"加载模拟数据"） |
| Loading | 算法/识别服务运行中 | 控件置灰 + 简短"规划中..."/"识别中..."文案 |
| Error · 无结果 | 算法返回 `valid=false` | 红色警告图标 + 原因 + 建议动作（放宽参数/查看下一档） |
| Simulation-only | 所有模拟数据展示位 | 标题或图例含"模拟"字样，状态栏 `setSimulationMode(true)` 常亮 |

错误提示一律内联（红/黄边框 + tooltip），不使用模态弹窗打断操作流。

---

## 7. 设计系统要点

详细色板/字体/组件规格见 `docs/前端UI详细设计文档.md`（2024 V2.1，作为参考，不复制全文）。本文档只固定 shell 必须遵守的最小集：

- 背景：`#1E1E1E`；面板：`#252526`；边框：`#3A3A3A`。
- 主色：军绿 `#4A7A4C`；告警：红 `#D32F2F`；MOS 候选：绿半透明 `rgba(0,200,83,30%)`。
- 字体：标题 16px / 正文 14px / 辅助 12px；中文字体依赖系统 Noto CJK。
- 间距基准：4px；面板内边距 8px；分割条 1px。
- 跑道/MOS 坐标系：本地跑道坐标系，原点跑道一端中心，X 长度 / Y 宽度，单位米。

新增颜色/字号必须经本文档更新，不允许单页自行扩展。

---

## 8. 当前页面对应表

| 导航项 | 实现状态 | 中心区内容 | 备注 |
|--------|---------|-----------|------|
| 态势(0) | 部分 | 当前固定为探测页内容 | 切换未实现 |
| 探测(1) | 已实现 | VideoStreamPanel + InfoArea | 唯一交互流程入口 |
| 决策(2) | 未实现 | - | 后续功能评审后确定内容 |
| 设备(3) | 未实现 | - | `DeviceControlView` 已编译未接入 |
| 统计(4) | 未实现 | - | - |
| 配置(5) | 未实现 | - | - |

---

## 9. Feature-Delta 集成规则

新功能进入 UI 时按以下顺序落：

1. **数据先行**：扩展 `Types.h` 或新增 Core 数据结构，UI 不得先于数据存在。
2. **按需隔离外部能力**：只有获批功能确有外部边界时才定义最小接口；模拟实现使用 `Mock*`，注释标注"模拟/占位"。
3. **页面挂点**：复用导航 `navigationChanged` 信号 + `QStackedWidget`，不新建顶层窗口。
4. **状态归一**：业务状态进入统一工作流或状态仓库，UI 仅回读。
5. **文案合规**：所有模拟入口/结果含"模拟"字样；真实接入须先经用户授权（`AGENTS.md` 安全边界）。
6. **文档同步**：新页接入后更新本文档第 8 节；shell 变更同步第 2、3 节。

---

## 10. 待评审问题

- Q1：目标导航最终保留 6 项，还是按批准的业务工作区重新收敛？
- Q2：`TargetDetailPanel`、`DecisionView`、`DeviceControlView` 是继续演进还是删除后重建？
- Q3：左侧目标列表在哪些页面常驻，哪些页面应使用独立上下文？
- Q4：当前工具栏中的装饰性文字入口应删除、隐藏还是逐项实现？
- Q5：UXR/MOS 获批后分别进入现有"探测/决策"工作区，还是新增独立页面？

---

**文档结束**
