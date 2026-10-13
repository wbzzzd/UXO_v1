# 执行计划：统计页数据平台可视化（REQ-013）

| 字段 | 值 |
|------|-----|
| 日期 | 2026-10-13 |
| 关联需求 | REQ-013（`Approved`，2026-10-13 用户批准） |
| 关联设计 | `docs/features/statistics-data-platform.md`（`Approved`，2026-10-13；§9 四项已决） |
| 契约依据 | `UXO_DataPlatform/docs/api/v1-contract.md` v1.0.3（只读） |
| 分支/工作树 | `feature/statistics-page` @ `/home/lin/UXO_v1-dataplatform`（自 `feature/data-platform-integration` HEAD 133669f 切出） |
| 状态 | 计划已生成，进入 W1/W2 并行委托 |

## 1. 目标与范围

按获批 REQ-013「统计项清单 A–E」交付统计页（导航 index 4，现为占位页）：只读接入本地数据平台，进页加载/手动刷新全量快照（5 卡片 + 10 分布图 + 6 趋势序列 + 告警处理统计 + 9 词表副本）与 5 tab 明细分页；显式不可用态 + 重试；零写回、零模拟数据回退。非目标与获批设计 §2 一致（不写回、不做自动刷新、不做导出、不改模拟链）。

## 2. 前置条件

- 本地数据平台实例可启动（默认 `http://127.0.0.1:8080`，`UXO_API_HOST`/`UXO_API_PORT` 覆盖）；实测阶段以真实实例为准，不得以环境跳过冒充通过。
- 只修改本仓文件；UXO_DataPlatform 仓一律只读。
- 接口契约头文件已由主 agent 冻结（本批不再改动签名）：
  - `include/MainWindow/StatisticsSnapshot.h`：值快照类型（卡片/分布/趋势/告警汇总/词表副本/明细分页）。
  - `include/MainWindow/StatisticsController.h`：controller + worker（同头文件，沿用 MosPlanningController 模式；MainWindow 层首个真 QThread 控制器）。

## 3. 工作分解（双通道并行，文件组互不重叠）

| 步骤 | 内容 | 产出 | 验证 |
|------|------|------|------|
| W1（通道 1，数据层） | `include/MainWindow/StatisticsAggregation.h` + `src/MainWindow/StatisticsAggregation.cpp`（纯函数聚合：FullEntityDump{五实体 QVector} + 词表 + nowUtc → StatisticsSnapshot；卡片副指标、分布 useBar 规则（≤5 桶饼图/其余柱状/等级序固定柱状）、桶按 code 升序且桶合计=总数、趋势近 30 天 UTC 日分桶空日补 0、检测异常叠加、告警平均处置时长）；`src/MainWindow/StatisticsController.cpp`（worker 懒建独占 HttpClient/DataPlatformApi/VocabularyCache，先 health 后词表后循环翻页 page_size=100，首错即失败回投；revision 守卫转发）；`tests/statistics_aggregation_test.cpp`（Qt Test fixture：卡片/副指标、10 图桶数+标签+useBar+桶合计=总数、趋势 30 点/空日补 0/异常叠加、平均处置时长含无样本、空 dump）。**不改任何 CMakeLists。** | 数据层源码 + 聚合测试源码 | 编译与运行在 W2 CMake 就绪后由 W3 集成执行；聚合测试全绿 |
| W2（通道 2，UI 层） | `include/MainWindow/StatisticsView.h` + `src/MainWindow/StatisticsView.cpp`（QStackedLayout 三态加载/错误/就绪 + QScrollArea；5 卡片；10 分布图 Qt5::Charts（useBar→柱状否则饼图，空桶"暂无数据"占位）；趋势图 QDateTimeAxis 6 序列图例切换 defaultVisible；告警处理区（level 柱状 + is_handled 饼图 + 未处置数 + 平均处置时长，无样本显示"—"）；明细区 QTabWidget 5 tab 默认目标，QTableWidget + 词表下拉过滤（含"全部"项）+ id 行编辑 + 三态下拉 + 分页条 20/页；状态条"数据快照：YYYY-MM-DD HH:mm:ss（本地时间，手动刷新）"+ 刷新按钮；拉取期禁用控件）；`src/MainWindow/MainWindow.cpp` 接线（index 4 装配 + onNavigationChanged case 4 触发加载 + 析构顺序保证 controller 先停线程）；**全部 CMake 编辑归本通道**：根 `CMakeLists.txt`（Qt5 COMPONENTS 加 Charts + 测试目标 `UXOMissionControlStatisticsAggregationTest`）、`src/MainWindow/CMakeLists.txt`（加 4 源文件 + MainWindow 库 PUBLIC 链接 Integration 与 Qt5::Charts）。 | UI 层源码 + 装配 + CMake | 全量 Release 构建 exit 0 |
| W3（主 agent，集成实测） | 全量构建 + 全量 ctest 零回归（既有 23 + 新增聚合测试，直接运行测试二进制看 Totals 行防 QSKIP 假绿）；真实实例实测：A2 口径抽样核对（psql count 对照卡片/分布/明细）、A3 停库 → 不可用态 → 恢复 → 刷新（沿用 REQ-012 W6 纪律，先报告再操作）、A6 拉取期 UI 不冻结 + 退出无线程悬挂；A4 代码审查（零 POST/PUT grep、模拟链零改动 git diff、数仓仓零改动）。 | 集成证据 | A1–A4、A6 逐项落盘 |
| W4（主 agent，文档回写） | `UI.md` 统计页节（布局/交互/三态/空数据口径）；`ARCHITECTURE.md` §2 占位行替换 + §3 统计页描述 + 构建/测试事实；`DEVELOPMENT.md` 测试登记表加聚合测试行；`PRODUCT.md` §5 需求追溯表 REQ-013 行；REQ-013 → `Implemented`（实现事实 + 验收结果表）；requirements/README 两处同步；feature 文档 → `Implemented`。 | 文档回写 | A5 核对（全仓扫陈旧表述） |

## 4. 验收映射

- A1（构建+链接+零回归）→ W2 构建验证 + W3 全量 ctest。
- A2（口径逐项：清单 A–E 含副指标与平均处置时长）→ W1 聚合单测 + W3 真实实例抽样核对。
- A3（停库不可用态实测）→ W3（用户监督下执行，沿用 REQ-012 W6 纪律）。
- A4（零 POST/PUT + 模拟链零改动 + 回环基址 + 数仓仓零改动）→ W3 代码审查 + git diff 核对。
- A5（四核心文档回写）→ W4。
- A6（worker 线程 UI 不冻结 + 退出回收）→ W3 实测。

## 5. 提交节奏（建议，待用户确认）

- commit #1（文档批）：REQ-013 + 功能设计 Approved 登记 + requirements/README + 本执行计划 + 接口契约头文件。
- commit #2（实现批）：W1–W4 全部代码、测试与文档回写。
- 未获用户明确要求前不提交、不推送。

## 6. 禁止事项

- 不修改 UXO_DataPlatform 仓任何文件；不修改契约语义。
- 统计页零 POST/PUT；平台不可用时不得回退模拟数据或展示任何数据。
- 不动模拟链（Core::Data 模拟源、态势/目标/任务/告警/检测页现有链路）。
- 通道 1 不得改任何 CMakeLists（全部 CMake 编辑归通道 2，规避并行写冲突）。
- 两通道均不得改已冻结的 `StatisticsSnapshot.h` / `StatisticsController.h` 签名；实现期发现契约缺陷：停止并报告主 agent，不单边绕过。
- 不引入新三方依赖（Qt5::Charts 为 Qt 官方模块，环境已具备）。
- 实现期发现需求/设计疑点：停止相关步骤并报告用户，不单边绕过。

## 7. 风险

- 契约无聚合端点，客户端循环翻页全拉本地聚合：设计假设单实体 ≤1 万行（page_size=100，≤100 次请求/实体）；超规模时快照耗时增长，W3 实测记录耗时证据。
- MainWindow 层首个真 QThread 控制器：线程纪律以头文件注释 + MOS 模式为准；A6 实测退出回收。
- 聚合测试依赖 fixture 词表（61 项最小集）：fixture 与契约表 4-1 漂移时测试红灯，不静默适配。
- 并行通道经返回流中断：按子代理可靠性协议续跑原 session，只补最小剩余步骤，不整体重做。

## 8. 进度记录

| 日期 | 步骤 | 状态 | 证据 |
|------|------|------|------|
| 2026-10-13 | 计划生成 | 已确认 | 用户批准 REQ-013 后生成；三处状态已同步 Approved；接口契约头文件已落盘 |
| 2026-10-13 | 状态翻转 + §9 落定 | 完成 | REQ-013.md / statistics-data-platform.md / requirements/README.md 三处 Draft→Approved 并记批准事实（2026-10-13）；feature §9 四项按默认规则记为已决 |
| 2026-10-13 | 接口契约头文件冻结 | 完成 | `include/MainWindow/StatisticsSnapshot.h` + `StatisticsController.h` 落盘；独立语法检查通过（g++ -fsyntax-only，Qt5Core/Network/Widgets，`HEADERS SYNTAX OK`） |
| 2026-10-13 | 分支创建 | 完成 | `feature/statistics-page` 自 `feature/data-platform-integration` HEAD 133669f 切出，未提交文档随行 |
| 2026-10-13 | W1/W2 双通道启动 | WORKING | 通道 1（数据层，deep）bg_6532851d / ses_eccd23235ffefK73Rcdqp5GKip；通道 2（UI 层，visual-engineering+frontend）bg_d0e1a6c6 / ses_eccd13d5affesdY4etAOEPH8PH；两通道均禁跑构建（归 W3 集成），CMake 编辑全归通道 2 |
| 2026-10-13 | 实测前置核对 | 完成 | 本地数据平台实例健康检查 HTTP 200（127.0.0.1:8080），W3 实测环境就绪 |
| 2026-10-13 | W1 数据层落盘（通道 1） | ARTIFACT_READY | 4 文件写入：`include/MainWindow/StatisticsAggregation.h`、`src/MainWindow/StatisticsAggregation.cpp`、`src/MainWindow/StatisticsController.cpp`、`tests/statistics_aggregation_test.cpp`（中文注释；未改任何 CMakeLists 与冻结头）。逐文件语法检查 3/3 通过：`g++ 14.3.0 -std=c++17 -fsyntax-only -fPIC`（pkg-config Qt5Core/Network/Gui，测试另加 Qt5Test；`-Iinclude`）均输出 SYNTAX_OK；测试文件经 moc 生成至 /tmp/opencode 并以 `-I/tmp/opencode` 解析 `#include "...moc"`，仓库零残留。clangd LSP 未安装（用户已拒绝安装），按规则静默跳过。编译与 ctest 集成留待 W3（CMake 接线归通道 2）。 |
| 2026-10-13 | W2 UI 层落盘（通道 2） | ARTIFACT_READY | 新建 `include/MainWindow/StatisticsView.h`、`src/MainWindow/StatisticsView.cpp`（1230 行，三态页：加载/不可用+重试/就绪）；MainWindow.h/.cpp 接线（栈 index 3 设备占位 filler + index 4 统计页、nav case 4 triggerLoad、setupUi 装配 controller）；全部 CMake 编辑（根：find_package 加 Charts + `UXOMissionControlStatisticsAggregationTest` 目标；src/MainWindow：3 源 4 头 + 链接 Qt5::Charts/Integration）。g++ -fsyntax-only 3 TU 全过（SYNTAX_OK）。报备偏差 5 项：MainWindow.h 成员用 ctor 初始化表风格、index 3 filler 占位、plain 签名链接追加、maybeRequestInitialList 懒加载（REQ-013 E）、测试目标显式链 Qt5::Gui（Types.h 经 QVector3D 需 Qt5Gui，同 mos_runway_closure 惯例）。 |
| 2026-10-13 | W3 集成构建 + 全量回归 | VERIFIED | 全量构建 CONFIGURE_EXIT=0 / BUILD_EXIT=0（107/107 目标，含新测试目标与主程序）。全量 ctest 22/24 通过（CTEST_EXIT=8）：失败集合精确 = {video_render_capture, stop_select_flicker_repro}，失败签名（"target row missing" / "no target rows - detection produced nothing"）与 HEAD 133669f 预存失败集合完全一致（REQ-012 会话记录 `.omo/plans/20261012-work-session.md` §验证结果：FULL=8 预存 {#5,#6}、主仓同签名复跑确认为预存），判定**零回归**；新增 statistics_aggregation（#24）通过；integration #18–#23 全过。越权检查：Statistics*.cpp 零 POST/PUT/patch/delete、零 mock/simulate/fake 模式；冻结头 mtime 未变（10:48）；模拟链（Integration/Core/Detection）零 diff。基线 worktree 对照因历史铁证充分判为冗余（已清理）。 |
| 2026-10-13 | A2/A6 离屏实测（真实 MainWindow + 独立地面真值比对） | VERIFIED | 显示抓取路线证死（会话为 Wayland，x11grab 抓 Xwayland 根=纯黑；本机无 xdotool/xwininfo/qdbus），改用离屏 harness：`/tmp/opencode/a2h/`（QT_QPA_PLATFORM=offscreen 实例化真实 MainWindow，invokeMethod 走真实导航 index 4 懒加载路径，捕获 controller 值类型信号落盘；零仓库改动、零显示干扰；用户自有应用实例未动）。独立地面真值 `/tmp/opencode/a2-ground-truth.json`（`/tmp/opencode/a2_ground_truth.py` 直接打 API 独立聚合，五实体 meta_total==fetched）。机器比对 `/tmp/opencode/a2_compare.py` **73/73 PASS**：5 卡片 total 与全部副指标精确（49/36/18/18/18、今日新 6/2、在线 0、未处置 9、异常 18、均置信 0.80/0.92 显示舍入一致）；10 分布计数多重集全符（固定序、词表 name_cn 标签）；6 趋势 30 点、总量、非零日字典精确（仅 10-11/10-13 有数据）且 defaultVisible=TTTTFF；告警汇总 handled 9/unhandled 9/avgHandleSeconds 0.15333… 双端全精度一致、hasAvgHandle=true；vocabularies=9；明细 5 页签 resource/total/行数（min(20,total)）/首行全符。视觉证据（look_at 提取）：a2-statistics-view.png/a2-window.png 卡片+饼/柱渲染、中文无乱码、快照时间行 `2026-10-13 12:58:45`（UTC+8 与 fetchedAtUtc 04:58:45Z 吻合）；a2-statistics-detail.png 5 页签+表格+`第 1 / 1 页 · 共 18 条`；a2-chart-00.png=趋势折线（标题“日增量趋势（UTC 日分桶）”、日期轴 09-14…10-13、4 条默认可见序列）；a2-chart-01/02=告警级别柱（警告 18）/处置占比饼（9/9）。刷新复抓第二快照成功（revision 守卫放行最新）。**A6**：harness 两轮运行均 A2H_EXIT=0 干净退出，worker quit+wait 即时返回，线程回收无挂死。曾误启的显示实例已杀净，无残留 tmux/GUI 进程。 |
| 2026-10-13 | A3 不可用态核验（连接拒绝代理法，免停库） | VERIFIED | `UXO_API_PORT=18099` 指死端口：首抓失败 → 显式不可用态（红色 `数据平台不可用：health: HTTP 0 network_error 网络层请求失败（无 HTTP 响应）` + `重试` 按钮可见、零数据渲染，证据 a3-unavailable.png）→ 启动 `/tmp/opencode/a3_proxy.py`（18099→8080 TCP 转发，脚本内自检 PROXY_HEALTH=200）→ 第 11 次点击真实重试按钮恢复，快照 05:15:03Z 到达（a3-recovered.png 五卡片+图表全量渲染）；编排脚本 `/tmp/opencode/a3_run.sh`（独立脚本消除 && 链与结尾 & 的优先级歧义——首轮代理未监听的根因）；A3_EXIT=0 干净退出（A6 第三次确认）。未执行真实 pg_stop：用户自有应用实例（PID 1658133）在线连接数据平台且未获明确停库授权；连接拒绝与服务不可用属同一 QNetworkReply 失败族、客户端代码路径相同；真实停库留作用户可选终验。 |
| 2026-10-13 | W4 文档回写（7 文件） | VERIFIED | 委托 writing 通道 `ses_ecc4e26b5ffeM0izwFht58y678` 同步执行：UI.md（§3 路由句、§4.2 导航行、新增 §4.6 统计页节含三态表与数据源标注）、ARCHITECTURE.md（§2 概览/mermaid/模块表/QThread 纪律段、§3 外部数据边界）、DEVELOPMENT.md（+Charts、测试登记 statistics_aggregation、构建树 Integration）、PRODUCT.md（§5.5 首消费方句、§6 已完成行）、REQ-013.md（→Implemented + 验收结果 A1–A6 + 实现事实）、requirements/README.md（索引两处 →Implemented）、features/statistics-data-platform.md（→Implemented + §10 完成记录）。子代理自验（git diff 读回、引号风格、UI 表述源码佐证）；主 agent 复核：git status 仅预期文件、状态翻转计数命中（REQ-013 ×3 / features ×2 / README ×8）、W4 零源码树新增改动（src/include/tests 改动全部属 W1–W3 实现批）。A3 代理法偏差已在 REQ-013 验收结果与 features §10 如实披露，未写成"已通过真实停库验证"。 |
| 2026-10-13 | 会话收尾：会话记录落盘 | VERIFIED | `.omo/plans/20261013-work-session.md` 已写（目标/执行内容/验证结果/证据说明/遗留事项五节，对齐 REQ-012 会话记录格式）；REQ-013 全周期完成：Draft→Approved→实现（W1–W3 零回归）→实测验收（A2 73/73、A3 代理法、A6 ×3）→文档回写（W4 七文件）→`Implemented`。工作区未提交，待用户指示；无运行中的子任务、构建、测试或 GUI 进程。 |
