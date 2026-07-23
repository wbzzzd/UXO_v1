# UI 设计系统

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](README.md)
token 来源（CURRENT）：[`include/Common/GlobalStyle.h`](../../include/Common/GlobalStyle.h)、[`src/Common/GlobalStyle.cpp`](../../src/Common/GlobalStyle.cpp)

> 本文是六页 UI 共用的视觉与组件契约。所有 HTML 原型实现只能取本文列出的 token 值，不得硬编码新值；如需新增 token，必须先在本文登记并标注用途与 CURRENT 出处。CURRENT Qt 实现以 QSS 表达，本文以 CSS 变量表达同一组值，便于 HTML 原型复用。

## 1. 颜色 token

### 1.1 背景与面板

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-bg` | `#1E1E1E` | `Colors::Background` | 主窗口背景 |
| `--color-panel` | `#252526` | `Colors::PanelBackground` | 左右侧面板背景、菜单项选中态背景、下拉弹窗背景 |
| `--color-toolbar` | `#2D2D2D` | `Colors::ToolbarBackground` | 工具栏、菜单栏、表头背景 |
| `--color-menu` | `#2D2D2D` | `Colors::MenuBackground` | 菜单背景（与工具栏同色） |

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
| `--color-status-online` | `#4CAF50` | `Colors::StatusOnline` | 设备在线 |
| `--color-status-offline` | `#888888` | `Colors::StatusOffline` | 设备离线 |
| `--color-status-busy` | `#FFB74D` | `Colors::StatusBusy` | 设备忙碌 |
| `--color-status-error` | `#FF5252` | `Colors::StatusError` | 设备错误 |

威胁与优先级共用同一组色阶：高/P0 红、中/P1 橙、低/P2 黄。颜色不得作为唯一信息，必须同时给出文字标签（如“高威胁”“P0”）。

### 1.4 文本与边框

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-text-primary` | `#FFFFFF` | `Colors::TextPrimary` | 主文本 |
| `--color-text-secondary` | `#AAAAAA` | `Colors::TextSecondary` | 辅助文本、未选中标签 |
| `--color-text-disabled` | `#888888` | `Colors::TextDisabled` | 禁用文本 |
| `--color-border` | `#3C3C3C` | `Colors::Border` | 控件边框、分隔线、表格网格线 |
| `--color-border-focus` | `#4A7A4C` | `Colors::BorderFocus` | 输入框聚焦边框（与主色同值） |

### 1.5 交互态扩展色

以下值在 CURRENT QSS 中作为字面量出现，本试点登记为 token 以便 HTML 复用：

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--color-selection` | `#2A3F54` | `GlobalStyle.cpp` `getMainWindowStyle`/`getTableWidgetStyle` 列表选中态 | 列表/表格行选中背景 |
| `--color-row-hover` | `#2A2A2A` | `GlobalStyle.cpp` 列表 hover | 列表/表格行 hover 背景 |

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
| `--font-size-title` | `16px` | `Fonts::TitleSize` | 窗口标题、模块标题 |
| `--font-size-body` | `14px` | `Fonts::BodySize` | 正文、按钮、输入框、表格 |
| `--font-size-caption` | `12px` | `Fonts::CaptionSize` | 时间戳、状态说明、小字提示 |
| `--font-weight-title` | `bold` | `Fonts::TitleWeight` | 标题字重 |
| `--font-weight-body` | `normal` | `Fonts::BodyWeight` | 正文字重 |

字体族优先微软雅黑，回退思源黑体、黑体，最后 sans-serif。HTML 原型必须使用相同顺序，避免环境差异导致渲染漂移。

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
| `--size-toolbar-button-height` | `32px` | `Sizes::ToolbarButtonHeight` | 工具栏按钮高 |
| `--size-target-item-height` | `56px` | `Sizes::TargetItemHeight` | 左面板目标行高 |
| `--size-task-item-height` | `80px` | `Sizes::TaskItemHeight` | 左面板任务行高 |
| `--size-device-item-height` | `64px` | `Sizes::DeviceItemHeight` | 左面板设备行高 |

### 3.3 内边距与圆角

来自 CURRENT QSS 字面量：

| Token | 值 | CURRENT 出处 | 用途 |
|------|----|------|------|
| `--space-button-pad-y` | `6px` | `getButtonStyle`/`getMainWindowStyle` QPushButton padding | 按钮上下内边距 |
| `--space-button-pad-x` | `16px` | 同上 | 按钮左右内边距 |
| `--space-input-pad-y` | `6px` | `getLineEditStyle` padding | 输入框/下拉框上下内边距 |
| `--space-input-pad-x` | `8px` | 同上 | 输入框/下拉框左右内边距 |
| `--space-tab-pad-y` | `8px` | `getTabWidgetStyle` QTabBar padding | 标签页上下内边距 |
| `--space-tab-pad-x` | `16px` | 同上 | 标签页左右内边距 |
| `--space-table-item-pad` | `8px` | `getTableWidgetStyle` QTableWidget::item padding | 表格单元格内边距 |
| `--space-toolbar-gap` | `8px` | `getMainWindowStyle` QToolBar spacing | 工具栏按钮间距 |
| `--space-toolbar-pad` | `8px` | 同上 QToolBar padding | 工具栏内边距 |
| `--radius-control` | `4px` | 按钮/输入框/下拉框 border-radius | 控件圆角 |
| `--radius-scroll-handle` | `5px` | `getScrollBarStyle` handle border-radius | 滚动条手柄圆角 |
| `--size-checkbox` | `16px` | `getMainWindowStyle` QCheckBox::indicator | 复选框指示器尺寸 |
| `--size-scrollbar` | `10px` | `getScrollBarStyle` width/height | 滚动条宽/高 |
| `--size-combo-dropdown` | `24px` | `getComboBoxStyle` drop-down width | 下拉箭头区宽 |
| `--min-scroll-handle` | `30px` | `getScrollBarStyle` min-height/min-width | 滚动条手柄最小尺寸 |

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

CURRENT 在 1280x720 下决策面板存在约 5px 底部溢出（`UI.md` 第 3 节已知问题）。TARGET 通过右面板弹性高度与决策区末两行可滚动修正，但 prototype 实现属于后续任务。

### 7.1 弹性区域

- 导航栏宽、左面板宽、状态栏高、菜单栏高、工具栏高在三视口下固定不变。
- 右面板宽：CURRENT 固定 360px；TARGET 允许 360–420px 弹性，默认 360px，4K 下可到 420px。
- 中心区宽高、告警区高、右面板各 section 高度为弹性，按 splitter 比例分配。

### 7.2 字体与控件尺寸

字体与控件 token 为固定 px，不随视口缩放。4K 下通过增加中心区留白与三维场景视野范围适配，不放大字号。
