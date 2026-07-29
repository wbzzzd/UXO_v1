# 六页一览表

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](../design-system.md)
应用壳：[docs/ui/application-shell.md](../application-shell.md)

> 本文是六页 UI 的一览表。每页一段：定位、关键区域、控件 ID 前缀、HTML 原型路径、截图路径。六页均已交付逐控件清单（详见上表"详细清单"列）。

## 总览

| 页面（中文） | English | 导航前缀 | 详细清单 | HTML 原型 | 1920×1080 截图 |
|--------------|---------|----------|----------|-----------|----------------|
| [态势](#1-态势页-situation-sit) | Situation | `SIT-` | [`situation.md`](situation.md) | [`../prototypes/situation/index.html`](../prototypes/situation/index.html) | [`../images/situation/overview-1920x1080.png`](../images/situation/overview-1920x1080.png) |
| [探测](#2-探测页-detection-det) | Detection | `DET-` | [`detection.md`](detection.md) | [`../prototypes/detection/index.html`](../prototypes/detection/index.html) | [`../images/detection/overview-1920x1080.png`](../images/detection/overview-1920x1080.png) |
| [决策](#3-决策页-decision-dec) | Decision | `DEC-` | [`decision.md`](decision.md) | [`../prototypes/decision/index.html`](../prototypes/decision/index.html) | [`../images/decision/overview-1920x1080.png`](../images/decision/overview-1920x1080.png) |
| [设备](#4-设备页-devices-dev) | Devices | `DEV-` | [`devices.md`](devices.md) | [`../prototypes/devices/index.html`](../prototypes/devices/index.html) | [`../images/devices/overview-1920x1080.png`](../images/devices/overview-1920x1080.png) |
| [统计](#5-统计页-statistics-sta) | Statistics | `STA-` | [`statistics.md`](statistics.md) | [`../prototypes/statistics/index.html`](../prototypes/statistics/index.html) | [`../images/statistics/overview-1920x1080.png`](../images/statistics/overview-1920x1080.png) |
| [配置](#6-配置页-configuration-cfg) | Configuration | `CFG-` | [`configuration.md`](configuration.md) | [`../prototypes/configuration/index.html`](../prototypes/configuration/index.html) | [`../images/configuration/overview-1920x1080.png`](../images/configuration/overview-1920x1080.png) |

所有页面共享同一套设计系统（`design-system.md`）与应用壳（`application-shell.md`），仅左/右面板内容、中心区内容、工具栏与状态栏细节按页变化。

## 1. 态势页 Situation (SIT)

**定位**：系统默认页面（导航 `SIT-NAV-01` 默认选中）。一屏呈现：左侧目标/任务/设备列表，中心视频流与告警/操作日志，右侧三维态势地图、设备状态、决策建议。所有数据来自本地模拟 fixture，所有操作仅修改内存，不连接真实设备。

**关键区域**：

| 区域 | 位置 | 内容 |
|------|------|------|
| A 左面板 | 320px 固定 | 搜索栏、状态子标签、三标签表格（目标/任务/设备） |
| B 中心上 | stretch 3 | 视频流面板（模拟占位） |
| C 中心下 | stretch 2 | 告警面板、模拟流程与操作日志、批量操作条 |
| D 右面板 | 360px | 三维态势地图、模拟设备状态、模拟决策建议 |

**CURRENT 主控件**：`LeftPanelWidget`、`VideoStreamPanel`、`AlertPanel`、`DetectionControlPanel`、`BatchOperationBar`、`RightPanelWidget`（含 `SituationView`、`DeviceStatusPanel`、`DecisionSuggestionPanel`）。

**详细清单**：[`situation.md`](situation.md)（648 行，逐控件规格）。

## 2. 探测页 Detection (DET)

**定位**：探测目标管理页（导航 `DET-NAV-02`）。一屏呈现：左侧探测任务列表与扫描设备状态；中心目标表格（类型/威胁/置信度/状态/探测源），支持搜索、筛选、排序、行选择；右侧目标证据 Tab（图像/光谱/历史）与模拟确认/拒绝操作。所有数据为本地固定模拟场景。

**关键区域**：

| 区域 | 位置 | 内容 |
|------|------|------|
| A 左面板 | 260px | 探测任务列表、扫描设备状态、新增目标（禁用占位） |
| B 中心 | 弹性 | 目标表格（5 个模拟目标）、搜索栏、筛选、排序、行选择 |
| C 右面板 | 320px | 目标证据 Tab（图像/光谱/历史）、模拟确认/拒绝、置信度详情 |

**控件 ID 前缀**：`DET-`，包括 `DET-LP-TASK-*`、`DET-CE-TABLE`、`DET-CE-SEARCH`、`DET-RP-CONFIRM`、`DET-RP-REJECT` 等。

**HTML 原型**：[`../prototypes/detection/index.html`](../prototypes/detection/index.html)（单文件，内联 CSS+JS，5 个模拟目标）。

**详细清单**：[`detection.md`](detection.md)（492 行，逐控件规格）。

## 3. 决策页 Decision (DEC)

**定位**：MOS 起降带规划工作区（导航 `DEC-NAV-03`）。一屏呈现：左侧损毁目标列表（弹坑/UXO，带威胁/状态标记）；中心上方跑道 3000m×50m 俯视图（叠加弹坑/UXO/MOS 矩形），中心下方算法参数表单（MOSPlanParams）；右侧候选起降方案卡片 + 选定详情 + 修复优先级 + 决策草案。所有数据为本地固定模拟场景。功能依据：`docs/dev/architecture-uxo-recognition-and-mos.md`。

**关键区域**：

| 区域 | 位置 | 内容 |
|------|------|------|
| A 左面板 | 260px | 损毁目标列表（4 个模拟目标：弹坑/UXO，带威胁/状态/坐标/尺寸） |
| B 中心上 | flex:1 | 跑道俯视图（3000×50m 跑道 + 4 个目标圆圈 + 3 个 MOS 矩形 + 图例 + 缩放） |
| B' 中心下 | 固定高度 | 算法参数栏（MOSPlanParams，10 个可编辑输入 + 2 个只读 + 校验状态 + 重新规划） |
| C 右面板 | 380px | 候选方案卡片×3 + 选定详情摘要 + 修复优先级列表 + 决策草案 |

**控件 ID 前缀**：`DEC-`，包括 `DEC-LP-TARGET-LIST`、`DEC-CE-RUNWAY`、`DEC-CE-PARAMS`、`DEC-RP-PLANS`、`DEC-RP-DETAIL`、`DEC-RP-PRIORITY`、`DEC-RP-DRAFT` 等。

**HTML 原型**：[`../prototypes/decision/index.html`](../prototypes/decision/index.html)（单文件，4 个模拟损毁点 + 3 档起降方案）。

**详细清单**：[`decision.md`](decision.md)（逐控件规格）。

## 4. 设备页 Devices (DEV)

**定位**：设备状态与管理页（导航 `DEV-NAV-04`）。一屏呈现：左侧设备列表与搜索筛选；中心设备详情（实时状态/历史指标/任务记录）；右侧可用性检查、模拟指派、设备推荐。所有设备状态为本地模拟。

**关键区域**：

| 区域 | 位置 | 内容 |
|------|------|------|
| A 左面板 | 260px | 设备列表（4 个模拟设备）、搜索、类型筛选 |
| B 中心 | 弹性 | 设备详情、实时状态、历史指标（模拟）、任务记录 |
| C 右面板 | 320px | 可用性检查、模拟指派、设备推荐 |

**控件 ID 前缀**：`DEV-`，包括 `DEV-LP-DEV-*`、`DEV-CE-DETAIL`、`DEV-RP-CHECK`、`DEV-RP-ASSIGN`、`DEV-RP-RECOMMEND` 等。

**HTML 原型**：[`../prototypes/devices/index.html`](../prototypes/devices/index.html)（单文件，4 个模拟设备）。

**详细清单**：[`devices.md`](devices.md)（463 行，逐控件规格）。

## 5. 统计页 Statistics (STA)

**定位**：当前会话指标统计页（导航 `STA-NAV-05`）。一屏呈现：左侧指标分类（总览/目标/任务/设备/告警）与会话信息；中心 KPI 卡片、图表视图、表格视图切换；右侧筛选（时间/类型/威胁）与状态示例。所有指标为当前会话本地固定数据，无历史数据库、无导出、无回放。

**关键区域**：

| 区域 | 位置 | 内容 |
|------|------|------|
| A 左面板 | 260px | 指标分类导航、会话信息（ID/时长/数据来源） |
| B 中心 | 弹性 | KPI 卡片（4 张）、图表视图（目标类型/威胁等级分布）、表格视图切换 |
| C 右面板 | 320px | 筛选（时间/类型/威胁）、数据口径说明、空会话与错误状态示例 |

**控件 ID 前缀**：`STA-`，包括 `STA-LP-METRIC-NAV`、`STA-CE-KPI-*`、`STA-CE-CHART`、`STA-CE-TABLE`、`STA-RP-FILTER-*` 等。

**HTML 原型**：[`../prototypes/statistics/index.html`](../prototypes/statistics/index.html)（单文件，5 个模拟目标 + 4 张 KPI 卡片 + 2 张图表）。

**详细清单**：[`statistics.md`](statistics.md)（531 行，逐控件规格）。

## 6. 配置页 Configuration (CFG)

**定位**：系统事实与临时展示页（导航 `CFG-NAV-06`）。一屏呈现：左侧配置分类（系统信息/界面显示/数据存储/网络通信/安全控制/关于）与系统版本信息；中心区分只读事实、临时（仅本会话）、禁用三类配置；右侧安全边界清单与变更记录（模拟）。所有配置不持久化，会话结束即清空。

**关键区域**：

| 区域 | 位置 | 内容 |
|------|------|------|
| A 左面板 | 260px | 配置分类导航、系统版本信息（只读） |
| B 中心 | 弹性 | 系统事实（只读）、界面显示（临时）、数据存储/网络通信（禁用）、安全控制（锁定）、关于 |
| C 右面板 | 320px | 安全边界清单（5 项全部禁用/锁定）、变更记录（模拟） |

**控件 ID 前缀**：`CFG-`，包括 `CFG-LP-NAV`、`CFG-CE-TOGGLE-COORD`、`CFG-CE-BTN-APPLY`、`CFG-SB-EMERGENCY` 等。

**安全边界**：紧急停止、排爆执行、设备控制指令、数据库写入、外部通信五项全部禁用并标注"锁定/禁用"，原型中按钮均 `disabled`。

**HTML 原型**：[`../prototypes/configuration/index.html`](../prototypes/configuration/index.html)（单文件，6 个配置分类，禁用项以单行摘要呈现）。

**详细清单**：[`configuration.md`](configuration.md)（574 行，逐控件规格）。
