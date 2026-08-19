# 探测页面设计

状态：`CURRENT 部分实现（AI 自动检测工作流）/ TARGET 契约保留`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](../design-system.md)
应用壳：[docs/ui/application-shell.md](../application-shell.md)
一览：[docs/ui/pages/index.md](index.md)
原型：[docs/ui/prototypes/detection/index.html](../prototypes/detection/index.html)
截图：[docs/ui/images/detection/overview-1920x1080.png](../images/detection/overview-1920x1080.png)

> 本文是探测页面（detection page）的完整设计契约。每个交互控件拥有稳定 `DET-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据，控件 ID、区域结构和状态规则不得在实现阶段自行变更。所有视觉值取自 `design-system.md`。

CURRENT 来源：
- **本页已实现为导航 index 1 独立页面（`DetectionView`，objectName `detectionPage`）。** 视频（态势页 PiP）每 3 秒抽帧送 `DetectionEngine` 真实 ONNX 推理，结果自动填充左侧结果表；点击行查看干净原图 + 分类结果（异常热力图在右侧独立模块展示），确认/拒绝（误报）人工二次校验联动目标状态机。
- 导航栏/菜单栏/工具栏/状态栏：见 [`application-shell.md`](../application-shell.md)
- 页面实现与关联组件：
  - [`src/MainWindow/DetectionView.cpp`](../../../src/MainWindow/DetectionView.cpp)（探测页布局与结果交互）
  - [`src/Detection/DetectionEngine.cpp`](../../../src/Detection/DetectionEngine.cpp)（PatchCore + YOLOv8-cls 双阶段 ONNX 推理）
  - [`src/MainWindow/VideoStreamPanel.cpp`](../../../src/MainWindow/VideoStreamPanel.cpp)（视频播放与每 3 秒抽帧）
  - [`src/MainWindow/LeftPanelWidget.cpp`](../../../src/MainWindow/LeftPanelWidget.cpp)（态势页目标表，与探测页/详情浮层三向联动）
  - [`include/Core/Data/Types.h`](../../../include/Core/Data/Types.h)（数据类型枚举）
- 功能契约：[`docs/features/detection-onnx-integration.md`](../features/detection-onnx-integration.md)

## 1. 页面概述

探测页面是系统的第二页（导航 `DET-NAV-02` 默认选中）。它一屏呈现：左侧搜索栏、类型/威胁/状态三维筛选器与探测目标表（5 个模拟目标）；中心目标证据 Tab（识别证据/探测来源/状态历史）与模拟确认/拒绝操作条；右侧目标详情字段与状态历史时间线。所有数据来自 HTML 原型内嵌的本地固定 fixture（5 条模拟目标），所有操作仅修改原型显示状态，不调用真实传感器或 AI，不连接真实设备、不写入数据库、不执行排爆动作。

页面整体布局由应用壳定义（见 `application-shell.md` 第 2 节）。本文档化页面内部的三个内容区域与复用的应用壳控件：

| 区域 | 位置 | 内部组件 | CURRENT 映射 |
|------|------|----------|--------------|
| A | 左面板 | 搜索栏、三维筛选器组、目标表 | 未按原型实现为独立左面板；等价物为 `DetectionView` 结果表 `m_resultTable`（7 列），搜索/筛选未实现 |
| B | 中心区 | 证据面板头、证据 Tab 栏、证据内容区、确认操作条 | 已实现为独立页面（`DetectionView`，导航 index1）：顶部摘要条 + 中栏 `m_viewerLabel` 标注图查看器 + 底部操作条；证据 Tab 未实现 |
| C | 右面板 | 目标详情、状态历史时间线 | 已实现于 `DetectionView` 内：`m_detailLabel` 详情、`m_timelineLabel` 状态时间线、`m_heatmapLabel` 热力图；分类 Top-3（`m_classLabel`）在中栏 |

应用壳控件（导航栏、菜单栏、工具栏、状态栏）的详细规格见 `application-shell.md`，本文第 5 节给出本页专属的 `DET-*` ID 与简要规格。

## 2. 区域 A：左面板

宽 360px 固定（`--size-left-panel-width`），背景 `--color-panel`，外边距 8px，间距 8px。从上到下：搜索栏工具条 -> 筛选器组 -> 目标表。

### 2.1 搜索栏工具条

背景 `--color-toolbar`，圆角 `--radius-control`，下边距 8px。内边距 `8px 6px`，间距 8px。包含搜索框、清除按钮、刷新按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|-----------|---------|------|---------|---------------|------|
| `DET-LP-SEARCH` | - | input | 搜索栏左，弹性宽 | 按文字过滤目标表 | 高 28px；默认 `--color-bg` 背景、`--color-border` 边框、圆角 `--radius-control`；focus 边框 `--color-border-focus`；placeholder `搜索目标 ID 或类型...`、`--color-text-disabled`；内边距 `0 8px` | input 事件触发，遍历表格行，按目标 ID、类型、威胁字段做大小写不敏感包含匹配，不匹配的行 `display:none` | Tab 聚焦，Esc 不清空（无 Esc 绑定） | 同上；空文本显示全部目标 | 未实现（CURRENT 探测页 `DetectionView` 无搜索框） | 无 |
| `DET-LP-CLEAR` | 清除 | button | 搜索框右，50x28 | 清空搜索框并恢复全部行 | 默认 `--color-toolbar` 背景、`--color-text-secondary` 文本、1px `--color-border` 边框、圆角 `--radius-control`；hover 背景 `--color-border`、文本 `--color-text-primary`；字号 `--font-size-caption` | 清空搜索框值，触发 input 事件重新过滤 | Tab 聚焦，Enter 触发 | 同上 | 未实现（CURRENT 无清除按钮，态势页 `LeftPanelWidget` 仅有搜索框无清除按钮） | 无 |
| `DET-LP-REFRESH` | 刷新 | button | 清除右，主按钮 | 请求刷新模拟探测数据 | 主要按钮变体：`--color-primary` 背景、`--color-text-primary` 文本；hover `--color-primary-hover`；高 28px，内边距 `0 12px`，字号 `--font-size-caption` | 空操作（JS 绑定 `()=>{}`，无实际刷新逻辑） | Tab 聚焦，Enter 触发 | 点击后无视觉反馈，不重新加载数据 | 未实现（探测页无刷新按钮；清空结果经态势页地图工具栏 [重置] 触发 `MainWindow` 复位流程调用 `m_detectionView->clearResults()`） | 无设备通信，仅内存同步（CURRENT）；原型为空操作 |

### 2.2 筛选器组

背景 `--color-toolbar`，圆角 `--radius-control`，内边距 `8px 6px`，间距 6px。三行筛选器，每行结构：标签（52px 宽，`--color-text-secondary`，`--font-size-caption`）+ chips 容器（弹性，`flex-wrap`）。

每个 chip 为 `<span>` 元素，内边距 `3px 8px`，字号 11px，默认 `--color-bg` 背景、`--color-text-secondary` 文本、1px `--color-border` 边框、圆角 10px。hover：背景 `--color-border`、文本 `--color-text-primary`。

**类型与状态筛选 chip active 态**：背景 `--color-primary`、白色文本、边框 `--color-primary`。

**威胁筛选 chip active 态**按威胁等级着色：高=`--color-threat-high` 背景、白色文本；中=`--color-threat-medium` 背景、黑色文本；低=`--color-threat-low` 背景、黑色文本。威胁行的"全部"chip 使用 `threat-high` 类，active 时为红色背景白色文本。

| ID | 标签 | 类型 | 位置 | 用途 | 默认选中 | chip 列表 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|----------|-----------|---------|------|---------|---------------|------|
| `DET-LP-FILTER-TYPE` | 类型 | span 容器 | 筛选器组第 1 行 | 按目标类型筛选目标表 | `全部` | 全部(all)、反跑道雷、航弹、火箭弹、集束弹 | 点击 chip：同组移除其他 active，当前 chip 加 active，`filters.type` 更新为 chip 的 `data-value`，调用 `applyFilters` 按 AND 逻辑过滤 | chip 为 `<span>` 无 tabindex，不可键盘聚焦（无障碍缺口） | 同上；默认选中"全部"显示所有类型 | 未实现（CURRENT `LeftPanelWidget` 声明 `m_typeFilterCombo` 成员但 `setupUi` 未实例化） | 无 |
| `DET-LP-FILTER-THREAT` | 威胁 | span 容器 | 筛选器组第 2 行 | 按威胁等级筛选目标表 | `全部` | 全部(all)、高、中、低 | 同上，`filters.threat` 更新 | 同上 | 同上；active chip 按威胁等级着色（高=红、中=橙、低=黄） | 未实现（CURRENT 声明 `m_threatFilterCombo` 成员但未实例化） | 无 |
| `DET-LP-FILTER-STATUS` | 状态 | span 容器 | 筛选器组第 3 行 | 按模拟状态筛选目标表 | `全部` | 全部(all)、已发现(Detected)、已确认(Confirmed)、处置中(Disposing)、已完成(Disposed) | 同上，`filters.status` 更新 | 同上 | 同上 | 未实现（CURRENT 无状态筛选器） | 无 |

`applyFilters` 逻辑：遍历表格所有行，对每个目标检查 `type`/`threat`/`status` 三个条件，三者均为 `all` 或匹配目标值时显示，否则隐藏。过滤后调用 `updateCount` 更新 `DET-LP-TABLE-COUNT` 显示可见行数。

> 注：筛选 chip 为 `<span>` 元素，无 `tabindex` 属性，不可通过 Tab 键聚焦。原型存在无障碍缺口，TARGET 实现时应补齐 `tabindex` 与键盘激活。

### 2.3 目标表

`--color-panel` 背景，1px `--color-border` 边框，圆角 `--radius-control`，弹性填充剩余空间。分表头栏与表内容两部分。

#### 2.3.1 表头栏

高 32px，背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`，间距 8px。从左到右：标题"探测目标列表"（`--font-size-body`，加粗）+ 计数标签 + 弹性留白 + 排序按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|-----------|---------|------|---------|---------------|------|
| `DET-LP-TABLE-COUNT` | 5 个目标 | span | 表头栏标题右 | 显示当前可见目标数量 | `--color-text-secondary`，`--font-size-caption` | 无（只读） | 不可聚焦 | 初始显示"5 个目标"；筛选或搜索后由 `updateCount` 更新为可见行数 | `DetectionView.cpp` `m_summaryLabel`（显示 `已分析 N 帧 · 异常 M`，由 `updateSummaryLabel` 维护） | 无 |
| `DET-LP-SORT-THREAT` | 威胁排序 | span | 表头栏右 1 | 按威胁等级排序目标表 | 默认 `--color-text-secondary` 文本、透明背景、1px `--color-border` 边框、圆角 3px、字号 11px；hover 背景 `--color-border`；active（`.active` 类）背景 `--color-primary`、白色文本（原型从未添加 active 类） | 空操作（JS 绑定 `()=>{}`） | span 无 tabindex，不可键盘聚焦 | 点击无效果；原型中排序功能为占位，不改变表格行顺序 | 未实现（CURRENT 无排序按钮） | 无 |
| `DET-LP-SORT-CONF` | 置信度排序 | span | 表头栏右 2 | 按置信度排序目标表 | 同上 | 同上 | 同上 | 同上 | 未实现 | 无 |

> 注：表头 `<th>` 元素带有 `data-sort` 属性（type/threat/confidence/status）和排序箭头视觉，但 JS 未绑定 click 事件。仅表头栏的排序按钮（`DET-LP-SORT-THREAT`/`DET-LP-SORT-CONF`）有 click 事件（空操作）。原型中排序功能为占位。

#### 2.3.2 目标表 `DET-LP-TARGET-TABLE`

7 列，单选行模式，可点击行选择，交替行色。表头背景 `--color-toolbar`、主文本色、内边距 `6px 4px`，`position:sticky` 顶部固定。行高 36px。

| 列 | 表头 | 宽度 | 内容 | 文字色 |
|----|------|------|------|--------|
| 0 | 目标 ID | 80px | `target.id`（如 `target-001`） | `--color-text-primary` |
| 1 | 类型 | 70px | `target.type`（如 `反跑道雷`），按威胁等级着色 | 高=`--color-threat-high`、中=`--color-threat-medium`、低=`--color-threat-low` |
| 2 | 威胁 | 50px | `<span class="threat-tag {level}">` 高/中/低 | 标签色：高=`--color-threat-high`、中=`--color-threat-medium`、低=`--color-threat-low`，背景为对应色 20% 透明度 |
| 3 | 置信度 | 50px | `XX%`（confidence 整数） | `--color-text-primary` |
| 4 | 位置 | 80px | `X:n Y:n`（position.x、position.y 取整） | `--color-text-primary` |
| 5 | 状态 | 60px | `<span class="status-tag {status}">` 已发现/已确认/处置中/已完成 | 已发现=`--color-threat-medium`、已确认=`--color-primary`、处置中=`--color-threat-high`、已完成=`--color-text-disabled`，背景为对应色 15% 透明度 |
| 6 | 探测源 | 60px | `target.source`（如 `UAV-1`） | `--color-text-primary` |

行样式：默认 `--color-panel`，偶数行 `--color-panel`（同色，交替行色规则 `nth-child(even)` 设置同色背景）；hover `--color-row-hover`；选中 `--color-selection`。鼠标手型。

| 字段 | 值 |
|------|----|
| ID | `DET-LP-TARGET-TABLE` |
| 类型 | table |
| 位置 | 左面板，筛选器组下方，弹性填充 |
| 用途 | 显示模拟探测目标列表，单选触发目标选择 |
| 默认值 | 5 条模拟目标（见下方模拟数据），默认选中第 1 行 `target-001` |
| 点击结果 | 单击行 -> 移除其他行 `selected` 类，当前行加 `selected`；`selectedTarget` 更新为对应目标对象；调用 `updateDetail` 更新 `DET-CE-TARGET` 与 `DET-CE-CONFIRM-INFO` |
| 双击结果 | 无（原型未绑定双击事件） |
| 键盘 | `<tr>` 无 tabindex，不可键盘聚焦（无障碍缺口） |
| 五态 | 正常：列表展示；加载：骨架行（原型未实现）；空：`暂无目标`（原型未实现）；错误：边框 `--color-status-error` + `目标加载失败`（原型未实现）；禁用：不适用 |
| 原型行为 | 单击选中行，同步更新中心区证据面板头目标标签与确认信息标签；右面板详情与时间线不随选择更新（始终显示 target-001 数据） |
| CURRENT 映射 | `DetectionView.cpp` `m_resultTable`（objectName `detectionResultTable`，7 列，不可编辑）；`cellClicked` -> `onResultSelected`：更新中/右栏并发出 `resultSelected`，经 `MainWindow::onSelectTargetEverywhere` 参与三向选中联动 |
| 安全 | 无设备控制，仅前端选择 |

模拟目标数据（JS 内嵌 fixture）：

| ID | 类型 | 威胁 | 置信度 | 位置 | 状态 | 探测源 |
|----|------|------|--------|------|------|--------|
| target-001 | 反跑道雷 | 高 | 86% | X:109 Y:34 | 已发现(Detected) | UAV-1 |
| target-002 | 航弹 | 高 | 72% | X:87 Y:52 | 已发现(Detected) | UAV-2 |
| target-003 | 火箭弹 | 中 | 91% | X:45 Y:78 | 已确认(Confirmed) | GPR-1 |
| target-004 | 集束弹 | 低 | 65% | X:23 Y:90 | 已发现(Detected) | UAV-1 |
| target-005 | 反跑道雷 | 中 | 78% | X:156 Y:12 | 处置中(Disposing) | UAV-2 |

## 3. 区域 B：中心区

弹性宽（`flex:1`），背景 `--color-bg`，垂直布局。包含证据面板头、证据 Tab 栏、证据内容区、确认操作条。从上到下依次排列。

### 3.1 证据面板头

高 36px，背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 12px`，间距 12px。标题"目标详情"（`--font-size-body`，加粗，`--color-text-primary`）+ 目标标签。

| ID | 标签 | 类型 | 位置 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|-----------|---------|------|---------|---------------|------|
| `DET-CE-TARGET` | target-001 · 反跑道雷 · 高威胁 | span | 证据面板头标题右 | 显示当前选中目标的摘要 | `--color-text-secondary`，`--font-size-caption` | 无（只读） | 不可聚焦 | 初始显示 `target-001 · 反跑道雷 · 高威胁`；选中表格行后由 `updateDetail` 更新为 `{id} · {type} · {threat}威胁` | `DetectionView.cpp` 顶部摘要条（`[模拟]` 标识 + `m_summaryLabel` 已分析/异常计数）与底部 `m_statusLabel`（`当前目标: %1 · 状态: %2`，未选中时 `未选中结果`）共同承担摘要显示 | 无 |

### 3.2 证据 Tab 栏

水平排列，背景 `--color-toolbar`，底部 1px `--color-border` 边框。三个 Tab 为 `<div>` 元素，`tabindex="0"`，内边距 `8px 20px`，`--font-size-body`。

默认态：透明背景、`--color-text-secondary` 文本、底部 2px 透明边框。hover：背景 `--color-border`、文本 `--color-text-primary`。选中态：背景 `--color-bg`、`--color-text-primary` 文本、底部 2px `--color-primary` 边框。focus-visible：2px `--color-border-focus` 外轮廓。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|--------|---------|------|---------|---------------|------|
| `DET-CE-TAB-RECOG` | 识别证据 | div(tabindex=0) | Tab 栏左 | 切换到识别证据视图 | 选中 | 背景 `--color-bg`、主文本色、底部 2px `--color-primary` | 移除其他 Tab 的 selected，当前加 selected | Tab 聚焦（tabindex=0）；Enter/Space 在部分浏览器触发 click，不保证跨浏览器一致 | 默认选中；点击切换 selected 类，但不改变 `DET-CE-CONTENT` 内容（始终显示识别证据卡片） | 未实现 | 无 |
| `DET-CE-TAB-SOURCE` | 探测来源 | div(tabindex=0) | Tab 栏中 | 切换到探测来源视图 | 未选中 | 同上 | 同上 | 同上 | 同上；原型中"探测来源"视图无内容 | 未实现 | 无 |
| `DET-CE-TAB-STATE` | 状态历史 | div(tabindex=0) | Tab 栏右 | 切换到状态历史视图 | 未选中 | 同上 | 同上 | 同上 | 同上；原型中"状态历史"视图无内容 | 未实现 | 无 |

> 注：原型中 Tab 切换仅改变 selected 类，不切换 `DET-CE-CONTENT` 内容。三个 Tab 始终显示相同的识别证据卡片。TARGET 实现时应补齐 Tab 内容切换。

### 3.3 证据内容区

`DET-CE-CONTENT` 为弹性容器，`overflow:auto`，内边距 16px，间距 12px。包含两张证据卡片。

| ID | 标签 | 类型 | 位置 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|-----------|---------|------|---------|---------------|------|
| `DET-CE-CONTENT` | - | div 容器 | 证据 Tab 栏下方，弹性填充 | 承载证据卡片列表 | 背景 `--color-bg`，`overflow:auto`，内边距 16px，间距 12px | 无 | 不可聚焦 | 始终显示两张识别证据卡片；Tab 切换不改变内容 | `DetectionView.cpp` 中栏：`m_viewerLabel` 标注图（检测框叠加，随窗口等比缩放）+ `m_heatmapLabel` 热力图（无热力图时占位文字）+ `m_classLabel` 分类 Top-3；原型证据卡片形式未实现 | 无 |
| `DET-CE-RECOG-1` | [模拟] 识别结果 | div 卡片 | 证据内容区第 1 张 | 展示模拟识别结果证据 | 背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 12px，间距 8px | 无（只读） | 不可聚焦 | 固定显示 target-001 的识别结果：类型 反跑道雷、置信度 86%、尺寸 120cm x 25cm、金属外壳；来源"识别算法 v1.0（占位）"；标注 `[模拟数据，不连接真实识别系统]` | 未实现 | 模拟数据，不连接真实识别系统 |
| `DET-CE-RECOG-2` | [模拟] 融合分析 | div 卡片 | 证据内容区第 2 张 | 展示模拟多源融合分析证据 | 同上 | 无（只读） | 不可聚焦 | 固定显示融合分析：来源 UAV-1 可见光 + GPR-1 地质雷达、一致性 0.82、冲突无、建议聚能引爆；标注 `[模拟数据，不执行真实处置]` | 未实现 | 模拟数据，不执行真实处置 |

证据卡片内部结构：头部（标题 `--font-size-body` 加粗 + 来源标签 `--color-text-secondary` 自动右对齐）+ 正文（`--color-text-secondary`，`--font-size-caption`，行高 1.6）+ 元信息行（`--color-text-disabled`，11px，间距 16px）。

`DET-CE-RECOG-1` 元信息：采集 14:30:02、处理 14:30:03、确认 待人工确认。
`DET-CE-RECOG-2` 元信息：融合时间 14:30:05、置信度 82%。

### 3.4 确认操作条

高 56px，背景 `--color-toolbar`，顶部 1px `--color-border` 边框，内边距 `0 16px`，间距 16px。从左到右：确认信息标签 + 拒绝消息标签（默认隐藏）+ 弹性留白 + 模拟确认按钮 + 拒绝确认按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 样式与状态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|-----------|---------|------|---------|---------------|------|
| `DET-CE-CONFIRM-INFO` | 当前目标: target-001 · 状态: 已发现 | span | 确认条左 | 显示当前选中目标与模拟状态 | `[模拟]` 前缀为 `--color-status-busy` 色、`--font-size-caption`、加粗；目标信息为 `--color-text-primary`、`--font-size-body` | 无（只读） | 不可聚焦 | 初始显示 `当前目标: target-001 · 状态: 已发现`；选中行或确认后由 `updateDetail` 更新为 `当前目标: {id} · 状态: {statusLabel}` | `DetectionView.cpp` `m_statusLabel`（`当前目标: %1 · 状态: %2`；未选中时 `未选中结果`） | 无 |
| `DET-CE-REJECT-MSG` | - | span | 确认信息右，弹性宽 | 显示拒绝/无法确认的提示消息 | 默认 `display:none` 隐藏；显示时 `--color-danger` 色、`--font-size-caption`、弹性宽 | 无（只读） | 不可聚焦 | 确认按钮点击时若目标状态非 Detected，显示 `该目标已确认或正在处置，无法重复确认`；确认成功时隐藏 | 未实现；CURRENT 以按钮禁用态阻止重复操作（确认/拒绝仅选中且 `Pending` 时启用），无提示文本 | 模拟提示，无实际拒绝操作 |
| `DET-CE-CONFIRM` | 模拟确认 | button | 确认条右 1 | 模拟确认当前目标 | 主要按钮变体：`--color-primary` 背景、`--color-text-primary` 文本；hover `--color-primary-hover`；高 36px，内边距 `0 20px`，`--font-size-body`；disabled 不适用（始终启用） | 若 `selectedTarget.status === 'Detected'`：隐藏 `DET-CE-REJECT-MSG`，将状态改为 `Confirmed`，更新 `selectedTarget.statusLabel` 为 `已确认`，调用 `updateDetail` 更新 `DET-CE-TARGET` 与 `DET-CE-CONFIRM-INFO`，更新表格行第 6 列状态标签为 `已确认`。若状态非 `Detected`：显示 `DET-CE-REJECT-MSG` 文本 `该目标已确认或正在处置，无法重复确认` | Tab 聚焦，Enter/Space 触发 | 同上；确认后表格行状态标签从橙色"已发现"变为绿色"已确认" | `DetectionView.cpp` `m_confirmButton`：仅选中且 `Pending` 时启用；点击发出 `targetConfirmed(targetId)` -> `MainWindow::onTargetConfirmed` -> `selectTarget` + `requestSelectedTargetStatus(Confirmed)`，并回写态势页左表状态列 | 仅修改原型显示状态，不调用真实传感器或 AI |
| `DET-CE-REJECT` | 拒绝确认 | button | 确认条右 2 | 拒绝确认当前目标 | **始终禁用**：`--color-border` 背景、`--color-text-disabled` 文本（inline style 覆盖 `.confirm-btn` 默认样式）；tooltip `拒绝确认（需先选择目标）`；高 36px，内边距 `0 20px` | 无（disabled，不响应点击） | 不可聚焦（disabled） | 始终禁用；tooltip 提示"需先选择目标"但 JS 从不启用此按钮 | `DetectionView.cpp` `m_rejectButton`：已实现拒绝确认（原型此处为始终禁用占位）；仅选中且 `Pending` 时启用，点击发出 `targetRejected(targetId)` -> `MainWindow::onTargetRejected` -> `selectTarget` + `markSelectedTargetFalseAlarm`（标记误报）并回写左表状态列 | 模拟占位，无实际效果 |

> 注：原型中 `DET-CE-REJECT` 始终禁用（tooltip 暗示选中后应启用，但 JS 从未实现）。CURRENT `DetectionView` 已实现拒绝：仅选中且 `Pending` 时启用，拒绝将目标标记为误报（FalseAlarm）。

## 4. 区域 C：右面板

宽 380px 固定（`--size-right-panel-width`），背景 `--color-panel`，垂直布局。两段：目标详情（`flex:3`）+ 状态历史时间线（`flex:2`）。

### 4.1 目标详情

`DET-RP-DETAIL` 为内容容器，`overflow:auto`，内边距 8px。头部 32px：背景 `--color-toolbar`，标题 `[模拟] 目标详情`（`--font-size-body`，加粗，`--color-text-primary`），底部 1px `--color-border` 边框。

详情字段为垂直排列的 `detail-field` 块，每块包含标签（`--color-text-secondary`，11px）+ 值（`--color-text-primary`，`--font-size-body`）。字段间距 10px。

| 字段 | 值 | 特殊样式 |
|------|----|----------|
| 目标 ID | target-001 | - |
| 类型 | 反跑道雷 | `--color-threat-high`（红色） |
| 威胁等级 | 高 | threat-tag 标签，红色背景 20% 透明度 |
| 识别置信度 | 86% | `--color-primary`（绿色），加粗 |
| 坐标位置 | X:109 Y:34 Z:0 | - |
| 探测时间 | 14:30:02 | - |
| 探测源 | UAV-1 侦察无人机 | - |
| 尺寸估计 | 120cm x 25cm | - |
| 建议处置 | 聚能引爆 | `--color-primary`（绿色） |

| 字段 | 值 |
|------|----|
| ID | `DET-RP-DETAIL` |
| 类型 | div 容器（只读） |
| 位置 | 右面板上段，flex:3 |
| 用途 | 展示选中目标的详细字段（只读） |
| 默认值 | target-001 的 9 个字段（见上表） |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：字段列表；加载：骨架；空：`请选择目标`（原型未实现）；错误：不适用（数据为本地模拟）；禁用：不适用 |
| 原型行为 | 固定显示 target-001 的详情字段，不随表格行选择更新（`updateDetail` 不更新此区域） |
| CURRENT 映射 | `DetectionView.cpp` `m_detailLabel`：按选中结果逐行显示 目标 ID/类型/威胁等级/置信度/最大异常分/帧时间/推理耗时/探测源（无选中时 `--`） |
| 安全 | 只读展示，模拟数据，无操作入口 |

### 4.2 状态历史时间线

`DET-RP-TIMELINE` 为内容容器，`overflow:auto`，内边距 8px。头部 32px：背景 `--color-toolbar`，标题 `[模拟] 状态历史`（`--font-size-body`，加粗），底部 1px `--color-border` 边框。

时间线为垂直排列的 `timeline-item` 块。每项结构：圆点（12px 圆形，按状态着色）+ 内容区（时间 `--color-text-disabled` 10px + 标签 `--color-text-primary` `--font-size-caption`）。

圆点颜色：已发现(Detected)=`--color-threat-medium`、已确认(Confirmed)=`--color-primary`、处置中(Disposing)=`--color-threat-high`、已完成(Disposed)=`--color-text-disabled`。

| 字段 | 值 |
|------|----|
| ID | `DET-RP-TIMELINE` |
| 类型 | div 容器（只读） |
| 位置 | 右面板下段，flex:2 |
| 用途 | 展示选中目标的状态变更历史（只读） |
| 默认值 | 1 条历史记录：14:30:02 已发现（Detected），圆点橙色 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：时间线列表；加载：骨架；空：`暂无历史`（原型未实现）；错误：不适用；禁用：不适用 |
| 原型行为 | 固定显示 target-001 的 1 条历史记录，不随表格行选择或模拟确认更新；底部标注 `[模拟数据，待人工确认]` |
| CURRENT 映射 | `DetectionView.cpp` `m_timelineLabel`：`HH:mm:ss 已发现（AI）` 起始，人工确认/拒绝后追加 `已确认（人工）`/`已拒绝（人工）`（无选中时 `--`） |
| 安全 | 只读展示，模拟数据 |

## 5. 应用壳控件

本节文档化探测页复用的应用壳控件（导航栏、菜单栏、工具栏、状态栏），为其分配 `DET-*` ID。详细规格（尺寸、间距、跨页一致性规则）见 `application-shell.md`。

### 5.1 导航栏

宽 80px 固定（`--size-nav-width`），背景 `--color-bg`，右边框 1px `--color-border`。垂直布局：Logo + 间距 + 6 个导航项 + 弹性留白。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|--------|---------|------|---------|---------------|------|
| `DET-NAV-LOGO` | UXO | div | 导航栏顶，40px 高 | 品牌标识 | `--color-primary` 色，18px，加粗，字间距 2px，居中 | 不适用 | 不适用 | 无 | 不可聚焦 | 同 CURRENT | 见 `application-shell.md` 第 3 节 | 无 |
| `DET-NAV-01` | 态势 | div | 导航项 1，56px 高 | 切换到态势页 | `--color-text-secondary`，左边框 3px 透明 | 背景 `--color-row-hover`、文本 `--color-text-primary` | 背景 `--color-selection`、左边框 `--color-primary`、文本 `--color-text-primary`、加粗 | 移除其他项 selected，当前加 selected（无实际页面跳转） | div 无 tabindex，不可键盘聚焦 | 仅切换高亮，不跳转页面 | 见 `application-shell.md` 第 3 节 | 无 |
| `DET-NAV-02` | 探测 | div | 导航项 2，56px 高 | 切换到探测页（当前页） | 选中 | 同上 | 同上 | 同上 | 同上 | 默认选中；点击仅保持选中 | 见 `application-shell.md` 第 3 节 | 无 |
| `DET-NAV-03` | 决策 | div | 导航项 3，56px 高 | 切换到决策页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `DET-NAV-04` | 设备 | div | 导航项 4，56px 高 | 切换到设备页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `DET-NAV-05` | 统计 | div | 导航项 5，56px 高 | 切换到统计页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `DET-NAV-06` | 配置 | div | 导航项 6，56px 高 | 切换到配置页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |

导航项图标统一为 `◎`（18px）。导航项内边距由 flex 居中控制，字号 `--font-size-caption`，间距 4px。

> 注：原型中导航点击仅切换 selected 类，不执行实际页面跳转（单页原型）。CURRENT Qt 客户端中导航切换通过 `QStackedWidget` 实现，见 `application-shell.md`。

### 5.2 菜单栏

高 30px（`--size-menu-bar-height`），背景 `--color-menu`，底部 1px `--color-border` 边框，内边距 `0 4px`。4 个菜单项为 `<button>` 元素，内边距 `6px 12px`，`--font-size-body`。

默认态：透明背景、`--color-text-primary` 文本。hover：背景 `--color-border`。禁用态（`data-disabled="true"`）：`--color-text-disabled` 文本，hover 不变背景。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DET-MENU-FILE` | 文件(&F) | button | 菜单栏左 1 | 文件菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无（无 JS 事件绑定） | Tab 聚焦，Enter 触发 | 点击无效果，不展开下拉菜单 | 见 `application-shell.md` 第 4 节 | 无 |
| `DET-MENU-VIEW` | 视图(&V) | button | 菜单栏左 2 | 视图菜单（占位） | 同上 | 同上 | 不适用 | 无 | 同上 | 同上 | 见 `application-shell.md` 第 4 节 | 无 |
| `DET-MENU-TOOLS` | 工具(&T) | button | 菜单栏左 3 | 工具菜单（禁用占位） | `--color-text-disabled` 文本 | 不变背景 | `data-disabled="true"`，tooltip `占位控件，未实现` | 无（disabled） | 不可聚焦 | **禁用并标注"占位"**，不响应点击 | 见 `application-shell.md` 第 4 节 | 无 |
| `DET-MENU-HELP` | 帮助(&H) | button | 菜单栏左 4 | 帮助菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无 | Tab 聚焦，Enter 触发 | 点击无效果 | 见 `application-shell.md` 第 4 节 | 无 |

### 5.3 工具栏

高 32px（`--size-toolbar-height`），背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`，间距 8px。从左到右：导出列表标签（禁用）+ 刷新探测按钮 + 弹性留白 + 模拟目标计数标签。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DET-TB-EXPORT` | 导出列表 | span | 工具栏左 1 | 导出目标列表（禁用占位） | `--color-text-disabled`，`--font-size-caption` | 不变（disabled） | `data-disabled="true"`，tooltip `占位控件，未实现` | 无 | 不可聚焦 | **禁用并标注"占位"** | 见 `application-shell.md` 第 5 节 | 无 |
| `DET-TB-REFRESH` | 刷新探测 | button | 工具栏左 2 | 刷新模拟探测数据 | `--color-text-secondary` 文本、透明背景、1px `--color-border` 边框、圆角 `--radius-control`、`--font-size-caption`；内边距 `4px 8px` | 背景 `--color-border`、文本 `--color-text-primary` | 不适用 | 无（JS 未绑定 click 事件） | Tab 聚焦，Enter 触发 | 点击无效果；原型仅 `DET-LP-REFRESH` 有 click 绑定（空操作），此按钮无绑定 | 见 `application-shell.md` 第 5 节 | 无 |
| `DET-TB-COUNT` | 模拟目标: 5 | span | 工具栏右，弹性留白后 | 显示模拟目标总数 | `--color-text-secondary`，`--font-size-caption`，内边距 4px | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 初始显示 `模拟目标: 5`；筛选或搜索时不更新（始终显示总数 5） | 见 `application-shell.md` 第 5 节 | 无 |

### 5.4 状态栏

高 28px，背景 `--color-bg`，顶部 1px `--color-border` 边框，内边距 `0 16px`，间距 16px。从左到右：设备状态标签 + 分隔线 + 模拟模式标签 + 分隔线 + 告警滚动区（弹性）+ 紧急停止按钮。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DET-SB-DEVICE` | 设备: 2/2 在线 | span | 状态栏左 1 | 显示模拟设备在线状态 | `--color-text-primary`，`--font-size-caption` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `设备: 2/2 在线`，不随操作变化 | 见 `application-shell.md` 第 6 节 | 模拟数据 |
| `DET-SB-SIM` | [模拟模式] | span | 状态栏左 2，分隔线后 | 标注当前为模拟模式 | `--color-status-busy`，`--font-size-caption`，加粗 | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `[模拟模式]` | 见 `application-shell.md` 第 6 节 | 模拟标注 |
| `DET-SB-ALARM` | - | div 容器 | 状态栏中，弹性宽 | 展示模拟告警滚动条目 | `min-width:400px`，`overflow:hidden`；条目 `--color-status-busy` 色、`--font-size-caption`、`--color-toolbar` 背景、内边距 `2px 8px`、圆角 `--radius-control` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 1 条告警 `模拟告警: target-001 待确认` | 见 `application-shell.md` 第 6 节 | 模拟告警 |
| `DET-SB-EMERGENCY` | 紧急停止 | button | 状态栏右，80x20 | 紧急停止所有设备（禁用占位） | **始终禁用**：`--color-border` 背景、`--color-text-disabled` 文本、11px、加粗、圆角 3px；tooltip `危险占位：无设备停止效果，本原型禁用` | 不适用 | `disabled` + `data-disabled="true"` | 无（disabled，不响应点击） | 不可聚焦 | **模拟占位，无实际效果**；原型中禁用并标注"危险占位" | 见 `application-shell.md` 第 6 节 | 模拟占位，无设备停止效果 |

状态栏分隔线为 1px 宽、18px 高的 `--color-border` 竖线。

## 6. 状态规则汇总

每个区域必须定义五态。下表汇总各区域的五态实现：

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 左面板搜索栏 | 可输入搜索 | 不适用（前端过滤） | 不适用 | 不适用 | 不适用 |
| 左面板筛选器 | chips 可点击 | 不适用 | 不适用 | 不适用 | 不适用 |
| 左面板目标表 | 列表展示 | 骨架行（原型未实现） | `暂无目标`（原型未实现） | 边框 `--color-status-error` + `目标加载失败`（原型未实现） | 不适用 |
| 证据 Tab 栏 | Tab 可切换 | 不适用 | 不适用 | 不适用 | 不适用 |
| 证据内容区 | 卡片列表 | 骨架卡（原型未实现） | `暂无证据`（原型未实现） | `证据加载失败`（原型未实现） | 不适用 |
| 确认操作条 | 确认按钮可用（目标为 Detected 时） | 不适用 | 不适用 | 不适用 | 拒绝按钮始终禁用；确认按钮在非 Detected 状态时显示拒绝消息 |
| 右面板目标详情 | 字段列表 | 骨架（原型未实现） | `请选择目标`（原型未实现） | 不适用（数据为本地模拟） | 不适用 |
| 右面板状态历史 | 时间线列表 | 骨架（原型未实现） | `暂无历史`（原型未实现） | 不适用 | 不适用 |
| 导航栏 | 导航项可点击 | 不适用 | 不适用 | 不适用 | 不适用 |
| 菜单栏 | 菜单项可点击 | 不适用 | 不适用 | 不适用 | 工具菜单禁用占位 |
| 工具栏 | 按钮可点击 | 不适用 | 不适用 | 不适用 | 导出列表禁用占位 |
| 状态栏 | 只读展示 | 不适用 | 不适用 | 不适用 | 紧急停止禁用占位 |

原型已实现的空态：无（所有空态均为 TARGET 补齐目标）。原型中所有区域的五态均为"正常"态展示，加载/空/错误态需在 TARGET 实现时补齐。状态颜色不得作为唯一信息，必须同时给出文字。

## 7. 交互流程

### 7.1 搜索与筛选流程

1. 用户在 `DET-LP-SEARCH` 输入文字。
2. `input` 事件触发，遍历表格所有行。
3. 按目标 ID、类型、威胁字段做大小写不敏感包含匹配（`String.includes`）。
4. 不匹配的行设为 `display:none`，匹配的行恢复显示。
5. 用户点击 `DET-LP-CLEAR`，清空搜索框值并重新触发 `input` 事件，恢复全部行。

筛选流程：
1. 用户点击某筛选组的 chip。
2. 同组其他 chip 移除 `active` 类，当前 chip 添加 `active`。
3. `filters` 对象更新对应字段（`type`/`threat`/`status`）为 chip 的 `data-value`。
4. `applyFilters` 遍历所有行，按 `type`/`threat`/`status` 三个条件做 AND 逻辑过滤。
5. `updateCount` 更新 `DET-LP-TABLE-COUNT` 显示可见行数（如 `3 个目标`）。

搜索与筛选叠加时，两套条件均生效（搜索先执行，筛选后执行，两者结果取交集）。

### 7.2 目标选择流程

1. 用户在 `DET-LP-TARGET-TABLE` 单击目标行。
2. 移除其他行的 `selected` 类，当前行添加 `selected`。
3. `selectedTarget` 更新为对应目标对象。
4. `updateDetail` 执行：
   - `DET-CE-TARGET` 更新为 `{id} · {type} · {threat}威胁`。
   - `DET-CE-CONFIRM-INFO` 更新为 `当前目标: {id} · 状态: {statusLabel}`。
5. **注意**：`DET-RP-DETAIL` 和 `DET-RP-TIMELINE` 在原型中不随选择更新，始终显示 `target-001` 的固定数据。TARGET 实现时应补齐右面板随选择更新的逻辑。

### 7.3 模拟确认流程

目标状态机：`Detected -> Confirmed`（原型仅实现此一步状态转移）。

1. 用户点击 `DET-CE-CONFIRM`。
2. 若 `selectedTarget.status === 'Detected'`：
   - 隐藏 `DET-CE-REJECT-MSG`。
   - `selectedTarget.status` 改为 `'Confirmed'`，`statusLabel` 改为 `'已确认'`。
   - `updateDetail` 更新 `DET-CE-TARGET` 与 `DET-CE-CONFIRM-INFO`。
   - 更新表格行第 6 列（状态列）状态标签为 `<span class="status-tag confirmed">已确认</span>`（绿色）。
3. 若 `selectedTarget.status !== 'Detected'`：
   - `DET-CE-REJECT-MSG` 显示文本 `该目标已确认或正在处置，无法重复确认`。
   - 目标状态不变。

所有变更仅修改原型 JS 内存中的 `selectedTarget` 对象与 DOM 显示，不调用真实传感器或 AI，不写入数据库，重启后清空。

### 7.4 证据 Tab 切换流程

1. 用户点击 `DET-CE-TAB-RECOG`/`DET-CE-TAB-SOURCE`/`DET-CE-TAB-STATE`。
2. 移除其他 Tab 的 `selected` 类，当前 Tab 添加 `selected`。
3. **注意**：原型中 Tab 切换不改变 `DET-CE-CONTENT` 内容，三个 Tab 始终显示相同的识别证据卡片（`DET-CE-RECOG-1` 与 `DET-CE-RECOG-2`）。TARGET 实现时应补齐 Tab 内容切换。

### 7.5 刷新流程

1. 用户点击 `DET-LP-REFRESH`（左面板刷新按钮）。
2. 空操作（JS 绑定 `()=>{}`，无实际刷新逻辑）。
3. `DET-TB-REFRESH`（工具栏刷新探测按钮）无 JS 事件绑定，点击同样无效果。

### 7.6 导航切换流程

1. 用户点击导航项 `DET-NAV-01` 至 `DET-NAV-06`。
2. 移除其他导航项的 `selected` 类，当前项添加 `selected`。
3. **注意**：原型为单页，不执行实际页面跳转。CURRENT Qt 客户端中导航切换通过 `QStackedWidget` 实现，见 `application-shell.md`。

## 8. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280x720 | 导航栏 80px + 左面板 360px + 右面板 380px = 820px 固定，中心区 460px；证据卡片（内边距 12px + 内容）需在 460px 内完整显示；确认操作条（56px 高）中 `[模拟]` + 信息标签 + 2 个按钮需在 460px 内不溢出；筛选器组在 360px 左面板内 chip 可能换行（`flex-wrap`）；右面板目标详情 9 个字段需在 380px 内不截断；工具栏导出标签 + 刷新按钮 + 计数标签需在 1280px 内完整显示 |
| 1920x1080 | 默认尺寸；中心区 1100px；所有控件按 token 展示；证据卡片与确认操作条有充足水平空间 |
| 3840x2160 | 固定区域不变（导航 80px + 左面板 360px + 右面板 380px = 820px）；中心区 3020px 弹性放大；证据卡片与确认操作条水平方向有大量留白；字号与控件尺寸保持固定 px |

原型 `.app` 容器固定为 `1920x1080`（`width:1920px;height:1080px`），不随视口缩放。TARGET 实现时应使布局响应三视口，固定面板宽度不变，中心区弹性。

## 9. 安全清单

本页面所有控件必须遵守以下安全约束：

| 控件 | 约束 |
|------|------|
| 模拟确认按钮 `DET-CE-CONFIRM` | 仅修改原型 JS 内存中的目标状态与 DOM 显示，不调用真实传感器或 AI，不发送设备命令 |
| 拒绝确认按钮 `DET-CE-REJECT` | 始终禁用，模拟占位，无实际效果 |
| 搜索框 `DET-LP-SEARCH` | 仅前端过滤，无设备通信 |
| 筛选器 `DET-LP-FILTER-*` | 仅前端过滤，无设备通信 |
| 排序按钮 `DET-LP-SORT-*` | 占位，空操作，无实际排序 |
| 刷新按钮 `DET-LP-REFRESH`/`DET-TB-REFRESH` | 空操作，无网络请求，无设备通信 |
| 目标表 `DET-LP-TARGET-TABLE` | 仅前端选择，无设备控制 |
| 证据卡片 `DET-CE-RECOG-*` | 模拟数据，不连接真实识别系统 |
| 目标详情 `DET-RP-DETAIL` | 只读展示，模拟数据 |
| 状态历史时间线 `DET-RP-TIMELINE` | 只读展示，模拟数据 |
| 紧急停止 `DET-SB-EMERGENCY` | 模拟占位，无实际效果，原型中禁用 |
| 导出列表 `DET-TB-EXPORT` | 禁用占位，不执行导出 |
| 工具菜单 `DET-MENU-TOOLS` | 禁用占位，不展开 |
| 导航项 `DET-NAV-*` | 仅切换高亮，不执行实际页面跳转（单页原型） |

所有模拟操作与结果必须带"模拟"或"演示"字样。涉及目标详情、证据卡片、状态历史的内容若来自本地 fixture，必须在区域标题或控件旁标注"模拟数据"。本页面不实现登录、角色切换、外部通信、持久化、UXR、MOS。不提供设备控制命令，不执行排爆动作。

## 10. CURRENT 映射总结

| 页面元素 | CURRENT 源码位置 |
|---------|------------------|
| **页面级** | **已实现为独立页面**：`DetectionView`（导航 index1，`MainWindow.cpp` 页面栈 slot 1） |
| 搜索框 `DET-LP-SEARCH` | 未实现（探测页无搜索框） |
| 刷新按钮 `DET-LP-REFRESH` | 未实现（清空结果经态势页 [重置] 流程调用 `m_detectionView->clearResults()`） |
| 清除按钮 `DET-LP-CLEAR` | 未实现（CURRENT 无清除按钮） |
| 类型筛选器 `DET-LP-FILTER-TYPE` | 未实现（CURRENT 声明 `m_typeFilterCombo` 成员但 `setupUi` 未实例化） |
| 威胁筛选器 `DET-LP-FILTER-THREAT` | 未实现（CURRENT 声明 `m_threatFilterCombo` 成员但 `setupUi` 未实例化） |
| 状态筛选器 `DET-LP-FILTER-STATUS` | 未实现 |
| 目标表 `DET-LP-TARGET-TABLE` | `DetectionView.cpp` `m_resultTable`（7 列）；`cellClicked` -> `onResultSelected` -> `resultSelected` -> 三向选中联动 |
| 目标计数 `DET-LP-TABLE-COUNT` | `DetectionView.cpp` `m_summaryLabel`（已分析 N 帧 · 异常 M） |
| 排序按钮 `DET-LP-SORT-*` | 未实现 |
| 证据面板头 `DET-CE-TARGET` | 顶部摘要条（`[模拟]` + `m_summaryLabel`）与底部 `m_statusLabel` 共同承担 |
| 证据 Tab `DET-CE-TAB-*` | 未实现 |
| 证据内容 `DET-CE-CONTENT` | 中栏 `m_viewerLabel` 标注图 + `m_heatmapLabel` 热力图 + `m_classLabel` 分类 Top-3 |
| 证据卡片 `DET-CE-RECOG-*` | 未实现 |
| 模拟确认按钮 `DET-CE-CONFIRM` | `DetectionView.cpp` `m_confirmButton`：`targetConfirmed` -> `onTargetConfirmed` -> `requestSelectedTargetStatus(Confirmed)` |
| 确认信息标签 `DET-CE-CONFIRM-INFO` | `DetectionView.cpp` `m_statusLabel`（`当前目标: %1 · 状态: %2`） |
| 拒绝消息 `DET-CE-REJECT-MSG` | 未实现 |
| 拒绝确认按钮 `DET-CE-REJECT` | `DetectionView.cpp` `m_rejectButton`：`targetRejected` -> `onTargetRejected` -> `markSelectedTargetFalseAlarm`（误报） |
| 目标详情 `DET-RP-DETAIL` | `DetectionView.cpp` `m_detailLabel`（逐行显示选中结果字段） |
| 状态历史时间线 `DET-RP-TIMELINE` | `DetectionView.cpp` `m_timelineLabel`（已发现/已确认/已拒绝 追加式） |
| 导航栏 `DET-NAV-*` | 见 `application-shell.md` 第 3 节 |
| 菜单栏 `DET-MENU-*` | 见 `application-shell.md` 第 4 节 |
| 工具栏 `DET-TB-*` | 见 `application-shell.md` 第 5 节 |
| 状态栏 `DET-SB-*` | 见 `application-shell.md` 第 6 节 |
| 数据类型枚举 | `Types.h`（TargetType/ThreatLevel/TargetStatus） |

## 11. DET-* ID 索引

下表列出本文档化的全部 `DET-*` ID，供 HTML 原型与 Playwright 测试对齐：

| ID | 控件 | 区域 |
|----|------|------|
| `DET-NAV-LOGO` | 导航栏 Logo | 应用壳 |
| `DET-NAV-01` | 导航项：态势 | 应用壳 |
| `DET-NAV-02` | 导航项：探测（选中） | 应用壳 |
| `DET-NAV-03` | 导航项：决策 | 应用壳 |
| `DET-NAV-04` | 导航项：设备 | 应用壳 |
| `DET-NAV-05` | 导航项：统计 | 应用壳 |
| `DET-NAV-06` | 导航项：配置 | 应用壳 |
| `DET-MENU-FILE` | 菜单：文件 | 应用壳 |
| `DET-MENU-VIEW` | 菜单：视图 | 应用壳 |
| `DET-MENU-TOOLS` | 菜单：工具（禁用占位） | 应用壳 |
| `DET-MENU-HELP` | 菜单：帮助 | 应用壳 |
| `DET-TB-EXPORT` | 工具栏：导出列表（禁用占位） | 应用壳 |
| `DET-TB-REFRESH` | 工具栏：刷新探测 | 应用壳 |
| `DET-TB-COUNT` | 工具栏：模拟目标计数 | 应用壳 |
| `DET-LP-SEARCH` | 搜索框 | 左面板 |
| `DET-LP-CLEAR` | 清除搜索按钮 | 左面板 |
| `DET-LP-REFRESH` | 刷新按钮 | 左面板 |
| `DET-LP-FILTER-TYPE` | 类型筛选器 | 左面板 |
| `DET-LP-FILTER-THREAT` | 威胁筛选器 | 左面板 |
| `DET-LP-FILTER-STATUS` | 状态筛选器 | 左面板 |
| `DET-LP-TARGET-TABLE` | 目标表 | 左面板 |
| `DET-LP-TABLE-COUNT` | 目标计数 | 左面板 |
| `DET-LP-SORT-THREAT` | 威胁排序按钮 | 左面板 |
| `DET-LP-SORT-CONF` | 置信度排序按钮 | 左面板 |
| `DET-CE-TARGET` | 证据面板目标标签 | 中心区 |
| `DET-CE-TAB-RECOG` | 识别证据 Tab | 中心区 |
| `DET-CE-TAB-SOURCE` | 探测来源 Tab | 中心区 |
| `DET-CE-TAB-STATE` | 状态历史 Tab | 中心区 |
| `DET-CE-CONTENT` | 证据内容区 | 中心区 |
| `DET-CE-RECOG-1` | 识别结果证据卡片 | 中心区 |
| `DET-CE-RECOG-2` | 融合分析证据卡片 | 中心区 |
| `DET-CE-CONFIRM-INFO` | 确认信息标签 | 中心区 |
| `DET-CE-REJECT-MSG` | 拒绝消息标签 | 中心区 |
| `DET-CE-CONFIRM` | 模拟确认按钮 | 中心区 |
| `DET-CE-REJECT` | 拒绝确认按钮（禁用） | 中心区 |
| `DET-RP-DETAIL` | 目标详情 | 右面板 |
| `DET-RP-TIMELINE` | 状态历史时间线 | 右面板 |
| `DET-SB-DEVICE` | 设备状态 | 状态栏 |
| `DET-SB-SIM` | 模拟模式标签 | 状态栏 |
| `DET-SB-ALARM` | 告警滚动区 | 状态栏 |
| `DET-SB-EMERGENCY` | 紧急停止按钮（禁用占位） | 状态栏 |

导航栏、菜单栏、工具栏、状态栏的详细规格见 `application-shell.md` 第 3 节（导航栏）、第 4 节（菜单栏）、第 5 节（工具栏）、第 6 节（状态栏）。
