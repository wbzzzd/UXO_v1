# 统计页数据平台可视化

状态：Implemented（2026-10-13 验收；2026-10-13 用户批准）
关联产品需求：REQ-013
关联版本：待确认

## 1. 问题与目标

导航栏"统计"（index 4）是占位项：页面栈无对应页面，点击回落态势工作区。REQ-012 交付的 Integration 适配器库已经契约一致性验证，但只被测试目标链接，运行中的 UXOMissionControl 与数据平台零交互。

目标：新建统计页，作为适配器首个应用侧消费方，让指挥席用户直观查看本地数据平台中的真实数据统计（2026-10-12 用户确认范围：五类统计全做、只读、进页加载+手动刷新、显式不可用态）。

## 2. 范围与非目标

**范围**：

- 新建统计页 `StatisticsView`（MainWindow 层）：加入页面栈，导航 index 4 路由到此页（不再回落态势工作区）。
- 页面内容（统计项逐项定义——做完后用户可见的全部信息——见 REQ-013「统计项清单」，为权威清单；本节为呈现与口径设计）：
  - 概览卡片区：5 卡片；主指标 = 对应实体列表 `meta.total`；副指标为全量快照本地推导值（口径见 §5），一律不依赖词表项语义假设；
  - 分布图区：10 张（目标 3：类型/状态/威胁等级；设备 2：类型/状态；任务 2：类型/状态；告警 1：级别；检测记录 2：模型类/复核状态）；桶标签经 VocabularyCache 取 `name_cn`；桶合计 = 对应实体总数（实现内置一致性断言）；`alarms.type` 为自由文本（词表待定），不做告警类型分布；
  - 趋势区：近 30 天 UTC 日分桶折线；默认三线 = 目标探测数（`detect_time`）/ 检测记录数（`analyzed_at`，叠加其中异常数）/ 告警数（`create_time`）；设备/任务登记数（`create_time`）可切换、默认隐藏；
  - 告警处理区：级别分布柱状 + 处置占比饼图（`is_handled`）+ 未处置数 + 平均处置时长（已处置且 `handle_time` 非空告警的 `handle_time − create_time` 均值）；
  - 明细区：五实体 tab + 契约 §7 起步过滤集下拉（词表显示名经 VocabularyCache）+ 服务端分页表格（页大小默认 20）。
- `StatisticsController` + worker 线程：拉取与聚合在后台线程执行，结果以值类型快照经排队信号回投 UI 线程。
- 页面三态：加载中 / 就绪 / 不可用（含重试按钮）；不可用态不展示任何数据。
- `UXOMissionControl` 目标新增链接 `Integration` 与 `Qt5::Charts`；核心文档回写（见 §8）。

**非目标**：

- 任何写操作：不调用 POST/PUT（契约 v1 无 DELETE）；页面不提供任何编辑、确认、处置入口。
- 服务端聚合/统计端点（属契约变更，走双仓流程另立项）；录制 fixture 回归。
- 定时自动刷新；端点设置界面（沿用 `UXO_API_HOST`/`UXO_API_PORT` 环境变量）。
- 态势/探测/决策三页与模拟链（SimulationWorkflow/DroneTelemetrySimulator/DetectionEngine）任何改动；设备（index 3）、配置（index 5）占位页不在本批。
- 历史回放、导出、打印、自定义报表。

## 3. 用户流程

1. 用户点击导航"统计" → 页面立即进入加载态（进度指示），后台执行：健康检查 → 五实体全量拉取（`page_size=100` 循环翻页至累计 = `meta.total`）→ 本地聚合 → 回投快照。
2. 加载成功 → 就绪态：概览卡片、分布图、趋势图、告警统计、明细列表展示本次快照；页顶显示数据快照时间。
3. 健康检查失败或拉取失败 → 不可用态：显式"数据平台不可用"占位（含失败原因摘要与"重试"按钮），不展示任何数据。
4. 用户点击"刷新" → 重复 1–3；加载期间用户可自由切换页面（结果回投时若已离开则静默丢弃，不阻塞）。

## 4. 需求增量

关联 REQ-013（统计页数据平台可视化）。无其他需求增量。

## 5. 架构增量

- 新增 MainWindow 层 `StatisticsView`（页面）与 `StatisticsController`（拥有 worker 线程）；依赖方向 `MainWindow → Integration → Core` 不变，Integration 库零改动。
- 线程纪律：`DataPlatformApi`/`HttpClient` 含 `QNetworkAccessManager`（线程亲和），必须在 worker 线程内构造、使用、销毁；UI 线程只接收不可变结果快照（值类型，Qt 排队连接）。
- 拉取协议：health（§3）→ 五实体分页循环（§7，`page_size=100`）→ 词表经 `VocabularyCache`（维度显示名映射）；全部只读 GET。
- 聚合口径：总数 = `meta.total`；分布 = 全量行按词表维度分桶（桶合计须等于总数，实现内置一致性断言）；趋势 = 按 UTC 日分桶近 30 天（含当日，空日补 0）；告警占比 = is_handled 布尔分桶；卡片副指标与平均处置时长按 REQ-013「统计项清单」口径本地推导（目标今日新探测/平均置信度、设备近 24h 在线记录数、任务今日新建、告警未处置数、检测记录异常数/平均置信度；平均处置时长 = 已处置且 `handle_time` 非空告警的 `handle_time − create_time` 均值）；副指标不依赖任何词表项语义假设。
- 数据规模假设：本地实例单实体 ≤ 约 1 万行（100 页）时全量拉取为秒级；超出需契约聚合端点（另立项，见 REQ-013 备注）。
- 明细列表直接消费分页 API（不进全量快照），过滤器映射契约 §7 起步过滤集，页大小默认 20。
- 新增依赖：`Qt5::Charts`（开发环境已具备 `Qt5Charts` cmake 模块）；`UXOMissionControl` 链接 `Integration`。

## 6. UI 增量

`UI.md` 新增"统计页"契约节：

- 信息架构：导航第 5 项"统计"获得真实页面；页面分区 = 顶部状态条（快照时间+刷新按钮）→ 概览卡片区 → 图表区（分布/趋势/告警处理）→ 明细区（实体 tab+过滤+分页表）。
- 页面状态机：加载中（进度指示、内容区骨架）/ 就绪（快照展示）/ 不可用（占位+重试，零数据展示）。
- 设计系统：沿用 GlobalStyle 设计 token 与属性化 QSS 约定（containerBg/btnVariant/labelRole 等）；图表配色用语义色板，威胁等级等维度颜色与既有目标标注一致。
- 真实/模拟区分：本页只展示数据平台真实数据，不涉及 [模拟] 角标；不可用态文案明确指向数据平台连接，不与模拟演示混淆。

## 7. 验收标准

与 REQ-013 验收标准一致：

- A1 构建：Release 构建 exit 0；`UXOMissionControl` 链接 `Integration` + `Qt5::Charts`；既有 ctest 基线零回归。
- A2 功能正确性（对本地真实实例）：REQ-013 验收标准 A2 各口径逐项核对通过（总数=meta.total、分布桶合计=总数、趋势按日分桶、告警占比、明细过滤/分页与契约 §7 一致）。
- A3 不可用态：监督下停库实测：进页显式不可用+重试、零数据展示；恢复后重试加载成功。
- A4 安全边界：代码审查确认页面交互零 POST/PUT 调用；模拟链零改动；默认回环；UXO_DataPlatform 仓零改动。
- A5 文档回写：REQ-013 状态、ARCHITECTURE.md、UI.md、DEVELOPMENT.md 同步。
- A6 线程与响应：加载期间 UI 不冻结（可切换页面、控件响应）；应用退出时 worker 线程正确回收。

## 8. 对核心文档的影响

- `PRODUCT.md`：§5 外部接入登记追加 REQ-013 行（批准时）。
- `ARCHITECTURE.md`：§2 模块图新增 StatisticsView/StatisticsController 与 MainWindow→Integration 依赖；§3"适配器库已验证未接入装配"行更新为首个消费方事实；补 worker 线程纪律说明。
- `UI.md`：新增统计页契约节（见 §6）；导航契约中"统计"由占位改实页。
- `DEVELOPMENT.md`：测试登记表新增统计页相关套件行；构建依赖补 Qt5::Charts 说明。
- `docs/requirements/README.md`：REQ-013 登记与状态流转（本次登记 `Draft`）。

## 9. 待确认事项

全部已决（2026-10-13 批准时采用默认规则）：

- 分布图图型映射：桶数 ≤5 用饼图、其余用柱状；等级序维度（threat_level、alarm_level）固定柱状。
- 明细区默认 tab：目标；行序为服务端默认序（列表端点无排序参数）。
- 快照时间呈现：页顶状态条"数据快照：YYYY-MM-DD HH:mm:ss（本地时间，手动刷新）"。
- 空数据口径：0 行实体的图表显示"暂无数据"占位，不隐藏（保持布局稳定）。

## 10. 完成记录

2026-10-13 实现、实测与文档回写完成（REQ-013 状态同步翻转 `Implemented`；逐项证据见执行计划 `.omo/plans/20261013-statistics-page-plan.md` §8 进度记录）：

- 交付：`StatisticsSnapshot.h`、`StatisticsController.h`+`.cpp`、`StatisticsAggregation.h`+`.cpp`、`StatisticsView.h`+`.cpp`（MainWindow 库 3 源 4 头），MainWindow 接线（页面栈 index 4、导航 case 4 进页加载、明细页签懒加载），根 CMake（`find_package` 增 `Charts`、新测试目标 `UXOMissionControlStatisticsAggregationTest`）与 `src/MainWindow/CMakeLists.txt`（链接 `Qt5::Charts` 与 `Integration`）。
- A1 构建：全量 Release 构建 107/107 目标退出码 0；全量 ctest 22/24，失败集合精确等于 HEAD 133669f 预存失败 {`video_render_capture`、`stop_select_flicker_repro`}，零回归；新增 `statistics_aggregation`（#24）与 integration #18–#23 全过。
- A2 功能正确性：独立地面真值比对 73/73 PASS——离屏 harness 实例化真实 MainWindow 走真实导航路径捕获快照，与直打 API 自行聚合的地面真值逐项核对（卡片主/副指标、10 分布、6 趋势、告警汇总、明细 5 页签全精度一致）；截图确认图表渲染与中文显示无乱码。
- A3 不可用态：以连接拒绝代理法验证（`UXO_API_PORT` 指向死端口）——显式不可用态、重试按钮、零数据渲染，代理接通后重试恢复；未执行真实停库，留作可选终验（用户监督下）。
- A6 线程与响应：harness 三次运行均干净退出，worker `quit`+`wait` 即时回收无挂死。
- 核心文档回写：`ARCHITECTURE.md`（§2/§3）、`UI.md`（§3/§4.2/新增 §4.6）、`DEVELOPMENT.md`（§2/§4/§5）、`PRODUCT.md`（§5.5/§6）、需求索引与 REQ-013 状态。
