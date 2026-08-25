# 应用程序壳设计

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](README.md)
设计系统：[docs/ui/design-system.md](design-system.md)
CURRENT 来源：[`src/MainWindow/MainWindow.cpp`](../../src/MainWindow/MainWindow.cpp)、[`src/MainWindow/NavigationWidget.cpp`](../../src/MainWindow/NavigationWidget.cpp)、[`src/MainWindow/StatusBarWidget.cpp`](../../src/MainWindow/StatusBarWidget.cpp)、[`include/Common/GlobalStyle.h`](../../include/Common/GlobalStyle.h)

> 本文定义六页 UI 共用的应用程序壳：窗口、区域比例、导航栏、菜单栏、工具栏、状态栏。壳是页面容器，页面内部控件规格见各页 `pages/<page>.md`（详见 `pages/index.md` 一览表）。所有视觉值必须取自 `design-system.md`。

## 1. 窗口

| 属性 | 值 | CURRENT 出处 |
|------|----|------|
| 标题 | `排弹抢修指挥系统 V1.0` | `MainWindow.cpp` `setWindowTitle(tr("排弹抢修指挥系统 V1.0"))` |
| 最小尺寸 | `1280 x 720` | `MainWindow.cpp` `setMinimumSize(1280, 720)` |
| 默认尺寸 | `1920 x 1080` | `MainWindow.cpp` `resize(1920, 1080)` |
| 背景 | `--color-bg` | `MainWindow.cpp` `centralWidget->setStyleSheet("background-color: #1E1E1E;")` |

CURRENT 窗口标题前缀与早期参考图（历史 `image1.png`，已删除）标注的“排弹抢修系统指挥席 V1.0”略有差异，本试点以源码为准。

## 2. 区域布局

CURRENT 主区域为 `QHBoxLayout`：导航栏（80px）+ `QStackedWidget` 主页面栈（`m_pageStack`，objectName `mainPageStack`，三页：0 态势工作区 = 左面板 + 设备资源条 + 地图工具栏 + 战术地图主舞台；1 探测 `DetectionView`；2 决策 `DecisionView`，`mainLayout->addWidget(m_pageStack, 1)`），`setContentsMargins(0,0,0,0)`、`setSpacing(0)` 无外边距。下图为态势页原型基线布局；探测/决策页内部布局见 `pages/detection.md` 与 `pages/decision.md`。

```
┌────────────────────────────────────────────────────────────────────┐
│ 菜单栏 (高 30px)                                                      │
├────────────────────────────────────────────────────────────────────┤
│ 工具栏 (高 32px, 不可移动)                                            │
├────┬──────────────┬──────────────────────────┬──────────────────────┤
│ 导 │  左面板       │  中心区                    │  右面板              │
│ 航 │  (宽 320px)   │  ┌────────────────────┐   │  (宽 360px,         │
│ 栏 │              │  │ 视频流区 (stretch 3) │   │   弹性 360–420px)   │
│    │              │  ├────────────────────┤   │                      │
│ 80 │              │  │ 信息区 (stretch 2)   │   │                      │
│ px │              │  │  ├ 信息面板头 28px ├ │   │                      │
│    │              │  │  ├ 告警|探测 (1:1)  │ │   │                      │
│    │              │  │  └ 批量操作条(隐藏) │ │   │                      │
│    │              │  └────────────────────┘   │                      │
├────┴──────────────┴──────────────────────────┴──────────────────────┤
│ 状态栏 (宿主 28px, 内容 22px)                                         │
└────────────────────────────────────────────────────────────────────┘
```

> [CURRENT 漂移] 上图为原型布局。CURRENT 主窗口实际为：导航栏 + `QStackedWidget` 三页（0 态势、1 探测 `DetectionView`、2 决策 `DecisionView`）。态势页内为左面板 320px + 设备资源条 + 地图工具栏 + 战术地图主舞台；右面板与信息区（告警/探测控制/批量操作条）未在当前主窗口实例化（源文件保留，见 [UI.md](../UI.md)），本节相关行仅描述原型基线。

### 2.1 区域尺寸与比例

| 区域 | 尺寸 | 比例 | CURRENT 出处 |
|------|------|------|------|
| 导航栏 | 宽 80px 固定 | - | `NavigationWidget.cpp` `setFixedWidth(80)` |
| 左面板 | 宽 320px 固定 | - | `GlobalStyle.h` `Sizes::LeftPanelWidth` |
| 中心区 | 弹性 | stretch 1 | 原型基线（CURRENT 未实例化，见第 2 节漂移注） |
| 右面板 | 宽 360px 固定；360–420px 弹性（TARGET） | - | `GlobalStyle.h` `Sizes::RightPanelWidth` 常量保留（面板未实例化，见第 2 节漂移注） |
| 视频流区 | 弹性 | stretch 3 | 原型基线（CURRENT 未实例化，见第 2 节漂移注） |
| 信息区 | 弹性 | stretch 2 | 原型基线（CURRENT 未实例化，见第 2 节漂移注） |
| 信息面板头 | 高 28px 固定 | - | 原型基线（CURRENT 未实例化，见第 2 节漂移注） |
| 告警区 | 弹性 | stretch 1 | 原型基线（CURRENT 未实例化，见第 2 节漂移注） |
| 探测控制区 | 弹性 | stretch 1 | 原型基线（CURRENT 未实例化，见第 2 节漂移注） |
| 批量操作条 | 隐藏（默认） | - | 原型基线（CURRENT 未实例化，见第 2 节漂移注） |
| 状态栏 | 宿主 28px / 内容 22px 固定 | - | `MainWindow.cpp` `kStatusBarHostHeight=28` / `kStatusBarContentHeight=22` |
| 菜单栏 | 高 30px | - | `GlobalStyle.h` `Sizes::MenuBarHeight` |
| 工具栏 | 高 32px 固定 | - | `MainWindow.cpp` `createMapToolbar`（`m_mapToolbar->setFixedHeight(kMapToolbarHeight)`，`kMapToolbarHeight=32`） |

> 注：`GlobalStyle.h` 声明 `StatusBarHeight=28`，但 `StatusBarWidget.cpp` 内容 `setFixedHeight(22)`，宿主 `QStatusBar` 28px 含内边距。以源码实际渲染为准。

### 2.2 区域分隔

- 导航栏与左面板间：导航栏右侧 1px 边框（`NavigationWidget.cpp` `border-right`）。
- 中心区 splitter 手柄：垂直 1px、水平 1px，背景 `--color-border`。
- 信息面板头：底部 1px 边框 `--color-border`，背景 `--color-toolbar`。
- 状态栏：顶部 1px 边框 `--color-border`。

## 3. 导航栏

固定宽 80px，背景 `--color-bg`，右侧 1px 边框 `--color-border`。从上到下：UXO logo（高 40px，主色，18px 字号加粗，字间距 2px）→ 16px 间距 → 6 个导航按钮 → 弹性留白。

### 3.1 导航项清单

CURRENT 通过 `QStackedWidget` 路由：index0=态势 live、index1=探测 live（`DetectionView`）、index2=决策 live（`DecisionView`），index3/4/5 未实现独立页面，回退到态势页。`onNavigationChanged` 不再仅 `qDebug`，而是切换 `QStackedWidget` 当前页。本试点原型保持与 CURRENT 一致的路由行为。

每个导航项为一个按钮，固定高 56px，无外边距，左侧 3px 透明边框。图标与文字双行显示，字号 `--font-size-caption`，居中对齐。TARGET 原型自原型基线同步批次起经 `@font-face` 引入 vendored FA 字体渲染实心图标（`--size-icon-nav` 16px），与 CURRENT 图标方案一致；CURRENT Qt 自阶段2 批次9 起为 `QToolButton`（`Qt::ToolButtonTextUnderIcon`，objectName `DEC-NAV-01`..`06`）配 Font Awesome 实心图标，图标映射与状态色见 `design-system.md` 第 8 节。

| ID | 标签 | 图标 | CURRENT index | 用途 | 默认态 | hover | 选中态 |
|----|------|------|------|------|--------|-------|--------|
| `SIT-NAV-LOGO` | UXO | - | - | 仅展示，不可交互 | 主色文字 | 同默认 | 同默认 |
| `SIT-NAV-01` | 态势 | `fa_map_location_dot` | 0 | 态势 live 页面（默认选中） | 透明背景、辅助色文字 | 背景 `--color-row-hover`、主文本色 | 背景 `--color-selection`、主色左边框 3px、主文本色、加粗 |
| `SIT-NAV-02` | 探测 | `fa_satellite_dish` | 1 | 探测 live 页面（`DetectionView`，见 `pages/detection.md`） | 同上 | 同上 | 同上 |
| `SIT-NAV-03` | 决策 | `fa_scale_balanced` | 2 | 决策 live 页面（`DecisionView`，MOS P0） | 同上 | 同上 | 同上 |
| `SIT-NAV-04` | 设备 | `fa_microchip` | 3 | 未实现独立页面（回退态势占位） | 同上 | 同上 | 同上 |
| `SIT-NAV-05` | 统计 | `fa_chart_column` | 4 | 未实现独立页面（回退态势占位） | 同上 | 同上 | 同上 |
| `SIT-NAV-06` | 配置 | `fa_gear` | 5 | 未实现独立页面（回退态势占位） | 同上 | 同上 | 同上 |

### 3.2 交互

| 字段 | 值 |
|------|----|
| 点击结果 | 切换该项为选中态，左侧 3px 主色边框出现，背景变为 `--color-selection`；CURRENT 通过 `QStackedWidget` 路由：`SIT-NAV-01` -> index0（态势 live）、`SIT-NAV-02` -> index1（探测 live `DetectionView`）、`SIT-NAV-03` -> index2（决策 live `DecisionView`）、其余 -> 回退态势页。 |
| 键盘 | Tab 聚焦到按钮；Enter/Space 触发点击。 |
| 原型行为 | 当前页项渲染为不可跳转的占位项；已实现页面（探测/决策）为真实 `<a>` 链接互跳，语义同 CURRENT 的 `QStackedWidget` 路由。`SIT-NAV-04`/`05`/`06` 为占位项：点击仅切换选中高亮并以 tooltip 提示"未实现页面（占位）"，不发生路由、无中心区回退渲染。 |
| CURRENT 映射 | `NavigationWidget.cpp` `setupUi`、`setCurrentIndex`、`updateSelection`、`applyNavIcon`（批次9 图标状态色重建）；`MainWindow.cpp` `onNavigationChanged`（切换 `QStackedWidget` 当前页） |
| 安全 | 无设备控制、无副作用 |

## 4. 菜单栏

高 30px，背景 `--color-menu`，下边框 1px `--color-border`，主文本色，字号 `--font-size-body`。菜单项内边距 `6px 12px`，选中背景 `--color-border`，按下背景 `--color-primary`。

CURRENT 共 5 个顶级菜单。本试点仅文档化菜单结构与行为，不在 HTML 原型中实现完整下拉（下拉属于交互细节，留待后续任务）。

### 4.1 菜单清单

下表给每个**保留**或**禁用占位**的菜单项分配稳定 `SIT-*` ID；**省略**项不分配 ID，原型中不渲染。下拉菜单本身在 HTML 原型中不实现完整弹出层（留待后续任务），ID 仅用于菜单项存在性与禁用态校验。

| ID | 顶级菜单 | 子项 | 快捷键 | CURRENT 行为 | TARGET 原型行为 |
|----|---------|------|--------|------|---------|
| `SIT-MENU-NEW-TASK` | 文件(&F) | 新建任务 | Ctrl+N | `on_actionNewTask()`（占位） | 禁用并标注“占位” |
| `SIT-MENU-OPEN-PLAN` | | 打开预案 | Ctrl+O | `on_actionOpenPlan()`（占位） | 禁用并标注“占位” |
| `SIT-MENU-SAVE-PLAN` | | 保存方案 | Ctrl+S | `on_actionSavePlan()`（占位） | 禁用并标注“占位” |
| `SIT-MENU-EXIT` | | 退出 | Ctrl+Q | `on_actionExit()` | 保留（窗口关闭） |
| `SIT-MENU-VIEW-LEFT` | 视图(&V) | 显示左侧面板 | - | `on_actionViewLeftPanel()` 切换左面板可见 | 保留 |
| `SIT-MENU-VIEW-RIGHT` | | 显示右侧面板 | - | `on_actionViewRightPanel()` 切换右面板可见 | 保留 |
| `SIT-MENU-VIEW-STATUS` | | 显示状态栏 | - | `on_actionViewStatusBar()` 切换状态栏可见 | 保留 |
| `SIT-MENU-VIEW-TOP` | | 视角 -> 顶视 | - | lambda 调 `situationView()->topView()` | 保留（同 `SIT-RP-TOP`） |
| `SIT-MENU-VIEW-SIDE` | | 视角 -> 侧视 | - | lambda 调 `situationView()->sideView()` | 保留（同 `SIT-RP-SIDE`） |
| `SIT-MENU-SETTINGS` | 工具(&T) | 系统设置 | - | `on_actionSystemSettings()`（占位） | 禁用并标注“占位” |
| - | | 历史回放 | - | lambda 空 | 省略 |
| - | | 日志查看 | - | lambda 空 | 省略 |
| - | | 数据同步 | - | lambda 空 | 省略 |
| - | 设备(&D) | 打开设备控制台 | - | lambda 空 | **省略**（占位且无实际效果，详见第 7 节） |
| `SIT-MENU-ABOUT` | 帮助(&H) | 关于 | - | `on_actionAbout()` | 保留（关于对话框） |

### 4.2 菜单项交互

| 字段 | 值 |
|------|----|
| 默认态 | 顶级菜单文本 `--color-text-primary`、`--font-size-body`、内边距 `6px 12px`；子项背景 `--color-menu`、主文本色 |
| hover | 顶级菜单背景 `--color-border`；子项背景 `--color-border` |
| focus | 同 hover（键盘聚焦） |
| active（按下） | 顶级菜单背景 `--color-primary` |
| disabled | 禁用占位项（`SIT-MENU-NEW-TASK`/`SIT-MENU-OPEN-PLAN`/`SIT-MENU-SAVE-PLAN`/`SIT-MENU-SETTINGS`）：文本 `--color-text-disabled`，不可点击，附 tooltip“占位控件，未实现” |
| 点击结果 | 保留项触发对应槽；禁用项无响应；省略项不渲染 |
| 键盘 | Alt+F/V/T/H 展开对应顶级菜单；Down/Up 在子项间移动；Enter 触发（保留项）或无响应（禁用项） |
| 原型行为 | HTML 原型不实现完整下拉弹出层；仅渲染 5 个顶级菜单为可点击文本，附 tooltip 标注其包含的子项与禁用/省略状态。完整下拉交互属后续任务。 |
| CURRENT 映射 | `MainWindow.cpp` `createMenuBar`（第 176-204 行） |
| 安全 | `SIT-MENU-VIEW-TOP`/`SIT-MENU-VIEW-SIDE` 仅切换本地三维相机视角，无设备控制；`SIT-MENU-EXIT` 触发窗口关闭，无外部副作用 |

## 5. 工具栏

高 32px 固定（`kMapToolbarHeight=32`）；CURRENT 地图工具栏为 `QWidget`（`m_mapToolbar`），非 `QToolBar`。

CURRENT 工具栏为 7 个 `QPushButton`：探测控制组（重置/开始/结束）启用并连接真实槽（`onResetDetection`/`onStartDetection`/`onStopDetection`，connects 第 383-386 行）；视角复位/图层/测量/坐标拾取为禁用占位（`setEnabled(false)` 第 343/351/359/367 行，`onResetViewClicked` 第 933 行不可达）。早期参考图中“同步状态/书签/设备控制台”CURRENT 未实现，原型按省略处理（见 5.1 与第 7 节）。

### 5.1 工具栏项清单

| ID | 标签 | 类型 | CURRENT 行为 | TARGET 原型行为 |
|----|------|------|------|---------|
| `SIT-TB-DET-RESET` | 重置 | QPushButton（启用） | 触发 `onResetDetection()`：停止视频、复位遥测模拟器、清空目标/检测框/航迹与探测页结果 | 启用，点击状态栏追加 `[模拟] 探测已重置` 告警（详见 `pages/situation.md`） |
| `SIT-TB-DET-START` | 开始 | QPushButton（启用） | 触发 `onStartDetection()`：播放视频并启动遥测模拟器 | 启用，点击状态栏追加 `[模拟] 探测已开始` 告警 |
| `SIT-TB-DET-STOP` | 结束 | QPushButton（启用） | 触发 `onStopDetection()`：停止视频并回 0s、停止遥测模拟器 | 启用，点击状态栏追加 `[模拟] 探测已结束，视频回 0s` 告警 |
| `SIT-TB-RESET` | 视角复位 | QPushButton（禁用占位） | 按钮禁用，槽 `onResetViewClicked()` 不可达 | 可点击，JS 模拟复位（画中画/选中/浮层复位，详见 `pages/situation.md`）；CURRENT 禁用为已文档化差异 |
| `SIT-TB-LAYER` | 图层 | QPushButton（禁用占位） | 仅展示，禁用 | 禁用并标注“占位” |
| `SIT-TB-MEASURE` | 测量 | QPushButton（禁用占位） | 仅展示，禁用 | 禁用并标注“占位” |
| `SIT-TB-PICK` | 坐标拾取 | QPushButton（禁用占位） | 仅展示，禁用 | 禁用并标注“占位” |
| `SIT-TB-SYNC` | 同步状态 | - | 未实现（早期参考图视觉占位，`createMapToolbar` 未创建） | 省略 |
| `SIT-TB-BOOKMARK` | 书签 | - | 未实现（早期参考图视觉占位，`createMapToolbar` 未创建） | 省略 |
| `SIT-TB-CONSOLE` | 设备控制台 | - | 未实现（早期参考图视觉占位，`createMapToolbar` 未创建） | **省略**（详见第 7 节） |

CURRENT 工具栏 7 个按钮均附 12px Font Awesome 实心图标（`setIconSize(QSize(12, 12))`），工具栏末端为 `[模拟]` 角标标签；历史 QLabel 占位样式已随重写移除。原型工具栏样式见 `pages/situation.md` §6.3。

### 5.2 工具栏交互

| 字段 | 值 |
|------|----|
| 点击结果 | CURRENT：探测控制组 3 键触发对应槽（本地模拟视频/遥测操作）；视角复位/图层/测量/坐标拾取禁用无响应。原型：探测控制组与视角复位可点击（追加模拟告警/模拟复位），占位按钮无响应。 |
| 键盘 | CURRENT：探测控制组 3 键可聚焦，Enter 触发；禁用占位不可聚焦。原型：占位按钮 Tab 可聚焦、Enter/Space 无操作。 |
| 原型行为 | 探测控制组 3 键为启用按钮（点击在状态栏追加模拟告警）；视角复位可点击执行 JS 模拟复位；图层/测量/坐标拾取以禁用样式呈现并附 tooltip “占位，未实现”（详见 `pages/situation.md` §6.3）。 |
| CURRENT 映射 | `MainWindow.cpp` `createMapToolbar`（第 303-388 行，connects 第 383-386 行） |
| 安全 | 探测控制组仅操作本地模拟视频/遥测；视角复位仅影响本地三维相机视角；均无设备控制 |

## 6. 状态栏

宿主高 28px，内容高 22px，背景 `--color-bg`，顶部 1px 边框 `--color-border`。水平 `QHBoxLayout`，左右内边距 16px，间距 16px。从左到右：设备状态标签 | 分隔符 | 最低电量标签 | 分隔符 | 模拟模式标签 | 分隔符 | 告警滚动区（弹性）| 留白 | 紧急停止按钮。

分隔符为 1px x 18px 的 `QFrame`，背景 `--color-border`。

### 6.1 状态栏控件清单

| ID | 标签 | 类型 | 默认值 | 用途 | 样式 |
|----|------|------|--------|------|------|
| `SIT-SB-DEVICE` | `设备: 3/5 在线` | QLabel（只读） | 来自 `loadMockData` | 显示在线/总数 | 主文本色，字号 `--font-size-caption`，鼠标手型（CURRENT 设置但无点击） |
| `SIT-SB-BATTERY` | `最低电量: 85%` | QLabel（只读） | 来自 `loadMockData` | 显示最低电量百分比 | 颜色随电量变化：>60% `--color-status-online`、20–60% `--color-status-busy`、<20% `--color-status-error`；字号 `--font-size-caption` |
| `SIT-SB-SIM` | `[模拟模式]` | QLabel（只读） | 默认隐藏，`setSimulationMode(true)` 后显示 | 标注当前为本地模拟 | `--color-status-busy` 文本，字号 `--font-size-caption`，加粗 |
| `SIT-SB-ALARM` | 告警滚动区 | QScrollArea（只读） | 来自 `addAlarm` | 横向滚动展示告警 | 透明背景，最小宽 400px，高 18px；告警条样式见下 |
| `SIT-SB-EMERGENCY` | `紧急停止（模拟占位）` | QPushButton | 恒禁用 | **危险占位**，详见第 6.3 与第 7 节 | 见 6.3 |

### 6.2 告警条样式（`SIT-SB-ALARM` 内子项）

每条告警为 QLabel，高 18px，文本色 `--color-status-busy`，字号 `--font-size-caption`，背景 `--color-toolbar`，内边距 `2px 8px`，圆角 `--radius-control`。告警条之间间距 10px。

### 6.3 紧急停止按钮 `SIT-SB-EMERGENCY`

CURRENT 为恒禁用占位钮：固定 128x20px（宽度容纳完整“紧急停止（模拟占位）”文本），字号 11px（小于 `--font-size-caption`，CURRENT 字面量），加粗，tooltip“模拟占位，无实际效果”。阶段2 批次9 起附 `fa_hand` 图标（12px；按钮为禁用占位态，图标渲染 `--color-text-disabled`，映射见 `design-system.md` 第 8 节）。五页 HTML 原型（态势/探测/设备/统计/配置；决策页无急停）同样附 `fa_hand` 图标（`.em-icon` 12px，`--size-icon-action`，随按钮文本色 token 渲染）。

| 状态 | 背景 | 文本 | 边框 |
|------|------|------|------|
| 禁用（恒禁用，唯一可见态） | `--color-toolbar` | `--color-text-disabled` | 1px `--color-border`，圆角 3px |
| 启用基础样式（代码保留，不可达） | `--color-danger` | `--color-text-disabled` | 同上 |

现行内联样式表仅含基础与 `:disabled` 两组规则（历史 hover/pressed 变体已随禁用占位改造移除）。

CURRENT 行为：按钮恒禁用（`setEnabled(false)`），点击与键盘均无响应。`onEmergencyStop` 槽（`QMessageBox::warning` 确认框 + `emergencyStopClicked` 信号）与 `clicked` 连接仍保留在源码中，但因恒禁用不可达；`MainWindow` 亦无该信号消费者，不会停止任何设备。

TARGET 原型行为：**按钮禁用并标注“模拟占位，无实际效果”**，不弹确认框，不发信号。CURRENT 已按此落地（见上）。原因详见第 7 节。

| 字段 | 值 |
|------|----|
| 点击结果 | 禁用，无响应 |
| 键盘 | 不可聚焦 |
| 原型行为 | 显示为禁用态，附 tooltip“模拟占位，无实际效果” |
| CURRENT 映射 | `StatusBarWidget.cpp` `setupUi`（恒禁用 + 占位标注 + tooltip）、`onEmergencyStop`（不可达遗留）、`MainWindow.cpp`（无 `emergencyStopClicked` 消费者） |
| 安全 | **危险占位**：历史可点击版本的确认框文案“所有设备将立即停止”会误导用户认为有真实停止效果；现已恒禁用并标注“模拟占位”，该误导路径不可达。 |

## 7. 遗漏与禁用清单

本试点对 CURRENT 中存在但以下问题的控件做禁用或省略处理：

| 控件 | 问题 | 处理 |
|------|------|------|
| 紧急停止按钮 `SIT-SB-EMERGENCY` | 历史版本可点击、弹确认框并发出无消费者信号，文案“所有设备将立即停止”暗示真实停止效果，违反安全边界；CURRENT 已改为恒禁用占位，确认框路径不可达（见 6.3） | 禁用并标注“模拟占位，无实际效果”（CURRENT 已落地对齐） |
| 设备菜单“打开设备控制台”（无 ID，省略） | lambda 空，无实际效果，且暗示真实设备控制 | 省略菜单项 |
| 工具栏“设备控制台”标签 `SIT-TB-CONSOLE` | 早期参考图视觉占位，CURRENT `createMapToolbar` 未创建 | 省略 |
| 工具栏“同步状态”“书签”标签 `SIT-TB-SYNC`/`SIT-TB-BOOKMARK` | 早期参考图视觉占位，CURRENT `createMapToolbar` 未创建 | 省略 |
| 菜单“历史回放/日志查看/数据同步”（无 ID，省略） | lambda 空 | 省略 |
| 菜单“新建任务/打开预案/保存方案/系统设置” `SIT-MENU-NEW-TASK`/`SIT-MENU-OPEN-PLAN`/`SIT-MENU-SAVE-PLAN`/`SIT-MENU-SETTINGS` | 槽函数占位 | 禁用并标注“占位” |
| 导航 `SIT-NAV-02` | 已实现独立探测页面（`DetectionView`） | 路由到探测 live 页面（详见 `pages/detection.md`） |
| 导航 `SIT-NAV-03` | 已实现独立决策页面（`DecisionView` MOS P0） | 路由到决策 live 页面（详见 `pages/decision.md`） |
| 导航 `SIT-NAV-04`/`05`/`06` | 未实现独立页面 | 保留可点击高亮（点击仅切换选中态，tooltip"未实现页面（占位）"），无中心区回退渲染 |

## 8. 视口适配

| 视口 | 壳行为 |
|------|--------|
| 1280x720 | 导航 80px、左面板 320px、右面板 360px、状态栏 28px 均固定不变；中心区收缩；视频流与信息区按 3:2 比例分配；信息区高度可能不足以容纳告警与探测控制并排，触发 splitter 滚动 |
| 1920x1080 | 默认尺寸；所有区域按 token 比例展开；信息面板头 28px、状态栏 28px 固定 |
| 3840x2160 | 固定区域不变；中心区与右面板弹性区按比例放大；右面板可至 420px（TARGET）；字号与控件尺寸保持固定 px |

CURRENT 在 1280x720 下态势页右面板旧 `DecisionSuggestionPanel` 仍存在约 5px 底部溢出（`UI.md` 第 3 节已知问题）。该问题属于态势页壳，与决策页 `DecisionView` 无关：`DecisionView` 三视口几何 TSV 全部 `overflow_rows=0`（证据 `.omo/evidence/mos-p0-qt-final/REPORT.md`）。TARGET 通过右面板弹性高度修正态势页溢出，prototype 实现属于后续任务。

## 9. CURRENT 映射总结

| 壳元素 | CURRENT 源码位置 |
|--------|------------------|
| 窗口尺寸与标题 | `MainWindow.cpp` `setupUi`（第 136-175 行：标题“排弹抢修指挥系统 V1.0”、最小 1280x720、默认 1920x1080） |
| 菜单栏构造 | `MainWindow.cpp` `createMenuBar`（第 176-204 行） |
| 地图工具栏构造 | `MainWindow.cpp` `createMapToolbar`（第 303-388 行：重置/开始/结束/视角复位/图层/测量/坐标拾取 按钮与 `[模拟]` 角标，部分为禁用占位，按钮图标为批次9 Font Awesome 实心图标；全局工具栏未实现） |
| 主布局与页面栈 | `MainWindow.cpp` `createMainLayout`（第 205-302 行，含态势页装配与 `m_pageStack` 三页：0 态势/1 探测/2 决策） |
| 状态栏宿主 | `MainWindow.cpp` `createStatusBar`（第 389-399 行） |
| 状态栏内容 | `StatusBarWidget.cpp` `setupUi`（第 46-156 行） |
| 紧急停止槽 | `StatusBarWidget.cpp` `onEmergencyStop`（第 200-211 行，恒禁用不可达） |
| 导航栏 | `NavigationWidget.cpp` 全文 |
| 导航切换槽 | `MainWindow.cpp` `onNavigationChanged`（切换 `QStackedWidget` 当前页：index1=探测 live、index2=决策 live、其余回退态势页） |
| 信号连接 | `MainWindow.cpp` `createConnections`（第 400-492 行） |
| 模拟数据加载 | `MainWindow.cpp` `loadMockData`（第 493-540 行） |
