# UI 设计系统

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](README.md)
token 来源（CURRENT）：[`include/Common/GlobalStyle.h`](../../include/Common/GlobalStyle.h)、[`src/Common/GlobalStyle.cpp`](../../src/Common/GlobalStyle.cpp)

> 本文是六页 UI 共用的视觉与组件契约。所有 HTML 原型实现只能取本文列出的 token 值，不得硬编码新值；如需新增 token，必须先在本文登记并标注用途与 CURRENT 出处。CURRENT Qt 实现以 QSS 表达，本文以 CSS 变量表达同一组值，便于 HTML 原型复用。

## 1. 颜色 token

> 阶段3 补登记（2026-08-20）：`GlobalStyle.h` 存量 9 个未入表 token（批次6 A5 完备性缺口）已登记入下列各表，值与头文件逐值核对一致。

### 1.1 背景与面板

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-bg` | `#1E1E1E` | `Colors::Background` | 主窗口背景 |
| `--color-panel` | `#252526` | `Colors::PanelBackground` | 左右侧面板背景、菜单项选中态背景、下拉弹窗背景 |
| `--color-toolbar` | `#2D2D2D` | `Colors::ToolbarBackground` | 工具栏、菜单栏、表头背景 |
| `--color-menu` | `#2D2D2D` | `Colors::MenuBackground` | 菜单背景（与工具栏同色） |
| `--color-video` | `#000000` | `Colors::VideoBackground` | 视频流背景（视频视口底色） |
| `--color-viewport-dark` | `#161616` | `Colors::ViewportDark` | 视口暗底（视频/预览区域） |

### 1.2 强调色

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-primary` | `#4A7A4C` | `Colors::PrimaryGreen` | 主要按钮、进度条、强调色、聚焦边框、选中态下划线 |
| `--color-primary-hover` | `#5A8A5C` | `Colors::PrimaryGreenHover` | 主要按钮 hover |
| `--color-danger` | `#D32F2F` | `Colors::DangerRed` | 危险按钮、紧急停止占位 |
| `--color-danger-hover` | `#B71C1C` | `Colors::DangerRedHover` | 危险按钮 hover |

### 1.3 威胁与优先级语义色

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-threat-high` | `#FF5252` | `Colors::ThreatHigh` | 高威胁标记、P0 优先级、错误状态 |
| `--color-threat-medium` | `#FFB74D` | `Colors::ThreatMedium` | 中威胁标记、P1 优先级、忙碌状态 |
| `--color-threat-low` | `#FFF176` | `Colors::ThreatLow` | 低威胁标记、P2 优先级 |
| `--color-priority-p0` | `#FF5252` | `Colors::PriorityP0` | P0 优先级（别名，值同威胁高色） |
| `--color-priority-p1` | `#FFB74D` | `Colors::PriorityP1` | P1 优先级（别名，值同威胁中色） |
| `--color-priority-p2` | `#FFF176` | `Colors::PriorityP2` | P2 优先级（别名，值同威胁低色） |
| `--color-status-online` | `#4CAF50` | `Colors::StatusOnline` | 设备在线 |
| `--color-status-offline` | `#888888` | `Colors::StatusOffline` | 设备离线 |
| `--color-status-busy` | `#FFB74D` | `Colors::StatusBusy` | 设备忙碌 |
| `--color-status-error` | `#FF5252` | `Colors::StatusError` | 设备错误 |

威胁与优先级共用同一组色阶：高/P0 红、中/P1 橙、低/P2 黄。颜色不得作为唯一信息，必须同时给出文字标签（如“高威胁”“P0”）。

### 1.4 文本与边框

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-text-primary` | `#FFFFFF` | `Colors::TextPrimary` | 主文本 |
| `--color-text-secondary` | `#B8B8B8` | `Colors::TextSecondary` | 辅助文本、未选中标签（阶段2 批次7：#AAAAAA->#B8B8B8，对 `#1E1E1E` 底对比度 4.8:1->约 6.3:1，达 WCAG AA） |
| `--color-text-disabled` | `#888888` | `Colors::TextDisabled` | 禁用文本 |
| `--color-border` | `#3C3C3C` | `Colors::Border` | 控件边框、分隔线、表格网格线 |
| `--color-border-focus` | `#4A7A4C` | `Colors::BorderFocus` | 输入框聚焦边框（与主色同值） |
| `--color-text-dim` | `#666666` | `Colors::TextDim` | 暗淡文本（极小字号辅助信息） |
| `--color-border-light` | `#5A5A5A` | `Colors::BorderLight` | 浅色控件边框（小型按钮） |

### 1.5 交互态扩展色

以下值在 CURRENT QSS 字面量或 `GlobalStyle.h` 常量中出现，本试点登记为 token 以便 HTML 复用：

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-selection` | `#2A3F54` | `Colors::SelectionBackground` | 列表/表格行选中背景 |
| `--color-selection-border` | `#3A5F7A` | `Colors::SelectionBorder` | 选中态边框 |
| `--color-tier-blue` | `#42A5F5` | `Colors::TierBlue` | 档位高亮蓝（区别于威胁色阶） |
| `--color-row-hover` | `#2A2A2A` | `GlobalStyle.cpp` 列表 hover | 列表/表格行 hover 背景 |
| `--color-hover` | `#363636` | `Colors::HoverBackground` | 控件 hover 背景色 |
| `--color-card-selected-border` | `#5B9BD5` | `Colors::CardSelectedBorder` | 卡片选中态边框蓝 |
| `--color-button-gray` | `#4A4A4A` | `Colors::ButtonGray` | 灰色按钮背景（非主要操作按钮；值同 `--color-taxiway` 但语义不同） |
| `--color-row-selected` | `#2E3D2F` | `Colors::RowSelected` | 单一绿色选中态背景（与 `--color-selection` 并存的绿色系行选中） |

### 1.6 场景色

仅用于三维态势视图（Qt3D 场景），不用于控件：

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-ground` | `#2D4A2D` | `Colors::Ground` | 草地 |
| `--color-runway` | `#3D3D3D` | `Colors::Runway` | 跑道 |
| `--color-taxiway` | `#4A4A4A` | `Colors::Taxiway` | 滑行道 |

## 2. 字体 token

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--font-family` | `"Microsoft YaHei", "Source Han Sans SC", "SimHei", sans-serif` | `Fonts::Family` | 全局字体族 |
| `--font-size-title` | `17px` | `Fonts::TitleSize` | 窗口标题、模块标题（阶段2 批次7：16->17） |
| `--font-size-body` | `15px` | `Fonts::BodySize` | 正文、按钮、输入框、表格（阶段2 批次7：14->15） |
| `--font-size-caption` | `13px` | `Fonts::CaptionSize` | 时间戳、状态说明、小字提示（阶段2 批次7：12->13） |
| `--font-weight-title` | `bold` | `Fonts::TitleWeight` | 标题字重 |
| `--font-weight-body` | `normal` | `Fonts::BodyWeight` | 正文字重 |

字体族优先微软雅黑，回退思源黑体、黑体，最后 sans-serif。HTML 原型必须使用相同顺序，避免环境差异导致渲染漂移。

字号覆盖档位（2026-08 批次5 收敛登记）：Qt 侧 `GlobalStyle` 属性词汇提供 `fontSize` 覆盖档位 9/10/11/12/13/14px（`QLabel[fontSize="N"]`）、`fontWeight="bold"` 与等宽字族 `fontFamily="mono"`（`'Consolas','Courier New',monospace`）。档位为存量内联字号的收敛登记（沿用基线已有值，非新增视觉值），用于坐标、徽标、紧凑数值列等小字场景，不随全局字号 token 缩放；批次7 全局字号提升（caption 12->13、body 14->15）后，12/14 档位不再与 `--font-size-caption`/`--font-size-body` 等值，属预期层级变化（小字场景保持原密度）。

## 3. 尺寸 token

### 3.1 窗口与区域

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--size-window-width` | `1920px` | `Sizes::WindowWidth` | 默认窗口宽 |
| `--size-window-height` | `1080px` | `Sizes::WindowHeight` | 默认窗口高 |
| `--size-nav-width` | `80px` | `Sizes::NavigationBarWidth` | 左侧导航栏宽（固定不可拖拽） |
| `--size-left-panel-width` | `320px` | `Sizes::LeftPanelWidth` | 左面板宽（固定不可拖拽） |
| `--size-right-panel-width` | `360px` | `Sizes::RightPanelWidth` | 右面板宽（CURRENT 固定；TARGET 允许 360–420px 弹性，见 application-shell） |
| `--size-status-bar-height` | `22px` | `StatusBarWidget.cpp` `setFixedHeight(22)` | 状态栏高（注意：`GlobalStyle.h` 声明 28，CURRENT 实际渲染 22，以源码为准） |
| `--size-toolbar-height` | `32px` | `Sizes::ToolbarHeight` | 工具栏高 |
| `--size-menu-bar-height` | `30px` | `Sizes::MenuBarHeight` | 菜单栏高 |

### 3.2 控件

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--size-button-min-width` | `80px` | `Sizes::ButtonMinWidth` | 按钮最小宽 |
| `--size-button-height` | `32px` | `Sizes::ButtonHeight` | 按钮高 |
| `--size-icon-button` | `24px` | `Sizes::IconButtonSize` | 图标按钮尺寸 |
| `--size-icon-nav` | `16px` | `NavigationWidget.cpp` `setIconSize(QSize(16,16))` | 导航 FA 字体图标尺寸（阶段2 批次9，见第 8 节） |
| `--size-icon-action` | `12px` | `MainWindow.cpp`/`StatusBarWidget.cpp` `setIconSize(QSize(12,12))` | 地图工具栏/紧急停止 FA 字体图标尺寸（阶段2 批次9，见第 8 节） |
| `--size-toolbar-button-height` | `32px` | `Sizes::ToolbarButtonHeight` | 工具栏按钮高 |
| `--size-target-item-height` | `56px` | `Sizes::TargetItemHeight` | 左面板目标行高 |
| `--size-task-item-height` | `80px` | `Sizes::TaskItemHeight` | 左面板任务行高 |
| `--size-device-item-height` | `64px` | `Sizes::DeviceItemHeight` | 左面板设备行高 |

### 3.3 内边距与圆角

来自 CURRENT QSS 字面量：

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--space-button-pad-y` | `7px` | `getButtonStyle`/`getMainWindowStyle` QPushButton padding | 按钮上下内边距（阶段2 批次8 6->7px） |
| `--space-button-pad-x` | `16px` | 同上 | 按钮左右内边距 |
| `--space-input-pad-y` | `6px` | `getLineEditStyle` padding | 输入框/下拉框上下内边距 |
| `--space-input-pad-x` | `8px` | 同上 | 输入框/下拉框左右内边距 |
| `--space-tab-pad-y` | `8px` | `getTabWidgetStyle` QTabBar padding | 标签页上下内边距 |
| `--space-tab-pad-x` | `16px` | 同上 | 标签页左右内边距 |
| `--space-table-item-pad` | `8px` | `getTableWidgetStyle` QTableWidget::item padding | 表格单元格内边距 |
| `--space-toolbar-gap` | `8px` | `getMainWindowStyle` QToolBar spacing | 工具栏按钮间距 |
| `--space-toolbar-pad` | `8px` | 同上 QToolBar padding | 工具栏内边距 |
| `--radius-control` | `6px` | 按钮/输入框/下拉框 border-radius | 控件圆角（阶段2 批次8 4->6px） |
| `--radius-scroll-handle` | `5px` | `getScrollBarStyle` handle border-radius | 滚动条手柄圆角 |
| `--size-checkbox` | `16px` | `getMainWindowStyle` QCheckBox::indicator | 复选框指示器尺寸 |
| `--size-scrollbar` | `10px` | `getScrollBarStyle` width/height | 滚动条宽/高 |
| `--size-combo-dropdown` | `24px` | `getComboBoxStyle` drop-down width | 下拉箭头区宽 |
| `--min-scroll-handle` | `30px` | `getScrollBarStyle` min-height/min-width | 滚动条手柄最小尺寸 |

> **`--space-panel-gap` 评估结论（阶段2 批次8）**：暂不引入。当前应用壳为零间距贴边格局（`MainWindow` 各布局 `setSpacing(0)`/`setContentsMargins(0,0,0,0)`，面板以 1px 边框分隔），引入面板间距属布局级变更，需外壳与各视图容器联动改造、三视口几何回归面大，且与"信息密度保持军事指控风格"的验收约束冲突；如后续需要呼吸感，另立批次单独评审。

## 4. 动画 token

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--anim-short` | `150ms` | `Animation::DurationShort` | 短动画（hover 反馈） |
| `--anim-normal` | `200ms` | `Animation::DurationNormal` | 标准动画（展开、切换） |
| `--anim-long` | `300ms` | `Animation::DurationLong` | 长动画（页面过渡） |
| `--anim-easing` | `ease-in-out` | `Animation::Easing` | 默认动画曲线 |

HTML 原型的所有过渡必须使用以上时长与曲线，避免出现未登记的动画值。

## 5. 组件原语

以下原语描述每个控件类型在默认/hover/focus/active/disabled 五态下的视觉。控件清单（`pages/situation.md`）按此原语组合，不重复定义五态。

### 5.1 按钮 Button

| 状态 | 背景 | 文本 | 边框 | 备注 |
|------|------|------|------|------|
| 默认（次要） | `--color-toolbar` | `--color-text-primary` | `1px solid --color-border` | 圆角 `--radius-control` |
| 默认（主要） | `--color-primary` | `--color-text-primary` | 无 | `primary` 变体 |
| 默认（危险） | `--color-danger` | `--color-text-primary` | 无 | `danger` 变体 |
| hover | 主要→`--color-primary-hover`；危险→`--color-danger-hover`；次要→`--color-border` | 同上 | 同上 | |
| focus | 同默认 | 同上 | `1px solid --color-border-focus` | 键盘聚焦可见 |
| active（按下） | 回到默认背景色 | 同上 | 同上 | QSS pressed 回到默认色 |
| disabled | `--color-border` | `--color-text-disabled` | 无 | 不可点击 |

按钮统一高 `--size-button-height`，最小宽 `--size-button-min-width`，内边距 `--space-button-pad-y --space-button-pad-x`，字号 `--font-size-body`。

### 5.2 输入框 LineEdit

| 状态 | 背景 | 文本 | 边框 |
|------|------|------|------|
| 默认 | `--color-bg` | `--color-text-primary` | `1px solid --color-border` |
| focus | 同上 | 同上 | `1px solid --color-border-focus` |
| disabled | `--color-toolbar` | `--color-text-disabled` | `1px solid --color-border` |
| placeholder | 同上 | `--color-text-secondary` | 同上 |

圆角 `--radius-control`，内边距 `--space-input-pad-y --space-input-pad-x`，字号 `--font-size-body`。

### 5.3 下拉框 ComboBox

| 状态 | 背景 | 文本 | 边框 |
|------|------|------|------|
| 默认 | `--color-bg` | `--color-text-primary` | `1px solid --color-border` |
| hover | 同上 | 同上 | 同上（CURRENT 无 hover 区分） |
| focus | 同上 | 同上 | `1px solid --color-border-focus` |
| 弹出层 | `--color-panel` | `--color-text-primary` | `1px solid --color-border`，选中项背景 `--color-primary` |
| disabled | `--color-toolbar` | `--color-text-disabled` | `1px solid --color-border` |

下拉箭头区宽 `--size-combo-dropdown`。

### 5.4 表格 TableWidget

| 部分 | 背景 | 文本 | 备注 |
|------|------|------|------|
| 表头 | `--color-toolbar` | `--color-text-primary` | 下边框 `1px solid --color-border`，内边距 `--space-table-item-pad` |
| 行默认 | `--color-bg` | `--color-text-primary` | 单元格内边距 `--space-table-item-pad`，下边框 `1px solid --color-border` |
| 行 hover | `--color-row-hover` | 同上 | |
| 行选中 | `--color-selection` | `--color-text-primary` | 单选为主 |
| 网格线 | `--color-border` | — | `gridline-color` |

CURRENT 表格为单选模式（`LeftPanelWidget` 目标表 `setSelectionMode(QAbstractItemView::SingleSelection)`）。

### 5.5 标签页 TabWidget

| 状态 | 背景 | 文本 | 下边框 |
|------|------|------|--------|
| 未选中 | `--color-toolbar` | `--color-text-secondary` | 无 |
| 选中 | `--color-bg` | `--color-text-primary` | `2px solid --color-primary` |
| hover（未选中） | `--color-border` | `--color-text-secondary` | 无 |

内边距 `--space-tab-pad-y --space-tab-pad-x`，字号 `--font-size-body`。

### 5.6 复选框 CheckBox

| 状态 | 指示器背景 | 指示器边框 |
|------|------|------|
| 未选中 | `--color-bg` | `1px solid --color-border` |
| 选中 | `--color-primary` | `1px solid --color-primary` |
| disabled | `--color-toolbar` | `1px solid --color-border` |

指示器尺寸 `--size-checkbox`，圆角 `3px`。

### 5.7 滚动条 ScrollBar

| 部分 | 背景 | 尺寸 |
|------|------|------|
| 轨道（vertical） | `--color-toolbar` | 宽 `--size-scrollbar` |
| 手柄 | `--color-text-secondary` | 圆角 `--radius-scroll-handle`，最小 `--min-scroll-handle` |
| 手柄 hover | `--color-text-primary` | 同上 |
| add-line/sub-line | 高/宽 0 | CURRENT 隐藏两端按钮 |

### 5.8 列表行（目标/任务/设备）

| 状态 | 背景 | 文本 |
|------|------|------|
| 默认 | `--color-bg` | `--color-text-primary` |
| hover | `--color-row-hover` | 同上 |
| 选中 | `--color-selection` | 同上 |

行高分别取 `--size-target-item-height`、`--size-task-item-height`、`--size-device-item-height`。

### 5.9 状态徽章

用于设备状态、威胁等级、任务状态等小标签。圆角 `--radius-control`，字号 `--font-size-caption`，内边距 `2px 6px`：

| 类型 | 背景 | 文本 |
|------|------|------|
| 在线 | `--color-status-online` | `--color-text-primary` |
| 离线 | `--color-status-offline` | `--color-text-primary` |
| 忙碌 | `--color-status-busy` | `--color-text-primary`（深色背景用白字） |
| 错误 | `--color-status-error` | `--color-text-primary` |
| 高威胁 | `--color-threat-high` | `--color-text-primary` |
| 中威胁 | `--color-threat-medium` | `--color-text-primary` |
| 低威胁 | `--color-threat-low` | `#1E1E1E`（浅黄背景用深字） |

低威胁黄底必须使用深色文本以保证对比度。

### 5.10 进度条 ProgressBar

| 部分 | 背景 |
|------|------|
| 整体 | `--color-bg`，边框 `1px solid --color-border` |
| 已填充 | `--color-primary` |
| 文本 | `--color-text-primary`，居中 |

CURRENT `DecisionSuggestionPanel` 用 `QProgressBar` 显示模拟置信度。

### 5.11 三维视图容器

`SituationView` 是 Qt3DWindow，无 QSS 控件样式。本试点在 HTML 原型中以占位画布表达，背景使用 `--color-ground`，标记用威胁色。容器标题栏高 32px（`RightPanelWidget` 中 section header `setFixedHeight(32)`），背景 `--color-panel`，标题字号 `--font-size-body` 加粗。

### 5.12 决策页属性词汇（Qt 侧收敛登记）

2026-08 批次6 将决策页 `DecisionView` 的内联 QSS 收敛为 `GlobalStyle` 属性词汇，均为存量基线值的等价迁移（非新增视觉值）：`sectionTitle="panel"/"inset"` 区块标题、`chipStyle="warnBadge"/"simTag"` 紧凑徽标（ThreatMedium 描边）、`statusDot="high"/"medium"/"online"/"uxo"/"device"` 状态圆点（uxo 黄 `#FFEB3B` 为存量字面量收敛登记，token 表无等值项）、`slotStyle="title"/"item"` 禁用占位插槽、`btnVariant="zoom"` 缩放按钮、`btnVariant="toolBg"` 工具栏按钮保底底色（ToolbarBackground）、`btnVariant="zoomReset"` 缩放行复位钮（外观同 zoom 但不声明 min-width，宽度回落基础 QPushButton 规则）与 `btnVariant="mainBg"` 主区底色按钮保底（仅声明 Background，用于参数栏复位钮），以及 `QSplitter[containerBg="main"]::handle` 把手底色规则（Background）。HTML 原型不新增使用这些值，状态徽章语义仍以 5.9 与 6.3 为准。

## 6. 状态规则

### 6.1 五态必填

每个可交互区域与控件必须定义 `正常 / 加载 / 空 / 错误 / 禁用` 五态。状态颜色不得作为唯一信息，必须同时给出文字。

| 状态 | 视觉信号 | 文字信号 |
|------|----------|----------|
| 正常 | 默认 token | 有数据 |
| 加载 | 骨架屏或 spinner，区域不可交互 | “加载中…” |
| 空 | 占位图标 + 引导文案 | “暂无数据”/“暂无告警” |
| 错误 | `--color-status-error` 边框 + 图标 | 错误描述 |
| 禁用 | `--color-text-disabled` + 不可点击 | 说明禁用原因 |

### 6.2 模拟标注

所有模拟操作与结果必须带“模拟”或“演示”字样。涉及设备状态、决策建议、告警的内容若来自本地 fixture，必须在区域标题或控件旁标注“模拟数据（只读）”。

### 6.3 威胁与优先级色阶

高/P0 红、中/P1 橙、低/P2 黄三档与设备状态色（在线绿、离线灰、忙碌橙、错误红）不得混用。威胁色只用于目标和任务，状态色只用于设备和系统状态。

## 7. 视口规则

本试点在三种视口下检查布局。token 为固定 px 值（来自 CURRENT），视口适配通过区域弹性比例实现，不缩放 token。

| 视口 | 用途 | 关键约束 |
|------|------|----------|
| 1280x720 | 最小可用 | 区域不溢出；右面板决策区末两行不被截断；左面板表格列宽不裁切文字；导航栏与状态栏尺寸固定不变 |
| 1920x1080 | 默认与权威截图 | 默认设计尺寸；整体图 `images/situation/overview-1920x1080.png` 按此视口生成；中心三维区与告警区按比例填充 |
| 3840x2160 | 4K | 控件密度与间距按 token 保持；中心区与面板按比例放大；不出现大片留白或控件过小；字体仍用固定 px 值 |

### 7.0 决策页（MOS）CURRENT Qt 三视口事实

决策页历史三视口几何证据采集于本轮单档位渲染修正之前，不能作为修正后的 fresh 多视口证据；以下旧证据事实与态势页壳的 1280 已知问题相互独立：

- **零溢出**：1280×720、1920×1080、3840×2160 三视口下，`DecisionView` 窗口级几何 TSV 全部 `overflow_rows=0`（18 份 TSV，113 字节 header-only），24 张 PNG 人工核对无溢出或裁切。证据见 `.omo/evidence/mos-p0-qt-final/REPORT.md`。该验证仅覆盖决策页，不覆盖态势页右面板。
- **缩放公式**：`MosRunwayWidget` 使用 `clamp(min(w/1920, h/1080), 1, 2)` 等比缩放跑道 QPainter 内容，不依赖 DPR（设备像素比）。该缩放仅作用于 `DecisionView` 内部跑道画布，不作用于全局壳（菜单/导航/状态栏/字体）。
- **4K 限制**：全局壳（菜单栏、导航栏、状态栏、字体 token）在 3840×2160 下不按比例放大，这是跨轮次已知模式（pre-existing），非决策页回归。决策页跑道画布缩放上限为 2×。
- **1280 标签换行**：MOS 参数栏部分标签（如"模拟处理假设数""扫描步长"）在 1280×720 下换行但无截断，布局紧凑可用。

CURRENT 态势页在 1280×720 下右面板旧 `DecisionSuggestionPanel` 仍存在约 5px 底部溢出（`UI.md` 第 3 节历史记录）。该问题属于态势页壳，与决策页 `DecisionView` 的零溢出验证无关。TARGET 通过右面板弹性高度与决策区末两行可滚动修正，但 prototype 实现属于后续任务。

### 7.1 弹性区域

- 导航栏宽、左面板宽、状态栏高、菜单栏高、工具栏高在三视口下固定不变。
- 右面板宽：CURRENT 固定 360px；TARGET 允许 360–420px 弹性，默认 360px，4K 下可到 420px。
- 中心区宽高、告警区高、右面板各 section 高度为弹性，按 splitter 比例分配。

### 7.2 字体与控件尺寸

字体与控件 token 为固定 px，不随视口缩放。4K 下通过增加中心区留白与三维场景视野范围适配，不放大字号。

### 7.3 决策页（MOS）CURRENT Qt 实现映射

以下 CURRENT Qt 事实仅适用于决策页 `DecisionView`，不修改六页 TARGET token 契约；态势页等其余五页仍以本节 §1–§6 的 TARGET token 为准。

- **档位选中语义（checked tier）**：`MosParamsPanel` 与候选方案卡片使用互斥 checked 选中态。选中档位：蓝色高亮 `--color-tier-blue` 背景 + 边框；未选中档位（enabled-but-unchecked）：中性默认色，可点击切换。`no-solution` 状态下：无可行档位 `DEC-TB-PLAN-1` **禁用**（dimmed gray），更高可行档位（如 `DEC-TB-PLAN-3`）**启用但不 checked**（中性可选替代，非当前选择）。该语义由 `tierSelectionCheckedStateIsUnambiguous` 测试锁定，避免"档位3 被误读为当前选中"的歧义。
- **参数栏字段尺寸**：`MosParamsPanel` 输入框字段高 22px、标签字号 11px（小于 `--font-size-caption` 13px，DecisionView 本地字面量）。该尺寸仅用于决策页参数栏，不替换 §2/§3.2 的全局字体/控件 token。
- **生成器模态响应式**：`MosGeneratorDialog` 固定 1012×700px，在三视口下均完整可见（4K 下大量留白，不缩放）。底部三按钮（下载 JSON / 取消 / 应用生成）始终可见，不裁切。
- **本地/合成边界**：`DecisionView` 全部数据为本地种子化 fixture（`mulberry32` 确定性生成），`MosRunwayWidget` QPainter 自绘跑道/弹坑/合成避让几何/MOS 矩形；JSON 导出仅通过 `QSaveFile` 向明确本地路径单向写入合成 fixture 工件，不提供 import/reload/运行时持久化/外部集成通道；不联网、不写入数据库、不控制设备。
- **单档位渲染**：P0 仅渲染当前模拟选择档位（`m_selectedTier`）的 MOS 矩形，不渲染未选中档位；全档位叠加对比视图为 P1 Draft（见 `docs/features/mos-planning.md` MOS-008）。选中档位中属于该档位 `repairedIds` 的障碍物以 `Qt::DashLine` 虚线轮廓 + 斜十字标记绘制，模拟"已处理"假设，不暗示真实修复或安全结论。
- **各向同性米坐标系**：`MosRunwayWidget` 使用单一各向同性像素/米比例 `pxPerM = min(pxPerMX, pxPerMY)`（X/Y 共享），不使用 HTML 原型的独立叠层分离。障碍物影响圆像素半径 `obstacleRadiusPx = influenceRadius × pxPerM`（米坐标 × 各向同性比例，无钳制/系数），paint 与 hitTest 共用同一半径。
- **参数初始值**：`MosParamsPanel` 的跑道长度 L 初始值为 300（范围 100..6000），最小起降长度 minLength 初始值为 100（范围 1..6000），均为本地测试阶段可调初始值，非永久领域/机型/安全默认值。HTML 原型表格中的 3000/460 为 TARGET 展示值（MOS-006 `L_min=460` 示例），与 Qt 当前初始值不同。

## 8. 图标体系（阶段2 批次9，CURRENT Qt 登记）

CURRENT Qt 客户端图标采用 Font Awesome 6 Free 实心字形：第三方库 vendored 于 `third_party/QtAwesome/`（静态库 `libQtAwesome.a`，顶层 `CMakeLists.txt` `add_subdirectory` 接入），经 `UiIcons` 助手（`include/MainWindow/UiIcons.h`）统一封装。业务控件只依赖 `UiIcons` 与 `GlobalStyle` 颜色令牌，不感知第三方库实例生命周期（单例挂 `qApp` 随应用销毁）。

- `UiIcons::init()`：幂等初始化，须在 `QApplication` 构建后调用（`MainWindow` 构造期执行）；字体加载失败时返回 false，`icon()` 返回空 `QIcon`，控件降级为纯文本，不阻塞启动。
- `UiIcons::navGlyph(index)`：导航 6 项码点（0..5，越界返回 0）。
- `UiIcons::icon(character, color, active, disabled)`：以 `fa::fa_solid` 风格生成图标；`color`/`active`/`disabled` 对应常规/悬停/禁用状态色，无效 `QColor` 表示不生成该状态。

HTML 原型本轮未接入 FA 字体，维持既有文本字形方案（如导航 ◎ 近似、✓/× 等符号）；如后续需要原型对齐，须先在本节评审映射后再实现。

### 8.1 图标映射（14 枚）

| 区域 | 控件 | 字形 | CURRENT 出处 |
|------|------|------|------|
| 导航 | 态势 | `fa_map_location_dot` | `UiIcons.cpp` `kNavGlyphs[0]` |
| 导航 | 探测 | `fa_satellite_dish` | `kNavGlyphs[1]` |
| 导航 | 决策 | `fa_scale_balanced` | `kNavGlyphs[2]` |
| 导航 | 设备 | `fa_microchip` | `kNavGlyphs[3]` |
| 导航 | 统计 | `fa_chart_column` | `kNavGlyphs[4]` |
| 导航 | 配置 | `fa_gear` | `kNavGlyphs[5]` |
| 地图工具栏 | 重置 | `fa_rotate_right` | `MainWindow.cpp` `createMapToolbar` |
| 地图工具栏 | 开始 | `fa_play` | 同上 |
| 地图工具栏 | 结束 | `fa_stop` | 同上 |
| 地图工具栏 | 视角复位 | `fa_expand` | 同上 |
| 地图工具栏 | 图层 | `fa_layer_group` | 同上 |
| 地图工具栏 | 测量 | `fa_ruler` | 同上 |
| 地图工具栏 | 坐标拾取 | `fa_location_crosshairs` | 同上 |
| 状态栏 | 紧急停止（模拟占位，恒禁用） | `fa_hand` | `StatusBarWidget.cpp` |

### 8.2 尺寸与状态色约定

- **尺寸**：导航图标 `--size-icon-nav`（16px，`QToolButton` 高 56px、`Qt::ToolButtonTextUnderIcon` 图上文字下双行布局）；地图工具栏与紧急停止图标 `--size-icon-action`（12px）。
- **状态色**：`QIcon` 的状态色无法由 QSS 驱动，图标色跟随所在控件文本色令牌，状态切换时由代码重建图标（导航在 `NavigationWidget::applyNavIcon`，随选中态 repolish 一并重建）：
  - 导航：未选中 `--color-text-secondary`，选中与悬停（color-active）`--color-text-primary`，与 navBtn QSS 文本色对齐（计划 2.2 草案曾拟"选中态主色"，实现取 QSS 文本色对齐以与按钮文字一致）。
  - 地图工具栏 flat 按钮与紧急停止占位钮：常规 `--color-text-primary`、禁用 `--color-text-disabled`（不设 color-active），与 flat/占位 QSS 文字色对齐；紧急停止恒为禁用态，实际渲染禁用色（计划 2.2 草案"危险操作红色"不适用于禁用占位钮，红色仅存在于其启用态样式表）。
- **语义约束**：图标不得作为唯一信息载体，挂载点均保留文字标签（导航双行、工具栏图标与文字并排）；紧急停止为模拟占位（禁用、不可点击），`fa_hand` 仅作视觉标记，不改变安全边界（见 `docs/ui/README.md` 第 5 节）。

## 9. 深度投影（REQ-010 阶段3，CURRENT Qt 登记）

**实现事实**：QSS 不支持 `box-shadow`，投影由 `QGraphicsDropShadowEffect` 消费 `GlobalStyle::Elevation` 常量实现。六页 HTML 原型本轮无 box-shadow/elevation 消费点，无原型同步项。

| Token | 值 | GlobalStyle 常量 | 消费端 |
|---|---|---|---|
| `--elevation-overlay` | `0 4px 12px rgba(0,0,0,0.4)` | `Elevation::OverlayBlurRadius=12 / OverlayOffsetY=4 / OverlayShadowAlpha=102` | 态势页 PiP（`videoPiP`）与目标详情浮层（`targetDetailOverlay`）的阴影底衬 |
| `--elevation-modal` | `0 8px 24px rgba(0,0,0,0.5)` | `Elevation::ModalBlurRadius=24 / ModalOffsetY=8 / ModalShadowAlpha=128` | 模拟损毁分布生成器对话框内容卡片 `DEC-GEN-CARD` |

### 9.1 实现方式与已知限制

- **浮动面板阴影底衬（`FloatingShadowLayer`，`MainWindow.cpp` 匿名命名空间）**：`VideoStreamPanel` 子树含原生 `QVideoWidget`，effect 的栅格渲染路径无法捕获原生子窗口内容（视频黑屏风险），故投影由与浮层同几何的底衬承载：底衬挂 Overlay 档 effect，浮层完全遮盖底衬本体，仅四周投影可见。底衬几何与显隐由 `repositionFloatingWidgets` 同步；浮层自行隐藏（关闭按钮/重置）经 `MainWindow::eventFilter` 的 Hide 事件同步隐藏底衬。
- **交换模式已知限制**：视频全屏、地图浮窗（`m_videoIsMain`）时，底衬位于原生视频窗口之下会被遮挡，浮窗此轮不投影。
- **模态对话框**：`DEC-GEN-MODAL` 无边框化（`Qt::FramelessWindowHint` + `WA_TranslucentBackground`，去掉系统浅灰标题栏），窗口 `568x424` 四周留 24/36px 投影泄露区（左/右/上 24px、下 36px，Modal 档 blur 24 + offsetY 8），内容卡片 `DEC-GEN-CARD`（内区宽 520 参考尺寸不变，高 364 = 原 360 加 4px 余量吸收卡片 1px 描边的盒模型占用，`PanelBackground` 底 + 1px `Border` + 3px 圆角）挂 Modal 档 effect。不支持拖拽移动（模态居中展示），记录为已知限制。视口缩放：窗口与卡片几何随 `viewportScale` 等比缩放（窗口 568x424、卡片边距/内区 24/520/364 × scale，由 `DecisionViewLayout.cpp` 的 `applyViewportScale` 统一消费，遵循 §7 三视口规则）。
