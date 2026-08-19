# 设备页面设计

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](../design-system.md)
应用壳：[docs/ui/application-shell.md](../application-shell.md)
一览：[docs/ui/pages/index.md](index.md)
原型：[docs/ui/prototypes/devices/index.html](../prototypes/devices/index.html)
截图：[docs/ui/images/devices/overview-1920x1080.png](../images/devices/overview-1920x1080.png)

> 本文是设备页面（devices page）的完整设计契约。每个交互控件拥有稳定 `DEV-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据，控件 ID、区域结构和状态规则不得在实现阶段自行变更。所有视觉值取自 `design-system.md`。

CURRENT 来源：
- 应用壳（导航栏/菜单栏/工具栏/状态栏）见 [application-shell.md](../application-shell.md) 第 3-6 节
- [`src/MainWindow/DeviceStatusPanel.cpp`](../../../src/MainWindow/DeviceStatusPanel.cpp)（态势页右面板只读设备状态卡片，可作为设备列表数据源复用）
- [`src/MainWindow/DeviceControlView.cpp`](../../../src/MainWindow/DeviceControlView.cpp)（设备控制台占位壳，仅含空 Tab，未作为独立页面接入）
- [`include/Core/Data/Types.h`](../../../include/Core/Data/Types.h)（`DeviceInfo`/`DeviceStatus` 数据类型）

> 设备页在 CURRENT Qt 客户端中未作为独立页面实现。CURRENT 只有态势页右面板的 `DeviceStatusPanel`（只读卡片）和未接入的 `DeviceControlView`（空 Tab 壳）。本文档化的页面内容区控件（区域 A/B/C）CURRENT 映射均标注"未实现"。仅 `DEV-TB-CONSOLE`（设备控制台）与 `DeviceControlView` 相关，且为禁用占位。

## 1. 页面概述

设备页面是设备状态与管理页（导航 `DEV-NAV-04` 默认选中）。一屏呈现：左侧设备列表与搜索筛选；中心设备详情（基本信息/运行状态/能力参数/维护信息/可用性检查）；右侧模拟指派与可用设备推荐。所有设备状态为本地模拟（4 个模拟设备），"模拟指派"仅修改原型显示状态，不发送真实设备指令、不操控设备、不执行真实任务。

页面整体布局由应用壳定义（见 `application-shell.md` 第 2 节）。本文档化页面内部的三个内容区域，以及设备页原型中实例化的应用壳控件（使用 `DEV-*` 前缀）：

| 区域 | 位置 | 内部组件 | CURRENT 主控件 |
|------|------|----------|----------------|
| A | 左面板 | 搜索工具条、类型/状态筛选、设备卡片列表 | 未实现（`DeviceStatusPanel` 仅态势页右面板只读卡片，可复用为数据源） |
| B | 中心 | 中心头（设备标识）、详情网格（4 张只读卡片）、可用性检查 | 未实现 |
| C | 右面板 | 模拟指派卡片、可用设备推荐列表 | 未实现 |
| 应用壳 | 导航/菜单/工具栏/状态栏 | `DEV-NAV-*`/`DEV-MENU-*`/`DEV-TB-*`/`DEV-SB-*` 实例 | 见 `application-shell.md` 第 3-6 节 |

模拟设备数据（原型 `script` 内硬编码，4 个）：

| ID | 名称 | 类型 | 状态 | 状态文本 | 电量 | 附加信息 | 可指派 |
|----|------|------|------|----------|------|----------|--------|
| `device-001` | 侦察无人机 Alpha | UAV | online | 模拟在线 | 82% | 信号: 良好 | 是 |
| `device-002` | 排爆机器人 Bravo | Robot | busy | 模拟忙碌 | 45% | 任务: target-005 | 否 |
| `device-003` | 排爆无人机 Charlie | UAV | online | 模拟在线 | 91% | 信号: 良好 | 是 |
| `device-004` | 探地雷达 Delta | GPR | offline | 模拟离线 | -- | 原因: 通信中断 | 否 |

## 2. 区域 A：左面板（设备列表）

宽 360px 固定（`--size-left-panel-width`），背景 `--color-panel`，外边距 8px，间距 8px，纵向溢出隐藏。从上到下：搜索工具条 -> 筛选组 -> 设备列表。

### 2.1 搜索工具条

背景 `--color-toolbar`，圆角 `--radius-control`，内边距 `8px 6px`，间距 8px，flex 横向居中。包含搜索框与刷新按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|-----------|---------|------|---------|---------------|------|
| `DEV-LP-SEARCH` | - | QLineEdit（HTML `<input>`） | 搜索栏左，弹性宽 | 按文字过滤设备列表 | 高 28px；默认 `--color-bg` 背景、1px `--color-border` 边框、圆角 `--radius-control`、内边距 `0 8px`、主文本色、`--font-size-body`；focus 边框 `--color-border-focus`（`outline:none`）；placeholder `搜索设备...`、`--color-text-disabled` | 输入文本实时过滤设备卡片：匹配 `id`/`name`/`type`（不区分大小写包含匹配），不匹配的卡片 `display:none`，空文本显示全部 | Tab 聚焦，Esc 不清空（原型无 Esc 绑定） | 同上；监听 `input` 事件，每次输入立即重过滤 | 未实现 | 无 |
| `DEV-LP-REFRESH` | 刷新 | QPushButton（HTML `<button>`） | 搜索框右，高 28px | 请求刷新模拟设备状态 | `btn-icon` 变体：高 28px、内边距 `0 10px`、`--color-toolbar` 背景、`--color-text-secondary` 文本、1px `--color-border` 边框、圆角 `--radius-control`、`--font-size-caption`；hover 背景 `--color-border`、文本 `--color-text-primary` | 无效果（原型未绑定 click 处理器，仅视觉按钮） | Tab 聚焦，Enter 触发（无效果） | **占位按钮**：tooltip 无；点击无响应。原型中仅为视觉占位，不触发任何刷新。如未来启用，需先在本文登记信号路径 | 未实现 | 无设备通信，仅本地占位 |

### 2.2 筛选组

背景 `--color-toolbar`，圆角 `--radius-control`，内边距 `8px 6px`，纵向间距 6px。两行筛选：类型与状态。每行为 `filter-row`（flex 横向，间距 6px）：左侧标签（52px 固定宽，`--color-text-secondary`，`--font-size-caption`）+ 右侧 chip 组。

chip 样式：内边距 `3px 8px`，字号 11px，默认 `--color-bg` 背景、`--color-text-secondary` 文本、1px `--color-border` 边框、圆角 10px（药丸形）、`cursor:pointer`、过渡动画 `--anim-short` `--anim-easing`。hover：背景 `--color-border`、文本 `--color-text-primary`。active：背景 `--color-primary`、白色文本、边框 `--color-primary`。

#### 2.2.1 类型筛选 `DEV-LP-FILTER-TYPE`

| 字段 | 值 |
|------|----|
| ID | `DEV-LP-FILTER-TYPE` |
| 类型 | chip 组容器（HTML `<div>`，含 4 个 `<span class="chip">`） |
| 位置 | 筛选组第一行，标签"类型"右侧 |
| 用途 | 按设备类型过滤设备列表 |
| chip 列表 | `全部`（`data-value="all"`，默认 active）/ `无人机`（`UAV`）/ `机器人`（`Robot`）/ `探地雷达`（`GPR`） |
| 默认态 | `全部` active，其余默认 |
| 点击结果 | 点击任一 chip：移除同组（`data-filter="type"`）所有 chip 的 `active` 类，为被点击 chip 加 `active` 类，更新 `filters.type` 为该 chip 的 `data-value`，调 `applyFilters()` 重新过滤设备列表 |
| 过滤逻辑 | `filters.type === 'all'` 显示全部；否则仅显示 `device.type === filters.type` 的卡片 |
| 键盘 | chip 不可原生聚焦（HTML `<span>`）；原型无键盘支持 |
| 原型行为 | 同上；类型与状态筛选叠加生效（两者均需满足才显示） |
| CURRENT 映射 | 未实现 |
| 安全 | 无 |

#### 2.2.2 状态筛选 `DEV-LP-FILTER-STATUS`

| 字段 | 值 |
|------|----|
| ID | `DEV-LP-FILTER-STATUS` |
| 类型 | chip 组容器（HTML `<div>`，含 4 个 `<span class="chip">`） |
| 位置 | 筛选组第二行，标签"状态"右侧 |
| 用途 | 按设备状态过滤设备列表 |
| chip 列表 | `全部`（`data-value="all"`，默认 active）/ `在线`（`online`）/ `忙碌`（`busy`）/ `离线`（`offline`） |
| 默认态 | `全部` active，其余默认 |
| 点击结果 | 同 2.2.1，更新 `filters.status`，调 `applyFilters()` |
| 过滤逻辑 | `filters.status === 'all'` 显示全部；否则仅显示 `device.status === filters.status` 的卡片 |
| 键盘 | chip 不可原生聚焦；原型无键盘支持 |
| 原型行为 | 同上；与类型筛选叠加 |
| CURRENT 映射 | 未实现 |
| 安全 | 状态文字与颜色同时呈现（圆点 + 文本），颜色不作为唯一信息 |

> 设备可用/忙碌/离线原因同时用文字和状态圆点标识。离线设备（`device-004`）卡片体显示"原因: 通信中断"文字说明。

### 2.3 设备列表 `DEV-LP-DEVICE-LIST`

弹性填充剩余空间，纵向滚动（`overflow:auto`），纵向排列，间距 6px。包含 4 张设备卡片。

卡片样式：背景 `--color-toolbar`，1px `--color-border` 边框，左侧 3px `--color-border` 边框，圆角 `--radius-control`，内边距 12px，`cursor:pointer`，过渡动画。hover：背景 `#363636`。selected：背景 `--color-selection`、左侧边框 `--color-primary`。unavailable：`opacity:0.6`。

卡片结构（`device-card-header` + `device-card-body`）：

- 头部（flex 横向，间距 8px，下边距 6px）：状态圆点（10px 圆形）+ 设备名称（弹性宽，主文本色，`--font-size-body`，加粗）+ 类型徽章（11px，`--color-text-secondary`，`--color-bg` 背景，圆角 3px，内边距 `2px 6px`）。
- 体部（flex 横向，间距 16px，11px，`--color-text-secondary`）：状态文本（加粗，按状态着色）+ 电量标签 + 附加信息（信号/任务/原因）。

状态圆点与文本颜色：online=`--color-status-online`、busy=`--color-status-busy`、offline=`--color-status-offline`。

电量颜色：>60%（high）=`--color-status-online`、20~60%（mid）=`--color-status-busy`、<20%（low）=`--color-status-error`、`--`（离线）=`--color-text-disabled`。

| 字段 | 值 |
|------|----|
| ID | `DEV-LP-DEVICE-LIST`（列表容器）；每张卡片以 `data-device="{deviceId}"` 标识（无独立 ID） |
| 类型 | QWidget 容器（HTML `<div>`，含 4 张 `<div class="device-card">`） |
| 位置 | 左面板下段，筛选组下方，弹性填充 |
| 用途 | 展示模拟设备列表，单击选中触发详情更新 |
| 默认值 | 加载后含 4 张模拟设备卡片，`device-001` 默认 selected |
| 默认选中 | `device-001`（HTML 中 `class="device-card selected"`） |
| 点击结果 | 单击任一卡片：移除所有卡片的 `selected` 类，为被点击卡片加 `selected` 类，更新 `selectedDevice` 为对应设备，调 `updateDetail()` 同步中心头 `DEV-CE-DEVICE`、右面板 `DEV-RP-ASSIGN-DEVICE`/`DEV-RP-ASSIGN-STATUS`/`DEV-RP-ASSIGN-BTN` 状态 |
| 不可选设备 | `device-004`（offline，`unavailable` 类）仍可点击选中，但选中后右面板指派按钮禁用；`device-002`（busy）同理 |
| 键盘 | 卡片不可原生聚焦（HTML `<div>`）；原型无键盘支持 |
| 五态 | 正常：卡片列表；加载：骨架卡（原型未实现）；空：`暂无设备`（原型未实现）；错误：`--color-status-error` 边框 + `设备加载失败`（原型未实现）；禁用：不适用 |
| 原型行为 | 单击选中并联动中心与右面板；筛选/搜索通过 `display:none` 隐藏不匹配卡片，不改变选中态；不可用设备可选中但指派按钮禁用 |
| CURRENT 映射 | 未实现（`DeviceStatusPanel.cpp` `setupUi`/`refreshList`/`updateDeviceStatus` 为态势页只读卡片，可作为数据源参考，但无点击选择与详情联动） |
| 安全 | 设备状态为本地模拟；不可用设备不可指派（按钮禁用）；不发送真实设备指令 |

> **原型限制**：选中 `device-002`/`device-003`/`device-004` 后，中心详情网格（2.3.1 节）与可用性检查（2.3.2 节）内容**不更新**，仍显示 `device-001` 的静态数据。仅中心头 `DEV-CE-DEVICE` 与右面板指派卡片随选择更新。如需完整联动，属后续任务。

### 2.3.1 详情网格（只读，区域 B 内容）

位于中心区 `center-content` 内，2 列网格（`grid-template-columns:1fr 1fr`），间距 12px。4 张只读详情卡片，均为 `device-001` 静态数据。

详情卡片样式：背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 16px，纵向间距 10px。卡片标题：`--font-size-body`，主文本色，加粗，下边框 1px `--color-border`，下内边距 6px。字段：纵向间距 2px；标签 `--color-text-secondary`、11px；值主文本色、`--font-size-body`。值变体：`highlight`（`--color-primary` 加粗）、`warn`（`--color-threat-medium` 加粗）、`danger`（`--color-danger` 加粗）。

| 卡片 | 标题 | 字段（标签:值） |
|------|------|-----------------|
| 基本信息 | 基本信息 | 设备 ID: `device-001` / 设备名称: `侦察无人机 Alpha` / 设备类型: `UAV 侦察无人机`（highlight）/ 当前状态: `模拟在线`（highlight）/ 归属单位: `模拟侦察组` |
| 运行状态 | 运行状态 | 电量: `82%`（highlight）/ 信号强度: `良好（模拟）` / 通信链路: `[模拟] 本地模拟链路`（warn）/ 当前任务: `无` / 最后心跳: `14:30:15` |
| 能力参数 | 能力参数（模拟） | 载荷类型: `可见光 + 红外` / 最大航程: `5km（模拟）` / 续航时间: `30min（模拟）` / 支持处置: `侦察、监视` |
| 维护信息 | 维护信息（模拟） | 上次检修: `2026-07-20` / 下次检修: `2026-08-20` / 累计运行: `128 小时` / 故障记录: `无`（`--color-status-online`） |

> 详情网格无 `data-testid`（只读展示区）。原型中内容为 `device-001` 静态 HTML，不随选中设备更新。CURRENT 映射：未实现。

### 2.3.2 可用性检查（只读，区域 B 内容）

位于详情网格下方，`availability-panel` 容器。背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 16px，纵向间距 8px。

标题：`[模拟] 可用性检查`，`--font-size-body`，主文本色，加粗。

5 行检查项（`avail-row`，flex 横向，间距 8px，内边距 `6px 0`，下边框 1px `--color-border`，末行无边框）：标签（100px 固定宽，`--color-text-secondary`，`--font-size-caption`）+ 值（弹性，主文本色，`--font-size-caption`）+ 状态徽章（内边距 `2px 8px`，圆角 3px，10px，加粗）。

| 行 | 标签 | 值 | 徽章 | 徽章样式 |
|----|------|----|------|----------|
| 1 | 设备状态 | 在线，可接受指派 | 可用 | ok（`rgba(76,175,80,0.2)` 背景、`--color-status-online` 文本） |
| 2 | 电量充足 | 82%（>30% 阈值） | 满足 | ok |
| 3 | 通信正常 | [模拟] 本地链路正常 | 占位 | warn（`rgba(255,183,77,0.2)` 背景、`--color-threat-medium` 文本） |
| 4 | 无占用任务 | 当前无执行中任务 | 满足 | ok |
| 5 | 类型匹配 | 支持侦察任务类型 | 满足 | ok |

> 可用性检查无 `data-testid`（只读展示区）。内容为 `device-001` 静态 HTML，不随选中设备更新。`通信正常` 标注"占位"以表明通信链路为模拟占位，不接入真实链路。CURRENT 映射：未实现。

## 3. 区域 B：中心区（设备详情）

弹性填充（`flex:1`），背景 `--color-panel`，纵向布局，溢出隐藏。从上到下：中心头 -> 中心内容区（含详情网格与可用性检查，见 2.3.1/2.3.2）。

### 3.1 中心头 `DEV-CE-DEVICE`

高 40px，背景 `--color-toolbar`，下边框 1px `--color-border`，flex 横向，内边距 `0 16px`，间距 12px。左侧标题"设备详情"（`--font-size-title`，主文本色，加粗），右侧副标题 `DEV-CE-DEVICE`。

| 字段 | 值 |
|------|----|
| ID | `DEV-CE-DEVICE` |
| 标签 | - |
| 类型 | QLabel（只读，HTML `<span class="center-subtitle">`） |
| 位置 | 中心头右侧 |
| 用途 | 显示当前选中设备的 ID 与名称 |
| 默认值 | `device-001 · 侦察无人机 Alpha` |
| 样式 | `--color-text-secondary` 文本、`--font-size-caption` |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | `updateDetail()` 触发时更新文本为 `{selectedDevice.id} · {selectedDevice.name}`；选中设备即同步 |
| CURRENT 映射 | 未实现 |
| 安全 | 仅显示模拟设备标识，无设备控制 |

### 3.2 中心内容区

弹性填充（`flex:1`），纵向滚动（`overflow:auto`），内边距 16px，纵向间距 16px。包含详情网格（2.3.1 节）与可用性检查（2.3.2 节），均为只读。无独立 `data-testid`。

## 4. 区域 C：右面板（模拟指派与推荐）

宽 380px 固定（`--size-right-panel-width`），背景 `--color-panel`，纵向布局，溢出隐藏。垂直两段，stretch 3/2：模拟指派（上，`flex:3`）+ 可用设备推荐（下，`flex:2`）。

每段结构：段头（`rp-section-header`，高 32px，背景 `--color-toolbar`，下边框 1px `--color-border`，flex 横向，内边距 `0 8px`，间距 8px）+ 段内容（`rp-section-content`，弹性，纵向滚动，内边距 12px，纵向间距 10px）。段标题：`flex:1`，`--font-size-body`，主文本色，加粗。

### 4.1 模拟指派

段标题：`[模拟] 模拟指派`。段内容包含一张指派卡片 `DEV-RP-ASSIGN`。

指派卡片样式：背景 `--color-toolbar`，圆角 `--radius-control`，内边距 12px，纵向间距 10px。包含 4 行字段 + 1 个按钮 + 1 条说明。

每行（`assign-row`，flex 横向，间距 8px，`--font-size-caption`）：标签（80px 固定宽，`--color-text-secondary`）+ 值（主文本色）。

| 字段 | 值 |
|------|----|
| ID | `DEV-RP-ASSIGN`（卡片容器） |
| 类型 | QWidget 容器（HTML `<div class="assign-card">`） |
| 位置 | 右面板上段内容区 |
| 用途 | 展示选中设备的指派信息与"模拟指派"操作入口 |
| 默认值 | 显示 `device-001` 的指派信息 |
| 点击结果 | 卡片本身无点击；内含子控件各自响应 |
| 键盘 | 卡片不可聚焦；子控件可聚焦 |
| 原型行为 | 随设备选择联动更新 `DEV-RP-ASSIGN-DEVICE`/`DEV-RP-ASSIGN-STATUS`/`DEV-RP-ASSIGN-BTN` |
| CURRENT 映射 | 未实现 |
| 安全 | "模拟指派"仅修改原型显示状态，不发送真实设备指令、不操控设备、不执行真实任务 |

卡片内子控件：

| ID | 标签 | 类型 | 默认值 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|--------|------|-----------|---------|------|---------|---------------|------|
| `DEV-RP-ASSIGN-DEVICE` | 选中设备 | QLabel（只读，HTML `<span>`） | `device-001 侦察无人机 Alpha` | 显示当前选中设备 ID 与名称 | 主文本色，`--font-size-caption` | 无（只读） | 不可聚焦 | `updateDetail()` 触发时更新为 `{id} {name}` | 未实现 | 无 |
| `DEV-RP-ASSIGN-STATUS` | 设备状态 | QLabel（只读，HTML `<span>`） | `模拟在线 · 可用` | 显示选中设备的可用状态 | 主文本色，`--font-size-caption`；可用时 `--color-status-online`，不可用时 `--color-status-error`（由 JS 设置 `style.color`） | 无（只读） | 不可聚焦 | `updateDetail()` 触发时：可用设备显示 `模拟在线 · 可用`（online 色）；不可用设备显示 `{statusLabel} · 不可用`（error 色） | 未实现 | 状态同时用文字与颜色呈现 |
| `DEV-RP-ASSIGN-TARGET` | 关联目标 | QLabel（只读，HTML `<span>`） | `target-001（来自决策页草案）` | 显示关联目标 ID | 主文本色，`--font-size-caption` | 无（只读） | 不可聚焦 | **静态**：不随设备选择更新，恒为 `target-001（来自决策页草案）` | 未实现 | 无 |
| `DEV-RP-ASSIGN-BTN` | 选择用于模拟指派 | QPushButton（HTML `<button class="assign-btn">`） | - | 执行"模拟指派"（仅记录原型状态） | 默认（可用时）：高 36px、内边距 `0 20px`、`--color-primary` 背景、`--color-text-primary` 文本、无边框、圆角 `--radius-control`、`--font-size-body`；hover `--color-primary-hover`；disabled：`--color-border` 背景、`--color-text-disabled` 文本、`cursor:not-allowed` | 可用时：弹出 `alert('[模拟] 已选择 {name} 用于模拟指派（原型状态记录，不发送真实命令）')`；不可用时：按钮禁用，无响应 | Tab 聚焦（可用时），Enter 触发 | `updateDetail()` 触发时：`selectedDevice.available` 为真则 `disabled=false`，否则 `disabled=true`；点击仅弹窗，不修改任何持久状态 | 未实现 | **模拟占位**：仅记录原型指派状态，不发送真实控制命令、不操控设备、不执行真实任务 |
| （无 ID） | 任务类型 | QLabel（只读） | `侦察确认（模拟）` | 显示任务类型 | 主文本色，`--font-size-caption` | 无 | 不可聚焦 | 静态，不随设备更新 | 未实现 | 无 |
| （无 ID） | 说明 | QLabel（只读，`assign-note`） | `[模拟] 此操作仅记录原型指派状态，不发送真实控制命令、不操控设备、不执行真实任务。` | 安全声明 | `--color-text-disabled`，11px，行高 1.5；前置 `[模拟]` 标签为 `--color-status-busy` 加粗（`sim-tag`） | 无 | 不可聚焦 | 静态声明，始终展示 | 未实现 | 明确标注模拟边界 |

### 4.2 可用设备推荐 `DEV-RP-RECOMMEND`

段标题：`[模拟] 可用设备推荐`。段内容容器 `DEV-RP-RECOMMEND`，包含一张推荐卡片（`recommend-card`，背景 `--color-toolbar`，圆角 `--radius-control`，内边距 12px，纵向间距 8px）。

推荐卡片含 4 行设备推荐（`rec-row`，flex 横向，间距 8px，内边距 `4px 0`）：状态圆点（10px 圆形）+ 设备名称（弹性，主文本色，`--font-size-caption`）+ 推荐原因（`--color-text-secondary`，11px）。

| 字段 | 值 |
|------|----|
| ID | `DEV-RP-RECOMMEND`（容器） |
| 类型 | QWidget 容器（HTML `<div class="rp-section-content">`，含 1 张 `<div class="recommend-card">`） |
| 位置 | 右面板下段内容区 |
| 用途 | 展示可用设备推荐列表（只读） |
| 默认值 | 4 行模拟设备推荐 |
| 点击结果 | 无（只读，行不可点击） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：推荐列表；加载：骨架行（原型未实现）；空：`暂无可用设备`（原型未实现）；错误：不适用（本地模拟）；禁用：不适用 |
| 原型行为 | 静态展示 4 行推荐；不随设备选择更新 |
| CURRENT 映射 | 未实现 |
| 安全 | 只读展示，无操作入口；不可用设备同时用文字与颜色标注原因 |

推荐行数据（静态）：

| 设备名称 | 圆点 | 推荐原因 | 原因颜色 |
|----------|------|----------|----------|
| 侦察无人机 Alpha | online（`--color-status-online`） | 类型匹配，电量充足 | `--color-text-secondary` |
| 排爆无人机 Charlie | online（`--color-status-online`） | 类型匹配，电量充足 | `--color-text-secondary` |
| 排爆机器人 Bravo | busy（`--color-status-busy`） | 忙碌中，无法指派 | `--color-threat-medium` |
| 探地雷达 Delta | offline（`--color-status-offline`） | 离线，无法指派 | `--color-status-error` |

> 不可用设备的推荐原因同时用文字（"忙碌中，无法指派"/"离线，无法指派"）与颜色（threat-medium/status-error）呈现，颜色不作为唯一信息。

## 5. 应用壳控件（DEV-* 实例）

本节文档化设备页原型中实例化的应用壳控件，均使用 `DEV-*` 前缀。共享布局、状态规则、CURRENT 映射见 `application-shell.md` 第 3-6 节。本节仅记录设备页实例的差异与具体值。

### 5.1 导航栏

固定宽 80px（`--size-nav-width`），背景 `--color-bg`，右侧 1px `--color-border` 边框，纵向布局。从上到下：UXO logo（高 40px，居中，主色，18px 加粗，字间距 2px）-> 16px 间距 -> 6 个导航项 -> 弹性留白。

导航项样式：高 56px，flex 纵向居中，左侧 3px 透明边框，`--color-text-secondary` 文本，`--font-size-caption`，间距 4px，过渡动画 `--anim-short`。图标 18px，单行高 1。hover：背景 `--color-row-hover`、文本 `--color-text-primary`。selected：背景 `--color-selection`、左侧边框 `--color-primary`、文本 `--color-text-primary`、加粗。

| ID | 标签 | 图标 | 用途 | 默认态 | hover | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|--------|-------|--------|---------|------|---------|---------------|------|
| `DEV-NAV-LOGO` | UXO | - | 仅展示产品标识 | 主色文字，18px 加粗，居中 | 同默认 | 同默认 | 无（仅展示） | 不可聚焦 | 同 CURRENT；无交互 | `application-shell.md` 第 3.1 节 | 无 |
| `DEV-NAV-01` | 态势 | ◎ | 切换到态势页（占位） | 透明背景、辅助色文本 | 背景 `--color-row-hover`、主文本色 | 背景 `--color-selection`、主色左边框 3px、主文本色、加粗 | 切换该项为选中态（移除其他项 selected）；CURRENT 仅 `qDebug`，不切换页面 | 不可原生聚焦（HTML `<div>`）；原型无键盘支持 | 点击仅高亮选中，不路由；中心区仍显示设备页内容 | `application-shell.md` 第 3 节 | 无 |
| `DEV-NAV-02` | 探测 | ◎ | 切换到探测页（占位） | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | 无 |
| `DEV-NAV-03` | 决策 | ◎ | 切换到决策页（占位） | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | 无 |
| `DEV-NAV-04` | 设备 | ◎ | 当前页面（默认选中） | 选中（HTML `class="nav-item selected"`） | 同上 | 同上 | 同上 | 同上 | 默认选中；点击保持选中 | 同上 | 无 |
| `DEV-NAV-05` | 统计 | ◎ | 切换到统计页（占位） | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | 无 |
| `DEV-NAV-06` | 配置 | ◎ | 切换到配置页（占位） | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | 无 |

> CURRENT 导航项为 `QPushButton`，原型为 `<div>`；交互差异见 `application-shell.md` 第 3.2 节。`DEV-NAV-01`/`02`/`03`/`05`/`06` 选中后中心区仍显示设备页内容，属占位高亮。

### 5.2 菜单栏

高 30px（`--size-menu-bar-height`），背景 `--color-menu`，下边框 1px `--color-border`，flex 横向，内边距 `0 4px`。4 个顶级菜单项为 `<button>`，内边距 `6px 12px`，`--font-size-body`，主文本色，透明背景。hover：背景 `--color-border`。disabled（`data-disabled="true"`）：文本 `--color-text-disabled`、`cursor:not-allowed`、hover 无背景变化。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DEV-MENU-FILE` | 文件(&F) | QPushButton（HTML `<button>`） | 菜单栏左1 | 文件菜单入口 | 主文本色、透明背景 | 背景 `--color-border` | 不适用 | 无下拉弹出（原型不实现完整下拉） | Tab 聚焦，Enter 触发（无下拉） | 仅渲染顶级文本，附 tooltip 无；下拉交互属后续任务 | `application-shell.md` 第 4 节 | 无 |
| `DEV-MENU-VIEW` | 视图(&V) | QPushButton | 菜单栏左2 | 视图菜单入口 | 同上 | 同上 | 不适用 | 同上 | 同上 | 同上 | 同上 | 无 |
| `DEV-MENU-TOOLS` | 工具(&T) | QPushButton | 菜单栏左3 | 工具菜单入口 | **禁用**：`data-disabled="true"`、文本 `--color-text-disabled`、`cursor:not-allowed` | 无背景变化（disabled） | 是 | 无响应（禁用） | Tab 聚焦但不可激活 | **禁用并标注"占位"**，附 tooltip `占位控件，未实现` | `application-shell.md` 第 4 节、第 7 节 | 禁用占位，无实际效果 |
| `DEV-MENU-HELP` | 帮助(&H) | QPushButton | 菜单栏左4 | 帮助菜单入口 | 主文本色、透明背景 | 背景 `--color-border` | 不适用 | 无下拉弹出 | 同上 | 同上 | 同上 | 无 |

> CURRENT 共 5 个顶级菜单（文件/视图/工具/设备/帮助）。原型省略"设备(&D)"菜单（含"打开设备控制台"，属安全省略，见 `application-shell.md` 第 7 节），并将"工具(&T)"禁用。原型不实现完整下拉弹出层，仅渲染顶级文本。

### 5.3 工具栏

高 32px（`--size-toolbar-height`），背景 `--color-toolbar`，下边框 1px `--color-border`，flex 横向，间距 8px（`--space-toolbar-gap`），内边距 `0 8px`（`--space-toolbar-pad`）。从左到右：设备控制台标签 -> 刷新设备按钮 -> 弹性留白 -> 模拟设备计数标签。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DEV-TB-CONSOLE` | 设备控制台 | QLabel 占位（HTML `<span class="tb-label">`） | 工具栏左1 | 设备控制台入口（占位） | **禁用**：`data-disabled="true"`、`--color-text-disabled` 文本、`cursor:not-allowed`、`--font-size-caption`、内边距 4px | 无背景变化（disabled） | 是 | 无响应（占位标签，不可点击） | 不可聚焦 | **禁用并标注"模拟占位，无实际效果"**，附 tooltip `占位控件，未实现` | `application-shell.md` 第 5 节、第 7 节；`DeviceControlView.cpp`（空 Tab 壳，未接入） | **禁用占位**：CURRENT `DeviceControlView` 仅含空 Tab 壳（无人机控制/机器人控制），无实际控制逻辑；原型禁用此入口以避免暗示真实设备控制 |
| `DEV-TB-REFRESH` | 刷新设备 | QPushButton（HTML `<button class="tb-btn">`） | 工具栏左2 | 刷新模拟设备状态 | `--color-text-secondary` 文本、透明背景、1px `--color-border` 边框、圆角 `--radius-control`、内边距 `4px 8px`、`--font-size-caption` | 背景 `--color-border`、文本 `--color-text-primary` | 不适用 | 无效果（原型未绑定 click 处理器） | Tab 聚焦，Enter 触发（无效果） | **占位按钮**：tooltip `刷新模拟设备状态`；点击无响应。如未来启用需先登记信号路径 | `application-shell.md` 第 5 节 | 无设备通信，仅本地占位 |
| `DEV-TB-COUNT` | 模拟设备: 4 | QLabel（只读，HTML `<span class="tb-label">`） | 工具栏右，弹性留白后 | 显示模拟设备总数 | 主文本色、`--font-size-caption`、内边距 4px | 同默认 | 不适用 | 无（只读） | 不可聚焦 | 静态显示 `模拟设备: 4`；不随筛选/搜索变化 | `application-shell.md` 第 5 节 | 无 |

> 工具栏 `DEV-TB-CONSOLE` 对应 CURRENT 的"设备控制台"占位（`application-shell.md` 第 7 节列为安全省略）。设备页原型将其保留为禁用占位标签而非完全省略，以展示安全边界标注。`DeviceControlView.cpp` 是 CURRENT 中对应的空壳控件（`QTabWidget` 含"无人机控制"/"机器人控制"两个空面板），但未作为独立页面或工具入口接入。

### 5.4 状态栏

高 28px（`--size-status-bar-height`），背景 `--color-bg`，上边框 1px `--color-border`，flex 横向，内边距 `0 16px`，间距 16px。从左到右：设备状态标签 -> 分隔符 -> 模拟模式标签 -> 分隔符 -> 告警滚动区（弹性）-> 紧急停止按钮。

分隔符为 1px x 18px，背景 `--color-border`。

| ID | 标签 | 类型 | 位置 | 用途 | 默认值 | 样式 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|------|---------|------|---------|---------------|------|
| `DEV-SB-DEVICE` | 设备: 3/4 在线 | QLabel（只读） | 状态栏左1 | 显示在线/总数设备统计 | `设备: 3/4 在线`（3 个在线：device-001/003 在线 + device-002 忙碌计为非在线；实际原型硬编码为 3/4） | 主文本色，`--font-size-caption` | 无（只读） | 不可聚焦 | 静态显示，不随设备状态变化 | `application-shell.md` 第 6.1 节 | 无 |
| `DEV-SB-SIM` | [模拟模式] | QLabel（只读） | 状态栏左2，分隔符后 | 标注当前为本地模拟 | `[模拟模式]` | `--color-status-busy` 文本、`--font-size-caption`、加粗 | 无（只读） | 不可聚焦 | 始终显示 | `application-shell.md` 第 6.1 节 | 明确标注模拟模式 |
| `DEV-SB-ALARM` | - | QWidget 容器（HTML `<div class="status-alarm">`，只读） | 状态栏中，弹性 | 横向滚动展示模拟告警 | 含 1 条告警项：`模拟告警: Delta 离线` | 透明背景，最小宽 400px，高 18px，`overflow:hidden`；告警项：高 18px、`--color-status-busy` 文本、`--font-size-caption`、背景 `--color-toolbar`、内边距 `2px 8px`、圆角 `--radius-control`、右外边距 10px、`white-space:nowrap` | 无（只读） | 不可聚焦 | 静态展示 1 条模拟告警；不滚动、不更新 | `application-shell.md` 第 6.1 节、第 6.2 节 | 只读，无操作入口 |
| `DEV-SB-EMERGENCY` | 紧急停止 | QPushButton（HTML `<button class="emergency-btn">`） | 状态栏右 | 紧急停止（**危险占位**） | - | **禁用**：`disabled`、`data-disabled="true"`、背景 `--color-border`、文本 `--color-text-disabled`、`cursor:not-allowed`；启用态样式（未使用）：80x20px、`--color-danger` 背景、`--color-text-primary` 文本、11px 加粗、无边框、圆角 3px | 禁用，无响应 | 不可聚焦 | **禁用并标注"模拟占位，无实际效果"**，附 tooltip `危险占位：无设备停止效果，本原型禁用`；不弹确认框、不发信号 | `application-shell.md` 第 6.3 节、第 7 节；`StatusBarWidget.cpp` `onEmergencyStop`（CURRENT 弹确认框但无消费者） | **危险占位**：CURRENT 仅弹确认框并发出 `emergencyStopClicked` 信号，但 `MainWindow` 未连接，实际不会停止任何设备；文案"所有设备将立即停止"会误导用户。原型禁用此按钮以避免误导 |

## 6. 状态规则汇总

每个区域必须定义五态。下表汇总各区域的五态实现：

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 左面板搜索框 | 可输入过滤 | 不适用 | 不适用 | 不适用 | 不适用（始终可交互） |
| 左面板刷新按钮 | 可点击（无效果） | 不适用 | 不适用 | 不适用 | 不适用 |
| 左面板类型/状态筛选 | chip 列表可选 | 不适用 | 不适用 | 不适用 | 不适用 |
| 左面板设备列表 | 卡片列表 | 骨架卡（原型未实现） | `暂无设备`（原型未实现） | `--color-status-error` 边框 + `设备加载失败`（原型未实现） | 不适用（始终可交互） |
| 中心设备详情头 | 显示选中设备标识 | 不适用 | `未选择设备`（原型未实现） | 不适用 | 不适用 |
| 中心详情网格 | 4 张只读卡片 | 骨架（原型未实现） | `请选择设备查看详情`（原型未实现） | 不适用 | 不适用 |
| 中心可用性检查 | 5 行检查项 | 骨架（原型未实现） | `请选择设备`（原型未实现） | 不适用 | 不适用 |
| 右面板模拟指派卡片 | 显示指派信息 | 骨架（原型未实现） | `请选择设备`（原型未实现） | 不适用 | 指派按钮按设备可用性禁用 |
| 右面板指派按钮 | 可用时可点击 | 不适用 | 不适用 | 不适用 | 不可用设备时禁用 |
| 右面板推荐列表 | 4 行推荐 | 骨架行（原型未实现） | `暂无可用设备`（原型未实现） | 不适用（本地模拟） | 不适用（只读） |
| 导航栏 | 6 项可点击高亮 | 不适用 | 不适用 | 不适用 | 不适用 |
| 菜单栏 | 4 项（工具禁用） | 不适用 | 不适用 | 不适用 | `DEV-MENU-TOOLS` 禁用 |
| 工具栏 | 3 项（控制台禁用） | 不适用 | 不适用 | 不适用 | `DEV-TB-CONSOLE` 禁用 |
| 状态栏 | 4 项（紧急停止禁用） | 不适用 | 不适用 | 不适用 | `DEV-SB-EMERGENCY` 禁用 |

原型已实现的空态：无（所有区域均有默认模拟数据）。加载态、空态、错误态在原型中均未实现，文字必须同时给出（颜色不作为唯一信息）。CURRENT 映射均为未实现。

## 7. 交互流程

### 7.1 设备选择流程

1. 用户在 `DEV-LP-DEVICE-LIST` 单击设备卡片。
2. 原型 JS 移除所有卡片的 `selected` 类，为被点击卡片加 `selected` 类。
3. 更新 `selectedDevice` 为对应设备对象。
4. 调 `updateDetail()` 同步：
   - 中心头 `DEV-CE-DEVICE` 更新为 `{id} · {name}`。
   - 右面板 `DEV-RP-ASSIGN-DEVICE` 更新为 `{id} {name}`。
   - 右面板 `DEV-RP-ASSIGN-STATUS` 更新：可用设备显示 `模拟在线 · 可用`（online 色）；不可用设备显示 `{statusLabel} · 不可用`（error 色）。
   - 右面板 `DEV-RP-ASSIGN-BTN`：可用设备 `disabled=false`；不可用设备 `disabled=true`。
5. **不更新**：中心详情网格（2.3.1 节）、可用性检查（2.3.2 节）、`DEV-RP-ASSIGN-TARGET`、推荐列表（4.2 节）均为静态，仍显示初始 `device-001` 数据。

### 7.2 筛选与搜索流程

类型/状态筛选与搜索可叠加生效：

1. 用户点击 `DEV-LP-FILTER-TYPE` 中某 chip -> 更新 `filters.type` -> 调 `applyFilters()`。
2. 用户点击 `DEV-LP-FILTER-STATUS` 中某 chip -> 更新 `filters.status` -> 调 `applyFilters()`。
3. 用户在 `DEV-LP-SEARCH` 输入文本 -> 遍历设备卡片，匹配 `id`/`name`/`type`（包含、不区分大小写）。
4. `applyFilters()` 遍历所有卡片：类型与状态均满足才显示（`display:''`），否则隐藏（`display:none`）。
5. 搜索与筛选独立运行：搜索通过 input 事件直接设 `display`，筛选通过 `applyFilters()` 设 `display`。两者最后执行的覆盖前者（原型已知行为：搜索后切换筛选会重置搜索的隐藏效果，反之亦然）。
6. 隐藏的卡片不改变 `selected` 类；若选中卡片被隐藏，仍保持选中，中心与右面板仍显示该设备。

### 7.3 模拟指派流程

1. 用户在 `DEV-LP-DEVICE-LIST` 选中一个**可用**设备（`available === true`）。
2. `DEV-RP-ASSIGN-BTN` 启用。
3. 用户点击 `DEV-RP-ASSIGN-BTN` -> 弹出 `alert`：`[模拟] 已选择 {name} 用于模拟指派（原型状态记录，不发送真实命令）`。
4. 用户关闭弹窗。**不修改**任何持久状态、不发送任何指令、不操控设备。
5. 若选中**不可用**设备（busy/offline），`DEV-RP-ASSIGN-BTN` 禁用，点击无响应。

## 8. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280x720 | 左面板 360px 固定，右面板 380px 固定，中心区弹性（约 470px 宽）；详情网格 2 列在各半宽内完整显示（每卡片约 220px）；可用性检查 5 行需完整显示；工具栏 3 项（控制台+刷新+计数）需在 32px 高内完整显示；状态栏告警区最小 400px，紧急停止 80px 不溢出 |
| 1920x1080 | 默认尺寸；左面板 360px、中心区弹性、右面板 380px；所有控件按 token 展示；详情网格 2 列间距 12px；右面板两段 3:2 分配 |
| 3840x2160 | 固定区域不变；中心区弹性区按比例放大；字号与控件尺寸保持固定 px；左/右面板宽度不变 |

原型在 1280x720 下需验证：详情网格 4 张卡片不溢出、可用性检查 5 行完整可见、右面板指派卡片末行说明不被截断。

## 9. 安全清单

本页面所有控件必须遵守以下安全约束：

| 控件 | 约束 |
|------|------|
| 设备列表 `DEV-LP-DEVICE-LIST` | 仅本地模拟数据；不可用设备可选中但不可指派（按钮禁用） |
| 模拟指派按钮 `DEV-RP-ASSIGN-BTN` | **模拟占位**：仅记录原型指派状态，不发送真实控制命令、不操控设备、不执行真实任务；不可用设备时禁用 |
| 可用性检查 | 只读展示，"通信正常"标注"占位"以表明模拟链路 |
| 设备控制台 `DEV-TB-CONSOLE` | **禁用占位**：标注"模拟占位，无实际效果"；CURRENT `DeviceControlView` 为空壳，无实际控制逻辑 |
| 紧急停止 `DEV-SB-EMERGENCY` | **危险占位**：禁用并标注"模拟占位，无实际效果"；CURRENT 仅弹确认框无消费者，原型禁用以避免误导 |
| 工具菜单 `DEV-MENU-TOOLS` | 禁用占位 |
| 刷新按钮 `DEV-LP-REFRESH`/`DEV-TB-REFRESH` | 占位，无设备通信，无网络请求 |
| 推荐列表 `DEV-RP-RECOMMEND` | 只读，无操作入口；不可用设备同时用文字与颜色标注 |
| 导航项 `DEV-NAV-01` 至 `DEV-NAV-06` | 仅高亮切换，不路由页面、不触发设备动作 |

所有模拟操作与结果必须带"模拟"或"演示"字样。涉及设备状态、可用性、推荐的内容若来自本地模拟，必须在区域标题或控件旁标注"模拟"（如 `[模拟] 模拟指派`、`[模拟] 可用性检查`、`[模拟模式]`）。设备可用/忙碌/离线原因必须同时用文字和状态圆点标识，颜色不作为唯一信息。不可用设备不可指派（按钮禁用）。本页面不实现登录、角色切换、外部通信、持久化、UXR、MOS，不描述真实设备控制或排弹动作。

## 10. CURRENT 映射总结

| 页面元素 | CURRENT 源码位置 |
|---------|------------------|
| 设备页整体 | 未实现（CURRENT 无独立设备页；仅态势页含设备相关只读组件） |
| 左面板设备列表 | 未实现（`DeviceStatusPanel.cpp` `setupUi`/`refreshList`/`updateDeviceStatus` 为态势页右面板只读卡片，无点击选择与详情联动） |
| 搜索框 | 未实现 |
| 类型/状态筛选 | 未实现 |
| 中心设备详情头 | 未实现 |
| 中心详情网格 | 未实现 |
| 中心可用性检查 | 未实现 |
| 右面板模拟指派 | 未实现 |
| 右面板指派按钮 | 未实现 |
| 右面板推荐列表 | 未实现 |
| 设备控制台占位 | `DeviceControlView.cpp`（空 `QTabWidget` 壳，含"无人机控制"/"机器人控制"空面板，未接入任何页面） |
| 导航栏 | `application-shell.md` 第 3 节（`NavigationWidget.cpp`） |
| 菜单栏 | `application-shell.md` 第 4 节（`MainWindow.cpp` `createMenuBar`） |
| 工具栏 | `application-shell.md` 第 5 节（`MainWindow.cpp` `createMapToolbar`） |
| 状态栏 | `application-shell.md` 第 6 节（`StatusBarWidget.cpp`） |
| 紧急停止 | `application-shell.md` 第 6.3 节（`StatusBarWidget.cpp` `onEmergencyStop`，`MainWindow` 未连接 `emergencyStopClicked`） |
| 模拟数据类型 | `Types.h`（`DeviceInfo`/`DeviceStatus` 枚举） |

## 11. DEV-* ID 索引

下表列出本文档化的全部 `DEV-*` ID，供 HTML 原型与 Playwright 测试对齐：

| ID | 控件 | 区域 |
|----|------|------|
| `DEV-NAV-LOGO` | 导航栏 logo | 应用壳 |
| `DEV-NAV-01` | 态势导航项 | 应用壳 |
| `DEV-NAV-02` | 探测导航项 | 应用壳 |
| `DEV-NAV-03` | 决策导航项 | 应用壳 |
| `DEV-NAV-04` | 设备导航项（默认选中） | 应用壳 |
| `DEV-NAV-05` | 统计导航项 | 应用壳 |
| `DEV-NAV-06` | 配置导航项 | 应用壳 |
| `DEV-MENU-FILE` | 文件菜单 | 应用壳 |
| `DEV-MENU-VIEW` | 视图菜单 | 应用壳 |
| `DEV-MENU-TOOLS` | 工具菜单（禁用占位） | 应用壳 |
| `DEV-MENU-HELP` | 帮助菜单 | 应用壳 |
| `DEV-TB-CONSOLE` | 设备控制台标签（禁用占位） | 应用壳 |
| `DEV-TB-REFRESH` | 刷新设备按钮（占位） | 应用壳 |
| `DEV-TB-COUNT` | 模拟设备计数标签 | 应用壳 |
| `DEV-LP-SEARCH` | 搜索框 | 左面板 |
| `DEV-LP-FILTER-TYPE` | 类型筛选 chip 组 | 左面板 |
| `DEV-LP-FILTER-STATUS` | 状态筛选 chip 组 | 左面板 |
| `DEV-LP-REFRESH` | 刷新按钮（占位） | 左面板 |
| `DEV-LP-DEVICE-LIST` | 设备列表容器 | 左面板 |
| `DEV-CE-DEVICE` | 选中设备标识标签 | 中心 |
| `DEV-RP-ASSIGN` | 模拟指派卡片容器 | 右面板 |
| `DEV-RP-ASSIGN-TARGET` | 关联目标标签 | 右面板 |
| `DEV-RP-ASSIGN-DEVICE` | 选中设备标签 | 右面板 |
| `DEV-RP-ASSIGN-STATUS` | 设备状态标签 | 右面板 |
| `DEV-RP-ASSIGN-BTN` | 模拟指派按钮 | 右面板 |
| `DEV-RP-RECOMMEND` | 可用设备推荐列表容器 | 右面板 |
| `DEV-SB-SIM` | 模拟模式标签 | 应用壳 |
| `DEV-SB-EMERGENCY` | 紧急停止按钮（禁用占位） | 应用壳 |
| `DEV-SB-DEVICE` | 设备统计标签 | 应用壳 |
| `DEV-SB-ALARM` | 告警滚动区 | 应用壳 |

导航栏、菜单栏、工具栏、状态栏的共享布局与 CURRENT 映射见 `application-shell.md` 第 3 节（导航栏）、第 4 节（菜单栏）、第 5 节（工具栏）、第 6 节（状态栏）。本文使用 `DEV-*` 前缀实例化这些控件，行为与 `application-shell.md` 一致。
