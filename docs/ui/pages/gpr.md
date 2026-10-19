# 雷达页（GPR / 探地雷达）

> 状态：CURRENT 已实现（Qt 直接实现，无 HTML 原型）/ 本地模拟数据 / REQ-012 待 §7.4 人工走查后转 Implemented
> 需求：[REQ-012](../../requirements/REQ-012.md)（Approved）｜ 功能契约：[gpr-display.md](../../features/gpr-display.md)（Approved）
> 上级：[README.md](../README.md) ｜ 设计系统：[design-system.md](../design-system.md) ｜ 应用壳：[application-shell.md](../application-shell.md) ｜ 一览：[index.md](index.md)
> 截图：无 HTML 原型截图；Qt 三视口截图在 `.omo/evidence/gpr-display/`（本地证据，不入库）

## CURRENT 来源

- 页面装配与回放控制：`src/MainWindow/GprView.cpp`（头 `include/MainWindow/GprView.h`）
- B-Scan 伪彩剖面：`src/MainWindow/GprBScanWidget.cpp`（头 `include/MainWindow/GprBScanWidget.h`）
- 疑似目标面板：`src/MainWindow/GprSuspectPanel.cpp`（头 `include/MainWindow/GprSuspectPanel.h`）
- GPR 库（include/GPR/ + src/GPR/）：`GprMalaIo`（.rad/.rd3 只读解析）、`GprBScanData`（通道数据与整通道预载）、`GprT0Estimator`（首波/t0 估计）、`GprThresholdDetector`（阈值候选检测）
- 主壳接入：`src/MainWindow/MainWindow.cpp:302-305`（QStackedWidget index 3）
- UI 契约测试：`tests/gpr_view_test.cpp`（objectName/信号契约，gpr-display.md §7.3）

## 1 页面概述

### 1.1 定位

探地雷达样例数据的展示与目标模拟演示页（REQ-012）。操作员在此回放 B-Scan 剖面、查看自动检测给出的疑似目标候选、人工复核后确认目标或标记误报；确认的目标经统一目标注入链路进入态势页目标表/地图/日志（REQ-006）。

页面演示"探测车已返回、数据已导入"之后的查看与校验流程；真实采集控制与实时接入不在本期范围（REQ-012 §4 非目标）。

### 1.2 页面结构

| 区域 | 内容 | 宽度/高度 |
|---|---|---|
| 顶部工具栏 | 频段、通道、增益模式、对比度、缩放、[模拟] 标注 | 整行 |
| 中心 B-Scan 区 | 伪彩剖面 + 回放游标 + 疑似目标标记 + 状态页 | 自适应 |
| 右栏 | 疑似目标面板（候选表 + 确认/误报） | 固定 320px |
| 底部回放条 | 播放/暂停、倍速、进度条、位置标签 | 整行 |

### 1.3 数据事实

- 样例数据 `/mnt/d/工作/排弹/数据集/001`（9p 只读挂载），MALA `.rad` 头文件 + `.rd3` int16 小端数据。
- H/L 两个频段 × 16 通道；单文件 7297 道 × 512 样本（H、L 各一份 .rd3）。
- 数据已入库（采集完成），本页只做查看与校验；展示数据为模拟/演示用途。
- 页面无 HTML 原型：雷达页是 REQ-012 增量，在六页原型体系之外由 Qt 直接实现，控件规格以本文档与 gpr-display.md §6 为准。

### 1.4 控件 ID 前缀

`GPR-`（T=工具栏，C=中心区，P=回放条，S=右栏面板）；Qt objectName 一律 `gpr*` 前缀，见 §11 映射表。

## 2 顶部工具栏

| ID | 标签/控件 | 功能 | 设计依据（为什么需要它） | CURRENT 映射 |
|---|---|---|---|---|
| GPR-T-01 | 频段选择（QComboBox：H / L） | 切换当前查看的频段数据 | 预研实测 H/L 数据近重复（道波形相关系数 0.953–0.981，gpr-display.md §5.4），默认 H 即可满足演示；保留切换入口用于对照验证与后续双频段扩展 | `gprBandCombo`（GprView） |
| GPR-T-02 | 通道选择（QComboBox：通道 1–16） | 切换当前查看的测线通道 | 16 条测线覆盖不同纵向位置；邻近通道对照是人工复核疑似目标的主要手段（真实异常在相邻剖面中位置连贯，噪声/杂波则不然） | `gprChannelCombo`（GprView） |
| GPR-T-03 | 增益模式（QComboBox：AGC / 固定 / 原始） | 切换剖面增益显示方式 | 原始信号深部反射微弱，不增益看不到目标深度双曲线。AGC（逐道滑动 RMS 时变增益）预研实测目视最有效，设为默认；固定（逐深度行白化，≤10×）与检测管线白化同族，用于对照；原始用于排查增益引入的伪影（§5.4） | `gprGainModeCombo`（GprView）→ `GprBScanWidget::GainMode` |
| GPR-T-04 | 对比度（QSlider，固定宽 90px） | 归一化幅值线性拉伸，增强/减弱反射视觉对比 | 不同通道噪声水平与目标强度差异大，操作员需要手动拉开目标与背景；重建经 120ms 重启式防抖，拖动连续触发合并为单次重建，避免阻塞事件循环 | `gprContrastSlider`（GprView）→ `GprBScanWidget::setContrast` |
| GPR-T-05 | 缩放（QSlider，固定宽 90px） | 水平方向显示倍率（1.0 = 适配控件宽度） | 7297 道压进一屏后单个双曲线只有几像素宽，水平放大才能辨认形态与分层 | `gprZoomSlider`（GprView）→ `GprBScanWidget::setZoom` |
| GPR-T-06 | [模拟] 标注（QLabel） | 常驻声明本页数据为模拟/演示用途 | §6.4 模拟来源标注：模拟数据不得与真实设备状态混淆，标注常驻不可关闭 | `gprSimTagLabel`（GprView） |

## 3 中心 B-Scan 区

| ID | 元素 | 功能 | 设计依据 | CURRENT 映射 |
|---|---|---|---|---|
| GPR-C-01 | B-Scan 伪彩剖面 | 道为横轴、深度为纵轴的伪彩渲染；diverging seismic 类色标（负-蓝 / 零-白 / 正-红），P99.8 截幅 + γ=0.8 显示映射；纵轴样本经逐通道 t0（firstBreakSample）校正后换算深度，图像第 0 行 = t0 样本 | §5.4 显示参数建议的实测选型；t0 校正是显示与检测的共同前置（直达波位置逐通道漂移，不校正深度轴会错位） | `GprBScanWidget::setScan / rebuildImage` |
| GPR-C-02 | 回放游标 | 当前道位置的竖线叠加，随播放/拖进度条移动；-1 隐藏 | 让"播放到哪了"在剖面上直接可见，是回放叙事的核心视觉锚点 | `GprBScanWidget::setCursorTrace` |
| GPR-C-03 | 疑似目标标记 | 候选在剖面上的外接框 + ID 标签叠加，与右栏候选表一一对应 | 候选必须落在剖面上可对照的位置，人工才能判断"这个框是不是真目标"（§3.1 步骤 3–5） | `GprBScanWidget::setSuspects` |
| GPR-C-04 | 状态页 | QStackedLayout 四页：ready（剖面）/ loading（解析检测中）/ empty（无数据）/ error（错误原因） | §6.1 页面五态要求；加载走 QtConcurrent 后台 worker，整通道数据一次性预载（bulk read）后解析，避免逐道 IO 卡顿 | `GprView::m_stack`（ready/loading/empty/error） |

## 4 底部回放条

| ID | 标签/控件 | 功能 | 设计依据 | CURRENT 映射 |
|---|---|---|---|---|
| GPR-P-01 | 播放/暂停（QPushButton） | 启动/暂停回放；到末道自动停止并复位文案 | 回放=重演采集过程：数据已入库，播放按采集速率把道逐列显现，让操作员以"车当时怎么开过来的"的方式浏览剖面（§2.2 设计决策：纯展示叙事，与检测解耦） | `gprPlayPauseButton`（GprView）→ `onPlayPauseClicked` |
| GPR-P-02 | 倍速（QComboBox：1×/2×/4×） | 调整回放速率 | 1× ≈ 81 道/秒，对齐 §5.4 实测采集点率（演示"真实节奏"）；2×/4× 用于快速过完整条测线 | `gprSpeedCombo`（GprView）；定时器 50ms tick（`m_playbackTimer`） |
| GPR-P-03 | 进度条（QSlider） | 显示并可拖动定位当前道；范围随通道，步进 10 道，按下即跟踪（tracking） | 7297 道全程 1× 约 90 秒，必须支持直接跳转；步进 10 道是定位精度与拖动流畅的折中 | `gprProgressSlider`（GprView）→ `onProgressSliderMoved` |
| GPR-P-04 | 位置标签（QLabel） | 显示"当前道 n/N \| RTK 经纬度 \| 时间戳" | 把抽象的道索引换算成地理坐标与时间，操作员据此把剖面位置对应到现场位置；元数据缺失时优雅降级显示 | `gprPositionLabel`（GprView）→ `updatePositionLabel` |

**播放与检测的关系（常见疑问）**：候选目标在通道加载完成时即由后台 worker 离线全量检测并一次性叠加（§2.2：检测管线作为既有数据处理，非实时流）；播放只移动游标，不参与检测。因此"播放前候选已标好"是设计使然——播放服务于浏览叙事，检测服务于复核效率，二者解耦。

## 5 右栏疑似目标面板

| ID | 标签/控件 | 功能 | 设计依据 | CURRENT 映射 |
|---|---|---|---|---|
| GPR-S-01 | 面板标题（QLabel） | "疑似目标"标题，含 [模拟][GPR自动检测] 常驻标注 | §6.4 模拟来源标注 + §2.2 候选定义（阈值检测输出仅为候选，非最终结论），双重标注防止把自动候选当成真实目标 | `gprSuspectPanel` / 标题 QLabel（GprSuspectPanel） |
| GPR-S-02 | 候选表（QTableWidget） | 列：ID / 道索引 / 深度(m) / 幅度；选中行高亮并联动剖面标记 | 阈值检测存在漏检与过检两类失效（§5.4 预研实测），必须给出候选全表供逐一复核；ID 与剖面标记一一对应 | `gprSuspectTable`（GprSuspectPanel） |
| GPR-S-03 | [确认目标]（QPushButton） | 将选中候选经 SimulationWorkflow 注入统一目标表（含模拟标注与 GPR 来源元数据），态势页目标表/地图/日志联动，状态栏提示"目标 N 已确认（模拟）" | REQ-006 统一目标注入链路的入口；确认是人工判断动作，自动检测无权直接写目标表（§5.3 状态所有权：面板只发意图信号） | `gprConfirmButton` → `confirmRequested(suspectId)` |
| GPR-S-04 | [标记误报]（QPushButton） | 将选中候选从候选表移除（仅本页视图，不改底层数据） | 过检候选需要出口，保持候选表只剩待复核项；操作只影响展示层，符合"样例数据只读"边界 | `gprFalsePositiveButton` → `falsePositiveRequested(suspectId)` |

状态规则：检测列表为空或无选中行时，GPR-S-03/04 保持禁用（§6.3 禁用态可见）；加载新通道时候选表整体刷新并清空选中。

## 6 应用壳复用控件

| 控件 | 说明 | 参照 |
|---|---|---|
| 左侧导航 | 雷达页为 REQ-012 增量导航项（QStackedWidget index 3，位于处置决策之后） | [application-shell.md](../application-shell.md) §2.1 |
| 顶部菜单栏 | 文件/视图/帮助，页面无关 | [application-shell.md](../application-shell.md) §2.2 |
| 底部状态栏 | 通道加载完成后显示"GPR：检测到 N 个疑似目标（模拟数据，自动检测）"；确认目标后显示"目标 N 已确认（模拟）" | [application-shell.md](../application-shell.md) §2.3 |

## 7 页面五态

| 状态 | 触发 | 表现 |
|---|---|---|
| 正常 | 通道加载并检测完成 | 剖面 + 候选标记 + 候选表可用，回放条可用 |
| 加载 | 切通道/切频段后 worker 运行中 | loading 状态页，工具栏与回放条禁用 |
| 空 | 样例目录无对应通道数据 | empty 状态页 + 原因文案，候选表清空禁用 |
| 异常 | 解析失败/t0 估计失败/检测失败 | error 状态页显示具体原因（§6.3 错误可见），不静默回退 |
| 禁用 | 候选表无候选或无选中 | [确认目标]/[标记误报] 禁用但可见（§6.3） |

## 8 交互流程

主流程（gpr-display.md §3.1，即 §7.4 人工走查路径）：

1. 打开雷达页 → 默认加载 H 频段通道 1，回放就绪
2. 后台解析 + t0 估计 + 阈值检测完成 → 剖面显示，候选标记叠加，候选表刷新，状态栏显示检测统计
3. 点击 [播放] → 游标按倍速推进，位置标签实时更新
4. 调整增益/对比度/缩放 → 查看剖面细节
5. 切换通道 → 重复 2–4，对照邻近剖面
6. 选中候选 → [确认目标] → 统一目标注入，态势页联动
7. 选中候选 → [标记误报] → 从候选表移除
8. 态势页目标表/地图/日志中可见已确认目标

## 9 设计依据专题

1. **为什么播放前候选已标好**：检测是加载时一次性离线批量处理（校验 + t0 + 阈值检测，QtConcurrent worker），不是随播放逐道实时检测；播放纯为展示叙事（§2.2）。
2. **为什么自动检测不直接生成目标**：阈值检测是占位算法，存在漏检与过检两类失效（§5.4），其输出定义就是"候选"；确认/误报的人机分工是 REQ-012 的核心验收点。
3. **为什么 H/L 近重复仍保留频段切换**：相关系数 0.953–0.981 是预研实测结论，保留切换用于对照验证该结论与后续双频段融合扩展。
4. **为什么 AGC 为默认增益**：§5.4 实测目视最有效；固定白化与检测管线同族便于对照；原始模式用于排查增益伪影。
5. **为什么对比度加 120ms 防抖**：滑块拖动会连续触发重建，全幅面 QImage 重建是重操作；防抖把拖动洪泛合并为单次重建（冻结修复的一部分），避免阻塞事件循环。
6. **为什么确认/误报只在选中时可用**：§6.3 禁用态可见——无选中时点击无意义，禁用并保持在场比点击后报错更可预期。
7. **为什么 [模拟] 双标注常驻**：§6.4——模拟数据与真实设备状态必须在 UI 上可区分，标注随控件常驻、不可关闭。
8. **为什么面板只发意图信号**：§5.3 状态所有权——目标注入由 GprView/MainWindow 经 SimulationWorkflow 统一处理，面板不持有目标状态，避免多源写入。

## 10 页面专属边界

- 样例数据目录只读挂载；页面不写回任何采集数据文件。
- 禁止实现真实设备控制、实时采集接入与外部通信命令（REQ-012 §4 非目标）。
- [确认目标] 只写统一目标表（模拟标注），不触发任何处置/安全关键动作。
- [标记误报] 只改本页展示层，不改底层数据与检测产物。
- 所有 GPR 控件 objectName `gpr*` 前缀为 UI 契约测试（tests/gpr_view_test.cpp）锁定，改名需同步测试。

## 11 CURRENT 映射

| ID | objectName | 源文件 |
|---|---|---|
| GPR-T-01 | `gprBandCombo` | GprView.cpp（setupUi） |
| GPR-T-02 | `gprChannelCombo` | GprView.cpp |
| GPR-T-03 | `gprGainModeCombo` | GprView.cpp → GprBScanWidget::GainMode |
| GPR-T-04 | `gprContrastSlider` | GprView.cpp → GprBScanWidget::setContrast（120ms 防抖） |
| GPR-T-05 | `gprZoomSlider` | GprView.cpp → GprBScanWidget::setZoom |
| GPR-T-06 | `gprSimTagLabel` | GprView.cpp |
| GPR-C-01/02/03 | （GprBScanWidget 绘制层，无独立 objectName） | GprBScanWidget.cpp（paintEvent/rebuildImage/setCursorTrace/setSuspects） |
| GPR-C-04 | （QStackedLayout 状态页） | GprView.cpp（m_stack） |
| GPR-P-01 | `gprPlayPauseButton` | GprView.cpp（onPlayPauseClicked/m_playbackTimer 50ms） |
| GPR-P-02 | `gprSpeedCombo` | GprView.cpp（m_speed = 1/2/4） |
| GPR-P-03 | `gprProgressSlider` | GprView.cpp（tracking，步进 10 道） |
| GPR-P-04 | `gprPositionLabel` | GprView.cpp（updatePositionLabel） |
| GPR-S-01/02/03/04 | `gprSuspectPanel` / `gprSuspectTable` / `gprConfirmButton` / `gprFalsePositiveButton` | GprSuspectPanel.cpp |

## 12 ID 索引

`GPR-T-01` `GPR-T-02` `GPR-T-03` `GPR-T-04` `GPR-T-05` `GPR-T-06` ｜ `GPR-C-01` `GPR-C-02` `GPR-C-03` `GPR-C-04` ｜ `GPR-P-01` `GPR-P-02` `GPR-P-03` `GPR-P-04` ｜ `GPR-S-01` `GPR-S-02` `GPR-S-03` `GPR-S-04`
