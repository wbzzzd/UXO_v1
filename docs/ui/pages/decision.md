# 决策页面设计

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](../design-system.md)
应用壳：[docs/ui/application-shell.md](../application-shell.md)
一览：[docs/ui/pages/index.md](index.md)
原型：[docs/ui/prototypes/decision/index.html](../prototypes/decision/index.html)
截图：[docs/ui/images/decision/overview-1920x1080.png](../images/decision/overview-1920x1080.png)

> 本文是决策页面（decision page）的完整设计契约。每个交互控件拥有稳定 `DEC-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据，控件 ID、区域结构和状态规则不得在实现阶段自行变更。所有视觉值取自 `design-system.md`。

CURRENT 来源：决策页在 CURRENT Qt 客户端中**未作为独立页面实现**。CURRENT 主窗口仅渲染态势页（`MainWindow` 在 `loadMockData` 后填充态势页各面板），导航栏点击 `决策` 仅高亮不路由（见 `application-shell.md` 第 3.2 节）。与决策页相关的 CURRENT 锚点仅有两个：

- [`src/MainWindow/DecisionView.cpp`](../../../src/MainWindow/DecisionView.cpp)：空壳类，构造函数仅置空 `m_missionList`、`m_targetTable` 两个成员指针，`setupUi` 未实现，未被 `MainWindow` 实例化。
- [`src/MainWindow/DecisionSuggestionPanel.cpp`](../../../src/MainWindow/DecisionSuggestionPanel.cpp)：态势页右面板下段的只读"模拟决策建议"子组件（见 `situation.md` 第 5.3 节），仅展示单个目标的方案/风险/置信度，无方案比较、无草案编辑、无风险评估。
- [`include/MainWindow/DecisionView.h`](../../../include/MainWindow/DecisionView.h)：声明空壳成员。
- [`include/Core/Data/Types.h`](../../../include/Core/Data/Types.h)：`TargetType`/`ThreatLevel`/`TargetStatus` 等枚举。

因此本文中页面内容区控件的 `CURRENT 映射` 大多标注"未实现"；少数与 `DecisionSuggestionPanel` 只读展示同源的控件指向该子组件；导航栏/菜单栏/工具栏/状态栏指向 `application-shell.md` 对应章节。所有方案、资源约束、风险评估数据均为本地固定模拟场景，不连接真实设备、不写入数据库、不下发任务、不执行排爆动作。

## 1. 页面概述

决策页面是决策与方案管理页（导航 `DEC-NAV-03` 默认选中）。它一屏呈现：左侧待决策目标列表（带威胁/状态标记），中心决策依据与候选处置方案比较及资源约束检查，右侧决策草案编辑与风险评估。所有数据来自本地固定模拟 fixture（4 个模拟目标 + 3 个候选方案），选择方案只产生原型草案状态，无执行语义控件，不提供执行/处置/下发按钮。紧急停止按钮为禁用占位。

页面整体布局由应用壳定义（见 `application-shell.md` 第 2 节）。本文档化页面内部的四个内容区域与应用壳在本页的呈现：

| 区域 | 位置 | 内部组件 | CURRENT 主控件 |
|------|------|----------|----------------|
| 壳 | 顶部/左侧/底部 | 菜单栏、工具栏、导航栏、状态栏 | `application-shell.md` 第 3 至 6 节（未实现页面路由） |
| A | 左面板 | 待决策目标列表（4 张模拟目标卡片） | 未实现（`DecisionView` 空壳） |
| B | 中心区 | 决策依据、候选方案比较（3 方案）、资源约束检查 | 未实现；只读展示与 `DecisionSuggestionPanel` 部分同源 |
| C | 右面板 | 决策草案编辑、风险评估 | 未实现 |

页面固定尺寸 1920×1080，与 `application-shell.md` 第 1 节窗口尺寸一致。

## 2. 应用壳元素

本节文档化决策页 HTML 原型中带 `DEC-*` ID 的壳元素。这些元素结构复用 `application-shell.md` 的导航栏、菜单栏、工具栏、状态栏规格，但 CURRENT 仅在态势页上下文中渲染壳，决策页未实现路由，故这些 `DEC-*` 变体的页面专属内容（如工具栏"决策状态"、状态栏"设备: 2/2 在线"、告警"target-001 待决策"）属 TARGET 原型行为，CURRENT 不存在。

### 2.1 导航栏

固定宽 80px，背景 `--color-bg`，右侧 1px `--color-border` 边框。从上到下：UXO logo（高 40px，主色，18px 加粗，字间距 2px，居中）-> 16px 间距 -> 6 个导航项 -> 弹性留白。

每个导航项为可点击块，固定高 56px，左侧 3px 透明边框，图标与文字双行显示，字号 `--font-size-caption`，居中对齐，间距 4px，过渡 `--anim-short` `--anim-easing`。图标为 18px 文本符号 `◎`。

| ID | 标签 | 位置 | 用途 | 默认态 | hover | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|--------|-------|--------|---------|------|---------|---------------|------|
| `DEC-NAV-LOGO` | UXO | 导航栏顶部，高 40px | 仅展示系统标识，不可交互 | 主色文字、18px 加粗、字间距 2px、居中 | 同默认 | 同默认 | 无（原型未绑定点击） | 不可聚焦 | 同默认，纯展示 | `application-shell.md` 第 3 节（导航栏）；CURRENT `NavigationWidget` 渲染态势页 logo，决策页未实现 | 无 |
| `DEC-NAV-01` | 态势 | 导航项第 1，index 0 | 切换选中态至态势页 | 透明背景、`--color-text-secondary` 文本 | 背景 `--color-row-hover`、主文本色 | 背景 `--color-selection`、主色左边框 3px、主文本色、加粗 | 切换该项为选中态（移除其余选中）；CURRENT 不路由页面 | Tab 聚焦，Enter/Space 触发 | 同 CURRENT：仅高亮，不路由；选中后中心区仍显示决策页内容 | `application-shell.md` 第 3 节；CURRENT `SIT-NAV-01` 默认选中 | 无 |
| `DEC-NAV-02` | 探测 | 导航项第 2，index 1 | 占位 | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | `application-shell.md` 第 3 节 | 无 |
| `DEC-NAV-03` | 决策 | 导航项第 3，index 2 | 当前页面，默认选中 | 选中态：背景 `--color-selection`、主色左边框 3px、主文本色、加粗 | 同默认（已选中） | 同默认 | 点击保持选中态 | Tab 聚焦，Enter/Space 触发 | 默认选中；点击不改变内容（已是当前页） | `application-shell.md` 第 3 节；CURRENT `SIT-NAV-03` 点击仅 `qDebug` 不路由 | 无 |
| `DEC-NAV-04` | 设备 | 导航项第 4，index 3 | 占位 | 同 `DEC-NAV-01` 默认态 | 同上 | 同上 | 切换选中态 | 同上 | 同 `DEC-NAV-01` | `application-shell.md` 第 3 节 | 无 |
| `DEC-NAV-05` | 统计 | 导航项第 5，index 4 | 占位 | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | `application-shell.md` 第 3 节 | 无 |
| `DEC-NAV-06` | 配置 | 导航项第 6，index 5 | 占位 | 同上 | 同上 | 同上 | 同上 | 同上 | 同上 | `application-shell.md` 第 3 节 | 无 |

> CURRENT 导航栏仅切换高亮，不切换页面（`onNavigationChanged` 仅 `qDebug`）。原型保持一致：所有 `DEC-NAV-*` 点击仅切换选中态，不路由到其他页面，中心区内容不变。原型 JS 对所有 `.nav-item` 绑定点击，移除其余 `selected` 并为当前项添加 `selected`。

### 2.2 菜单栏

高 30px，背景 `--color-menu`，底部 1px `--color-border` 边框，主文本色，字号 `--font-size-body`。菜单项为按钮，内边距 `6px 12px`，背景透明。hover 背景 `--color-border`。禁用项文本 `--color-text-disabled`、`cursor: not-allowed`、hover 背景不变。

CURRENT 共 5 个顶级菜单（见 `application-shell.md` 第 4 节），决策页原型按 `application-shell.md` 第 4.2 节约定**不实现完整下拉弹出层**，仅渲染 4 个顶级菜单为可点击文本。原型 JS 未对菜单按钮绑定点击处理器。

| ID | 标签 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DEC-MENU-FILE` | 文件(&F) | 菜单栏左 1 | 文件菜单入口 | 透明背景、主文本色、`--font-size-body` | 背景 `--color-border` | 不适用 | 无下拉弹出，无响应 | Alt+F 聚焦，Enter 无响应 | 渲染顶级文本，不实现下拉；CURRENT `createMenuBar` 文件菜单子项含新建/打开/保存/退出 | `application-shell.md` 第 4 节；CURRENT `MainWindow.cpp` `createMenuBar` | 无 |
| `DEC-MENU-VIEW` | 视图(&V) | 菜单栏左 2 | 视图菜单入口 | 同上 | 同上 | 不适用 | 同上 | Alt+V 聚焦 | 同上；CURRENT 视图菜单含显示面板与视角子项 | `application-shell.md` 第 4 节 | 无 |
| `DEC-MENU-TOOLS` | 工具(&T) | 菜单栏左 3 | 工具菜单入口（禁用占位） | `--color-text-disabled` 文本、`cursor: not-allowed`、tooltip "占位控件，未实现" | 背景不变（`data-disabled="true"`） | 是 | 无响应 | 不可聚焦/触发 | **禁用并标注"占位"**，附 tooltip "占位控件，未实现"；CURRENT 工具菜单含系统设置（占位） | `application-shell.md` 第 4 节、第 7 节（禁用清单） | 无 |
| `DEC-MENU-HELP` | 帮助(&H) | 菜单栏左 4 | 帮助菜单入口 | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无下拉弹出，无响应 | Alt+H 聚焦 | 渲染顶级文本，不实现下拉；CURRENT 帮助菜单含"关于" | `application-shell.md` 第 4 节 | 无 |

> 决策页菜单栏与态势页菜单栏结构一致（见 `application-shell.md` 第 4 节），完整下拉交互属后续任务。原型中 4 个顶级菜单仅作存在性与禁用态校验，不绑定点击。

### 2.3 工具栏

高 32px 固定，背景 `--color-toolbar`，底部 1px `--color-border` 边框，间距 `--space-toolbar-gap`，内边距 `--space-toolbar-pad`。从左到右：导出标签（禁用占位）-> 刷新按钮 -> 弹性留白 -> 决策状态标签。CURRENT 工具栏全部为态势页上下文（见 `application-shell.md` 第 5 节），决策页工具栏内容属 TARGET。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DEC-TB-EXPORT` | 导出决策 | span（标签占位） | 工具栏左 1 | 导出决策（禁用占位） | `--color-text-secondary` 文本、`--font-size-caption`、内边距 4px、tooltip "占位控件，未实现" | 背景不变 | 是（`data-disabled="true"`） | 无响应 | 不可聚焦 | **禁用并标注"占位"**，附 tooltip；不实现导出 | `application-shell.md` 第 5 节；CURRENT 工具栏无导出项 | 无；不写文件、不外发 |
| `DEC-TB-REFRESH` | 刷新决策 | QPushButton | 工具栏左 2 | 刷新模拟决策数据 | 透明背景、`--color-text-secondary` 文本、`--font-size-caption`、1px `--color-border` 边框、圆角 `--radius-control`、内边距 `4px 8px`、tooltip "刷新模拟决策数据" | 背景 `--color-border`、主文本色 | 不适用 | **原型未绑定点击处理器**，点击无可见效果 | Tab 聚焦，Enter 无响应 | 渲染为可点击按钮并附 tooltip；JS 未连接，点击不刷新数据；属后续补齐项 | `application-shell.md` 第 5 节；CURRENT 工具栏仅态势页"视角复位"可点击 | 无；即使启用也仅刷新本地模拟 |
| `DEC-TB-STATUS` | 决策状态: 草案选择中 | span（只读标签） | 工具栏右，弹性留白后 | 显示当前决策草案状态 | `--color-text-secondary` 文本、`--font-size-caption`、内边距 4px | 同默认 | 不适用 | 无（只读） | 不可聚焦 | 静态显示"决策状态: 草案选择中"；原型 JS 不更新此标签（草案状态变化反映在 `DEC-RP-DRAFT-STATUS`） | `application-shell.md` 第 5 节；CURRENT 工具栏无此标签 | 无 |

> `DEC-TB-REFRESH` 在原型中存在但未连接点击逻辑，本文如实记录此缺口。若后续补齐刷新行为，需先在本文登记点击结果与状态切换，并保持仅刷新本地模拟数据、不发起网络请求。

## 3. 区域 A：左面板

宽 320px 固定（`--size-left-panel-width`），背景 `--color-panel`，外边距 8px，间距 8px，`overflow: hidden`。从上到下：面板头 -> 目标列表。

### 3.1 面板头

高 32px，内边距 `0 4px`。标题 `"[模拟] 待决策目标"`，`--font-size-body`、主文本色、加粗。无 ID（只读标题）。

### 3.2 目标列表 `DEC-LP-TARGET-LIST`

弹性填充，`overflow: auto`，flex 列布局，间距 4px。包含 4 张模拟目标卡片。

每张卡片高自适应内容，背景 `--color-toolbar`，1px `--color-border` 边框，左侧 3px 透明边框，圆角 `--radius-control`，内边距 10px，`cursor: pointer`，过渡 `--anim-short` `--anim-easing`。hover 背景 `#363636`。选中态背景 `--color-selection`、左侧边框 `--color-primary`。

卡片内部两行：
- 第 1 行：flex，间距 8px，下边距 4px。目标 ID（`--font-size-caption`、`--color-text-secondary`、等宽字体）+ 类型名（`--font-size-body`、主文本色、加粗、`flex: 1`）+ 威胁徽章。
- 第 2 行：flex，间距 12px，字号 11px，`--color-text-secondary`。"置信度: NN%" + "状态: " + 状态文本。

威胁徽章：内边距 `2px 8px`，圆角 3px，字号 10px，加粗。等级着色：高 = 背景 `rgba(255,82,82,0.2)`、文本 `--color-threat-high`；中 = 背景 `rgba(255,183,77,0.2)`、文本 `--color-threat-medium`；低 = 背景 `rgba(255,241,118,0.2)`、文本 `--color-threat-low`。

状态文本着色：已发现 `--color-threat-medium`；已确认 `--color-primary`；处置中 `--color-threat-high`。

模拟目标 fixture（4 个，原型硬编码）：

| 目标 ID | 类型 | 威胁 | 置信度 | 状态 | 默认 |
|---------|------|------|--------|------|------|
| target-001 | 反跑道雷 | 高 | 86% | 已发现 | 选中 |
| target-002 | 航弹 | 高 | 72% | 已发现 | - |
| target-003 | 火箭弹 | 中 | 91% | 已确认 | - |
| target-004 | 集束弹 | 低 | 65% | 已发现 | - |

| 字段 | 值 |
|------|----|
| ID | `DEC-LP-TARGET-LIST` |
| 类型 | 列表容器（div，含 4 张卡片） |
| 位置 | 左面板，面板头下方，弹性 |
| 用途 | 展示模拟待决策目标，单击选中触发中心区与右面板联动 |
| 默认值 | 4 张卡片，`target-001` 选中 |
| 点击结果 | 单击任一卡片 -> 移除其余卡片 `selected`，当前卡片加 `selected`；`DEC-CE-TARGET` 文本更新为 `{id} · {类型} · {威胁}威胁`；`DEC-RP-DRAFT-TARGET` 更新为 `{id}`；`draftConfirmed` 置 `false`，`DEC-RP-DRAFT-STATUS` 重置为 `待确认`；`DEC-RP-DRAFT-PLAN` 保持当前选中方案不变 |
| 键盘 | 卡片本身不可聚焦（原型未设 `tabindex`）；仅鼠标可操作 |
| 五态 | 正常：卡片列表；加载：不适用（fixture 同步渲染）；空：`暂无待决策目标`（原型未含空态分支，属后续补齐）；错误：不适用（本地模拟）；禁用：不适用 |
| 原型行为 | 同上点击联动；选中目标后中心区决策依据与候选方案仍显示 `target-001` 的固定数据（原型未按目标切换依据/方案/约束，仅切换 `DEC-CE-TARGET` 副标题与草案目标） |
| CURRENT 映射 | 未实现；`DecisionView` 空壳声明 `m_targetTable` 但未实例化；`DecisionSuggestionPanel` 仅展示单目标只读建议，无列表 |
| 安全 | 仅本地选择，无设备控制、不下发任务 |

> 注：原型 JS 在目标切换时仅更新 `DEC-CE-TARGET`、`DEC-RP-DRAFT-TARGET`、`DEC-RP-DRAFT-STATUS`，未重新加载决策依据/候选方案/资源约束/风险评估的内容（这些区域始终显示 `target-001` 的固定模拟数据）。本文如实记录此原型行为；如后续按目标刷新各区域，需先在本文登记对应字段更新规则。

## 4. 区域 B：中心区

弹性填充，flex 列布局，`overflow: hidden`。从上到下：中心头 -> 中心内容（决策依据、候选方案比较、资源约束检查）。

### 4.1 中心头

高 40px，背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 16px`，间距 12px。标题 `决策支持`（`--font-size-title`、主文本色、加粗）+ 副标题 `DEC-CE-TARGET`。

| 字段 | 值 |
|------|----|
| ID | `DEC-CE-TARGET` |
| 类型 | span（只读副标题） |
| 位置 | 中心头右侧 |
| 用途 | 显示当前选中目标的摘要 |
| 默认值 | `target-001 · 反跑道雷 · 高威胁` |
| 样式 | `--color-text-secondary` 文本、`--font-size-caption` |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 目标卡片点击后由 JS 更新为 `{id} · {类型} · {威胁}威胁`；初始为 `target-001` 摘要 |
| CURRENT 映射 | 未实现；CURRENT 态势页 `DecisionSuggestionPanel::setTarget` 仅更新右面板只读建议，无中心头副标题 |
| 安全 | 无 |

### 4.2 中心内容容器

flex 1，`overflow: auto`，内边距 16px，flex 列布局，间距 16px。包含三个面板：决策依据、候选方案比较、资源约束检查。

### 4.3 决策依据面板

背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 16px，flex 列布局，间距 12px。

面板标题 `"[模拟] 决策依据"`（`--font-size-body`、主文本色、加粗、`flex: 1`），无 ID。

依据网格 3 列等宽，间距 12px，6 项。每项：标签（`--color-text-secondary`、`--font-size-caption`）+ 值（主文本色、`--font-size-body`）。`.highlight` 值为主色加粗，`.danger` 值为 `--color-danger` 加粗。原型对 `目标类型` 值内联 `color:var(--color-threat-high)`。

模拟依据 fixture（针对 `target-001`，硬编码）：

| 项 | 标签 | 值 | 样式 |
|----|------|----|------|
| 1 | 目标类型 | 反跑道雷 | `--color-threat-high`（内联） |
| 2 | 威胁等级 | 高 | `.danger`（`--color-danger` 加粗） |
| 3 | 识别置信度 | 86% | `.highlight`（主色加粗） |
| 4 | 位置环境 | 跑道区域 | 默认 |
| 5 | 周边人员 | 无（已疏散） | 默认 |
| 6 | 天气条件 | 晴（模拟） | 默认 |

决策依据面板无交互控件、无 `data-testid`，全部为只读展示。CURRENT 映射：未实现；`DecisionSuggestionPanel` 的"方案/风险/置信度"只读字段与此部分同源但结构不同（态势页为单卡纵向，此处为六项网格）。安全：只读，模拟数据。

### 4.4 候选方案比较 `DEC-CE-PLANS`

面板标题 `"[模拟] 候选处置方案比较"`（`--font-size-body`、主文本色、加粗），位于网格上方。下方为方案网格容器 `DEC-CE-PLANS`，3 列等宽，间距 12px，`margin-top: 12px`。

| 字段 | 值 |
|------|----|
| ID | `DEC-CE-PLANS` |
| 类型 | 网格容器（div，含 3 张方案卡） |
| 位置 | 中心内容区第 2 段 |
| 用途 | 容纳候选方案卡片，承载方案选择交互 |
| 默认值 | 3 张方案卡，`DEC-CE-PLAN-1` 选中 |
| 点击结果 | 容器本身无点击；点击内部方案卡触发 `DEC-CE-PLAN-*` 逻辑 |
| 键盘 | 不可聚焦 |
| 原型行为 | 同上；选中方案卡样式变化由子控件承担 |
| CURRENT 映射 | 未实现；`DecisionSuggestionPanel` 仅展示单个建议方案，无候选比较 |
| 安全 | 模拟方案数据，选择只产生草案状态，无执行语义 |

#### 4.4.1 方案卡片 `DEC-CE-PLAN-1` 至 `DEC-CE-PLAN-3`

每张方案卡背景 `--color-panel`，2px `--color-border` 边框，圆角 `--radius-control`，内边距 16px，flex 列布局，间距 10px，`cursor: pointer`，过渡 `--anim-short` `--anim-easing`。hover 边框色 `--color-text-secondary`。选中态边框色 `--color-primary`、背景 `rgba(74,122,76,0.08)`。

卡片内部：方案头（flex，间距 8px）+ 描述 + 指标列表。方案头含单选圆（16×16，圆形，2px `--color-text-secondary` 边框）+ 方案名（`--font-size-body`、主文本色、加粗、`flex: 1`）。选中态单选圆边框变 `--color-primary`，内含 8×8 主色实心圆点。

描述：`--color-text-secondary` 文本、`--font-size-caption`、行高 1.5。

指标列表 flex 列布局，间距 6px。每条指标行 flex，间距 8px，字号 11px：标签（64px 宽，`--color-text-secondary`）+ 进度条（`flex: 1`、高 6px、`--color-border` 背景、圆角 3px、`overflow: hidden`）+ 数值（40px 宽，右对齐，主文本色）。进度条填充高度 100%、圆角 3px，按等级着色：高 `--color-threat-high`、中 `--color-threat-medium`、低 `--color-status-online`。

模拟方案 fixture（3 个，硬编码）：

| ID | 方案名 | 描述 | 成功率 | 风险 | 耗时 | 默认 |
|----|--------|------|--------|------|------|------|
| `DEC-CE-PLAN-1` | 聚能引爆 | 使用聚能装药在安全距离引爆目标，适用于金属外壳反跑道雷。需要排爆机器人接近目标。 | 92%（填充 `low`/绿，宽 92%） | 高（填充 `high`/红，宽 70%） | 45min（填充 `medium`/橙，宽 50%） | 选中 |
| `DEC-CE-PLAN-2` | 人工拆除 | 由排爆人员近距离拆除引信。适用于需要保留证据的目标。风险最高。 | 78%（填充 `medium`，宽 78%） | 极高（填充 `high`，宽 95%） | 120min（填充 `high`，宽 90%） | - |
| `DEC-CE-PLAN-3` | 就地封存 | 对目标进行标记和封存，等待后续处置。适用于资源不足或环境不允许立即处置的情况。 | 95%（填充 `low`，宽 95%） | 低（填充 `low`，宽 20%） | 15min（填充 `low`，宽 15%） | - |

| 字段 | 值 |
|------|----|
| ID | `DEC-CE-PLAN-1` / `DEC-CE-PLAN-2` / `DEC-CE-PLAN-3` |
| 类型 | div（可点击方案卡，含 `data-plan` 属性） |
| 位置 | `DEC-CE-PLANS` 网格内，左/中/右 |
| 用途 | 选中候选处置方案，联动右面板草案 |
| 默认态 | 背景 `--color-panel`、2px `--color-border` 边框、单选圆空心 |
| hover | 边框色 `--color-text-secondary` |
| 选中态 | 边框色 `--color-primary`、背景 `rgba(74,122,76,0.08)`、单选圆实心主色 |
| disabled | 不适用（三卡始终可选） |
| 点击结果 | 移除其余方案卡 `selected`，当前卡加 `selected`；`selectedPlan` 更新为 `data-plan`；`DEC-RP-DRAFT-PLAN` 更新为方案名；若 `draftConfirmed` 为 `true`，则重置 `draftConfirmed=false` 并将 `DEC-RP-DRAFT-STATUS` 置 `待确认` |
| 键盘 | 卡片本身不可聚焦（原型未设 `tabindex`）；仅鼠标可操作 |
| 原型行为 | 同上；选择方案不触发任何执行，仅修改草案状态 |
| CURRENT 映射 | 未实现；`DecisionSuggestionPanel` 仅展示单个建议方案值，无候选比较与选择交互 |
| 安全 | 模拟方案，选择只产生草案状态，不下发任务、不执行处置 |

### 4.5 资源约束检查面板

背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 16px，flex 列布局，间距 8px。标题 `"[模拟] 资源约束检查"`（`--font-size-body`、主文本色、加粗）。

约束行 flex，间距 8px，内边距 `6px 0`，底部 1px `--color-border` 边框（最后一行无边框）。每行：标签（120px 宽，`--color-text-secondary`、`--font-size-caption`）+ 值（主文本色、`--font-size-body`、`flex: 1`）+ 状态徽章（内边距 `2px 8px`，圆角 3px，字号 10px，加粗）。状态徽章着色：`ok` 背景 `rgba(76,175,80,0.2)`、文本 `--color-status-online`；`warn` 背景 `rgba(255,183,77,0.2)`、文本 `--color-threat-medium`；`block` 背景 `rgba(255,82,82,0.2)`、文本 `--color-threat-high`。

模拟约束 fixture（5 项，针对 `target-001`，硬编码）：

| 项 | 标签 | 值 | 状态 |
|----|------|----|------|
| 1 | 可用设备 | 排爆机器人 Robot-1（在线） | 满足（`ok`） |
| 2 | 人员就位 | 模拟排爆组（待命） | 满足（`ok`） |
| 3 | 安全距离 | 已疏散，半径 500m 无人员 | 满足（`ok`） |
| 4 | 通信链路 | [模拟] 本地模拟链路 | 占位（`warn`） |
| 5 | 天气许可 | 晴，风力 3 级（模拟） | 满足（`ok`） |

资源约束检查面板无交互控件、无 `data-testid`，全部为只读展示。CURRENT 映射：未实现。安全：只读，"通信链路"明确标注 `[模拟]` 与 `占位` 状态，不接入真实通信。

## 5. 区域 C：右面板

宽 380px 固定（`--size-right-panel-width`），背景 `--color-panel`，flex 列布局，`overflow: hidden`。垂直两段：决策草案（`flex: 3`）+ 风险评估（`flex: 2`）。

每段头部高 32px，背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`，间距 8px。段标题 `flex: 1`、`--font-size-body`、主文本色、加粗。段内容区 `flex: 1`、`overflow: auto`、内边距 12px、flex 列布局、间距 10px。

### 5.1 决策草案段

段标题 `"[模拟] 决策草案"`。

#### 5.1.1 草案状态容器 `DEC-RP-DRAFT`

背景 `--color-toolbar`，圆角 `--radius-control`，内边距 12px，flex 列布局，间距 8px。包含三行状态 + 操作行 + 说明。

| 字段 | 值 |
|------|----|
| ID | `DEC-RP-DRAFT` |
| 类型 | div（草案状态容器） |
| 位置 | 决策草案段内容区 |
| 用途 | 容纳草案目标/方案/状态显示与操作按钮 |
| 默认值 | 目标 `target-001`、方案 `聚能引爆`、状态 `待确认` |
| 点击结果 | 容器本身无点击；交互由子控件承担 |
| 键盘 | 不可聚焦 |
| 原型行为 | 同上；容器随子控件状态变化更新文本 |
| CURRENT 映射 | 未实现 |
| 安全 | 草案仅本地状态，不执行、不下发、不控制设备 |

状态行 flex，间距 8px，字号 `--font-size-caption`。标签 `--color-text-secondary`，值主文本色加粗。`.selected` 变体值为主色。

| ID | 标签 | 默认值 | 用途 | 样式 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|--------|------|------|---------|------|---------|---------------|------|
| `DEC-RP-DRAFT-TARGET` | 当前目标: | `target-001` | 显示当前选中目标 ID | 值主文本色加粗、`--font-size-caption` | 无（只读） | 不可聚焦 | 目标卡片点击后由 JS 更新为该目标 ID | 未实现 | 无 |
| `DEC-RP-DRAFT-PLAN` | 选中方案: | `聚能引爆` | 显示当前选中方案 | 值 `.selected`（主色加粗）、`--font-size-caption` | 无（只读） | 不可聚焦 | 方案卡片点击后由 JS 更新为方案名 | 未实现 | 无 |
| `DEC-RP-DRAFT-STATUS` | 草案状态: | `待确认` | 显示草案确认状态 | 值主文本色加粗、`--font-size-caption` | 无（只读） | 不可聚焦 | 初始 `待确认`；`DEC-RP-DRAFT-CONFIRM` 点击后 `已确认（草案）`；目标/方案变更或重置后回 `待确认` | 未实现 | 无 |

#### 5.1.2 草案操作按钮

操作行 flex，间距 8px，上边距 4px。两个按钮，各 `flex: 1`，高 36px，内边距 `0 12px`，字号 `--font-size-caption`，圆角 `--radius-control`。

主要按钮变体（`DEC-RP-DRAFT-CONFIRM`）：背景 `--color-primary`、`--color-text-primary` 文本、无边框；hover（非禁用）背景 `--color-primary-hover`；禁用背景 `--color-border`、文本 `--color-text-disabled`。

次要按钮变体（`DEC-RP-DRAFT-RESET`，`.secondary`）：透明背景、`--color-text-secondary` 文本、1px `--color-border` 边框；hover 背景 `--color-border`。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `DEC-RP-DRAFT-CONFIRM` | 确认草案 | QPushButton | 操作行左 | 确认当前草案选择（仅原型状态） | 主要按钮变体、tooltip "确认草案选择（仅原型状态）" | 背景 `--color-primary-hover` | 不适用（始终可点击） | `draftConfirmed=true`；`DEC-RP-DRAFT-STATUS` 置 `已确认（草案）` | Tab 聚焦，Enter 触发 | 确认仅修改草案状态标签，不下发任务、不控制设备；确认后需到设备页进行模拟指派 | 未实现 | 仅原型状态切换，无执行语义 |
| `DEC-RP-DRAFT-RESET` | 重置 | QPushButton | 操作行右 | 重置草案为待确认 | 次要按钮变体、tooltip "重置草案" | 背景 `--color-border` | 不适用 | `draftConfirmed=false`；`DEC-RP-DRAFT-STATUS` 置 `待确认` | Tab 聚焦，Enter 触发 | 重置仅还原草案状态标签，不改变已选目标/方案 | 未实现 | 无执行语义 |

#### 5.1.3 草案说明（只读）

说明文本 `--color-text-disabled`、字号 11px、行高 1.5。前缀 `[模拟]` 标签为 `.sim-tag`（`--color-status-busy`、`--font-size-caption`、加粗）。全文：`[模拟] 此处仅记录草案选择状态，不执行真实处置、不下发任务、不控制设备。确认后需到设备页面进行模拟指派。` 无 ID（只读说明）。CURRENT 映射：未实现。安全：明确声明草案无执行语义。

### 5.2 风险评估段 `DEC-RP-RISK`

段标题 `"[模拟] 风险评估"`。段内容容器为 `DEC-RP-RISK`。

| 字段 | 值 |
|------|----|
| ID | `DEC-RP-RISK` |
| 类型 | div（风险评估内容容器） |
| 位置 | 右面板下段 |
| 用途 | 容纳风险评估卡片 |
| 默认值 | 4 项风险指标 + 模拟声明 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 静态展示模拟风险，不随目标/方案变化 |
| CURRENT 映射 | 未实现；`DecisionSuggestionPanel` 的"风险等级"单值只读字段与此部分同源但结构不同 |
| 安全 | 只读，附 `[模拟数据，不替代真实威胁评估]` 声明 |

风险评估卡片背景 `--color-toolbar`，圆角 `--radius-control`，内边距 12px，flex 列布局，间距 8px。

风险行 flex，间距 8px。标签 80px 宽、`--color-text-secondary`、`--font-size-caption`。风险条 `flex: 1`、高 8px、`--color-border` 背景、圆角 4px、`overflow: hidden`。填充高度 100%、圆角 4px，按等级着色：高 `--color-threat-high`、中 `--color-threat-medium`、低 `--color-status-online`。数值 `--color-text-primary` 文本、`--font-size-caption`、`flex: 1`。

模拟风险 fixture（4 项，针对 `target-001` 与 `聚能引爆`，硬编码）：

| 项 | 标签 | 填充等级 | 填充宽度 | 数值 | 数值样式 |
|----|------|---------|---------|------|---------|
| 1 | 人员风险 | `high` | 70% | 高 | 默认 |
| 2 | 设备风险 | `medium` | 40% | 中 | 默认 |
| 3 | 环境风险 | `low` | 15% | 低 | 默认 |
| 4 | 综合风险 | `high` | 65% | 高 | `--color-threat-high` 加粗 |

卡片底部 `.sim-tag`（字号 11px、上边距 4px）：`[模拟数据，不替代真实威胁评估]`。无 ID（只读声明）。

## 6. 状态栏

宿主高 28px，背景 `--color-bg`，顶部 1px `--color-border` 边框，flex 布局，内边距 `0 16px`，间距 16px。从左到右：设备状态标签 -> 分隔符 -> 模拟模式标签 -> 分隔符 -> 告警滚动区（弹性）-> 紧急停止按钮。

分隔符为 1px×18px 元素，背景 `--color-border`。CURRENT 状态栏在态势页上下文渲染（见 `application-shell.md` 第 6 节），决策页状态栏内容属 TARGET。

| ID | 标签 | 类型 | 位置 | 用途 | 默认值 | 样式 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|------|---------|------|---------|---------------|------|
| `DEC-SB-DEVICE` | 设备: 2/2 在线 | span（只读） | 状态栏左 1 | 显示在线/总数设备计数 | `设备: 2/2 在线` | 主文本色、`--font-size-caption` | 无（只读） | 不可聚焦 | 静态显示，原型 JS 不更新 | `application-shell.md` 第 6 节；CURRENT `SIT-SB-DEVICE` 显示态势页设备计数（3/5 在线） | 只读 |
| `DEC-SB-SIM` | [模拟模式] | span（只读） | 状态栏左 2 | 标注当前为本地模拟 | `[模拟模式]` | `--color-status-busy` 文本、`--font-size-caption`、加粗 | 无（只读） | 不可聚焦 | 静态显示 | `application-shell.md` 第 6 节；CURRENT `SIT-SB-SIM` | 无 |
| `DEC-SB-ALARM` | 告警滚动区 | div（只读） | 状态栏中，弹性 | 横向滚动展示模拟告警 | `模拟告警: target-001 待决策` | `flex: 1`、`min-width: 400px`、高 18px、`overflow: hidden`；告警条 `--color-status-busy` 文本、`--color-toolbar` 背景、内边距 `2px 8px`、圆角 `--radius-control`、右外边距 10px、不换行 | 无（只读） | 不可聚焦 | 静态显示单条模拟告警，原型 JS 不更新 | `application-shell.md` 第 6 节；CURRENT `SIT-SB-ALARM` | 只读 |
| `DEC-SB-EMERGENCY` | 紧急停止 | QPushButton | 状态栏右 | 紧急停止（**危险占位，禁用**） | 文本 `紧急停止`、`disabled`、`data-disabled="true"`、tooltip "危险占位：无设备停止效果，本原型禁用" | 80×20px、字号 11px、加粗；默认（启用样式）背景 `--color-danger`、`--color-text-primary` 文本、圆角 3px；禁用背景 `--color-border`、文本 `--color-text-disabled`、`cursor: not-allowed` | **禁用，无响应** | 不可聚焦 | 显示为禁用态，不弹确认框、不发信号；详见 `application-shell.md` 第 6.3、第 7 节 | `application-shell.md` 第 6.3 节；CURRENT `SIT-SB-EMERGENCY` 弹确认框并发出 `emergencyStopClicked` 但无消费者 | **危险占位**：CURRENT 文案"所有设备将立即停止"会误导用户，本原型禁用此按钮 |

## 7. 状态规则汇总

每个区域必须定义五态。下表汇总各区域的五态实现：

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 导航栏 | 6 项可点击，`DEC-NAV-03` 选中 | 不适用 | 不适用 | 不适用 | 不适用（`DEC-NAV-LOGO` 不可交互） |
| 菜单栏 | 3 项可点击，`DEC-MENU-TOOLS` 禁用占位 | 不适用 | 不适用 | 不适用 | `DEC-MENU-TOOLS` 禁用并标注"占位" |
| 工具栏 | `DEC-TB-REFRESH` 可点击、`DEC-TB-STATUS` 只读 | 不适用 | 不适用 | 不适用 | `DEC-TB-EXPORT` 禁用并标注"占位" |
| 目标列表 `DEC-LP-TARGET-LIST` | 4 张卡片，`target-001` 选中 | 不适用（fixture 同步渲染） | `暂无待决策目标`（原型未含空态分支，属后续补齐） | 不适用（本地模拟） | 不适用 |
| 决策依据面板 | 6 项只读展示 | 不适用 | 不适用 | 不适用 | 不适用（只读） |
| 候选方案 `DEC-CE-PLANS` | 3 张方案卡，`DEC-CE-PLAN-1` 选中 | 不适用 | `暂无候选方案`（原型未含空态分支，属后续补齐） | 不适用 | 不适用（三卡始终可选） |
| 资源约束面板 | 5 行只读展示 | 不适用 | 不适用 | 不适用 | 不适用（只读） |
| 决策草案 `DEC-RP-DRAFT` | 目标/方案/状态显示 + 两按钮可操作 | 不适用 | 不适用 | 不适用 | 按钮不适用禁用（始终可点击） |
| 风险评估 `DEC-RP-RISK` | 4 项只读展示 | 不适用 | 不适用 | 不适用 | 不适用（只读） |
| 状态栏 | 设备/模拟/告警只读，紧急停止禁用 | 不适用 | 不适用 | 不适用 | `DEC-SB-EMERGENCY` 禁用占位 |

原型已实现的空态：无。所有空态文字属后续补齐项，文字必须同时给出（颜色不作为唯一信息）。所有模拟数据与操作带 `[模拟]` 或 `演示` 字样。

## 8. 交互流程

### 8.1 目标选择流程

1. 用户在 `DEC-LP-TARGET-LIST` 单击目标卡片。
2. 原型 JS 移除其余卡片 `selected`，当前卡片加 `selected`。
3. `DEC-CE-TARGET` 文本更新为 `{id} · {类型} · {威胁}威胁`。
4. `DEC-RP-DRAFT-TARGET` 更新为目标 ID。
5. `draftConfirmed` 置 `false`，`DEC-RP-DRAFT-STATUS` 重置为 `待确认`。
6. `DEC-RP-DRAFT-PLAN` 保持当前选中方案不变（不因切换目标而清空方案）。

> 决策依据、候选方案、资源约束、风险评估均显示 `target-001` 固定模拟数据，不随目标切换刷新（原型行为，见各控件字段）。

### 8.2 方案选择流程

1. 用户在 `DEC-CE-PLANS` 内单击方案卡（`DEC-CE-PLAN-1`/`2`/`3`）。
2. 原型 JS 移除其余方案卡 `selected`，当前卡加 `selected`。
3. `selectedPlan` 更新为 `data-plan`（方案名）。
4. `DEC-RP-DRAFT-PLAN` 更新为方案名。
5. 若 `draftConfirmed` 为 `true`，则重置 `draftConfirmed=false` 并将 `DEC-RP-DRAFT-STATUS` 置 `待确认`（已确认草案因方案变更失效）。

选择方案不触发任何执行，仅修改草案状态。

### 8.3 草案确认/重置流程

草案状态机：`待确认 -> 已确认（草案） -> 待确认`。

1. 初始 `DEC-RP-DRAFT-STATUS` 为 `待确认`，`draftConfirmed=false`。
2. 点击 `DEC-RP-DRAFT-CONFIRM`：`draftConfirmed=true`，状态置 `已确认（草案）`。
3. 点击 `DEC-RP-DRAFT-RESET`：`draftConfirmed=false`，状态回 `待确认`。
4. 在 `已确认（草案）` 态下切换目标或方案：状态自动回 `待确认`，`draftConfirmed=false`。

确认仅记录草案选择状态，不执行真实处置、不下发任务、不控制设备。确认后需到设备页进行模拟指派。

### 8.4 导航流程

点击任一 `DEC-NAV-*` 项：原型 JS 仅切换该项为选中态（移除其余选中），不路由页面，中心区内容不变。`DEC-NAV-03` 为当前页，默认选中。

## 9. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280x720 | 导航 80px、左面板 320px、右面板 380px、状态栏 28px、菜单栏 30px、工具栏 32px 均固定不变；中心区收缩；候选方案 3 列网格需在中心宽内完整显示（每卡最小约 200px，3 卡 + 间距需约 640px）；资源约束 5 行垂直排列；决策草案两按钮（`flex:1`）在右面板 380px 内并排显示 |
| 1920x1080 | 默认尺寸；中心区决策依据 3 列、候选方案 3 列、资源约束按 token 展示；右面板草案段与风险评估段按 3:2 分配 |
| 3840x2160 | 固定区域不变；中心区与右面板弹性区按比例放大；字号与控件尺寸保持固定 px |

原型固定 1920×1080 渲染（`.app` 宽高固定 1920×1080）。1280×720 与 3840×2160 视口验证属后续任务（见 `README.md` 第 8 节）。

## 10. 安全清单

本页面所有控件必须遵守以下安全约束：

| 控件 | 约束 |
|------|------|
| 目标列表 `DEC-LP-TARGET-LIST` | 仅本地选择，不下发任务、不控制设备 |
| 候选方案 `DEC-CE-PLANS`/`DEC-CE-PLAN-1`/`2`/`3` | 模拟方案，选择只产生草案状态，无执行语义 |
| 决策草案 `DEC-RP-DRAFT` 及子控件 | 仅记录草案状态，不执行真实处置、不下发任务、不控制设备；确认后需到设备页进行模拟指派 |
| 草案确认 `DEC-RP-DRAFT-CONFIRM` | 仅原型状态切换，无执行语义 |
| 草案重置 `DEC-RP-DRAFT-RESET` | 仅还原草案状态，无副作用 |
| 风险评估 `DEC-RP-RISK` | 只读模拟数据，附 `[模拟数据，不替代真实威胁评估]` 声明 |
| 资源约束面板 | 只读，"通信链路"明确标注 `[模拟]` 与 `占位` 状态，不接入真实通信 |
| 决策依据面板 | 只读模拟数据 |
| 工具栏导出 `DEC-TB-EXPORT` | 禁用占位，不写文件、不外发 |
| 工具栏刷新 `DEC-TB-REFRESH` | 原型未连接，即使启用也仅刷新本地模拟数据，不发起网络请求 |
| 紧急停止 `DEC-SB-EMERGENCY` | 见 `application-shell.md` 第 6.3 节，禁用占位，不弹确认框、不发信号 |
| 菜单工具 `DEC-MENU-TOOLS` | 禁用占位 |
| 导航 `DEC-NAV-01`/`02`/`04`/`05`/`06` | 仅切换选中态，不路由页面，无副作用 |

本页面不实现执行、处置、下发、设备控制、外部通信、数据库写入、登录、角色切换、持久化、UXR、MOS。所有方案、资源约束、风险评估数据明确标注 `[模拟]`，不描述真实设备状态或真实威胁评估。无目标时（属后续补齐空态）必须呈现明确空状态。

## 11. CURRENT 映射总结

| 页面元素 | CURRENT 源码位置 |
|---------|------------------|
| 决策页容器 | 未实现；`DecisionView.cpp` 空壳（构造函数仅置空 `m_missionList`、`m_targetTable`，`setupUi` 未实现，未被 `MainWindow` 实例化） |
| 目标列表 | 未实现；`DecisionView` 声明 `m_targetTable` 未实例化 |
| 决策依据/候选方案/资源约束 | 未实现 |
| 决策草案/风险评估 | 未实现 |
| 只读决策建议（同源锚点） | `DecisionSuggestionPanel.cpp`（态势页右面板下段，仅单目标只读方案/风险/置信度，见 `situation.md` 第 5.3 节） |
| 导航栏 | `application-shell.md` 第 3 节；CURRENT `NavigationWidget.cpp`（仅高亮不路由） |
| 菜单栏 | `application-shell.md` 第 4 节；CURRENT `MainWindow.cpp` `createMenuBar` |
| 工具栏 | `application-shell.md` 第 5 节；CURRENT `MainWindow.cpp` `createToolBar`（仅态势页"视角复位"可点击） |
| 状态栏 | `application-shell.md` 第 6 节；CURRENT `StatusBarWidget.cpp`（紧急停止弹确认框无消费者） |
| 数据类型枚举 | `Types.h`（`TargetType`/`ThreatLevel`/`TargetStatus` 等） |

## 12. DEC-* ID 索引

下表列出本文档化的全部 `DEC-*` ID，供 HTML 原型与 Playwright 测试对齐：

| ID | 控件 | 区域 |
|----|------|------|
| `DEC-NAV-LOGO` | UXO 标识（只读） | 应用壳·导航栏 |
| `DEC-NAV-01` | 态势导航项 | 应用壳·导航栏 |
| `DEC-NAV-02` | 探测导航项 | 应用壳·导航栏 |
| `DEC-NAV-03` | 决策导航项（默认选中） | 应用壳·导航栏 |
| `DEC-NAV-04` | 设备导航项 | 应用壳·导航栏 |
| `DEC-NAV-05` | 统计导航项 | 应用壳·导航栏 |
| `DEC-NAV-06` | 配置导航项 | 应用壳·导航栏 |
| `DEC-MENU-FILE` | 文件菜单 | 应用壳·菜单栏 |
| `DEC-MENU-VIEW` | 视图菜单 | 应用壳·菜单栏 |
| `DEC-MENU-TOOLS` | 工具菜单（禁用占位） | 应用壳·菜单栏 |
| `DEC-MENU-HELP` | 帮助菜单 | 应用壳·菜单栏 |
| `DEC-TB-EXPORT` | 导出决策标签（禁用占位） | 应用壳·工具栏 |
| `DEC-TB-REFRESH` | 刷新决策按钮（未连接） | 应用壳·工具栏 |
| `DEC-TB-STATUS` | 决策状态标签（只读） | 应用壳·工具栏 |
| `DEC-LP-TARGET-LIST` | 待决策目标列表 | 左面板 |
| `DEC-CE-TARGET` | 选中目标副标题（只读） | 中心区·中心头 |
| `DEC-CE-PLANS` | 候选方案网格容器 | 中心区 |
| `DEC-CE-PLAN-1` | 聚能引爆方案卡 | 中心区 |
| `DEC-CE-PLAN-2` | 人工拆除方案卡 | 中心区 |
| `DEC-CE-PLAN-3` | 就地封存方案卡 | 中心区 |
| `DEC-RP-DRAFT` | 决策草案状态容器 | 右面板 |
| `DEC-RP-DRAFT-TARGET` | 草案当前目标（只读） | 右面板 |
| `DEC-RP-DRAFT-PLAN` | 草案选中方案（只读） | 右面板 |
| `DEC-RP-DRAFT-STATUS` | 草案状态（只读） | 右面板 |
| `DEC-RP-DRAFT-CONFIRM` | 确认草案按钮 | 右面板 |
| `DEC-RP-DRAFT-RESET` | 重置草案按钮 | 右面板 |
| `DEC-RP-RISK` | 风险评估容器（只读） | 右面板 |
| `DEC-SB-DEVICE` | 设备状态标签（只读） | 应用壳·状态栏 |
| `DEC-SB-SIM` | 模拟模式标签（只读） | 应用壳·状态栏 |
| `DEC-SB-ALARM` | 告警滚动区（只读） | 应用壳·状态栏 |
| `DEC-SB-EMERGENCY` | 紧急停止按钮（禁用占位） | 应用壳·状态栏 |

导航栏、菜单栏、工具栏、状态栏的共用规格见 `application-shell.md` 第 3 节（导航栏）、第 4 节（菜单栏）、第 5 节（工具栏）、第 6 节（状态栏）。决策页无态势页共享的 `SIT-*` 控件，所有页面内控件均使用 `DEC-*` 前缀。
