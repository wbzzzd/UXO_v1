# 探测页面设计

状态：`TARGET / 设计评审原型 / 本地模拟（结构 100% 镜像 CURRENT DetectionView）`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](../design-system.md)
应用壳：[docs/ui/application-shell.md](../application-shell.md)
一览：[docs/ui/pages/index.md](index.md)
原型：[docs/ui/prototypes/detection/index.html](../prototypes/detection/index.html)
截图：[docs/ui/images/detection/overview-1920x1080.png](../images/detection/overview-1920x1080.png)

> 本文是探测页面（detection page）的完整设计契约。每个交互控件拥有稳定 `DET-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据，控件 ID、区域结构和状态规则不得在实现阶段自行变更。所有视觉值取自 `design-system.md`。

CURRENT 来源：
- **本页已实现为导航 index 1 独立页面（`DetectionView`，objectName `detectionPage`）。** 视频（态势页 PiP）每 3 秒抽帧送 `DetectionEngine` 真实 ONNX 推理，结果自动填充左侧结果表；点击行查看干净原图 + 分类结果（异常热力图在右侧独立模块展示），确认/拒绝（误报）人工二次校验联动目标状态机。
- 导航栏/菜单栏/状态栏：见 [`application-shell.md`](../application-shell.md)
- 页面实现与关联组件：
  - [`src/MainWindow/DetectionView.cpp`](../../../src/MainWindow/DetectionView.cpp)（探测页布局与结果交互）
  - [`src/Detection/DetectionEngine.cpp`](../../../src/Detection/DetectionEngine.cpp)（PatchCore + YOLOv8-cls 双阶段 ONNX 推理）
  - [`src/MainWindow/VideoStreamPanel.cpp`](../../../src/MainWindow/VideoStreamPanel.cpp)（视频播放与每 3 秒抽帧）
  - [`src/MainWindow/LeftPanelWidget.cpp`](../../../src/MainWindow/LeftPanelWidget.cpp)（态势页目标表，与探测页/详情浮层三向联动）
  - [`include/Core/Data/Types.h`](../../../include/Core/Data/Types.h)（数据类型枚举）
- 功能契约：[`docs/features/detection-onnx-integration.md`](../features/detection-onnx-integration.md)

## 1. 页面概述

探测页面是系统的第二页（导航 `DET-NAV-02` 默认选中），定位为探测结果人工复核工作区：AI 自动检测产出的逐帧推理结果落入左侧结果表；操作员点击行查看视频帧画面、Top-3 分类与异常热力图，用底部操作条对异常目标执行确认/拒绝（误报）复核或移除记录。页面结构 100% 镜像 CURRENT `DetectionView`：顶栏（AI 检测开关 + 帧统计）-> 主区域（左结果表 / 中视频画面 / 右详情三栏）-> 底部操作条；地图工具栏是态势页专属壳元素，本页不渲染（见 `application-shell.md` 第 5 节）。

所有数据来自 HTML 原型内嵌的本地固定 fixture（5 条记录：3 条异常 `target-001` 反跑道雷 86%、`target-002` 航弹 72%、`target-003` 火箭弹 91%，2 条正常帧），所有操作仅修改原型内存显示状态，不调用真实传感器或 AI 推理，不连接真实设备、不写入数据库、不执行排爆动作。

页面专用尺寸 token（镜像 CURRENT `DetectionView`）：顶栏 `--size-topbar-height:32px`、底部操作条 `--size-actionbar-height:44px`、中心区最小宽 `--size-center-min-width:400px`、左面板 `--size-left-panel-width:360px`、右面板 `--size-right-panel-width:380px`。

| 区域 | 位置 | 内部组件 | CURRENT 映射 |
|------|------|----------|--------------|
| 顶栏 | 菜单栏下，高 32px | AI 检测开关、帧统计文本（只读） | `DetectionView.cpp` `m_topBar`（`m_aiCheckBox` 默认勾选 + 帧统计标签） |
| 左侧结果面板 | 主区域左，宽 360px | 检测结果 7 列表 + 空态遮罩 | `DetectionView.cpp` `m_targetTable`（`populateTargetTable`） |
| 中心区 | 主区域中，弹性（最小宽 400px） | 视频帧画面（REC/时间码/检测红框/空态）+ 分类文本条 | `DetectionView.cpp` `m_videoLabel` + `m_top3ClassificationText` |
| 右面板 | 主区域右，宽 380px | 目标详情（8 行）/ 异常热力图 / 状态时间线（纵向 3:2:2） | `DetectionView.cpp` `populateDetail`/热力图模块/`populateTimeline` |
| 底部操作条 | 主区域下，高 44px | 确认（主按钮）/拒绝/移除记录 + 状态文本 | `DetectionView.cpp` `m_actionBar`（`updateActionBar` 启用规则） |
| 状态栏 | 窗口底部，高 22px | 设备/电量/模拟模式/告警滚动/急停占位 | `StatusBarWidget.cpp`（见 `application-shell.md` 第 6 节） |

## 2. 左侧结果面板

宽 360px（`--size-left-panel-width`），背景 `--color-panel`，纵向面板头 + 表格区。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|----------|------|----------|--------------|------|
| `DET-LP-TARGET-TABLE` | 检测结果 | table | 左面板，面板头（32px，标题"检测结果"）下方弹性区 | 展示逐帧推理结果，行选择驱动全页联动 | 7 列：目标 ID/类型/威胁/置信度/时间/状态/探测源；表头 sticky、背景 `--color-toolbar`、`--font-size-caption`；行高 32px；异常行 类型/威胁/状态 文本 `--color-danger`，正常行 类型 `--color-status-online`（"正常"）、其余列"--"，探测源恒"AI 分析"；初始 5 行，首行选中（背景 `--color-selection`） | 行背景 `--color-row-hover` | 不适用 | 点击行：置为选中行并全页联动（中心画面/分类条/详情/热力图/时间线/操作条） | 原型行仅鼠标点击（无 tabindex）；CURRENT `QTableWidget` 支持键盘行选择 | 固定 fixture 5 行（3 异常 + 2 正常）；移除记录后行数减少，删空显示空态遮罩 | `DetectionView.cpp` `populateTargetTable`（第 362-405 行） | 只读展示与本地选择，无真实数据源 |

子元素（只读，不分配 ID）：
- 表空态遮罩：无记录时覆盖表格区，居中显示"等待检测结果"（`--color-text-secondary`，`--font-size-caption`）。

## 3. 中心区

弹性宽（最小 400px），纵向：视频帧画面（弹性高）+ 分类文本条（高 32px），间距 8px，内边距 8px。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|----------|------|----------|--------------|------|
| `DET-CE-VIEWER` | 视频帧画面 | div 容器 | 中心区上段，弹性高 | 展示选中记录的视频帧模拟画面与检测叠加层 | 固定渐变 + 网纹 + 十字准星模拟画面（原型视觉，非 token）；1px `--color-border` 边框、圆角 `--radius-control` | 不适用 | 不适用 | 无（只读展示） | 不可聚焦 | 有选中记录时：顶部 REC 录制指示（红点 1.2s 闪烁）、右下时间码（等宽 12px）；异常帧额外显示检测红框（2px `--color-threat-high` 边框、1s 闪烁、角标"ID 类型 置信度"）；空态整区遮罩"等待检测结果" | `DetectionView.cpp` `m_videoLabel`（显示真实抽帧画面） | 无 |
| `DET-CE-CLASS` | 分类文本条 | div 容器 | 中心区下段，高 32px | 展示当前帧 Top-3 分类文本 | 高 `--size-toolbar-height`、背景 `--color-toolbar`、1px `--color-border` 边框、圆角 `--radius-control`、内边距 0 12px；文本 `--font-size-caption` 主文本色，超长省略号 | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 异常帧"分类: 名称 xx%  \|  名称 yy%  \|  名称 zz%"（双空格分隔）；正常帧"分类: 无（未达分类阈值或无异常）"；空态"分类: --" | `DetectionView.cpp` `m_top3ClassificationText`（`top3ClassificationText` 第 59-76 行） | 无 |

## 4. 右面板

宽 380px（`--size-right-panel-width`），背景 `--color-panel`，纵向三段 flex 3:2:2（目标详情/异常热力图/状态时间线），每段头 32px（背景 `--color-toolbar`、标题加粗）。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|----------|------|----------|--------------|------|
| `DET-RP-DETAIL` | 目标详情 | div 容器 | 右面板上段（flex 3） | 展示选中记录 8 行键值详情 | 8 行键值对：标签宽 80px `--color-text-secondary`、值 `--color-text-primary`、`--font-size-caption`；威胁等级"高"红色（`--color-danger`）加粗 | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 行序：目标 ID/类型/威胁等级/置信度/最大异常分（4 位小数）/帧时间/推理耗时（"x ms"）/探测源（"AI 分析"）；正常帧 目标 ID/威胁/置信度为"--"、类型"正常"；空态全"--" | `DetectionView.cpp` `populateDetail`（第 465-487 行） | 无 |
| `DET-RP-HEATMAP` | 异常热力图 | div 容器 | 右面板中段（flex 2） | 展示异常帧热斑分布（本地模拟） | 画布深色背景、1px 边框、自带 8px 边距；3 个径向渐变模拟热斑（红/橙/黄，blur 6px） | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 仅异常帧显示热斑；正常帧与空态显示遮罩"无热力图" | `DetectionView.cpp` 右侧热力图模块（CURRENT 由真实推理异常分数驱动） | 无 |
| `DET-RP-TIMELINE` | 状态时间线 | div 容器 | 右面板下段（flex 2） | 展示选中记录状态流转 | 条目 `--font-size-caption` 主文本色、行高 1.6、保留换行；格式"HH:mm:ss  事件" | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 每条记录基线首条"HH:mm:ss  已发现（AI）"；确认/拒绝追加"HH:mm:ss  已确认（人工）"/"已拒绝（人工）"（取操作时刻）；空态"--" | `DetectionView.cpp` `populateTimeline`（第 492-501 行） | 无 |

## 5. 顶栏与底部操作条

顶栏高 32px（背景 `--color-toolbar`，下边框 1px）；底部操作条高 44px（背景 `--color-toolbar`，上边框 1px），按钮高 30px、间距 12px。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|----------|------|----------|--------------|------|
| `DET-TB-AI` | [AI] 自动检测 | button | 顶栏左 1 | AI 自动检测开关 | 初始激活态（镜像 `m_aiCheckBox` 默认勾选）：背景/边框 `--color-primary`、主文本色、加粗、`--font-size-caption`、内边距 3px 12px、圆角 `--radius-control`；tooltip"AI 自动检测开关（镜像 CURRENT m_aiCheckBox，本地模拟）" | 未激活态：边框 `--color-border`、辅助色文本；hover 边框 `--color-border-focus`、主文本色 | 不适用 | 切换激活/未激活视觉状态（仅 class 切换） | Tab 聚焦，Enter/Space 触发 | 本地仅切换视觉，不启停任何推理链路 | `DetectionView.cpp` `m_aiCheckBox` | 无 |
| `DET-TB-SUMMARY` | 帧统计 | span（只读） | 顶栏左 2 | 展示帧统计汇总 | `--font-size-caption`、`--color-text-secondary`，文本"已分析 X 帧 · 异常 Y" | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 初始"已分析 5 帧 · 异常 3"；移除记录后实时更新 | `DetectionView.cpp`（第 608 行，"已分析 %1 帧 · 异常 %2"） | 无 |
| `DET-CE-CONFIRM` | 确认 | button | 底部操作条左 1 | 确认当前异常目标（人工复核） | 主按钮（镜像 `GlobalStyle::getButtonStyle(true)`）：高 30px、内边距 0 16px、背景 `--color-primary`、主文本色、`--font-size-caption`、圆角 `--radius-control`；tooltip"确认当前异常目标（镜像 CURRENT m_confirmBtn，本地模拟）" | 背景 `--color-primary-hover` | 背景 `--color-border`、文本 `--color-text-disabled` | 置复核状态为"已确认"：状态列变"已确认"、时间线追加"已确认（人工）"、确认/拒绝转禁用 | Tab 聚焦，Enter/Space 触发 | 仅"异常且待复核"记录可用（镜像 `updateActionBar`）；本地模拟，无真实状态机回写 | `DetectionView.cpp` `m_confirmBtn`（`updateActionBar` 第 576-598 行） | 无 |
| `DET-CE-REJECT` | 拒绝 | button | 底部操作条左 2 | 拒绝当前异常目标（误报标记） | 次级按钮（镜像 `getButtonStyle(false)`）：透明背景、1px `--color-border` 边框、主文本色，尺寸同上；tooltip"拒绝当前异常目标（镜像 CURRENT m_rejectBtn，本地模拟）" | 背景 `--color-border` | 文本 `--color-text-disabled` | 置复核状态为"已拒绝"：状态列变"已拒绝"、时间线追加"已拒绝（人工）"、确认/拒绝转禁用 | 同上 | 同上 | `DetectionView.cpp` `m_rejectBtn` | 无 |
| `DET-CE-REMOVE` | 移除记录 | button | 底部操作条左 3 | 移除当前选中记录行 | 次级按钮，同上；tooltip"移除当前选中记录（镜像 CURRENT m_removeBtn，本地模拟）" | 同上 | 无记录时禁用（同上样式） | 删除当前行：删空进入空态；删末行回退上一行；否则下一行顶上；帧统计同步更新 | 同上 | 任意选中行可用（含正常帧），镜像 CURRENT | `DetectionView.cpp` `m_removeBtn` | 无 |
| `DET-CE-STATUS` | 状态文本 | span（只读） | 底部操作条右端（右对齐） | 展示当前目标与复核状态 | 12px、`--color-text-secondary` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | "当前目标: X · 状态: Y"；正常帧"当前目标: -- · 状态: 正常帧"；无记录"等待检测结果" | `DetectionView.cpp` `m_statusLabel` | 无 |

## 6. 应用壳复用控件

本页复用应用壳的导航栏、菜单栏与状态栏；完整规格与 CURRENT 映射以 `application-shell.md` 第 3/4/6 节为准，以下仅记录本页实例的差异要点。

### 6.1 导航栏

| ID | 标签 | 形态 | 行为 |
|----|------|------|------|
| `DET-NAV-LOGO` | UXO | 只读 logo | 仅展示 |
| `DET-NAV-01` | 态势 | 真实链接 `<a>` | 跳转 `../situation/index.html`（本页内不选中） |
| `DET-NAV-02` | 探测 | div（当前页） | 默认选中态（`--color-selection` 背景 + 主色左边框 3px） |
| `DET-NAV-03` | 决策 | 真实链接 `<a>` | 跳转 `../decision/index.html` |
| `DET-NAV-04` | 设备 | div 占位 | tooltip"未实现页面（占位）"，点击仅切换 selected 高亮，不跳转 |
| `DET-NAV-05` | 统计 | div 占位 | 同上 |
| `DET-NAV-06` | 配置 | div 占位 | 同上 |

图标（`--size-icon-nav` 16px）、五态与选中规则同态势页导航（`application-shell.md` 第 3 节；图标映射见 `design-system.md` 第 8 节）。已实现页面间为真实 `<a>` 链接互跳，语义同 CURRENT `QStackedWidget` 路由（本页为 index 1）。

### 6.2 菜单栏

4 个顶级菜单：`DET-MENU-FILE` 文件(&F)、`DET-MENU-VIEW` 视图(&V)、`DET-MENU-TOOLS` 工具(&T)（禁用占位，`data-disabled="true"`，tooltip"占位控件，未实现"）、`DET-MENU-HELP` 帮助(&H)。样式与交互同态势页菜单（`application-shell.md` 第 4 节）：点击切换 `active` 类（无对应样式规则，无视觉变化），不展开下拉菜单。

### 6.3 状态栏

| ID | 内容 | 说明 |
|----|------|------|
| `DET-SB-DEVICE` | 设备: 2/2 在线 | 只读 |
| `DET-SB-BATTERY` | 最低电量: 74% | 只读 |
| `DET-SB-SIM` | [模拟模式] | 加粗徽标 |
| `DET-SB-ALARM` | 4 条模拟告警 | `[模拟] target-001 待人工确认`/`[模拟] target-003 检测置信度 91%`/`[模拟] UAV-1 电量 82%`/`[模拟] Robot-1 待命中`，18s 无缝横向滚动（同态势页） |
| `DET-SB-EMERGENCY` | 紧急停止 | 恒禁用占位（tooltip"模拟占位，无实际效果"，fa-hand 图标），见 `application-shell.md` 第 6 节 |

## 7. 页面五态

| 状态 | 表现 |
|------|------|
| 正常 (normal) | 5 条 fixture 记录渲染、首行选中、全页联动可用 |
| 加载 (loading) | 原型为本地固定 fixture 即时渲染，无独立加载态 |
| 空 (empty) | 移除全部记录后：表遮罩"等待检测结果"、画面遮罩"等待检测结果"、分类"分类: --"、详情 8 行"--"、时间线"--"、热力图"无热力图"、操作条三按钮全禁用 + 状态"等待检测结果"、帧统计"已分析 0 帧 · 异常 0" |
| 错误 (error) | 原型无错误态（本地 fixture 不产生失败分支） |
| 禁用 (disabled) | 确认/拒绝：选中记录非异常或已复核时禁用；移除记录：无记录时禁用；工具菜单与急停恒禁用 |

## 8. 交互流程

1. **行选择联动**：点击结果表任意行 -> 该行置选中（背景 `--color-selection`）-> 中心画面（REC/时间码/异常帧红框）、分类文本条、右面板详情/热力图/时间线、底部操作条按钮与状态文本全部按该记录刷新。
2. **确认**：选中"异常且待复核"记录 -> 点击确认 -> 复核状态置"已确认"（状态列与状态文本同步），时间线追加"HH:mm:ss  已确认（人工）"，确认/拒绝按钮转禁用（移除仍可用）。
3. **拒绝**：同确认流程，复核状态置"已拒绝"，时间线追加"已拒绝（人工）"。
4. **移除记录**：点击移除 -> 当前行从表删除 -> 删空进入空态（见第 7 节）；删末行自动回退选中上一行；否则原位选中下一行；帧统计文本同步更新。
5. **AI 检测开关**：点击切换激活/未激活视觉状态，仅视觉演示（CURRENT 中该开关启停每 3 秒抽帧推理链路）。
6. **导航与菜单**：01/03 真实链接跳转对应页原型；04-06 占位项点击仅切换 selected 高亮；菜单项点击切换 active 类（无视觉变化）。

## 9. 已知偏差（文档化分歧）

| # | 偏差 | CURRENT | 原型 |
|---|------|---------|------|
| 1 | 初始选中态 | 初始为空态"等待检测结果" | 静态预选中第一条记录，便于评审完整联动 |
| 2 | 热力图数据 | 真实推理异常分数驱动 | 固定 3 个径向渐变模拟热斑 |
| 3 | AI 开关效果 | `m_aiCheckBox` 实际启停抽帧推理链路 | 仅切换视觉激活态 |
| 4 | 视频画面 | `m_videoLabel` 显示真实抽帧画面 | 固定渐变 + 网纹 + 十字准星模拟视觉，REC 与时间码为演示元素 |
| 5 | 复核联动 | 确认/拒绝回写目标状态机（与态势页三向联动） | 仅修改原型内存 fixture，刷新即还原 |

## 10. 安全边界

- 本页全部数据为原型内嵌本地 fixture，不调用真实传感器、`DetectionEngine` 推理或外部系统。
- 确认/拒绝/移除仅修改原型显示状态，不写入数据库、不触发真实状态机流转、不产生设备命令。
- 帧统计、时间线、热力图均为模拟演示值，不构成真实检测结果。
- 急停按钮恒禁用占位（无实际效果）；模拟内容均带"[模拟]"或"本地模拟"标注。

## 11. CURRENT 映射

| 页面元素 | CURRENT 源码位置 |
|----------|------------------|
| 页面装配（顶栏/结果表/视频/分类条/右面板/操作条） | `src/MainWindow/DetectionView.cpp` 构造与 `setupUi` |
| Top-3 分类文本 | `DetectionView.cpp` `top3ClassificationText`（第 59-76 行） |
| 结果表 7 列 | `DetectionView.cpp` `populateTargetTable`（第 362-405 行） |
| 目标详情 8 行 | `DetectionView.cpp` `populateDetail`（第 465-487 行） |
| 状态时间线 | `DetectionView.cpp` `populateTimeline`（第 492-501 行） |
| 操作条与启用规则 | `DetectionView.cpp` `updateActionBar`（第 576-598 行） |
| 空态 | `DetectionView.cpp` `showEmptyState`（第 612-623 行） |
| 帧统计文本 | `DetectionView.cpp`（第 608 行） |
| 复核状态文案 | `DetectionView.cpp` `reviewText` 映射（第 627-634 行） |
| 推理引擎（原型仅模拟其结果形态） | `src/Detection/DetectionEngine.cpp` |

## 12. ID 索引

| ID | 说明 | 所属 |
|----|------|------|
| `DET-MENU-FILE` | 菜单：文件 | 应用壳 |
| `DET-MENU-VIEW` | 菜单：视图 | 应用壳 |
| `DET-MENU-TOOLS` | 菜单：工具（禁用占位） | 应用壳 |
| `DET-MENU-HELP` | 菜单：帮助 | 应用壳 |
| `DET-TB-AI` | 顶栏：AI 自动检测开关 | 页面 |
| `DET-TB-SUMMARY` | 顶栏：帧统计文本（只读） | 页面 |
| `DET-NAV-LOGO` | 导航：UXO logo（只读） | 应用壳 |
| `DET-NAV-01` | 导航项：态势（已实现，链接跳转态势页原型） | 应用壳 |
| `DET-NAV-02` | 导航项：探测（当前页，选中态） | 应用壳 |
| `DET-NAV-03` | 导航项：决策（已实现，链接跳转决策页原型） | 应用壳 |
| `DET-NAV-04` | 导航项：设备（占位） | 应用壳 |
| `DET-NAV-05` | 导航项：统计（占位） | 应用壳 |
| `DET-NAV-06` | 导航项：配置（占位） | 应用壳 |
| `DET-LP-TARGET-TABLE` | 左面板：检测结果表 | 页面 |
| `DET-CE-VIEWER` | 中心：视频帧画面 | 页面 |
| `DET-CE-CLASS` | 中心：分类文本条 | 页面 |
| `DET-RP-DETAIL` | 右面板：目标详情 | 页面 |
| `DET-RP-HEATMAP` | 右面板：异常热力图 | 页面 |
| `DET-RP-TIMELINE` | 右面板：状态时间线 | 页面 |
| `DET-CE-CONFIRM` | 底部操作条：确认（主按钮） | 页面 |
| `DET-CE-REJECT` | 底部操作条：拒绝 | 页面 |
| `DET-CE-REMOVE` | 底部操作条：移除记录 | 页面 |
| `DET-CE-STATUS` | 底部操作条：状态文本（只读） | 页面 |
| `DET-SB-DEVICE` | 状态栏：设备在线 | 应用壳 |
| `DET-SB-BATTERY` | 状态栏：最低电量 | 应用壳 |
| `DET-SB-SIM` | 状态栏：模拟模式标注 | 应用壳 |
| `DET-SB-ALARM` | 状态栏：模拟告警滚动 | 应用壳 |
| `DET-SB-EMERGENCY` | 状态栏：紧急停止（禁用占位） | 应用壳 |
