# 决策页面设计

状态：`TARGET / P0 Approved / 设计评审原型 / 本地模拟`
批准：P0 `Approved`（2026-08-03 用户评审）；P1/P2 `Draft`；`docs/architecture/architecture-mos.md` `Draft`/来源资料
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](../design-system.md)
应用壳：[docs/ui/application-shell.md](../application-shell.md)
一览：[docs/ui/pages/index.md](index.md)
原型：[docs/ui/prototypes/decision/index.html](../prototypes/decision/index.html)
截图：[docs/images/decision/overview-1920x1080.png](../images/decision/overview-1920x1080.png)

> **2026-08-03 评审记录**：决策页 P0 TARGET 设计与 HTML 原型通过用户评审并获 `Approved`（仅 P0），HTML 原型作为设计证据被接受，但不构成 Qt 实现证据；P1/P2 仍 `Draft`。本次页面批准授权进入实现计划阶段，实现计划仍需独立批准，页面批准本身不授权任何代码实现。

> 本文是决策页面（decision page）的完整设计契约。决策页为 MOS（最小应急起降带）规划工作区：左侧损毁目标列表 + 中心跑道俯视图与算法参数 + 右侧候选起降方案与当前模拟选择摘要，底部 P1 扩展位以禁用占位形式保留。每个交互控件拥有稳定 `DEC-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据。所有视觉值取自 `design-system.md`。

CURRENT 来源：决策页 P0 Qt 实现已完成并通过验证。`DecisionView` 经 `MainWindow` 的 `QStackedWidget` index2 路由为 live 页面（导航"决策"= `DEC-NAV-03` 进入；导航"态势"= `DEC-NAV-01` 返回态势 live 页面 index0；index1 为探测 live 页面；设备/统计/配置未实现独立页面，导航保持态势工作区）。`DecisionView::setupUi` 已填充以下 live 组件：

- `src/MainWindow/DecisionView.cpp`：页面容器与布局，实例化子组件并接入 `MosPlanningController`。
- `src/MainWindow/MosRunwayWidget.cpp`：QPainter 2D 跑道俯视图，自绘跑道/弹坑/合成避让几何/MOS 矩形；缩放公式 `clamp(min(w/1920, h/1080), 1, 2)`，不依赖 DPR。
- `src/MainWindow/MosParamsPanel.cpp`：算法参数栏（10 可编辑 + 2 只读 + 实时校验横幅 + 规划状态横幅 + 重新规划）；字段高 22px、标签字号 11px（DecisionView 本地字面量）。
- `src/MainWindow/MosGeneratorDialog.cpp`：MOS-015 生成器模态，固定 1012×700px，底部三按钮始终可见；JSON 按钮仅发出目标路径请求，由 `MosPlanningController` 通过 `QSaveFile` 写出当前已提交 fixture。
- `src/MainWindow/MosPlanningController.cpp`：本地确定性状态机（planning -> loading -> result/error/empty），稳定 `DEC-*` ID。
- `src/MainWindow/DecisionSuggestionPanel.cpp`：态势页右面板下段只读"模拟决策建议"子组件（非决策页组件，保留为态势页锚点）。
- `include/Core/Data/Types.h`：`TargetType`/`ThreatLevel`/`TargetStatus` 等枚举。

验证证据：`mos_decision_ui`、`mos_decision_view` 与 `mos_ui_closure` 覆盖 happy / invalid / no-solution / tier / generator / controller-owned export 工作流。三视口（1280×720/1920×1080/3840×2160）历史证据见 `.omo/evidence/mos-p0-qt-closure-final/REPORT.md`；该证据采集于本轮单档位渲染修正之前，不能作为修正后的 fresh 多视口证据。离屏渲染不构成原生 GPU/窗口管理像素保真证据；WSLg xcb 烟雾仅证明进程在原生平台启动并存活。

> **CURRENT 与 TARGET 的关系**：本文档最初作为 TARGET P0 Approved 设计契约交付，HTML 原型是设计证据。CURRENT Qt 实现已落地该契约的 P0 子集，但本文下文的 TARGET 规格（如 `MOSPlanParams`/`MOSPlanResult` 概念模型名、HTML 原型本地 `state` 对象、`mulberry32()` 原型函数）仍保留为设计参考，不因 Qt 已实现而被删除。Qt 实现不声明 P1/P2 已实现：修复优先级排序、决策草案确认、导出规划报告仍为 `Draft`，在 Qt 中以禁用占位形式保留在右面板 `DEC-RP-P1-SLOT`。

MOS 功能依据：`docs/architecture/architecture-mos.md`（MOS 架构 `Draft`/来源资料，§§2-4）与 `docs/features/mos-planning.md` 的功能级 P0 Implemented 契约。该草稿只描述概念职责，不定义任何 C++ 类、头文件、接口或 Qt 契约：§2 把 `RunwayModel`/`DamagePoint`/`MOSPlanParams`/`MOSPlanResult` 列为概念模型名（合成 fixture 几何与派生结果，非真实跑道/工程/安全结论），§3 把纯 MOS 计算/估算描述为只返回派生值的服务职责，§4 把 QPainter 2D 可视化与参数配置面板描述为与核心解耦的 UI 展示职责。本页面是该草稿的 UI 参考呈现，文中出现的 `RunwayModel`/`DamagePoint`/`MOSPlanParams`/`MOSPlanResult` 等名称均为概念模型名，不是 CURRENT C++ 类或 Qt 契约。

MOS-015 生成器依据：`docs/features/mos-planning.md` 中 MOS-015 条目（P0 Implemented）——种子化本地随机生成弹坑与 UXO 分布，弹坑数量与半径范围受限，UXO 数量与当量范围受限，可下载 JSON 占位产物。该 JSON 属脱离设计期原型 fixture 产物，非运行时持久化/导入/集成；生成器不联网、不持久化。

原型契约声明：本页出现的 `state` 原型本地状态对象（含目标列表与种子等字段）、原型本地生成函数、`mulberry32()` 等 HTML 原型本地对象与函数名仅为原型展示机制，不是架构契约、C++ 类、Qt 接口或运行时会话实现；P0 Approved 会话边界见第 8.6 节。

## 1. 页面概述

决策页面是 MOS 起降带规划工作区（导航 `DEC-NAV-03` 默认选中）。P0 工作区一屏呈现完整可用的规划闭环：左侧损毁目标列表（弹坑/UXO，带威胁/状态标记），中心上方跑道俯视图（CURRENT Qt 测试阶段初始长度为 300m、宽度为 50m，参数可调；画布仅绘制当前选择档位的 MOS 矩形），中心下方算法参数栏（MOSPlanParams，10 个可编辑 + 2 个只读 + 校验与规划状态横幅 + 重新规划），右侧候选起降方案卡片 + 当前模拟选择摘要。右侧底部 P1 扩展位以禁用占位形式保留修复优先级排序、决策草案确认、导出规划报告三项 P1 增量，不参与 P0 流程。所有数据来自本地种子化模拟 fixture（4 个损毁点 + 3 档方案），选择方案仅刷新当前模拟选择摘要，无确认、下发或执行语义。

| 区域 | 位置 | 内部组件 | CURRENT 主控件 |
|------|------|----------|----------------|
| 壳 | 顶部/左侧/底部 | 菜单栏、MOS工具栏、导航栏、状态栏 | `application-shell.md` 第 3 至 6 节；决策页经 `QStackedWidget` index2 路由为 live 页面 |
| A | 左面板 260px | 损毁目标列表（4 张模拟目标卡片：弹坑/UXO） | 已实现：`DecisionView` 左面板损毁目标列表 |
| B | 中心区上 flex:1 | 跑道俯视图（跑道 + 目标影响圆 + 当前选择档位 MOS 矩形 + 标注通道 + 图例 + 缩放） | 已实现：`MosRunwayWidget`（QPainter 2D，单一各向同性 `pxPerM` 坐标比例；P0 仅绘制当前选择档位） |
| B' | 中心区下 | 算法参数栏（MOSPlanParams，10 个可编辑输入 + 2 个只读 + 校验横幅 + 规划状态横幅 + 重新规划） | 已实现：`MosParamsPanel`（字段高 22px、标签字号 11px，实时校验） |
| C | 右面板 380px | 候选方案卡片×N + 当前模拟选择摘要 + P1 扩展位（禁用占位） | 已实现：`DecisionView` 右面板候选方案卡片 + 当前模拟选择摘要；P1 扩展位恒禁用占位 |

页面固定尺寸 1920×1080，原型在小视口下等比缩放保持可用。

## 2. 应用壳元素

### 2.1 导航栏

固定宽 80px，背景 `--color-bg`，右侧 1px `--color-border` 边框。结构与态势页一致：UXO logo（高 40px，主色，18px 加粗，字间距 2px，居中）-> 16px 间距 -> 6 个导航项 -> 弹性留白。每个导航项高 56px，左侧 3px 透明边框，图标 `◎` + 文字双行，字号 `--font-size-caption`。选中态：背景 `--color-selection`、主色左边框 3px、主文本色、加粗。

| ID | 标签 | 默认 | 说明 |
|----|------|------|------|
| `DEC-NAV-LOGO` | UXO | 仅展示 | 不可交互 |
| `DEC-NAV-01` | 态势 | - | 点击路由回态势 live 页面（`QStackedWidget` index0） |
| `DEC-NAV-02` | 探测 | - | 点击切换到探测 live 页面（`QStackedWidget` index1） |
| `DEC-NAV-03` | 决策 | 选中 | 当前页面（`QStackedWidget` index2 live） |
| `DEC-NAV-04` | 设备 | - | 点击切换高亮，导航保持态势工作区（未实现独立页面） |
| `DEC-NAV-05` | 统计 | - | 点击切换高亮，导航保持态势工作区（未实现独立页面） |
| `DEC-NAV-06` | 配置 | - | 点击切换高亮，导航保持态势工作区（未实现独立页面） |

### 2.2 菜单栏

高 30px，背景 `--color-menu`。4 个顶级菜单：文件(F) / 视图(V) / 工具(T) / 帮助(H)。菜单项为可点击文本，hover 背景 `rgba(255,255,255,0.08)`。原型不实现下拉弹出层，点击无实际操作。

### 2.3 MOS 工具栏

高 40px，背景 `--color-toolbar`。与态势页工具栏不同，决策页工具栏承载 MOS 专用操作。工具栏起始处有一个只读 `P0 · 模拟规划` 范围徽标（非按钮、非交互、无 `data-testid`），安静标注当前 P0 模拟规划范围。其后从左到右两组：P0 操作组、档位切换组（动态生成）。工具栏不再保留 P1 禁用占位按钮；P1 扩展能力仅以禁用占位形式保留在右面板 `DEC-RP-P1-SLOT`，避免工具栏与右面板出现重复的 P1 入口。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | 点击结果 |
|----|------|------|------|------|--------|---------|
| `DEC-TB-GEN` | ◈ 生成损毁场景 | button | P0 组 1 | 打开 MOS-015 模拟损毁分布生成器模态 | 次要按钮 | 打开 `DEC-GEN-MODAL` |
| `DEC-TB-PARAMS` | ⚙ 参数设置 | button | P0 组 2 | 滚动到算法参数栏并聚焦首个输入 | 次要按钮 | 滚动 + 聚焦 |
| `DEC-TB-REPLAN` | ↻ 重新规划 | button | P0 组 3 | 按当前参数触发本地模拟重规划状态机 | 主要按钮 `--color-primary`；参数非法时禁用 | 进入 planning -> loading -> result/error/empty |
| `DEC-TB-PLAN-1` | 档位1·不含处理假设 | button | 档位组 1 | 切换当前模拟选择到档位1 | 次要按钮 | 切换选中态（互斥） |
| `DEC-TB-PLAN-2` | 档位2·部分处理假设 | button | 档位组 2 | 切换当前模拟选择到档位2 | 选中态 `--color-selection` | 切换选中态（互斥） |
| `DEC-TB-PLAN-3` | 档位3·更多处理假设 | button | 档位组 3 | 切换当前模拟选择到档位3 | 次要按钮 | 切换选中态（互斥） |

档位切换按钮数量随 `MOSPlanParams.tiers` 动态变化（2~5），ID 形如 `DEC-TB-PLAN-1..N`，互斥选中。点击切换后跑道图仅渲染选中档位 MOS 矩形（全档位叠加对比视图为 P1 Draft，见 `docs/features/mos-planning.md` MOS-008）+ 右面板对应方案卡片选中 + 当前模拟选择摘要刷新。不需要重跑算法。

参数非法时 `DEC-TB-REPLAN` 与参数栏内 `DEC-CE-PARAM-REPLAN` 同步禁用（见 4.2 校验规则）。

### 2.4 状态栏

高 22px，背景 `--color-bg`。从左到右：设备状态 -> 模拟模式徽章 -> 告警滚动 -> 当前分析目标。

| ID | 标签 | 用途 |
|----|------|------|
| `DEC-SB-DEVICE` | 模拟设备状态: 2/2 在线 | 只读，绿色在线圆点 |
| `DEC-SB-SIM` | 模拟模式 | 只读，橙色徽章 |
| `DEC-SB-ALARM` | 告警滚动区 | 只读，模拟告警文字横向滚动，明确标注"本地模拟，不执行真实处置" |
| `DEC-SB-TARGET` | 当前分析目标 | 只读，显示选中目标 ID + 威胁等级，随目标选择联动 |

## 3. 区域 A：左面板 -- 损毁目标列表

宽 260px 固定，背景 `--color-panel`，右侧 1px 边框。面板头高 36px，标题"[模拟] 损毁目标列表"，"模拟"为橙色边框小标签。

### 3.1 目标列表 `DEC-LP-TARGET-LIST`

弹性填充，`overflow-y: auto`。默认 4 张模拟目标卡片（MOS-015 生成器应用后数量与类型随参数变化，弹坑 1~8、UXO 0~5）：

| 目标 ID | 类型 | 威胁 | 状态 | 坐标 | 尺寸 |
|---------|------|------|------|------|------|
| 弹坑-001 | 弹坑 | 高 | 已发现 | 1200,25 | Ø8m |
| UXO-003 | 未爆弹 | 高 | 已发现 | 1500,30 | 影响45m |
| 弹坑-005 | 弹坑 | 中 | 模拟处理假设 | 800,15 | Ø6m |
| UXO-007 | 未爆弹 | 中 | 已发现 | 2200,35 | 影响30m |

卡片背景 `#2D2D2D`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 10px，`cursor: pointer`。选中态：蓝色边框 `#5B9BD5` + 背景 `--color-selection` + 1px 蓝色外阴影。

卡片内部三行：
- 第 1 行：ID（等宽字体 12px）+ 类型（粗体 13px）+ 威胁徽章（高=红 `--color-threat-high`，中=橙 `--color-threat-medium`）
- 第 2 行：状态文字（模拟处理假设带 ✓ 前缀并灰色）+ 坐标（等宽字体）
- 第 3 行：尺寸（等宽字体）

**功能依据**：`DamagePoint` 概念模型列表，属 `docs/architecture/architecture-mos.md` §2 的合成模拟损毁数据（算法输入概念），非 CURRENT C++ 类。每个目标在跑道俯视图上有对应的圆圈标注。

**交互**：点击卡片 -> 卡片高亮 + 跑道图对应圆圈脉冲高亮 + 状态栏 `DEC-SB-TARGET` 更新当前分析目标。

## 4. 区域 B：中心区 -- 跑道俯视图 + 算法参数

中心区 flex 列布局，上下分栏：上方跑道俯视图（flex:1）+ 下方算法参数栏（max-height 260px，可滚动）。

### 4.1 跑道俯视图 `DEC-CE-RUNWAY`

**功能依据**：QPainter 2D 可视化属 `docs/architecture/architecture-mos.md` §4 的 Draft UI 展示职责（自绘跑道/弹坑/合成避让几何/MOS 矩形，支持缩放/平移），概念职责，非 CURRENT C++ 类或 Qt 契约；功能级 P0 Implemented 边界见 `docs/features/mos-planning.md`。

HTML 原型标题为"跑道 3000m × 50m [模拟]"，标题右侧比例尺为"0 — 500m"；CURRENT Qt 测试阶段初始标题为"跑道 300m × 50m [模拟]"，长度参数可调整。跑道容器背景 `#1a2a1a`，跑道使用 `--color-runway` #3D3D3D 与 #555 边框，中线为虚线。

跑道内有 7 条刻度线（0%/16.67%/33.33%/50%/66.67%/83.33%/100%），底部标注距离。

#### 核心区 500m 叠层

跑道容器内、跑道上方的独立叠层 `#coreOverlay`（高 140px，半透明黑底 + 虚线边框），与跑道水平对齐、宽度一致。叠层纵向表示核心区横向偏移 y ∈ [-250, 250]（即 500m 核心宽度），中线 y=0 与跑道中线对齐。叠层左上角标注"核心区 500m (y: -250 ~ 250)"。目标圆圈渲染在此叠层内（不在跑道上），MOS 矩形仍渲染在跑道几何上。

> **CURRENT Qt 坐标系**：Qt 实现不使用 HTML 原型的独立叠层分离，而是采用单一各向同性像素/米比例 `pxPerM = min(pxPerMX, pxPerMY)`（X/Y 共用同一比例，见 `MosRunwayWidgetInternal.h`），跑道、目标圆圈与 MOS 矩形在同一 QPainter 坐标空间内按米坐标统一换算渲染，不存在视觉分离。

#### 目标圆圈

每个左面板目标在核心区叠层上有对应圆圈。横向位置按 `target.x / runwayLength × 100%` 投影（与跑道刻度对齐）；纵向位置按 `target.y` 投影到 y ∈ [-250, 250]：`top% = (target.y + 250) / 500 × 100%`，并加 15%~85% 视觉夹取防止标签裁切，实际 y 值保留在 `coord` 与 state 不变。视觉半径：HTML 原型按 `target.r × 2.2` 像素换算（仅可视化，非真实尺寸）并 clamp 在 18~60px（原型本地展示机制）。CURRENT Qt 实现的障碍物影响圆像素半径 `obstacleRadiusPx = influenceRadius × pxPerM`（米坐标 × 各向同性比例，无钳制/系数，paint 与 hitTest 共用，见 `MosRunwayWidgetInternal.h`）：

| 目标 | x 位置 | y | 类 | 视觉 | 标注 |
|------|---------|---|----|------|------|
| 弹坑-001 | 40% | 25 | crater | 红色半透明 `rgba(255,82,82,0.3)` + 红边框 | 弹坑-001 |
| UXO-003 | 50% | 30 | uxo | 黄色半透明 `rgba(255,235,59,0.25)` + 黄边框 | UXO-003 |
| 弹坑-005 | 27% | 15 | repaired | 透明 + 灰色虚线边框 + ✓ | 弹坑-005 |
| UXO-007 | 73% | 35 | uxo | 黄色半透明 + 黄边框 | UXO-007 |

模拟损毁分布生成器（MOS-015）产出的弹坑与 UXO 同样使用合成 fixture 坐标模型，y 在 [-40, 40] 范围内确定性生成（合成 fixture 几何，非真实跑道尺寸）。

圆圈标注仅显示目标 ID（如 `弹坑-001`、`UXO-003`），尺寸与威胁信息保留在左侧目标列表，不在跑道上重复，以减少标注碰撞。标注位于圆圈上方（`bottom:calc(100% + 4px)`），等宽字体 10px，半透明黑底。

圆圈 `cursor: pointer`，hover 阴影。选中态：`box-shadow: 0 0 16px` 对应颜色 + 边框 3px + 脉冲动画 `@keyframes pulse` 1.2s infinite。

**交互**：点击圆圈 -> 左面板对应卡片高亮（双向联动）+ 状态栏更新。

#### MOS 矩形（候选起降带）

候选档位矩形叠加在跑道上，绝对定位 div。HTML 原型以 `opacity: 0.35` 默认弱化未选中档位、`opacity: 1` 强调当前模拟选择档位。CURRENT Qt P0 仅渲染当前模拟选择档位（`m_selectedTier`）的 MOS 矩形，不渲染未选中档位；全档位叠加对比视图为 P1 Draft（见 `docs/features/mos-planning.md` MOS-008）。默认 3 档（随 `MOSPlanParams.tiers` 在 2~5 之间动态生成）：

| 档位 | 跑道位置 | 宽度 | 高度 | 边框 | 填充 | 标注 |
|------|---------|------|------|------|------|------|
| 档位1 | 14% | 15.3% | 60px | 橙色虚线 `--color-threat-medium` | `rgba(255,183,77,0.08)` | 档位1 460×15m |
| 档位2 | 39% | 17.3% | 72px | 绿色实线 `--color-status-online` | `rgba(76,175,80,0.15)` | 档位2 520×18m · 当前 |
| 档位3 | 60% | 21.7% | 80px | 蓝色虚线 `#98cbff` | `rgba(152,203,255,0.08)` | 档位3 650×20m |

矩形本身为纯几何元素（颜色/边框/填充），文字标注位于跑道下方独立的 `mos-label-lane` 标注通道，按各档位矩形水平中心对齐：`档位N <size>`，选中档位追加"· 当前"短标记；面积与完整方案说明保留在右侧候选卡片与当前模拟选择摘要，不在跑道上重复。档位2 默认选中，z-index 最高（7）。选中态：边框 3px + `box-shadow: 0 0 12px` 对应颜色。标注通道等宽字体 9px，半透明黑底，非交互；点击交互保留在跑道上的 MOS 矩形。

**功能依据**：`MOSPlanResult`（概念模型名，非 CURRENT C++ 类）多档方案，`docs/architecture/architecture-mos.md` §2 定义的模拟派生结果。矩形位置和尺寸对应 `MOSPlanResult` 的起降带坐标和尺寸。

**强调策略**：P0 仅渲染当前模拟选择档位（`m_selectedTier`）的 MOS 矩形，不渲染未选中档位。全档位叠加对比视图为 P1 Draft（MOS-008）。选中档位中属于该档位 `repairedIds` 的障碍物以 `Qt::DashLine` 虚线轮廓 + 斜十字标记绘制，模拟"已处理"假设，不暗示真实修复或安全结论。

**容纳约束**：HTML 原型本地生成函数先计算矩形宽度 `width`，再将确定性左端 `left` 夹取到 `[0, 100 - width]`，保证 2~5 档所有有效 MOS 矩形完整落在跑道范围内，不溢出左右边界。该函数只是原型本地展示机制，不是架构契约或 Qt 实现。

#### 图例

跑道下方左侧，按当前选中档位动态生成：弹坑（红圆）/ UXO（黄圆）/ 模拟处理假设（灰虚线圈 + 斜十字）/ 选中档位色块矩形。CURRENT Qt P0 仅渲染选中档位图例项，全档位图例叠加为 P1 Draft（MOS-008）。

#### 缩放控件

跑道下方右侧：`[+]` `[1×]` `[-]` `[复位]`。原型模拟缩放（0.5×~3×，步长 0.25），仅更新 `DEC-CE-ZOOM-LEVEL` 显示值，不实际缩放跑道几何。

| ID | 标签 | 用途 |
|----|------|------|
| `DEC-CE-ZOOM-IN` | + | 放大显示值（上限 3×） |
| `DEC-CE-ZOOM-LEVEL` | 1× | 只读，当前缩放显示 |
| `DEC-CE-ZOOM-OUT` | − | 缩小显示值（下限 0.5×） |
| `DEC-CE-ZOOM-RESET` | 复位 | 回到 1× |

### 4.2 算法参数栏 `DEC-CE-PARAMS`

**功能依据**：`MOSPlanParams`（概念模型名，非 CURRENT C++ 类，`docs/architecture/architecture-mos.md` §2 定义的合成规划参数）+ 参数配置面板（`docs/architecture/architecture-mos.md` §4 的 Draft UI 展示职责，概念，非 CURRENT Qt 类；功能级 P0 Implemented 边界见 `docs/features/mos-planning.md`）。

面板背景 `--color-panel`，1px 边框，圆角，内边距 10px 12px。三段结构：标题行 + 参数网格 + 校验与状态横幅 + 底部状态行。

标题行："算法参数 [模拟]" + `DEC-CE-PARAM-RESET`"恢复默认值"按钮（次要按钮样式）。

参数网格 6 列，12 个字段（10 可编辑 + 2 只读）：

| 字段 | data-testid | 类型 | 默认值 | 说明 |
|------|-------------|------|--------|------|
| 跑道长度 (m) | `DEC-CE-PARAM-LENGTH` | 可编辑 | TARGET 原型 3000；CURRENT Qt 300 | 本地测试阶段可调初始化值，不是领域固定值 |
| 跑道宽度 (m) | `DEC-CE-PARAM-WIDTH` | 可编辑 | 50 | `RunwayModel.width` |
| 最小起降长度 (m) | `DEC-CE-PARAM-MINLENGTH` | 可编辑 | TARGET 原型 460；CURRENT Qt 100 | 本地测试阶段可调初始化值，不是机型或安全阈值 |
| 最小起降宽度 (m) | `DEC-CE-PARAM-MINWIDTH` | 可编辑 | 15 | `MOSPlanParams.minWidth` |
| 合成 standoff 系数 K（非真实安全参数） | `DEC-CE-PARAM-K` | 可编辑 | **1.5** | 合成示例值，待领域确认 |
| 扫描步长 (m) | `DEC-CE-PARAM-STEP` | 可编辑 | 1 | Y 轴离散化步长（P0 简化：当前算法不使用此参数，仅做输入校验） |
| 回填速率 (m³/h) | `DEC-CE-PARAM-BACKFILL` | 可编辑 | 50 | 弹坑修复工时计算 |
| UXO 工时 (h/个) | `DEC-CE-PARAM-UXOHOURS` | 可编辑 | 8 | UXO 排除固定工时 |
| 扩展系数 | `DEC-CE-PARAM-EXPAND` | 可编辑 | 1.5 | 弹坑可见半径扩展 |
| 方案档位数 (2~5) | `DEC-CE-PARAM-TIERS` | 可编辑 | 3 | 递进方案数量 |
| 损毁点总数 | `DEC-CE-PARAM-DMGCOUNT` | 只读 | 4 | 当前场景目标数（派生） |
| 模拟处理假设数 | `DEC-CE-PARAM-REPAIRED` | 只读 | 1 | 当前场景模拟处理假设数（派生，绿色） |

输入框：高 26px，背景 `--color-bg`，1px `--color-border` 边框，圆角，内边距 0 8px，等宽字体 13px。focus 边框 `--color-border-focus`。非法数值时边框变红 `--color-threat-high`。只读字段以 `--color-toolbar` 背景呈现。

**K 值说明**：合成 standoff 系数 K 默认 1.5（非 1.2），CURRENT 标签为“合成 standoff 系数 K（非真实安全参数）”。K 仅用于本地模拟候选方案的几何间距估算，不代表任何真实 UXO 距离公式或安全因子。

**校验横幅** `DEC-CE-VALIDATION`：实时反馈参数合法性，逐字段判空/判数值/判范围/判整数。合法时绿色"参数校验通过 · K=1.5（模拟示例值，待领域确认）· 档位数=N"；非法时红色，列出全部具体错误（空值 / 非数值 / 越界 / 档位数非整数）。非法时 `DEC-CE-PARAM-REPLAN` 与 `DEC-TB-REPLAN` 两处重新规划按钮同步禁用。

**规划状态横幅** `DEC-CE-PLAN-STATE`：反映本地模拟规划状态机：

| 状态 | 颜色 | 文案 |
|------|------|------|
| planning | 蓝色 info | 规划中：本地模拟算法运行中… |
| loading | 橙色 warn | 加载中：生成候选方案… |
| result | 绿色 ok | 结果：已生成 N 档模拟候选方案（仅选中档位在跑道强调） |
| error | 红色 err | 错误：参数或场景无法生成有效方案，请调整后重规划 |
| empty | 橙色 warn | 空：当前场景无候选方案，请先生成损毁分布或调整参数 |

底部状态行：左侧"面积单调递增校验: ✓ 通过 / ✗ 未通过"（绿色通过 / 红色未通过），右侧 `DEC-CE-PARAM-REPLAN`"↻ 重新规划"主按钮。按钮在参数非法时禁用。

**功能依据**：面积单调递增校验是 `docs/architecture/architecture-mos.md` §2 定义的 `MOSPlanResult`（概念模型名，非 CURRENT C++ 类）校验标志（档位 1->2->...->N 面积应单调不降）。

> **CURRENT Qt 参数初始值**：CURRENT Qt `MosParamsPanel` 的跑道长度 L 初始值为 300（范围 100..6000），最小起降长度 minLength 初始值为 100（范围 1..6000），均为本地测试阶段可调初始值，非永久领域/机型/安全默认值（见 `MosParamsPanel.cpp`）。TARGET 原型中的 3000/460 仅保留为设计展示值与待业务确认示例，不是 CURRENT Qt 默认值。

## 5. 区域 C：右面板

宽 380px 固定，背景 `--color-panel`，左侧 1px 边框，`overflow-y: auto`。三个区域从上到下，用 `border-bottom` 分隔：候选方案、当前模拟选择摘要、P1 扩展位。

### 5.1 候选起降方案 `DEC-RP-PLANS`

**功能依据**：`MOSPlanResult`（概念模型名，非 CURRENT C++ 类，`docs/architecture/architecture-mos.md` §2）多档方案对比。

标题"[模拟] 候选起降方案"。默认 3 张方案卡片竖排（随档位数动态生成 2~5 张），ID 形如 `DEC-RP-PLAN-1..N`：

| 档位 | 标签 | 面积 | 尺寸 | 模拟处理耗时 | 涉及损毁点 | 工程量 | 模拟几何间距 |
|------|------|------|------|-------------|-----------|--------|-------------|
| 档位1·不含处理假设 | 最小面积(灰) | 6900m² | 460×15m | 0h(绿) | 0个 | 无(绿) | 8m |
| 档位2·部分处理假设 | 中间档位(蓝) | 9360m² | 520×18m | 12h(橙) | 3个 | 中等(橙) | 23m |
| 档位3·更多处理假设 | 最大面积(灰) | 13000m² | 650×20m | 28h(红) | 7个 | 高(红) | 35m |

每张卡片：左侧缩略图（48×24px 灰底迷你跑道+MOS 矩形示意，颜色与档位对应）+ 右侧内容。内容含档位名 + badge 一行，下方 6 项指标 3 列网格（可用面积 / 尺寸 / 模拟处理耗时 / 涉及损毁 / 工程量 / 模拟几何间距）。耗时颜色：绿=0h、橙=中等、红=高。选中态：蓝色边框 `#5B9BD5` + 微蓝背景 + 1px 蓝色外阴影。

**交互**：点击卡片 -> 高亮选中 + 跑道图对应 MOS 矩形强调（其余弱化）+ 工具栏档位按钮选中 + 当前模拟选择摘要刷新。

### 5.2 当前模拟选择摘要 `DEC-RP-DETAIL`

标题"当前模拟选择"（不带"模拟"小标签，因为整页已是模拟）。两段：

**大数字卡片（3 列）**：

| 指标 | 默认值 | 单位 | 颜色 |
|------|--------|------|------|
| 可用面积 | 9360 | m² 可用面积 | 默认 |
| 模拟处理耗时 | 12h | 模拟处理耗时 | 橙色（随档位耗时颜色变化） |
| 涉及损毁点 | 3 | 涉及损毁点 | 默认 |

**明细行**：

| 字段 | 默认值 | 说明 |
|------|--------|------|
| 当前模拟选择 | 档位2·部分处理假设 | 随选中档位更新 |
| 起降带 | 520×18m · 9360m² | 随选中档位更新 |
| 模拟几何间距 | 23m | 橙色 warn 色 |
| 模拟处理假设 | 12h · 3个损毁点 | 橙色 warn 色 |

**免责声明** `DEC-RP-DETAIL-NOTE`："[模拟] 此处仅用于本地方案对比。选择不构成确认、下发、执行，也不建立真实安全结论。"

字段用词严格遵循模拟边界："当前模拟选择"（非"推荐方案"）、"模拟几何间距"（非"安全余量"）、"模拟处理假设"（非"修复工期"）。

### 5.3 P1 扩展位 `DEC-RP-P1-SLOT`

**定位**：P1 增量扩展位，当前为禁用占位，不参与 P0 流程，亦不执行任何真实处置。保留该位是为了让 P1 增量后续可以接入而不改变主网格结构。

**结构**：
- 头部：`P1` 虚线小标签 + "扩展位（暂未实现）"标题，灰色 disabled 色。
- 说明："[模拟] 以下能力属 P1 增量，当前为空占位，不参与 P0 流程，亦不执行任何真实处置。"
- 占位列表（3 项，全部禁用）：

| 占位项 | 状态 |
|--------|------|
| 修复优先级排序 | P1 · 禁用 |
| 决策草案确认 | P1 · 禁用 |
| 导出规划报告 | P1 · 禁用 |

每项以 `p1-item` 样式呈现：深灰底、虚线边框、`cursor: not-allowed`、`opacity: 0.55`。

**重要**：旧版决策页中的"修复优先级列表 `DEC-RP-PRIORITY`"、"决策草案 `DEC-RP-DRAFT`"、"确认选定方案 `DEC-RP-DRAFT-CONFIRM`"已从 P0 退役。其中"确认选定方案"动作因可能暗示真实安全结论而被明确移除；修复优先级与决策草案确认降级为 P1 占位项，待 P1 评审通过后再赋予 ID 与规格。P0 不再保留任何确认或下发动作。

## 6. MOS-015 模拟损毁分布生成器 `DEC-GEN-MODAL`

**功能依据**：`docs/features/mos-planning.md` MOS-015 条目（P0 Implemented）——种子化本地随机生成弹坑与 UXO 分布，参数受限，可下载 JSON 占位产物。该 JSON 属脱离设计期原型 fixture 产物，非运行时持久化/导入/集成；生成器不联网、不持久化。

入口：工具栏 `DEC-TB-GEN`。模态浮层覆盖整个 `.app`，背景半透明黑遮罩 `rgba(0,0,0,0.55)`，z-index 50。

**模态结构**（宽 520px）：

| 部分 | 内容 |
|------|------|
| 头部 | "模拟损毁分布生成器 [模拟 · MOS-015]" + 关闭按钮 `DEC-GEN-CLOSE` |
| 弹坑参数节 | 数量 1~8（默认 2）、半径最小 m（默认 3）、半径最大 m（默认 6） |
| UXO 参数节 | 数量 0~5（默认 2）、当量最小 kg（默认 10）、当量最大 kg（默认 50）；标注"当量为模拟处理假设，非真实装药" |
| 种子节 | 随机种子（默认 42，整数，同种子可复现）+ 预览"弹坑N + UXON · 种子N" |
| 校验横幅 | `DEC-GEN-BANNER`，非法时红色列出具体错误，合法时绿色提示 |
| 底部 | 说明 + `[⬇ 下载模拟场景 JSON]` `[取消]` `[应用生成]` |

**字段 ID**：

| ID | 字段 | 默认 | 约束 |
|----|------|------|------|
| `DEC-GEN-CRATER-COUNT` | 弹坑数量 | 2 | 整数 1~8 |
| `DEC-GEN-CRATER-RMIN` | 半径最小 (m) | 3.00 | 浮点 0.1..100 |
| `DEC-GEN-CRATER-RMAX` | 半径最大 (m) | 6.00 | 浮点 0.1..100，≥ rmin |
| `DEC-GEN-UXO-COUNT` | UXO 数量 | 2 | 整数 0~5 |
| `DEC-GEN-UXO-YMIN` | 当量最小 (kg) | 10 | 正数 |
| `DEC-GEN-UXO-YMAX` | 当量最大 (kg) | 50 | 正数，≥ ymin |
| `DEC-GEN-SEED` | 随机种子 | 42 | 整数 |

**按钮 ID**：

| ID | 标签 | 行为 |
|----|------|------|
| `DEC-GEN-JSON` | ⬇ 下载模拟场景 JSON | 校验通过后下载 `mos-sim-scenario-seed<seed>-prototype.json`，文件含 `_prototype_note` 声明为原型占位 |
| `DEC-GEN-CANCEL` | 取消 | 关闭模态，不修改场景 |
| `DEC-GEN-APPLY` | 应用生成 | 校验通过后用 `mulberry32(seed)` 生成新目标列表替换原型本地状态对象的目标列表展示态，重置选中目标为 0，关闭模态并自动触发重规划。仅改动 HTML 原型本地内存演示态，不变更应用会话、不持久化、不恢复、不同步、不授权 |
| `DEC-GEN-CLOSE` | × | 关闭模态 |

**校验规则**：数量与种子必须为整数且在范围内；半径与当量必须为正数且 min ≤ max。非法字段高亮红边框，横幅列出全部错误。

**生成的 JSON 结构**（原型占位，不持久化）：

```json
{
  "_prototype_note": "本文件为设计评审原型本地模拟数据，不代表真实数据、不持久化、不用于实际操作。",
  "kind": "MOS simulated damage scenario (prototype)",
  "seed": 42,
  "crater": {"count": 2, "r_min_m": 3, "r_max_m": 6},
  "uxo": {"count": 2, "yield_min_kg": 10, "yield_max_kg": 50, "note": "当量为模拟处理假设"},
  "runway": {"length_m": 3000, "width_m": 50},
  "generated_at": "prototype-local"
}
```

**安全边界**：生成器全程本地种子化（`mulberry32`），不联网、不持久化、不写入数据库、不控制设备。JSON 下载为浏览器本地 Blob，属脱离设计期原型 fixture 产物，非运行时持久化、非运行时输入、非已批准外部集成通道，不回灌权威会话状态或外部系统；文件名含 `prototype` 字样，内容含 `_prototype_note` 免责声明。当量字段明确标注"模拟处理假设，非真实装药"。

## 7. 状态规则汇总

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 导航栏 | 6 项可点击，DEC-NAV-03 选中 | 不适用 | 不适用 | 不适用 | 不适用 |
| 菜单栏 | 4 项可点击 | 不适用 | 不适用 | 不适用 | 不适用 |
| MOS工具栏 P0 组 | 生成/参数/重规划可点击；重规划在参数非法时禁用 | 不适用 | 不适用 | 不适用 | 不适用 |
| MOS工具栏档位组 | N 档可点击互斥，DEC-TB-PLAN-2 默认选中 | 不适用 | 不适用 | 不适用 | 不适用 |
| 损毁目标列表 | 4 张卡片，弹坑-001 选中 | 不适用 | "暂无损毁目标"（生成器 UXO=0 且弹坑=0 时） | 不适用 | 不适用 |
| 跑道俯视图 | 跑道+圆圈+MOS矩形+图例+缩放 | 不适用 | 无目标时仅跑道与刻度 | 不适用 | 不适用 |
| 算法参数栏 | 10 输入可编辑 + 2 只读 | planning/loading 横幅 | 不适用 | error 横幅（参数非法） | 重规划按钮在参数非法时禁用 |
| 候选方案 | N 卡片，档位2 选中 | 不适用 | empty 横幅（无目标或无方案） | error 横幅 | 不适用 |
| 当前模拟选择摘要 | 3 大数字 + 4 明细行 + 免责声明 | 不适用 | 不适用 | 不适用 | 不适用 |
| P1 扩展位 | 不适用 | 不适用 | 不适用 | 不适用 | DEC-RP-P1-SLOT 内 3 项恒禁用 |
| 状态栏 | 设备/模拟/告警/目标 只读 | 不适用 | 不适用 | 不适用 | 不适用 |
| 生成器模态 | 字段可编辑，预览实时更新 | 不适用 | 不适用 | DEC-GEN-BANNER 红色错误 | 应用按钮在非法时无效 |

所有模拟数据与操作带 `[模拟]` 或"模拟"字样。

> **无解档位语义（CURRENT Qt，`NoFeasible`）**：当某档位在当前参数下无可行解时，该档位按钮（如 `DEC-TB-PLAN-1`）**禁用**（dimmed gray，不可点击）；更高可行档位（如 `DEC-TB-PLAN-3`）**启用但不 checked**（中性可选替代，非当前选择）。当前模拟选择摘要显示 `模拟几何间距：-`（无选择）或 `超出模拟范围`。该语义由 `tierSelectionCheckedStateIsUnambiguous` 测试锁定，避免"档位3 被误读为当前选中"的歧义：enabled-but-unchecked 是中性可选替代，checked 才是当前选择。

## 8. 交互流程

### 8.1 目标选择流程

1. 用户在 `DEC-LP-TARGET-LIST` 单击目标卡片。
2. 卡片高亮（蓝色边框 + 选中背景）。
3. 跑道图对应圆圈脉冲高亮（box-shadow + 边框加粗 + `@keyframes pulse`）。
4. 状态栏 `DEC-SB-TARGET` 更新为目标 ID + 威胁等级。
5. 反向：点击跑道图圆圈同样触发上述联动。

### 8.2 档位选择流程

1. 用户点击候选方案卡片、工具栏档位按钮或跑道 MOS 矩形（三处入口等价）。
2. 档位按钮选中态切换（互斥）。
3. 方案卡片高亮选中。
4. 跑道图仅渲染当前模拟选择档位 MOS 矩形（`m_selectedTier`），不渲染未选中档位；全档位叠加对比视图为 P1 Draft（MOS-008）。
5. 当前模拟选择摘要的大数字、明细行刷新。
6. 不重跑算法，仅刷新选择态。

> **Qt 实现合同（P0 Approved）**：P0 支持在已计算档位之间选择，仅渲染当前选择档位（`m_selectedTier`）的 MOS 矩形，不弱显全部档位；全档位叠加对比视图为 P1 Draft（MOS-008）。P1 进度/排序/丰富对比控件保持禁用。

### 8.3 参数修改与重规划流程

1. 用户在算法参数栏修改输入框值。
2. 输入实时校验：逐字段判空/判数值/判范围/判整数（档位数）；非法字段红边框 + `DEC-CE-VALIDATION` 横幅红色列出全部错误 + `DEC-CE-PARAM-REPLAN` 与 `DEC-TB-REPLAN` 两处重新规划按钮同步禁用。
3. 用户点击 `DEC-CE-PARAM-REPLAN` 或 `DEC-TB-REPLAN`（参数合法时可用）。
4. 规划状态机：planning（350ms）-> loading（500ms）-> result/error/empty。
5. `DEC-CE-PLAN-STATE` 横幅随状态变化。
6. result：按新参数（含档位数）确定性生成候选方案，刷新跑道矩形、工具栏档位按钮、右面板卡片；若当前选中档位不存在则回退到中间档。
7. empty：场景无目标时进入空态。
8. error：参数非法时直接进入错误态。

**重规划是本地确定性模拟**：原型本地使用 `mulberry32()` 以原型本地状态对象中的种子加 1000 作为输入，保证同参数同种子可复现，不调用真实算法、不联网。该种子与函数均为 HTML 原型本地展示机制，不是架构契约或 Qt 实现。

> **Qt 实现合同（P0 Approved）**：Qt 实现去除固定 350ms/500ms 人工延时；loading 状态仅在真实计算期间显示，以满足 P0 点击到渲染 max ≤200ms 门禁。HTML 原型的固定延时保留为原型行为，不构成 Qt 实现依据。

### 8.4 MOS-015 生成器流程

1. 用户点击 `DEC-TB-GEN` 打开 `DEC-GEN-MODAL`。
2. 修改弹坑/UXO/种子参数，预览实时更新。
3. 非法输入实时反馈（红边框 + 横幅错误）。
4. 可选点击 `DEC-GEN-JSON` 下载模拟场景 JSON（原型占位）。
5. 点击 `DEC-GEN-APPLY`：校验通过后用 `mulberry32(seed)` 生成新目标列表，替换原型本地状态对象的目标列表展示态，重置选中目标为 0，关闭模态，自动触发重规划（流程 8.3）。该动作仅改动 HTML 原型本地内存演示态，不变更应用会话、不持久化、不恢复、不同步、不授权。
6. 点击 `DEC-GEN-CANCEL` 或 `DEC-GEN-CLOSE` 或遮罩空白处：关闭模态，不修改场景。

> **Qt 实现合同（P0 Approved）**：`MosGeneratorDialog` 不直接写文件；它经 `DecisionView`/`MainWindow` 把明确目标路径交给 `MosPlanningController::exportFixture`。controller 仅通过 `QSaveFile` 写出当前已提交障碍物的 compact canonical bytes，且不改变会话、revision、日志或通知；不提供 import、reload、运行时持久化或外部集成通道。HTML 原型的浏览器 Blob 下载仅为原型行为。

### 8.5 P1 扩展位

P1 扩展位（修复优先级排序、决策草案确认、导出规划报告）恒禁用，P0 不提供任何确认、下发或导出动作。用户在 P0 阶段只能完成"目标选择 -> 参数调整 -> 重规划 -> 档位对比 -> 查看当前模拟选择摘要"的本地模拟闭环。

### 8.6 P0 Approved 会话边界

上述 8.3 与 8.4 流程在 HTML 原型中以本地展示机制演示。`docs/architecture/architecture-mos.md` §3 与 `docs/features/mos-planning.md` 的功能级 P0 Implemented 契约为批准的实现定义了一条 P0 Approved 共享会话边界：UI 请求 -> 应用层校验 -> 纯 fixture 构造/计算 -> 会话状态与中心内存日志原子提交 -> 一次变更通知 -> UI 回查与渲染；被拒绝的请求只返回原因并在同一内存日志原子追加一条拒绝记录，业务状态不变。

CURRENT Qt 实现的 `DecisionView` + `MosPlanningController` 已落地该边界的 P0 子集：本地确定性状态机（planning -> loading -> result/error/empty）、实时参数校验、档位互斥选中、生成器本地种子化 fixture、JSON 单向导出。该实现严格遵守本地/合成边界：所有数据为本地 fixture，不联网、不写入数据库、不控制设备、不提供 import/reload/运行时持久化/外部集成通道。`mos_decision_ui` 与 `mos_decision_view` 测试覆盖 happy / invalid / no-solution / tier / generator / export 工作流。历史三视口证据见 `.omo/evidence/mos-p0-qt-final/REPORT.md`，但采集时间早于本轮单档位渲染修正，不能作为修正后的 fresh 多视口证据。本页 HTML 原型中的 `state` 原型本地状态对象（含目标列表与种子等字段）、原型本地生成函数、`mulberry32()` 等名称只是原型本地展示机制，不是架构契约、C++ 类、Qt 接口或运行时会话实现；CURRENT Qt 实现中的对应 C++ 类（`MosPlanningController` 等）以源码为准，不与本页概念模型名一一对应。

## 9. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280×720 | 原型以 `fitToViewport` 等比缩放（`scale = min(w/1920, h/1080)`），保持 1920×1080 设计完整，不溢出、不裁切 |
| 1920×1080 | 默认尺寸；跑道图占满中心区上部；参数栏固定 max-height 260px 可滚动 |
| 3840×2160 | 等比缩放上限 1×，不放大；固定区域不变 |

原型固定 1920×1080 渲染，小视口等比缩小。这与旧版"固定不缩放"不同，目的是保证 1280×720 下决策工作区完整可用。

> **CURRENT Qt 视口事实**：Qt 实现的 `MosRunwayWidget` 使用 `clamp(min(w/1920, h/1080), 1, 2)` 缩放（不依赖 DPR）。历史三视口证据见 `.omo/evidence/mos-p0-qt-closure-final/REPORT.md`，但采集时间早于本轮单档位渲染修正，不能作为修正后的 fresh 多视口证据。4K 下全局应用壳缩放仍是预存在限制，非 DecisionView 本轮引入。

## 10. 安全清单

| 控件 | 约束 |
|------|------|
| 损毁目标列表 | 仅本地选择，不下发任务、不控制设备 |
| 候选方案 | 模拟方案，选择仅刷新当前模拟选择摘要，无确认、下发、执行语义 |
| 当前模拟选择摘要 | 仅本地对比展示，不建立真实安全结论 |
| 算法参数 | 修改参数仅触发本地模拟重规划，不接入真实设备 |
| 跑道俯视图 | 只读展示 + 缩放显示值，不控制设备 |
| 重新规划 | 本地确定性模拟（mulberry32 为 HTML 原型本地展示机制，非架构契约/Qt 实现），不调用真实算法、不联网 |
| MOS-015 生成器 | 本地种子化生成，不联网、不持久化、不写入数据库；JSON 下载为浏览器 Blob，属脱离设计期原型 fixture 产物，非运行时持久化/输入/集成，含 `_prototype_note` 免责声明 |
| P1 扩展位 | 恒禁用占位，不参与 P0 流程 |
| 导出规划报告 | P1 禁用占位，仅保留在右面板 `DEC-RP-P1-SLOT`，工具栏不再保留导出入口。P0 不导出报告；MOS-015 JSON 下载为浏览器本地 Blob 原型占位产物，非报告、非持久化 |
| 扩展位 | P1 禁用占位 |

本页面不实现执行、处置、下发、设备控制、外部通信、数据库写入、确认动作。所有数据标注 `[模拟]`。用词严格遵循模拟边界：使用"当前模拟选择"、"模拟几何间距"、"模拟处理假设"，不使用"推荐方案"、"确认选定方案"、"已确认"或任何真实安全结论。

## 11. CURRENT 映射总结

| 页面元素 | CURRENT 源码位置 |
|---------|------------------|
| 决策页容器 | `src/MainWindow/DecisionView.cpp`：`setupUi` 已填充，经 `MainWindow` `QStackedWidget` index2 路由为 live 页面 |
| 损毁目标列表 | `DecisionView` 左面板：4 张模拟目标卡片（弹坑/UXO），点击选中首个目标联动跑道图与右面板 |
| 跑道俯视图 | `src/MainWindow/MosRunwayWidget.cpp`：QPainter 2D，跑道/弹坑/合成避让几何/MOS 矩形；P0 仅渲染选中档位（`m_selectedTier`），全档位叠加为 P1 Draft（MOS-008）；选中档位 `repairedIds` 以 `Qt::DashLine` + 斜十字标记绘制；单一各向同性比例 `pxPerM = min(pxPerMX, pxPerMY)`，障碍物影响圆 `obstacleRadiusPx = influenceRadius × pxPerM`（无钳制/系数，paint 与 hitTest 共用）；缩放 `clamp(min(w/1920,h/1080),1,2)` 无 DPR |
| 算法参数 | `src/MainWindow/MosParamsPanel.cpp`：10 可编辑 + 2 只读 + 实时校验横幅 + 规划状态横幅 + 重新规划；字段高 22px、标签字号 11px；L 初始值 300（范围 100..6000）、minLength 初始值 100（范围 1..6000），均为本地测试阶段可调初始值 |
| 候选方案 | `DecisionView` 右面板候选方案卡片×N；档位互斥 checked 选中态（蓝色高亮 vs 中性可选） |
| 当前模拟选择摘要 | `DecisionView` 右面板当前模拟选择摘要区；显示 `模拟几何间距：-`（无选择）/数值或 `超出模拟范围` |
| MOS-015 生成器 | `MosGeneratorDialog` 发出导出请求；`MainWindow` 路由到 `MosPlanningController::exportFixture`，写出已提交 fixture canonical bytes |
| 状态机 | `src/MainWindow/MosPlanningController.cpp`：planning -> loading -> result/error/empty，稳定 `DEC-*` ID |
| P1 扩展位 | `DecisionView` 右面板 `DEC-RP-P1-SLOT` 恒禁用占位（修复优先级排序/决策草案确认/导出规划报告仍 `Draft`） |
| 导航栏 | `application-shell.md` 第 3 节；`DEC-NAV-03` 路由决策 live，`DEC-NAV-01` 路由回态势 live |
| 菜单栏 | `application-shell.md` 第 4 节 |
| 状态栏 | `application-shell.md` 第 6 节 |

## 12. DEC-* ID 索引

| ID | 控件 | 区域 | 状态 |
|----|------|------|------|
| `DEC-NAV-LOGO` | UXO 标识（只读） | 导航栏 | P0 |
| `DEC-NAV-01` | 态势导航项 | 导航栏 | P0 |
| `DEC-NAV-02` | 探测导航项 | 导航栏 | P0 |
| `DEC-NAV-03` | 决策导航项（默认选中） | 导航栏 | P0 |
| `DEC-NAV-04` | 设备导航项 | 导航栏 | P0 |
| `DEC-NAV-05` | 统计导航项 | 导航栏 | P0 |
| `DEC-NAV-06` | 配置导航项 | 导航栏 | P0 |
| `DEC-MENU-FILE` | 文件菜单 | 菜单栏 | P0 |
| `DEC-MENU-VIEW` | 视图菜单 | 菜单栏 | P0 |
| `DEC-MENU-TOOLS` | 工具菜单 | 菜单栏 | P0 |
| `DEC-MENU-HELP` | 帮助菜单 | 菜单栏 | P0 |
| `DEC-TB-GEN` | 生成损毁场景按钮 | MOS工具栏 | P0 |
| `DEC-TB-PARAMS` | 参数设置按钮 | MOS工具栏 | P0 |
| `DEC-TB-REPLAN` | 重新规划按钮 | MOS工具栏 | P0 |
| `DEC-TB-PLAN-1` | 档位1切换按钮 | MOS工具栏 | P0（动态） |
| `DEC-TB-PLAN-2` | 档位2切换按钮（默认选中） | MOS工具栏 | P0（动态） |
| `DEC-TB-PLAN-3` | 档位3切换按钮 | MOS工具栏 | P0（动态） |
| `DEC-TB-PLAN-N` | 档位N切换按钮（N=2~5） | MOS工具栏 | P0（动态，按 tiers 生成） |
| `DEC-LP-TARGET-LIST` | 损毁目标列表容器 | 左面板 | P0 |
| `DEC-CE-RUNWAY` | 跑道俯视图区段 | 中心区上 | P0 |
| `DEC-CE-ZOOM-IN` | 缩放放大按钮 | 中心区上 | P0 |
| `DEC-CE-ZOOM-LEVEL` | 缩放显示值（只读） | 中心区上 | P0 |
| `DEC-CE-ZOOM-OUT` | 缩放缩小按钮 | 中心区上 | P0 |
| `DEC-CE-ZOOM-RESET` | 缩放复位按钮 | 中心区上 | P0 |
| `DEC-CE-PARAMS` | 算法参数栏容器 | 中心区下 | P0 |
| `DEC-CE-PARAM-RESET` | 恢复默认值按钮 | 中心区下 | P0 |
| `DEC-CE-PARAM-LENGTH` | 跑道长度输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-WIDTH` | 跑道宽度输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-MINLENGTH` | 最小起降长度输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-MINWIDTH` | 最小起降宽度输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-K` | 合成 standoff 系数 K 输入（默认 1.5，非真实安全参数） | 中心区下 | P0 |
| `DEC-CE-PARAM-STEP` | 扫描步长输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-BACKFILL` | 回填速率输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-UXOHOURS` | UXO 工时输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-EXPAND` | 扩展系数输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-TIERS` | 方案档位数输入 | 中心区下 | P0 |
| `DEC-CE-PARAM-DMGCOUNT` | 损毁点总数（只读派生） | 中心区下 | P0 |
| `DEC-CE-PARAM-REPAIRED` | 模拟处理假设数（只读派生） | 中心区下 | P0 |
| `DEC-CE-VALIDATION` | 参数校验横幅 | 中心区下 | P0 |
| `DEC-CE-PLAN-STATE` | 规划状态横幅 | 中心区下 | P0 |
| `DEC-CE-PARAM-REPLAN` | 重新规划按钮（参数栏内） | 中心区下 | P0 |
| `DEC-RP-PLANS` | 候选方案容器 | 右面板 | P0 |
| `DEC-RP-PLAN-1` | 档位1卡片 | 右面板 | P0（动态） |
| `DEC-RP-PLAN-2` | 档位2卡片（默认选中） | 右面板 | P0（动态） |
| `DEC-RP-PLAN-3` | 档位3卡片 | 右面板 | P0（动态） |
| `DEC-RP-PLAN-N` | 档位N卡片（N=2~5） | 右面板 | P0（动态，按 tiers 生成） |
| `DEC-RP-DETAIL` | 当前模拟选择摘要容器 | 右面板 | P0 |
| `DEC-RP-DETAIL-NOTE` | 摘要免责声明 | 右面板 | P0 |
| `DEC-RP-P1-SLOT` | P1 扩展位容器（禁用占位） | 右面板 | P1 |
| `DEC-GEN-MODAL` | 生成器模态浮层 | 全屏浮层 | P0 |
| `DEC-GEN-CLOSE` | 模态关闭按钮 | 全屏浮层 | P0 |
| `DEC-GEN-CRATER-COUNT` | 弹坑数量输入 | 全屏浮层 | P0 |
| `DEC-GEN-CRATER-RMIN` | 弹坑半径最小输入 | 全屏浮层 | P0 |
| `DEC-GEN-CRATER-RMAX` | 弹坑半径最大输入 | 全屏浮层 | P0 |
| `DEC-GEN-UXO-COUNT` | UXO 数量输入 | 全屏浮层 | P0 |
| `DEC-GEN-UXO-YMIN` | UXO 当量最小输入 | 全屏浮层 | P0 |
| `DEC-GEN-UXO-YMAX` | UXO 当量最大输入 | 全屏浮层 | P0 |
| `DEC-GEN-SEED` | 随机种子输入 | 全屏浮层 | P0 |
| `DEC-GEN-BANNER` | 生成器校验横幅 | 全屏浮层 | P0 |
| `DEC-GEN-JSON` | 下载模拟场景 JSON 按钮 | 全屏浮层 | P0 |
| `DEC-GEN-CANCEL` | 取消按钮 | 全屏浮层 | P0 |
| `DEC-GEN-APPLY` | 应用生成按钮 | 全屏浮层 | P0 |
| `DEC-SB-DEVICE` | 设备状态标签（只读） | 状态栏 | P0 |
| `DEC-SB-SIM` | 模拟模式标签（只读） | 状态栏 | P0 |
| `DEC-SB-ALARM` | 告警滚动区（只读） | 状态栏 | P0 |
| `DEC-SB-TARGET` | 当前分析目标（只读） | 状态栏 | P0 |

**已退役 ID**（不在新原型中，文档不再分配）：

| 退役 ID | 原用途 | 退役原因 |
|---------|--------|----------|
| `DEC-RP-PRIORITY` | 修复优先级列表 | 降级为 P1 占位项，待 P1 评审通过后重新分配 ID |
| `DEC-RP-DRAFT` | 决策草案容器 | 降级为 P1 占位项，待 P1 评审通过后重新分配 ID |
| `DEC-RP-DRAFT-CONFIRM` | 确认选定方案按钮 | 移除：暗示真实安全结论，违反模拟边界 |
| `DEC-RP-DRAFT-EXPORT` | 草案导出报告按钮 | 合并到 P1 占位项"导出规划报告" |
| `DEC-TB-EXPORT` | 工具栏导出报告按钮（禁用占位） | 移除：与右面板 P1 槽位"导出规划报告"重复，工具栏不再保留 P1 入口 |
| `DEC-TB-EXTEND` | 工具栏扩展位按钮（禁用占位） | 移除：P1 扩展能力统一收敛到右面板 `DEC-RP-P1-SLOT`，避免重复入口 |
