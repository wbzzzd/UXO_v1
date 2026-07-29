# 统计页面设计

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](../design-system.md)
应用壳：[docs/ui/application-shell.md](../application-shell.md)
一览：[docs/ui/pages/index.md](index.md)
原型：[docs/ui/prototypes/statistics/index.html](../prototypes/statistics/index.html)
截图：[docs/ui/images/statistics/overview-1920x1080.png](../images/statistics/overview-1920x1080.png)

> 本文是统计页面（statistics page）的完整设计契约。每个交互控件拥有稳定 `STA-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据，控件 ID、区域结构和状态规则不得在实现阶段自行变更。所有视觉值取自 `design-system.md`。

CURRENT 来源：
- **本页在 CURRENT Qt 客户端中未实现为独立页面。** 以下 CURRENT 映射指向态势页复用子组件或标注"未实现"。
- 导航栏/菜单栏/工具栏/状态栏：见 [`application-shell.md`](../application-shell.md)
- 态势页复用子组件：
  - [`src/MainWindow/LeftPanelWidget.cpp`](../../../src/MainWindow/LeftPanelWidget.cpp)（目标/任务/设备列表数据源）
  - [`src/MainWindow/AlertPanel.cpp`](../../../src/MainWindow/AlertPanel.cpp)（告警计数数据源）
  - [`src/MainWindow/DeviceStatusPanel.cpp`](../../../src/MainWindow/DeviceStatusPanel.cpp)（设备在线计数数据源）
  - [`include/Core/Data/Types.h`](../../../include/Core/Data/Types.h)（数据类型枚举）

## 1. 页面概述

统计页面是系统的第五页（导航 `STA-NAV-05` 默认选中）。它一屏呈现：左侧指标分类导航（总览/目标/任务/设备/告警）与会话信息（ID/开始时间/运行时长/数据来源）；中心区 KPI 卡片（4 张：探测目标、执行任务、在线设备、告警数量）、图表视图（目标类型分布柱状图、威胁等级分布柱状图）、表格视图切换；右侧筛选器（时间范围/目标类型/威胁等级）与状态示例（数据口径说明、空会话示例、错误状态示例）。所有指标为当前会话本地固定数据，无历史数据库、无导出、无回放。

页面整体布局由应用壳定义（见 `application-shell.md` 第 2 节）。本文档化页面内部的三个内容区域与复用的应用壳控件：

| 区域 | 位置 | 内部组件 | CURRENT 映射 |
|------|------|----------|--------------|
| A | 左面板 | 指标分类导航、会话信息 | 未实现为独立页面 |
| B | 中心区 | KPI 卡片、图表视图、表格视图、视图切换 | 未实现为独立页面 |
| C | 右面板 | 筛选器（时间/类型/威胁）、状态示例 | 未实现 |

应用壳控件（导航栏、菜单栏、工具栏、状态栏）的详细规格见 `application-shell.md`，本文第 5 节给出本页专属的 `STA-*` ID 与简要规格。

## 2. 区域 A：左面板

宽 260px 固定（`--size-left-panel-width`），背景 `--color-panel`，内边距 8px，间距 8px，垂直布局。从上到下：指标分类标题 -> 指标分类导航 -> 会话信息卡片。

### 2.1 指标分类导航 `STA-LP-METRIC-NAV`

标题"指标分类"（`lp-title`，`--font-size-body`，加粗，`--color-text-primary`，内边距 `0 4px`）。下方为导航容器，垂直排列，间距 4px。每项高 44px，内边距 `0 12px`，圆角 `--radius-control`，左边框 3px 透明，字号 `--font-size-body`，图标 `◎` + 文字，间距 8px。

默认态：`--color-text-secondary` 文本。hover：背景 `--color-row-hover`、文本 `--color-text-primary`。选中态：背景 `--color-selection`、左边框 `--color-primary`、文本 `--color-text-primary`、加粗。

导航项通过 `data-metric` 属性标识（非 `data-testid`），容器本身持有 `STA-LP-METRIC-NAV` testid。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|--------|---------|------|---------|---------------|------|
| `STA-LP-METRIC-NAV` | - | div 容器 | 左面板上段 | 承载 5 个指标分类导航项 | 第一项"总览"选中 | 不适用 | 不适用 | 不适用 | 不可聚焦 | 承载子项，本身无独立点击 | 未实现 | 无 |
| `STA-LP-METRIC-NAV`（子项: `data-metric="overview"`） | 总览 | div | 导航项 1 | 切换到总览指标视图 | 选中 | 背景 `--color-row-hover` | 背景 `--color-selection`、左边框 `--color-primary` | 移除其他项 selected，当前加 selected | div 无 tabindex，不可键盘聚焦 | 默认选中；点击仅切换高亮，不切换中心区内容 | 未实现 | 无 |
| `STA-LP-METRIC-NAV`（子项: `data-metric="targets"`） | 目标统计 | div | 导航项 2 | 切换到目标统计视图 | 未选中 | 同上 | 同上 | 同上 | 同上 | 点击仅切换高亮，不切换中心区内容 | 未实现 | 无 |
| `STA-LP-METRIC-NAV`（子项: `data-metric="missions"`） | 任务统计 | div | 导航项 3 | 切换到任务统计视图 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 未实现 | 无 |
| `STA-LP-METRIC-NAV`（子项: `data-metric="devices"`） | 设备统计 | div | 导航项 4 | 切换到设备统计视图 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 未实现 | 无 |
| `STA-LP-METRIC-NAV`（子项: `data-metric="alerts"`） | 告警统计 | div | 导航项 5 | 切换到告警统计视图 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 未实现 | 无 |

> 注：指标分类导航项使用 `data-metric` 属性而非独立 `data-testid`，因此 5 个子项共享容器 `STA-LP-METRIC-NAV` 的 testid。原型中点击仅切换 `selected` 类，不切换中心区 KPI 与图表内容。TARGET 实现时应补齐指标分类联动中心区内容切换。

### 2.2 会话信息 `STA-LP-SESSION`

背景 `--color-toolbar`，圆角 `--radius-control`，内边距 12px，垂直布局，间距 6px。4 行键值对，每行两端对齐（`justify-content:space-between`），字号 11px。

| 字段 | 值 | 特殊样式 |
|------|----|----------|
| 会话 ID | session-demo-001 | 标签 `--color-text-secondary`，值 `--color-text-primary` |
| 开始时间 | 14:30:00 | 同上 |
| 运行时长 | 00:15:23 | 同上 |
| 数据来源 | [模拟] 本地固定 | 值为 `--color-status-busy` 色 |

| 字段 | 值 |
|------|----|
| ID | `STA-LP-SESSION` |
| 类型 | div 容器（只读） |
| 位置 | 左面板下段，指标导航下方 |
| 用途 | 展示当前会话 ID、开始时间、运行时长与数据来源标注 |
| 默认值 | 见上表 4 行固定值 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：键值对展示；加载：不适用（固定值）；空：不适用；错误：不适用（本地固定）；禁用：不适用 |
| 原型行为 | 固定显示 4 行会话信息；运行时长不实时更新（原型加载时固定为 00:15:23）；数据来源标注 `[模拟]` 与 `--color-status-busy` 色 |
| CURRENT 映射 | 未实现（CURRENT 无会话信息面板） |
| 安全 | 只读，模拟数据标注 |

## 3. 区域 B：中心区

弹性宽（`flex:1`），背景 `--color-bg`，垂直布局。包含中心头与中心内容区两部分。从上到下依次排列。

### 3.1 中心头

高 40px，背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 16px`，间距 12px。从左到右：标题"统计总览"（`--font-size-title`，加粗，`--color-text-primary`）+ 副标题"[模拟] 当前会话指标"（`--color-text-secondary`，`--font-size-caption`）+ 弹性留白 + 视图切换按钮组。

视图切换按钮组包含两个按钮，每个内边距 `4px 12px`，字号 `--font-size-caption`，默认 `--color-bg` 背景、`--color-text-secondary` 文本、1px `--color-border` 边框、圆角 `--radius-control`。hover：背景 `--color-border`。active：背景 `--color-primary`、白色文本、边框 `--color-primary`。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | active 态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|-----------|---------|------|---------|---------------|------|
| `STA-CE-VIEW-CHART` | 图表 | button | 视图切换组左 | 切换到图表视图（显示两张柱状图，隐藏明细表） | active | 背景 `--color-border` | 背景 `--color-primary`、白色文本 | 调用 `showChart()`：显示第一、二张图表面板，隐藏明细表面板；工具栏 `STA-TB-VIEW-CHART` 同步加 active、`STA-TB-VIEW-TABLE` 同步移除 active | Tab 聚焦，Enter 触发 | 默认 active；点击后图表视图可见，明细表隐藏 | 未实现 | 无 |
| `STA-CE-VIEW-TABLE` | 表格 | button | 视图切换组右 | 切换到表格视图（显示明细表，隐藏两张柱状图） | 未选中 | 同上 | 同上 | 调用 `showTable()`：隐藏第一、二张图表面板，显示明细表面板；工具栏 `STA-TB-VIEW-TABLE` 同步加 active、`STA-TB-VIEW-CHART` 同步移除 active | Tab 聚焦，Enter 触发 | 点击后明细表可见，两张柱状图隐藏 | 未实现 | 无 |

### 3.2 中心内容区 `STA-CE-CONTENT`

弹性容器（`flex:1`），`overflow:auto`，内边距 16px，间距 16px，垂直布局。包含 KPI 卡片网格、图表面板（两张柱状图）、明细表面板。视图切换通过 `display` 属性控制图表面板与明细表面板的显隐。

| 字段 | 值 |
|------|----|
| ID | `STA-CE-CONTENT` |
| 类型 | div 容器 |
| 位置 | 中心头下方，弹性填充 |
| 用途 | 承载 KPI 卡片、图表与明细表 |
| 默认值 | 图表视图（两张柱状图可见，明细表隐藏） |
| 点击结果 | 无（容器本身不可点击） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：内容展示；加载：不适用（原型同步渲染）；空：见第 6 节；错误：见第 6 节；禁用：不适用 |
| 原型行为 | 默认显示图表视图；视图切换按钮改变内部面板显隐 |
| CURRENT 映射 | 未实现 |
| 安全 | 无 |

### 3.3 KPI 卡片网格

4 列网格（`grid-template-columns:repeat(4,1fr)`），间距 12px。每张卡片背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 16px，垂直布局，间距 8px。结构：标签（`--color-text-secondary`，`--font-size-caption`）+ 数值（28px，加粗）+ 子标签（`--color-text-disabled`，11px）。

数值颜色变体：默认 `--color-text-primary`；`highlight` 类 `--color-primary`（绿色）；`warn` 类 `--color-threat-medium`（橙色）；`danger` 类 `--color-threat-high`（红色）。

| ID | 标签 | 类型 | 位置 | 用途 | 默认值 | 数值样式 | 子标签 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|----------|--------|---------|---------------|------|
| `STA-CE-KPI-TARGETS` | 探测目标 | span 数值 | KPI 卡片 1 | 显示探测目标总数 | `5` | `highlight`（`--color-primary` 绿色） | `[模拟] 3 已发现 / 1 已确认 / 1 处置中` | 固定显示 5，不随操作变化 | `LeftPanelWidget.cpp` `populateTargetList`（态势页复用，目标计数数据源） | 模拟数据 |
| `STA-CE-KPI-MISSIONS` | 执行任务 | span 数值 | KPI 卡片 2 | 显示执行中任务数 | `1` | 默认（`--color-text-primary` 白色） | `[模拟] 0 待执行 / 1 处置中 / 0 已完成` | 固定显示 1 | `LeftPanelWidget.cpp` `populateMissionList`（态势页复用，任务计数数据源） | 模拟数据 |
| `STA-CE-KPI-DEVICES` | 在线设备 | span 数值 | KPI 卡片 3 | 显示在线设备数与总数比 | `3/4` | `warn`（`--color-threat-medium` 橙色） | `[模拟] 1 设备离线` | 固定显示 3/4 | `DeviceStatusPanel.cpp` `refreshList`（态势页复用，设备计数数据源） | 模拟数据 |
| `STA-CE-KPI-ALERTS` | 告警数量 | span 数值 | KPI 卡片 4 | 显示告警总数 | `3` | `danger`（`--color-threat-high` 红色） | `[模拟] 1 警告 / 2 信息` | 固定显示 3 | `AlertPanel.cpp` `refreshList`（态势页复用，告警计数数据源） | 模拟数据 |

> 注：KPI 数值为原型加载时固定值，不随会话操作或指标分类导航切换而更新。4 张 KPI 卡片的标签和子标签为只读文本，无独立 `data-testid`，仅数值 span 持有 testid。

### 3.4 图表视图

图表视图包含两张柱状图面板，默认显示。视图切换到表格视图时两张面板均隐藏（`display:none`）。

#### 3.4.1 目标类型分布柱状图 `STA-CE-CHART`

第一张图表面板，背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 16px，垂直布局，间距 12px。

图表标题"[模拟] 目标类型分布"（`--font-size-body`，加粗，`--color-text-primary`）。

图表容器高 200px，柱状图沿底部对齐（`align-items:flex-end`），柱间距 24px，左侧 48px 留白用于 Y 轴，底部 1px `--color-border` 边框。Y 轴为绝对定位，宽 40px，从上到下标注 5、4、3、2、1、0，字号 10px，`--color-text-disabled`，右对齐。

| 柱组 | X 轴标签 | 柱高 | 柱色 | 柱顶数值 |
|------|----------|------|------|----------|
| 反跑道雷 | 反跑道雷 | 40% | `--color-chart-1`（#4A7A4C 绿色） | 2 |
| 航弹 | 航弹 | 20% | `--color-chart-2`（#4A90A4 蓝色） | 1 |
| 火箭弹 | 火箭弹 | 20% | `--color-chart-3`（#D4A44A 黄色） | 1 |
| 集束弹 | 集束弹 | 20% | `--color-chart-4`（#A44A4A 红色） | 1 |

图例位于图表下方，水平居中，间距 16px。每项为圆点（10x10，圆角 2px）+ 文字（11px，`--color-text-secondary`），4 项对应 4 种目标类型与柱色。

| 字段 | 值 |
|------|----|
| ID | `STA-CE-CHART` |
| 类型 | div 容器（柱状图，只读） |
| 位置 | 中心内容区，KPI 网格下方 |
| 用途 | 展示模拟目标按类型分布的柱状图 |
| 图表类型 | 垂直柱状图（bar chart），4 组柱，Y 轴 0-5 |
| 默认值 | 4 组柱：反跑道雷 2、航弹 1、火箭弹 1、集束弹 1 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：柱状图展示；加载：骨架（原型未实现）；空：空轴 + `暂无数据`（见第 4.2 节状态示例）；错误：错误图标 + `数据加载失败（模拟）`（见第 4.2 节状态示例）；禁用：不适用 |
| 原型行为 | 固定显示 4 组柱与图例；不随筛选器或指标分类导航变化 |
| CURRENT 映射 | 未实现 |
| 安全 | 模拟数据，只读展示 |

#### 3.4.2 威胁等级分布柱状图 `STA-CE-CHART-THREAT`

第二张图表面板，样式与第一张一致。

图表标题"[模拟] 威胁等级分布"（`--font-size-body`，加粗，`--color-text-primary`）。

Y 轴标注同第一张（5 至 0）。3 组柱：

| 柱组 | X 轴标签 | 柱高 | 柱色 | 柱顶数值 |
|------|----------|------|------|----------|
| 高威胁 | 高威胁 | 40% | `--color-threat-high`（#FF5252 红色） | 2 |
| 中威胁 | 中威胁 | 40% | `--color-threat-medium`（#FFB74D 橙色） | 2 |
| 低威胁 | 低威胁 | 20% | `--color-threat-low`（#FFF176 黄色） | 1 |

此图表无图例。

| 字段 | 值 |
|------|----|
| ID | `STA-CE-CHART-THREAT` |
| 类型 | div 容器（柱状图，只读） |
| 位置 | 中心内容区，目标类型分布图下方 |
| 用途 | 展示模拟目标按威胁等级分布的柱状图 |
| 图表类型 | 垂直柱状图（bar chart），3 组柱，Y 轴 0-5 |
| 默认值 | 3 组柱：高威胁 2、中威胁 2、低威胁 1 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：柱状图展示；加载：骨架（原型未实现）；空：空轴 + `暂无数据`；错误：错误图标 + `数据加载失败（模拟）`；禁用：不适用 |
| 原型行为 | 固定显示 3 组柱；不随筛选器或指标分类导航变化 |
| CURRENT 映射 | 未实现 |
| 安全 | 模拟数据，只读展示 |

### 3.5 目标明细表 `STA-CE-TABLE`

第三张面板，样式与图表面板一致。默认隐藏（`display:none`），仅在表格视图下显示。

图表标题"[模拟] 目标明细表"（`--font-size-body`，加粗，`--color-text-primary`）。

数据表（`data-table`），6 列，5 行模拟数据。表头背景 `--color-toolbar`，`--color-text-primary`，内边距 8px，底部 1px `--color-border` 边框。数据行内边距 8px，底部 1px `--color-border` 边框，偶数行背景 `--color-panel`。

| 列 | 表头 | 内容 | 文字色 |
|----|------|------|--------|
| 0 | 目标 ID | `target-001` 至 `target-005` | `--color-text-primary` |
| 1 | 类型 | 反跑道雷/航弹/火箭弹/集束弹 | `--color-text-primary` |
| 2 | 威胁 | 高/中/低 | 高=`--color-threat-high`、中=`--color-threat-medium`、低=`--color-threat-low` |
| 3 | 置信度 | `XX%` | `--color-text-primary` |
| 4 | 状态 | 已发现/已确认/处置中 | `--color-text-primary` |
| 5 | 探测源 | UAV-1/UAV-2/GPR-1 | `--color-text-primary` |

模拟目标明细数据：

| 目标 ID | 类型 | 威胁 | 置信度 | 状态 | 探测源 |
|---------|------|------|--------|------|--------|
| target-001 | 反跑道雷 | 高 | 86% | 已发现 | UAV-1 |
| target-002 | 航弹 | 高 | 72% | 已发现 | UAV-2 |
| target-003 | 火箭弹 | 中 | 91% | 已确认 | GPR-1 |
| target-004 | 集束弹 | 低 | 65% | 已发现 | UAV-1 |
| target-005 | 反跑道雷 | 中 | 78% | 处置中 | UAV-2 |

| 字段 | 值 |
|------|----|
| ID | `STA-CE-TABLE` |
| 类型 | div 容器（含 data-table） |
| 位置 | 中心内容区，威胁等级分布图下方 |
| 用途 | 展示模拟目标明细表 |
| 默认值 | 5 行模拟目标数据（见上表） |
| 点击结果 | 无（只读，行不可选择） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：表格展示；加载：骨架行（原型未实现）；空：`暂无目标`（原型未实现）；错误：`目标加载失败`（原型未实现）；禁用：不适用 |
| 原型行为 | 默认隐藏；切换到表格视图时显示 5 行固定数据；不随筛选器或指标分类导航变化 |
| CURRENT 映射 | 未实现 |
| 安全 | 模拟数据，只读展示 |

> 注：明细表数据与态势页 `LeftPanelWidget` 目标列表 fixture 一致（5 个模拟目标），但威胁列着色方式不同：态势表按 `typeName` 列着色，本表按威胁列独立着色。

## 4. 区域 C：右面板

宽 320px 固定（`--size-right-panel-width`），背景 `--color-panel`，垂直布局。两段：筛选器（`flex:2`）+ 状态示例（`flex:3`）。

### 4.1 筛选器

面板头 32px，背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`。标题"[模拟] 筛选"（`--font-size-body`，加粗，`--color-text-primary`）。

内容区 `overflow:auto`，内边距 12px，间距 10px，垂直布局。三行筛选器，每行结构：分组标签（`--color-text-secondary`，`--font-size-caption`，下边距 6px）+ chips 容器（`flex-wrap`，间距 4px）。

每个 chip 为 `<span>` 元素，内边距 `3px 8px`，字号 11px，默认 `--color-bg` 背景、`--color-text-secondary` 文本、1px `--color-border` 边框、圆角 10px。hover：背景 `--color-border`、文本 `--color-text-primary`。active：背景 `--color-primary`、白色文本、边框 `--color-primary`。

| ID | 标签 | 类型 | 位置 | 用途 | 默认选中 | chip 列表 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|----------|-----------|---------|------|---------|---------------|------|
| `STA-RP-FILTER-TIME` | 时间范围 | span 容器 | 筛选器组第 1 行 | 按时间范围筛选指标 | `当前会话` | 当前会话(active)、最近1小时(disabled)、最近24小时(disabled) | 点击 `当前会话`：同组移除其他 active，当前加 active。点击 `最近1小时`/`最近24小时`：不响应（`cursor:not-allowed`，`opacity:0.5`，tooltip `历史数据不可用`） | chip 为 `<span>` 无 tabindex，不可键盘聚焦 | 默认选中"当前会话"；其余两项为禁用占位，标注"历史数据不可用" | 未实现 | 无 |
| `STA-RP-FILTER-TYPE` | 目标类型 | span 容器 | 筛选器组第 2 行 | 按目标类型筛选图表与表格 | `全部` | 全部(active)、反跑道雷、航弹、火箭弹、集束弹 | 点击 chip：同组移除其他 active，当前加 active | 同上 | 默认选中"全部"；点击其他 chip 仅切换 active 高亮，不实际过滤中心区图表或表格 | 未实现 | 无 |
| `STA-RP-FILTER-THREAT` | 威胁等级 | span 容器 | 筛选器组第 3 行 | 按威胁等级筛选图表与表格 | `全部` | 全部(active)、高、中、低 | 同上 | 同上 | 同上；点击仅切换 active 高亮，不实际过滤 | 未实现 | 无 |

> 注：筛选 chip 为 `<span>` 元素，无 `tabindex` 属性，不可通过 Tab 键聚焦。原型存在无障碍缺口，TARGET 实现时应补齐 `tabindex` 与键盘激活。筛选器在原型中仅切换 active 高亮，不实际过滤中心区 KPI、图表或表格内容。时间范围筛选器中"最近1小时"与"最近24小时"为禁用占位，tooltip 标注"历史数据不可用"。

### 4.2 状态示例

面板头 32px，背景 `--color-toolbar`，底部 1px `--color-border` 边框。标题"状态示例"（`--font-size-body`，加粗，`--color-text-primary`）。

内容区 `overflow:auto`，内边距 12px，间距 10px，垂直布局。包含三张状态示例卡片，每张背景 `--color-toolbar`，圆角 `--radius-control`，内边距 12px，垂直布局，间距 8px。

此区域无独立 `data-testid`，三张卡片为只读说明文本，不承载交互控件。

#### 4.2.1 数据口径说明卡片

| 字段 | 值 | 特殊样式 |
|------|----|----------|
| 来源 | 本地固定模拟场景 | 标签 `--color-text-secondary`（60px 宽），值 `--color-text-primary` |
| 范围 | 当前会话内存数据 | 同上 |
| 持久化 | 无（刷新即清空） | 值为 `--color-threat-medium` 色 |

底部说明文字（`--color-text-disabled`，11px，行高 1.5）：`[模拟]` 标签（`--color-status-busy` 色，加粗）+ `本页所有指标均为本地会话固定数据，不暗示历史数据库、导出或回放功能已实现。`

#### 4.2.2 空会话状态示例卡片

标题"空会话状态示例"（`--font-size-caption`，`--color-text-primary`，加粗）。

说明文字（`--color-text-disabled`，11px，行高 1.5）：`当会话无数据时，KPI 显示 "0" 并标注 "[模拟] 无数据"，图表显示空轴和 "暂无数据" 文字，不显示零值误导。`

#### 4.2.3 错误状态示例卡片

标题"错误状态示例"（`--font-size-caption`，`--color-text-primary`，加粗）。

说明文字（`--color-text-disabled`，11px，行高 1.5）：`数据加载失败时，KPI 显示 "N/A"，图表区显示错误图标和 "数据加载失败（模拟）" 文字，不伪造历史结果。`

| 字段 | 值 |
|------|----|
| ID | 无（原型未分配 `data-testid`） |
| 类型 | div 容器（只读说明） |
| 位置 | 右面板下段 |
| 用途 | 展示数据口径、空会话状态与错误状态的文字说明 |
| 默认值 | 3 张卡片固定文本（见 4.2.1 至 4.2.3） |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 不适用（本身为状态说明文本，非数据承载区） |
| 原型行为 | 固定展示 3 张说明卡片 |
| CURRENT 映射 | 未实现 |
| 安全 | 模拟数据标注，无操作入口 |

> 注：状态示例区域为纯文字说明，描述了 KPI 与图表在空会话和错误状态下的预期展示规则。这些规则是 TARGET 设计意图，原型本身未在 KPI 与图表中实现空态与错误态视觉。

## 5. 应用壳控件

本节文档化统计页复用的应用壳控件（导航栏、菜单栏、工具栏、状态栏），为其分配 `STA-*` ID。详细规格（尺寸、间距、跨页一致性规则）见 `application-shell.md`。

### 5.1 导航栏

宽 80px 固定（`--size-nav-width`），背景 `--color-bg`，右边框 1px `--color-border`。垂直布局：Logo + 16px 间距 + 6 个导航项 + 弹性留白。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|--------|---------|------|---------|---------------|------|
| `STA-NAV-LOGO` | UXO | div | 导航栏顶，40px 高 | 品牌标识 | `--color-primary` 色，18px，加粗，字间距 2px，居中 | 不适用 | 不适用 | 无 | 不可聚焦 | 同 CURRENT | 见 `application-shell.md` 第 3 节 | 无 |
| `STA-NAV-01` | 态势 | div | 导航项 1，56px 高 | 切换到态势页 | `--color-text-secondary`，左边框 3px 透明 | 背景 `--color-row-hover`、文本 `--color-text-primary` | 背景 `--color-selection`、左边框 `--color-primary`、文本 `--color-text-primary`、加粗 | 移除其他项 selected，当前加 selected | div 无 tabindex，不可键盘聚焦 | 仅切换高亮，不跳转页面 | 见 `application-shell.md` 第 3 节 | 无 |
| `STA-NAV-02` | 探测 | div | 导航项 2，56px 高 | 切换到探测页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `STA-NAV-03` | 决策 | div | 导航项 3，56px 高 | 切换到决策页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `STA-NAV-04` | 设备 | div | 导航项 4，56px 高 | 切换到设备页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `STA-NAV-05` | 统计 | div | 导航项 5，56px 高 | 切换到统计页（当前页） | 选中 | 同上 | 同上 | 同上 | 同上 | 默认选中；点击仅保持选中 | 见 `application-shell.md` 第 3 节 | 无 |
| `STA-NAV-06` | 配置 | div | 导航项 6，56px 高 | 切换到配置页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |

导航项图标统一为 `◎`（18px）。导航项内边距由 flex 居中控制，字号 `--font-size-caption`，间距 4px。过渡动画 `--anim-short`（150ms）`--anim-easing`。

> 注：原型中导航点击仅切换 selected 类，不执行实际页面跳转（单页原型）。CURRENT Qt 客户端中导航切换通过 `QStackedWidget` 实现，见 `application-shell.md`。

### 5.2 菜单栏

高 30px（`--size-menu-bar-height`），背景 `--color-menu`，底部 1px `--color-border` 边框，内边距 `0 4px`。4 个菜单项为 `<button>` 元素，内边距 `6px 12px`，`--font-size-body`。

默认态：透明背景、`--color-text-primary` 文本。hover：背景 `--color-border`。禁用态（`data-disabled="true"`）：`--color-text-disabled` 文本，hover 不变背景。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `STA-MENU-FILE` | 文件(&F) | button | 菜单栏左 1 | 文件菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无（无 JS 事件绑定） | Tab 聚焦，Enter 触发 | 点击无效果，不展开下拉菜单 | 见 `application-shell.md` 第 4 节 | 无 |
| `STA-MENU-VIEW` | 视图(&V) | button | 菜单栏左 2 | 视图菜单（占位） | 同上 | 同上 | 不适用 | 无 | 同上 | 同上 | 见 `application-shell.md` 第 4 节 | 无 |
| `STA-MENU-TOOLS` | 工具(&T) | button | 菜单栏左 3 | 工具菜单（禁用占位） | `--color-text-disabled` 文本 | 不变背景 | `data-disabled="true"`，tooltip `占位控件，未实现` | 无（disabled） | 不可聚焦 | **禁用并标注"占位"**，不响应点击 | 见 `application-shell.md` 第 4 节 | 无 |
| `STA-MENU-HELP` | 帮助(&H) | button | 菜单栏左 4 | 帮助菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无 | Tab 聚焦，Enter 触发 | 点击无效果 | 见 `application-shell.md` 第 4 节 | 无 |

### 5.3 工具栏

高 32px（`--size-toolbar-height`），背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`，间距 8px。从左到右：导出报告标签（禁用）+ 回放标签（禁用）+ 图表视图按钮 + 表格视图按钮 + 弹性留白 + 当前会话标签。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | active 态 | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|-----------|----------|---------|------|---------|---------------|------|
| `STA-TB-EXPORT` | 导出报告 | span | 工具栏左 1 | 导出统计报告（禁用占位） | `--color-text-disabled`，`--font-size-caption`，内边距 4px | 不变（disabled） | 不适用 | `data-disabled="true"`，tooltip `占位控件，未实现` | 无 | 不可聚焦 | **禁用并标注"占位"** | 见 `application-shell.md` 第 5 节 | 无 |
| `STA-TB-REPLAY` | 回放 | span | 工具栏左 2 | 回放会话操作（禁用占位） | 同上 | 同上 | 不适用 | `data-disabled="true"`，tooltip `占位控件，未实现` | 无 | 不可聚焦 | **禁用并标注"占位"** | 见 `application-shell.md` 第 5 节 | 无 |
| `STA-TB-VIEW-CHART` | 图表 | button | 工具栏左 3 | 切换到图表视图 | `--color-text-secondary` 文本、透明背景、1px `--color-border` 边框、圆角 `--radius-control`、`--font-size-caption`，内边距 `4px 8px` | 背景 `--color-border`、文本 `--color-text-primary` | 背景 `--color-primary`、白色文本、边框 `--color-primary` | 不适用 | 调用 `showChart()`：显示两张柱状图面板，隐藏明细表；中心区 `STA-CE-VIEW-CHART` 同步加 active、`STA-CE-VIEW-TABLE` 同步移除 active | Tab 聚焦，Enter 触发 | 默认 active；点击切换到图表视图 | 见 `application-shell.md` 第 5 节 | 无 |
| `STA-TB-VIEW-TABLE` | 表格 | button | 工具栏左 4 | 切换到表格视图 | 同上（未 active） | 同上 | 同上 | 不适用 | 调用 `showTable()`：隐藏两张柱状图面板，显示明细表；中心区 `STA-CE-VIEW-TABLE` 同步加 active、`STA-CE-VIEW-CHART` 同步移除 active | Tab 聚焦，Enter 触发 | 点击切换到表格视图 | 见 `application-shell.md` 第 5 节 | 无 |
| `STA-TB-SESSION` | 当前会话: session-demo-001 | span | 工具栏右，弹性留白后 | 显示当前会话 ID | `--color-text-secondary`，`--font-size-caption`，内边距 4px | 不适用 | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `当前会话: session-demo-001`，与左面板 `STA-LP-SESSION` 会话 ID 一致 | 见 `application-shell.md` 第 5 节 | 模拟数据 |

> 注：工具栏视图切换按钮（`STA-TB-VIEW-CHART`/`STA-TB-VIEW-TABLE`）与中心头视图切换按钮（`STA-CE-VIEW-CHART`/`STA-CE-VIEW-TABLE`）功能等价，点击任一处均同步两处 active 状态与中心区面板显隐。导出报告与回放为禁用占位，标注"占位控件，未实现"。

### 5.4 状态栏

高 28px，背景 `--color-bg`，顶部 1px `--color-border` 边框，内边距 `0 16px`，间距 16px。从左到右：设备状态标签 + 分隔线 + 模拟模式标签 + 分隔线 + 告警滚动区（弹性）+ 紧急停止按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `STA-SB-DEVICE` | 设备: 3/4 在线 | span | 状态栏左 1 | 显示模拟设备在线状态 | `--color-text-primary`，`--font-size-caption` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `设备: 3/4 在线`，与 KPI `STA-CE-KPI-DEVICES` 一致 | 见 `application-shell.md` 第 6 节 | 模拟数据 |
| `STA-SB-SIM` | [模拟模式] | span | 状态栏左 2，分隔线后 | 标注当前为模拟模式 | `--color-status-busy`，`--font-size-caption`，加粗 | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `[模拟模式]` | 见 `application-shell.md` 第 6 节 | 模拟标注 |
| `STA-SB-ALARM` | - | div 容器 | 状态栏中，弹性宽 | 展示模拟告警滚动条目 | `min-width:400px`，`overflow:hidden`；条目 `--color-status-busy` 色、`--font-size-caption`、`--color-toolbar` 背景、内边距 `2px 8px`、圆角 `--radius-control` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 1 条告警 `模拟告警: 统计数据为本地会话固定` | 见 `application-shell.md` 第 6 节 | 模拟告警 |
| `STA-SB-EMERGENCY` | 紧急停止 | button | 状态栏右，80x20 | 紧急停止所有设备（禁用占位） | **始终禁用**：`--color-border` 背景、`--color-text-disabled` 文本、11px、加粗、圆角 3px；tooltip `危险占位：无设备停止效果，本原型禁用` | 不适用 | `disabled` + `data-disabled="true"` | 无（disabled，不响应点击） | 不可聚焦 | **模拟占位，无实际效果**；原型中禁用并标注"危险占位" | 见 `application-shell.md` 第 6 节 | 模拟占位，无设备停止效果 |

状态栏分隔线为 1px 宽、18px 高的 `--color-border` 竖线。

## 6. 状态规则汇总

每个区域必须定义五态。下表汇总各区域的五态实现：

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 左面板指标导航 | 导航项可点击 | 不适用 | 不适用 | 不适用 | 不适用 |
| 左面板会话信息 | 键值对展示 | 不适用（固定值） | 不适用 | 不适用 | 不适用 |
| 中心区 KPI 卡片 | 数值展示 | 骨架（原型未实现） | 数值显示 `0` + `[模拟] 无数据`（见 4.2.2 状态示例，原型未实现） | 数值显示 `N/A`（见 4.2.3 状态示例，原型未实现） | 不适用 |
| 中心区目标类型分布图 | 柱状图展示 | 骨架（原型未实现） | 空轴 + `暂无数据`（见 4.2.2 状态示例，原型未实现） | 错误图标 + `数据加载失败（模拟）`（见 4.2.3 状态示例，原型未实现） | 不适用 |
| 中心区威胁等级分布图 | 柱状图展示 | 骨架（原型未实现） | 同上 | 同上 | 不适用 |
| 中心区目标明细表 | 表格展示 | 骨架行（原型未实现） | `暂无目标`（原型未实现） | `目标加载失败`（原型未实现） | 不适用 |
| 中心区视图切换 | 按钮可点击 | 不适用 | 不适用 | 不适用 | 不适用 |
| 右面板筛选器 | chips 可点击（时间范围 2 项禁用） | 不适用 | 不适用 | 不适用 | 时间范围"最近1小时"/"最近24小时"禁用占位 |
| 右面板状态示例 | 说明卡片展示 | 不适用 | 不适用 | 不适用 | 不适用 |
| 导航栏 | 导航项可点击 | 不适用 | 不适用 | 不适用 | 不适用 |
| 菜单栏 | 菜单项可点击 | 不适用 | 不适用 | 不适用 | 工具菜单禁用占位 |
| 工具栏 | 按钮可点击 | 不适用 | 不适用 | 不适用 | 导出报告、回放禁用占位 |
| 状态栏 | 只读展示 | 不适用 | 不适用 | 不适用 | 紧急停止禁用占位 |

原型已实现的空态与错误态：无。原型在右面板状态示例区域以文字描述了空会话状态（KPI 显示 `0`、图表显示空轴与 `暂无数据`）与错误状态（KPI 显示 `N/A`、图表显示错误图标与 `数据加载失败（模拟）`）的预期展示规则，但未在 KPI 卡片与图表中实际实现这些状态视觉。加载/空/错误态需在 TARGET 实现时补齐。状态颜色不得作为唯一信息，必须同时给出文字。

## 7. 交互流程

### 7.1 视图切换流程

1. 用户点击工具栏 `STA-TB-VIEW-CHART` 或中心头 `STA-CE-VIEW-CHART`。
2. `showChart()` 执行：
   - 第一张图表面板（`STA-CE-CHART`）设为 `display:flex`。
   - 第二张图表面板（威胁等级分布）设为 `display:flex`。
   - 明细表面板（`STA-CE-TABLE`）设为 `display:none`。
   - 工具栏与中心头的图表按钮均加 `active` 类，表格按钮移除 `active` 类。
3. 用户点击工具栏 `STA-TB-VIEW-TABLE` 或中心头 `STA-CE-VIEW-TABLE`。
4. `showTable()` 执行：
   - 第一张图表面板设为 `display:none`。
   - 第二张图表面板设为 `display:none`。
   - 明细表面板设为 `display:flex`。
   - 工具栏与中心头的表格按钮均加 `active` 类，图表按钮移除 `active` 类。

> 注：工具栏与中心头各有一对视图切换按钮，点击任一按钮均同步两处 active 状态与中心区面板显隐。两对按钮功能完全等价。

### 7.2 指标分类导航流程

1. 用户点击 `STA-LP-METRIC-NAV` 中某导航项（总览/目标统计/任务统计/设备统计/告警统计）。
2. 移除其他导航项的 `selected` 类，当前项添加 `selected`。
3. **注意**：原型中指标分类切换仅改变高亮，不切换中心区 KPI、图表或表格内容。TARGET 实现时应补齐指标分类联动中心区内容切换。

### 7.3 筛选流程

1. 用户点击某筛选组的 chip。
2. 若 chip 未禁用（`cursor:not-allowed` 的 chip 不响应点击）：
   - 同组其他 chip 移除 `active` 类。
   - 当前 chip 添加 `active`。
3. **注意**：原型中筛选仅切换 active 高亮，不实际过滤中心区 KPI、图表或表格内容。时间范围筛选器中"最近1小时"与"最近24小时"为禁用占位，tooltip 标注"历史数据不可用"。

### 7.4 导航切换流程

1. 用户点击导航项 `STA-NAV-01` 至 `STA-NAV-06`。
2. 移除其他导航项的 `selected` 类，当前项添加 `selected`。
3. **注意**：原型为单页，不执行实际页面跳转。CURRENT Qt 客户端中导航切换通过 `QStackedWidget` 实现，见 `application-shell.md`。

## 8. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280x720 | 导航栏 80px + 左面板 260px + 右面板 320px = 660px 固定，中心区 620px；KPI 网格 4 列（每列约 140px 含间距）需在 620px 内完整显示；图表容器高 200px 需在 620px 宽内柱组不溢出（4 组柱 + 48px Y 轴留白 + 24px 间距）；明细表 6 列需在 620px 内不截断；筛选器 chip 在 320px 右面板内可能换行（`flex-wrap`）；工具栏导出报告 + 回放 + 图表 + 表格 + 会话标签需在 1280px 内完整显示 |
| 1920x1080 | 默认尺寸；中心区 1260px；所有控件按 token 展示；KPI 卡片与图表有充足水平空间 |
| 3840x2160 | 固定区域不变（导航 80px + 左面板 260px + 右面板 320px = 660px）；中心区 3180px 弹性放大；KPI 卡片与图表水平方向有大量留白；字号与控件尺寸保持固定 px |

原型 `.app` 容器固定为 `1920x1080`（`width:1920px;height:1080px`），不随视口缩放。TARGET 实现时应使布局响应三视口，固定面板宽度不变，中心区弹性。

## 9. 安全清单

本页面所有控件必须遵守以下安全约束：

| 控件 | 约束 |
|------|------|
| KPI 卡片 `STA-CE-KPI-*` | 只读展示，模拟数据，不连接真实设备或数据库 |
| 图表面板 `STA-CE-CHART`/`STA-CE-TABLE` | 只读展示，模拟数据 |
| 明细表 `STA-CE-TABLE` | 只读展示，行不可选择，无操作入口 |
| 视图切换按钮 `STA-CE-VIEW-*`/`STA-TB-VIEW-*` | 仅切换前端显示，无设备通信 |
| 指标分类导航 `STA-LP-METRIC-NAV` | 仅切换前端高亮，无设备通信 |
| 筛选器 `STA-RP-FILTER-*` | 仅切换前端高亮，不实际过滤数据，无设备通信 |
| 时间范围筛选"最近1小时"/"最近24小时" | 禁用占位，tooltip 标注"历史数据不可用" |
| 会话信息 `STA-LP-SESSION` | 只读展示，模拟数据标注 |
| 紧急停止 `STA-SB-EMERGENCY` | 模拟占位，无实际效果，原型中禁用 |
| 导出报告 `STA-TB-EXPORT` | 禁用占位，不执行导出 |
| 回放 `STA-TB-REPLAY` | 禁用占位，不执行回放 |
| 工具菜单 `STA-MENU-TOOLS` | 禁用占位，不展开 |
| 导航项 `STA-NAV-*` | 仅切换高亮，不执行实际页面跳转（单页原型） |

所有模拟数据与结果必须带"模拟"或"演示"字样。涉及 KPI 数值、图表、明细表、会话信息的内容若来自本地 fixture，必须在控件或区域旁标注"模拟数据"。本页面不实现登录、角色切换、外部通信、持久化、UXR、MOS。不提供设备控制命令，不执行排爆动作。不暗示历史数据库、导出或回放功能已实现。

## 10. CURRENT 映射总结

| 页面元素 | CURRENT 源码位置 |
|---------|------------------|
| **页面级** | **本页在 CURRENT Qt 客户端中未实现为独立页面** |
| 指标分类导航 `STA-LP-METRIC-NAV` | 未实现（CURRENT 无指标分类导航组件） |
| 会话信息 `STA-LP-SESSION` | 未实现（CURRENT 无会话信息面板） |
| KPI 探测目标 `STA-CE-KPI-TARGETS` | `LeftPanelWidget.cpp` `populateTargetList`（态势页复用，目标计数数据源） |
| KPI 执行任务 `STA-CE-KPI-MISSIONS` | `LeftPanelWidget.cpp` `populateMissionList`（态势页复用，任务计数数据源） |
| KPI 在线设备 `STA-CE-KPI-DEVICES` | `DeviceStatusPanel.cpp` `refreshList`（态势页复用，设备计数数据源） |
| KPI 告警数量 `STA-CE-KPI-ALERTS` | `AlertPanel.cpp` `refreshList`（态势页复用，告警计数数据源） |
| 目标类型分布图 `STA-CE-CHART` | 未实现（CURRENT 无柱状图组件） |
| 威胁等级分布图（无 testid） | 未实现 |
| 目标明细表 `STA-CE-TABLE` | `LeftPanelWidget.cpp` `populateTargetList`（态势页复用，目标 fixture 数据源，字段集不同） |
| 视图切换 `STA-CE-VIEW-*`/`STA-TB-VIEW-*` | 未实现 |
| 筛选器 `STA-RP-FILTER-*` | 未实现（CURRENT `LeftPanelWidget` 声明 `m_typeFilterCombo`/`m_threatFilterCombo` 成员但 `setupUi` 未实例化） |
| 状态示例区域 | 未实现 |
| 导航栏 `STA-NAV-*` | 见 `application-shell.md` 第 3 节 |
| 菜单栏 `STA-MENU-*` | 见 `application-shell.md` 第 4 节 |
| 工具栏 `STA-TB-*` | 见 `application-shell.md` 第 5 节 |
| 状态栏 `STA-SB-*` | 见 `application-shell.md` 第 6 节 |
| 数据类型枚举 | `Types.h`（TargetType/ThreatLevel/TargetStatus/MissionStatus/DeviceStatus/AlarmLevel） |

## 11. STA-* ID 索引

下表列出本文档化的全部 `STA-*` ID，供 HTML 原型与 Playwright 测试对齐：

| ID | 控件 | 区域 |
|----|------|------|
| `STA-NAV-LOGO` | 导航栏 Logo | 应用壳 |
| `STA-NAV-01` | 导航项：态势 | 应用壳 |
| `STA-NAV-02` | 导航项：探测 | 应用壳 |
| `STA-NAV-03` | 导航项：决策 | 应用壳 |
| `STA-NAV-04` | 导航项：设备 | 应用壳 |
| `STA-NAV-05` | 导航项：统计（选中） | 应用壳 |
| `STA-NAV-06` | 导航项：配置 | 应用壳 |
| `STA-MENU-FILE` | 菜单：文件 | 应用壳 |
| `STA-MENU-VIEW` | 菜单：视图 | 应用壳 |
| `STA-MENU-TOOLS` | 菜单：工具（禁用占位） | 应用壳 |
| `STA-MENU-HELP` | 菜单：帮助 | 应用壳 |
| `STA-TB-EXPORT` | 工具栏：导出报告（禁用占位） | 应用壳 |
| `STA-TB-REPLAY` | 工具栏：回放（禁用占位） | 应用壳 |
| `STA-TB-VIEW-CHART` | 工具栏：图表视图切换 | 应用壳 |
| `STA-TB-VIEW-TABLE` | 工具栏：表格视图切换 | 应用壳 |
| `STA-TB-SESSION` | 工具栏：当前会话标签 | 应用壳 |
| `STA-LP-METRIC-NAV` | 指标分类导航容器 | 左面板 |
| `STA-LP-SESSION` | 会话信息卡片 | 左面板 |
| `STA-CE-VIEW-CHART` | 中心头：图表视图切换 | 中心区 |
| `STA-CE-VIEW-TABLE` | 中心头：表格视图切换 | 中心区 |
| `STA-CE-CONTENT` | 中心内容区容器 | 中心区 |
| `STA-CE-KPI-TARGETS` | KPI：探测目标 | 中心区 |
| `STA-CE-KPI-MISSIONS` | KPI：执行任务 | 中心区 |
| `STA-CE-KPI-DEVICES` | KPI：在线设备 | 中心区 |
| `STA-CE-KPI-ALERTS` | KPI：告警数量 | 中心区 |
| `STA-CE-CHART` | 目标类型分布柱状图 | 中心区 |
| `STA-CE-CHART-THREAT` | 威胁等级分布柱状图 | 中心区 |
| `STA-CE-TABLE` | 目标明细表 | 中心区 |
| `STA-RP-FILTER-TIME` | 时间范围筛选器 | 右面板 |
| `STA-RP-FILTER-TYPE` | 目标类型筛选器 | 右面板 |
| `STA-RP-FILTER-THREAT` | 威胁等级筛选器 | 右面板 |
| `STA-SB-DEVICE` | 设备状态 | 状态栏 |
| `STA-SB-SIM` | 模拟模式标签 | 状态栏 |
| `STA-SB-ALARM` | 告警滚动区 | 状态栏 |
| `STA-SB-EMERGENCY` | 紧急停止按钮（禁用占位） | 状态栏 |

导航栏、菜单栏、工具栏、状态栏的详细规格见 `application-shell.md` 第 3 节（导航栏）、第 4 节（菜单栏）、第 5 节（工具栏）、第 6 节（状态栏）。

> 注：威胁等级分布柱状图面板与右面板状态示例区域在原型 HTML 中未分配 `data-testid`，无法通过 Playwright 定位器直接选取。TARGET 实现时应补齐 testid 并在本文登记。
