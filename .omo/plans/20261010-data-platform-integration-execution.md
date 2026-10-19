# 执行计划：数据平台接入（REQ-012 客户端适配器）

| 字段 | 值 |
|------|-----|
| 日期 | 2026-10-10 |
| 关联需求 | REQ-012（`Approved`，2026-10-10） |
| 关联设计 | `docs/features/data-platform-integration.md`（`Approved`，2026-10-10） |
| 契约依据 | `UXO_DataPlatform/docs/api/v1-contract.md` v1.0.3（只读） |
| 分支/工作树 | `feature/data-platform-integration` @ `/home/lin/UXO_v1-dataplatform` |
| 状态 | W1–W7 全部完成，commit #2 待用户确认 |

## 1. 目标与范围

按获批设计交付 P0 纯适配器层：新增 `Integration` 静态库，完整实现契约 v1.0.3 语义，并以对本地数据平台实例的契约一致性测试为标准门禁。非目标与获批设计 §2 一致，不在此重复。

## 2. 前置条件

- 本地数据平台实例可启动（UXO_DataPlatform 仓：pgdata + 服务端，默认 `http://127.0.0.1:8080`）。实例未运行时契约测试登记为环境跳过，并在 `DEVELOPMENT.md` 记录启动前置；不得以跳过冒充通过。
- 只修改本仓（UXO_v1-dataplatform 工作树）文件；UXO_DataPlatform 仓一律只读。

## 3. 工作分解

| 步骤 | 内容 | 产出 | 验证 |
|------|------|------|------|
| W1 | CMake 骨架：`src/Integration/`、`include/Integration/`，静态库目标链接 `Core` + `Qt5::Network`，根 CMake 登记 | 可构建空库 | Release 构建退出 0，现有 ctest 基线不回归 |
| W2 | HTTP 基础设施：`QNetworkAccessManager` 客户端、基址解析（默认回环，`UXO_API_HOST`/`UXO_API_PORT` 覆盖）、统一信封与分页解析、契约错误模型映射（`field`+`code` 为准，400 聚合完整保留，不解析 `message`） | ApiClient 内部实现 | 错误路径测试覆盖 |
| W3 | 契约 JSON ⇄ `Core::Types.h` 双向映射：五实体、UUID 两形式、时间戳三形式、可空字段显式 null、整数严格性（`12.0`/`"12"` 拒绝）、battery_level 取整、result null↔0 语义、raw_result 必填 | 映射层 + 映射单测 | 映射单测全绿（含类型严格性负例） |
| W4 | 词表（9 表 61 项）与模型类映射（8 行）加载与只读缓存 | 词表/映射客户端 | 对本地实例形状与顺序断言通过 |
| W5 | 五实体单条/列表查询、创建（POST）、全量更新（PUT）+ 分页与起步过滤集参数构造 | 实体客户端方法 | 对本地实例 CRUD 往返通过 |
| W6 | 契约一致性测试套件（ctest 入口）：健康 200/503 两路径、词表、映射、CRUD 往返、分页（越界 400/空页 200）、过滤集全参数、409 duplicate_id、400 聚合错误、类型严格性；测试数据只写本地实例 | `tests/` 契约测试 | A2/A3 全绿，`DEVELOPMENT.md` 测试表登记 |
| W7 | 核心文档回写：`ARCHITECTURE.md` §2 模块表/§3 结构问题表、`DEVELOPMENT.md` §2/§4；REQ-012 实现事实与验收结果；功能文档状态 → `Implemented` | 文档回写 | A5 核对 |

## 4. 验收映射

- A1 → W1 验证 + 全量 Release 构建
- A2/A3 → W4/W5/W6 对本地实例执行结果（契约 §8 共 10 个错误码：9/10 已实况断言，internal_error 无可靠触发器由解析层覆盖）
- A4 → 代码审查核对：默认基址仅回环、无非回环硬编码、测试数据只写本地实例、模拟链数据不经上报通道
- A5 → W7

## 5. 提交节奏（2026-10-10 已批准）

- commit #1（本批）：REQ-012 + 功能设计 Approved 登记 + `PRODUCT.md` §5.5 登记行 + 本执行计划 + 契约评审记录。
- commit #2（实现批）：W1–W7 全部代码、测试与文档回写。

## 6. 禁止事项

- 不修改 UXO_DataPlatform 仓任何文件；不修改契约语义（变更走契约 §9.2 双仓流程）。
- 不动 UI、MainWindow 接线与模拟状态权威；模拟链数据不得经上报通道。
- 不引入 `Qt5::Sql`、不接生产库、不做非回环部署形态；不实现认证/授权。
- 不新增无调用方的抽象（Repository/Store/事件总线）。
- 实现期发现契约语义疑点：停止相关步骤并报告用户，不单边绕过。

## 7. 风险

- 本地实例未运行导致契约测试无法执行：按 §2 环境跳过登记，完成定义以真实实例全绿为准。
- `Types.h` 枚举与契约词表漂移：W3 映射单测锁定当前对齐事实（8 枚举值域、9 模型类）；漂移即红灯，不静默适配。

## 8. 进度记录

| 日期 | 步骤 | 状态 | 证据 |
|------|------|------|------|
| 2026-10-10 | 计划生成 | 已确认 | 用户批准进入实现 |
| 2026-10-10 | W1 CMake 骨架 | 完成 | Release 构建 exit 0 + libIntegration.a 在盘；基线回归判定：#5/#6 为 HEAD 预先存在失败（主仓当前源码复跑同签名失败），失败集合零差异 |
| 2026-10-10 | W2 HTTP 基础设施 | 完成 | TDD 闭环：RED（integration_api_error 真实槽位 0/5、integration_http_client 6 红 2 平凡绿）→ GREEN（两测试全绿）；全量套件 17/19 仅 #5/#6 预存失败，零回归。注意：契约 §8 实为 10 个错误码，A3"9 个"表述将在 W7 回写时更正 |
| 2026-10-11 | W3 RED（映射单测） | 完成 | `tests/integration_entity_mapping_test.cpp`（1078 行，10 槽位）落盘；RED 取证 CFG=0/BLD=0/CT=8：10 槽位全红且全部死于 stub `return false` 首个正向断言，无平凡绿；ctest 注册为 #20 `integration_entity_mapping`。证据 `/tmp/w3-red-{configure,build,ctest}.log`、`/tmp/w3-red-rc.txt` |
| 2026-10-11 | W3 GREEN（映射实现） | 完成 | `src/Integration/EntityMapping.cpp` 990 行：10 个公开映射函数 + 14 个匿名 namespace helper 全落位；GREEN 取证 BLD=0 / CT=0（#20 `integration_entity_mapping` 12/12 槽位全绿）/ FULL=8（全量 20 测试失败集合精确 = {#5,#6} 预先存在，零回归）。UUID 钉点以测试为准更正：仅接受 36 位连字符形（十六进制大小写/花括号/首尾空白容忍），32 位无连字符紧凑形拒收；`parseTimestamp` 数字偏移（±HH:MM）改手工解析 + `addSecs` 换算（Qt 5 `t` 格式不解析数字偏移）。证据 `/tmp/w3-green-{build,ctest,full}.log`、`/tmp/w3-green-rc.txt` |
| 2026-10-11 | W4 词表与模型类映射缓存 | 完成 | `include/Integration/VocabularyCache.h`（70 行）+ `src/Integration/VocabularyCache.cpp`（233 行）+ `tests/integration_vocabulary_cache_test.cpp`（246 行，6 槽位），ctest 注册 #21 `integration_vocabulary_cache`；GREEN 取证 BLD=0 / CT=0（#21 全绿，真实本地实例路径非环境跳过）/ FULL=8（全量 21 测试失败集合精确 = {#5,#6} 预先存在，零回归）。加载层钉形状与顺序（9 表行序 = 表 4-1、items code 严格递增、映射 model_class 严格递增 + 单射 + target_type 0–11），项数与行数由测试钉定（追加式正典前向兼容）。证据 `/tmp/w4-green-{build,ctest,full}.log`、`/tmp/w4-green-rc.txt` |
| 2026-10-11 | W5 五实体 CRUD+分页过滤 | 完成 | `include/Integration/DataPlatformApi.h`（132 行，21 方法 + HealthStatus/PageQuery/PagedResult/5 过滤结构）+ `src/Integration/DataPlatformApi.cpp`（333 行）+ `tests/integration_data_platform_api_test.cpp`（599 行，12 槽位），ctest 注册 #22 `integration_data_platform_api`；TDD 闭环：RED 取证 BLD=0/CT=8（12/12 槽位非平凡红，全死于 stub-false 首个断言）→ GREEN 取证 BLD=0 / CT=0 / FULL=8（全量失败集合精确 = {#5,#6} 预先存在，零回归）；铁证：直接运行测试二进制 Totals: 14 passed, 0 failed, 0 skipped（3463ms，真实实例路径非 QSKIP）。修复一轮：契约 §7 行 429 布尔过滤查询串仅接受 true/false 字面量，实现出串 0/1 被判 400 invalid_format → 新增 addBoolFilter（-1 未设 / 0→false / 1→true，行 44/282/314）。证据 `/tmp/w5-red-{build,ctest,full}.log`、`/tmp/w5-red-rc.txt`、`/tmp/w5-green-{build,ctest,full}.log`、`/tmp/w5-green-rc.txt` |
| 2026-10-12 | W6 契约一致性套件 | 完成 | `tests/integration_contract_test.cpp`（270 行，9 槽位），ctest 注册 #23 `integration_contract`；门禁取证 BLD=0 / CT=8 / FULL=8（全量 23 测试失败集合精确 = {#5,#6} 预先存在，零回归）；铁证：直接运行测试二进制 Totals: 11 passed, 0 failed, 0 skipped（737ms，真实实例非 QSKIP）。触发器钉点：invalid_enum=class_name 过滤非表 5-1 值（§7:429）、out_of_range=longitude=200（§8:481）、not_null=POST detection-records 缺 raw_result（§6.5）、invalid_format=路径 UUID 非法 + is_handled=0 + threat_level 充字符串/布尔（§1.3 类型严格性；12.0 字面量 Qt5 JSON 不可表达由服务端保障）、聚合=type=99+longitude=200 同体两错 ≥2 条（§8:473）、404 变体 field=name/空串（§8:494-495）、PUT 路径与体 id 错配（§6.0，v1.0.3 判定顺序）；非法载荷一律经 HttpClient 原始 get/post/put（类型化客户端结构性阻却非法类型/越界枚举），合法基线经 targetToJson 生成完整表示后翻转单字段。用户裁定 B 完成一次性破坏性实测（pg_ctl 停库约 66 秒，10:48:31–10:49:37）：健康 503 专属形状 `{"status":"unavailable","database":"unreachable"}` 与契约 §3:113 逐字一致、database_unavailable 读/写路径 503 信封断言通过、恢复复核 200 connected 首次尝试即通；附带确认校验先于 DB 访问（停库时不完整 body 仍 400 not_null 聚合）。错误码实况断言终态 9/10，internal_error(500) 无可靠触发器由 `integration_api_error` 解析层兜底。DEVELOPMENT.md 测试表登记 6 个集成套件并补实例依赖/QSKIP/破坏性实测说明段。证据 `/tmp/w6-gate-{build,ctest,full}.log`、`/tmp/w6-gate-rc.txt`、`/tmp/w6-contract-direct.log`、`/tmp/w6-destructive-evidence.log` |
| 2026-10-12 | W7 文档回写 | 完成 | ARCHITECTURE.md §2（mermaid 新增 Integration→Core 节点、构建目标表 Integration 行、Qt5::Network 使用句、17→23 测试目标、开篇"无外部集成"精确化）+ §3（Qt5::Sql 行剔除 Qt5::Network、外部数据边界行改为"适配器库已验证未接入装配"）；DEVELOPMENT.md §2 数据平台实例行（§4 测试表 6 套件+QSKIP/破坏性实测段已于 W6 落盘）；REQ-012 状态 → Implemented，验收结果 A1–A5 逐项证据与实现事实（适配器库/客户端纪律/测试 #18–#23/已知边界）填写，缺口段改"立项时"时态；requirements/README.md 两处状态同步；feature 文档状态 → Implemented 并清除 Draft 括注与 A3 行"9 个错误码"陈旧措辞。A5 核对：全仓扫五类陈旧模式，修正 3 处，判定 3 处为合法历史记录（PRODUCT.md §5.5 批准事实、feature §8 文档影响清单、REQ-012 缺口立项时引述） |
