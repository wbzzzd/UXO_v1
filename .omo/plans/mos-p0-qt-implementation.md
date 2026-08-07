# mos-p0-qt-implementation - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** A local-simulation MOS P0 planning workspace in the Qt client: deterministic runway geometry, progressive repair scenarios, an interactive decision page, and evidence-backed correctness, responsiveness, and visual checks.

**Why this approach:** Pure Core calculation and an application-owned session keep business state testable and separate from Qt painting; revisioned worker results protect the UI from stale updates while preserving the approved one-notification flow.

**What it will NOT do:** It will not control real devices, use real safety parameters, connect AI/databases/networks, or implement MOS P1/P2 and unrelated pages. JSON is a one-way synthetic fixture artifact, never runtime persistence or input.

**Effort:** XL
**Risk:** High - greenfield continuous-geometry algorithm plus strict performance and UI evidence gates in a dirty multi-worktree repository.
**Decisions to sanity-check:** Closed-set tangency with finite-double canonicalization, synthetic radius formulas, P0 all-tier weak display, removal of prototype delays, worker/revision ownership, and one-way `QSaveFile` export.

Execution status: approved and resumed by the user in the canonical MOS worktree. Full execution detail follows below.

---

> TL;DR (machine): XL/high-risk plan for pure MOS Core, revision-safe session, DecisionView P0 routing, QtTest correctness/UI evidence, and Release performance gates.

## Scope
### Must have
实现已批准 MOS P0（REQ-007）本地模拟闭环：

- `Core::MOS` 纯数据模型、完整 P0 输入包络校验、最大轴对齐空矩形计算、递进档位、合成修复估算、`mulberry32` 生成器和单向 fixture JSON 工件。
- 应用拥有的 MOS 内存会话：输入、参数、当前选择、派生结果和操作日志；合法有解、合法无解、非法请求和陈旧异步结果的原子语义。
- 决策页 P0：损毁目标选择、跑道 QPainter 视图、参数校验/重规划、已计算档位选择与弱显、生成器弹窗、P0 模拟标识和最小导航 index 2 路由。
- 正确性 oracle、QtTest/CTest、Release 性能、100 进程确定性、10ms heartbeat、内存、视觉几何和实际运行证据。
- 所有可交付事实在实现并验证后回写核心文档；`REQ-007`/功能文档只有在证据齐全后才改为 `Implemented`。
### Must NOT have (guardrails, anti-slop, scope boundaries)
- 不实现 MOS P1/P2、UXR、REQ-001..006、其他五个 TARGET 页面、通用 MainWindow 重构、真实传感器/AI/数据库/通信/设备控制、真实安全执行或真实业务参数。
- 不把 `TargetInfo`、`RunwayInfo`、`DecisionSuggestionPanel` 或 `SimulationWorkflow` 改造成 MOS 领域模型；MOS 使用自己的 Core 类型和会话。
- 不把 JSON 导出当作运行时持久化、输入、导入、回灌或外部集成；不保存会话到数据库。
- 不保留 HTML 原型的固定 350ms/500ms 人工延时；不以离屏截图宣称真实屏幕/GPU/窗口管理器视觉等价。
- 不覆盖、恢复、重排或提交当前工作树已有的文档、归档、原型、图像和 `.omo/` 文件；不主动提交 Git。

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: TDD for Core validation/generator/planner/session contracts; tests-after for Qt composition where widgets must exist before interaction tests; framework is QtTest + CTest and standalone Release benchmark executable.
- Correctness evidence: separate `mos_planner_test` exhaustive oracle output, fixture-family assertions, stable reason-code assertions, and `ctest --test-dir build -R 'mos_(validation|fixture|planner|session)' --output-on-failure` logs.
- UI evidence: `mos_decision_ui_test` with stable `DEC-*` object names, `QSignalSpy::isValid()`, `QTest::mouseClick`, generated PNG/geometry TSV under `.omo/evidence/` for `1280x720`, `1920x1080`, `3840x2160`; actual WSLg run separately because offscreen is not on-screen proof.
- Performance evidence: Release-only JSON with compiler/Qt/CPU/RAM/display metadata, 100 warmups, 1000 samples, 5 fresh processes, p50/p95/p99/max/hash, heartbeat max dispatch, RSS, and 100 fresh-process canonical fixture hashes. Use `QElapsedTimer`, `QSignalSpy`, `QProcess`; do not claim an existing benchmark harness.
- Evidence path convention: `.omo/evidence/mos-p0-qt-implementation/task-1/` through `task-8/`; no evidence is written into `docs/` or source directories.

## Execution strategy
### Parallel execution waves
> Target 5-8 todos per wave. Fewer than 3 (except the final) means you under-split.

- Wave 1: contract clarification/fixture vectors, then pure models and validation.
- Wave 2: planner + independent oracle, then session/revision controller.
- Wave 3: DecisionView widgets and minimal MainWindow route; UI contract tests can begin after the page surface exists.
- Wave 4: Release benchmark/heartbeat and visual/manual evidence in parallel after all runtime behavior exists.
- Wave 5: core-document factual writeback and final verification only after every runtime gate passes.

Implementation worktree: `/home/lin/UXO_v1-mos-docs`, branch `docs/mos-prd-normalization`. This canonical MOS worktree contains the approved documents and prior MOS commits; all implementation, tests, evidence, and plan execution must remain here. Do not touch `/home/lin/UXO_v1-mos-implementation`, `/home/lin/UXO_v1`, or any other worktree; do not use local `main@f9277c5` as a base.

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| 1 | Approved docs and owner decisions | 2, 3 | none |
| 2 | 1 | 3, 4 | none |
| 3 | 1, 2 | 4, 7 | none |
| 4 | 2, 3 | 5, 6, 7 | none |
| 5 | 4 | 6 | 7 |
| 6 | 4, 5 | 7, 8 | 5 |
| 7 | 3, 4, 6 | 8 | 5, 6 |
| 8 | 1-7 | final verification wave | none |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [x] 1. 冻结 P0 几何、生成器和 UI 合同并建立独立 oracle fixture
  What to do / Must NOT do: 在实现分支（先确认已从 `main` 合并/重读 Approved 文档，并保留当前用户工作树）更新批准功能/UI 文档的技术补充：跑道坐标 `[0,L] × [-W/2,W/2]`；生成核心区 `x=[0,L], y=[-250,250]`；弹坑 `visibleRadius * expand`；UXO `K * cbrt(syntheticYield)`；稳定 fixture 顺序按 `floor(tierIndex * N / (T-1))` 形成嵌套集合；难度为 `无/中等/高` 序数标签；P0 选择已计算档位并弱显全部档位；Qt 去除人工 planning/loading 延时；JSON 仅单向 `QSaveFile` fixture 工件。补充闭集相切规则的有限 `double` 规范：X 不离散，禁止共享点，开放自由区端点向内部用 `std::nextafter` 生成可表示输出，长度不足则丢弃。新增 `tests/fixtures/mos_reference_vectors.mjs`、`tests/fixtures/mos_rng_vectors.json`、`tests/fixtures/verify_mos_contract.sh` 和 `tests/fixtures/verify_decision_contract.sh`；不得修改 P1/P2、真实安全语义或 HTML 行为。
  Parallelization: Wave 1 | Blocked by: approved docs merged into implementation worktree | Blocks: 2, 3
  References (executor has NO interview context - be exhaustive): `docs/features/mos-planning.md:58-131,144-151,191-232,245-258`; `docs/ui/pages/decision.md:12,27,370-415`; `docs/ui/prototypes/decision/index.html:573-602,1030-1079`; `docs/PRODUCT.md:189-205`; `.omo/drafts/mos-p0-qt-implementation.md` decisions/findings.
  Acceptance criteria (agent-executable): `git diff --check -- docs/features/mos-planning.md docs/ui/pages/decision.md`; `bash tests/fixtures/verify_mos_contract.sh --scenario=happy` and `bash tests/fixtures/verify_decision_contract.sh --scenario=happy` both exit 0; a contract scan finds the exact coordinate/formula/tier/timing/export rules; `tests/fixtures/mos_reference_vectors.mjs` generates `tests/fixtures/mos_rng_vectors.json` for seeds `0`, `42`, `-1`, `INT32_MIN`, `INT32_MAX`, draw-order/rounding expectations, and canonical-field order; a small independent oracle fixture set covers empty, no-solution, tangency, boundary, symmetric tie, overlap, five-tier nesting, and duplicate-seed cases.
  QA scenarios (name the exact tool + invocation): happy: `node tests/fixtures/mos_reference_vectors.mjs --check && bash tests/fixtures/verify_mos_contract.sh --scenario=happy && bash tests/fixtures/verify_decision_contract.sh --scenario=happy` exits 0, Evidence `.omo/evidence/mos-p0-qt-implementation/task-1/contract.log`; failure: `bash tests/fixtures/verify_mos_contract.sh --scenario=bad-y-bound` and `bash tests/fixtures/verify_decision_contract.sh --scenario=fixed-delay` each exit 1 and name the violated contract, Evidence `.omo/evidence/mos-p0-qt-implementation/task-1/contract-failure.log`.
  Commit: N | docs(contract): no commit without explicit user request

- [x] 2. 实现 Core MOS 数据模型、包络校验、确定性生成器和合成估算
  What to do / Must NOT do: 新增 `include/Core/MOS/MosTypes.h`, `MosValidation.h`, `MosFixtureGenerator.h`, `MosEstimator.h` 及对应 `src/Core/MOS/*.cpp`；新增 `tests/mos_validation_test.cpp` 与 `tests/mos_fixture_test.cpp`；在 `src/Core/CMakeLists.txt` 和根 `CMakeLists.txt` 注册。模型必须独立于 `TargetInfo`/`RunwayInfo`，所有字段标注模拟/合成语义；校验完整包络（有限值、跨字段边界、`S=width/step<=200`、`N<=13`、int32 seed、唯一非空 ID）；生成器端口化 32-bit `mulberry32`，固定抽取顺序、JS `Math.round` 半数向正无穷语义和已冻结坐标/公式；估算器只产生球体体积、合成回填时间、固定 UXO 工时和序数难度；序列化只生成单向 fixture bytes。不得使用 `std::uniform_*_distribution`、真实公式、外部 I/O 或运行时回灌。
  Parallelization: Wave 1 | Blocked by: 1 | Blocks: 3, 4
  References (executor has NO interview context - be exhaustive): `include/Core/Data/Types.h:30-63`; `include/Core/Simulation/DemoScenarioProvider.h:16-33`; `src/Core/CMakeLists.txt:1-10`; `CMakeLists.txt:36-100`; `docs/features/mos-planning.md:64-70,104-131,189-203`; `docs/ui/prototypes/decision/index.html:593-602,1030-1079`; `docs/DEVELOPMENT.md:104-113`.
  Acceptance criteria (agent-executable): `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`; `cmake --build build --target UXOMissionControlSimulationWorkflowTest UXOMissionControlMosValidationTest UXOMissionControlMosFixtureTest -j2`; `ctest --test-dir build -R 'mos_(validation|fixture)' --output-on-failure`; tests cover every input-envelope row with in/out/boundary cases, formula values, rejected input state preservation, all five seed vectors, exact field order/bytes, crater/UXO counts, and no runtime import path.
  QA scenarios (name the exact tool + invocation): happy: `ctest --test-dir build -R 'mos_(validation|fixture)' --output-on-failure` passes and canonical bytes match all vectors, Evidence `.omo/evidence/mos-p0-qt-implementation/task-2/happy.log`; failure: invalid width/step, duplicate ID, out-of-range seed, and `N=14` each return the specified stable error and leave input unchanged, Evidence `.omo/evidence/mos-p0-qt-implementation/task-2/failure-cases.log`.
   Commit: N | feat(core): no commit without explicit user request

- [x] 3. 实现连续 X 最大空矩形、递进规划和独立穷举正确性测试
  What to do / Must NOT do: 新增 `include/Core/MOS/MosPlanner.h` 与 `src/Core/MOS/MosPlanner.cpp`，实现 Y 边界枚举（50m/1m 为 50 cells/51 boundaries）、每个 Y band 的圆盘闭集禁区合并、连续 X 空隙、有限-double inward canonicalization、最小长宽、无解 reason `NO_FEASIBLE_RECTANGLE` 和五级总排序。实现复杂度目标 `O(T*S^2*N*logN)`，不得枚举 X 网格或 `O(L^2*S^2*N)`；按稳定 fixture 顺序生成嵌套修复集合，面积单调非减，破坏则整个复合结果拒绝。新增 `tests/mos_planner_test.cpp`，其中 oracle 独立实现小域候选枚举，不复用生产 planner 的几何辅助函数。
  Parallelization: Wave 2 | Blocked by: 1, 2 | Blocks: 4, 7
  References (executor has NO interview context - be exhaustive): `docs/features/mos-planning.md:64-66,91-102,104-127,191-232`; `docs/dev/architecture-mos.md:31-61`; `tests/simulation_workflow_test.cpp:21-197`; Qt 5.15 `QElapsedTimer` documentation `https://doc.qt.io/archives/qt-5.15/qelapsedtimer.html`.
  Acceptance criteria (agent-executable): `cmake --build build --target UXOMissionControlMosPlannerTest -j2`; `ctest --test-dir build -R mos_planner --output-on-failure`; every fixture family in `mos-planning.md` §8 has a named test; tangent boundary is rejected, one-ULP inward/outward cases are stable, no-solution is accepted with empty result, total ordering is unique, five-tier repaired sets are nested and area non-decreasing, and reduced-domain planner output exactly matches the independent oracle. A static review or test report demonstrates no X discretization and the documented complexity shape.
  QA scenarios (name the exact tool + invocation): happy: `ctest --test-dir build -R mos_planner --output-on-failure` and oracle comparison pass, Evidence `.omo/evidence/mos-p0-qt-implementation/task-3/happy.log`; failure: tangent, fully blocked runway, non-nested tier, and tie fixtures assert collision/error/rejection codes rather than silently returning a plan, Evidence `.omo/evidence/mos-p0-qt-implementation/task-3/failure-cases.log`.
   Commit: N | feat(core): no commit without explicit user request

### Todo 3 execution clarification

- The planner optimizes over canonical finite-double rectangles: obstacle-facing open X endpoints move inward with `std::nextafter`; minimum length and ordering are applied after canonicalization. A mathematical supremum that cannot be represented after canonicalization is not a valid output.
- `S` is computed once as the nearest integer to `W / step` after the existing integral/bound checks; Y boundaries are `-W/2 + W*i/S` for `i=0..S`, so `W=50, step=1` yields 50 cells and 51 boundaries.
- Public planner results distinguish accepted no-solution tiers (`valid=false`, reason `NO_FEASIBLE_RECTANGLE`) from rejected requests (`accepted=false`, stable rejection reason, empty tier results). No-solution area is zero for nested-area checks.
- `MosPlanner.h` owns public `MosRepairTier`, `MosRectangleResult`, `MosTierPlan`, and `MosProgressiveResult` value types plus `buildStableRepairTiers()` and `planProgressive()` entry points. Stable obstacle order is `craters` followed by `uxo`; provided tiers must have the configured count, tier 0 empty, known unique IDs, nested repaired sets, and a complete final tier. Invalid tier input rejects the entire composite.
- Each Y band projects every closed disk using the closest Y distance, clips/merges closed X intervals including singleton tangencies, and canonicalizes only the free X endpoints. The independent reduced-domain oracle must enumerate the same Y-boundary pairs and canonical X event candidates without calling production geometry helpers.

- [x] 4. 实现应用拥有的 MOS 会话、revision-safe replan controller 和原子日志
  What to do / Must NOT do: 新增 plain Core `MosPlanningSession`（输入/参数/结果/selectedTier/sequence log）和 MainWindow-library `MosPlanningController`/`MosReplanWorker`；`MainWindow` 按值或唯一拥有 controller/session，DecisionView 只发请求和渲染快照。worker 只消费不可变快照，不持有或修改会话；请求带单调 revision，只有最新 revision 可以提交。合法有解：状态+接受日志+一个 `mosStateChanged` 通知；合法无解：空结果+接受日志+一个通知；非法/非嵌套/陈旧结果：业务状态不变，拒绝路径仅追加规定日志，陈旧完成不发通知。JSON export 只通过明确的目标路径调用 `QSaveFile`，不提供 import/reload。新增 `tests/mos_session_test.cpp` 和受控 reverse-completion 测试 seam；不得引入通用 Repository/Store/event bus 或重构 REQ-001。
  Parallelization: Wave 2 | Blocked by: 2, 3 | Blocks: 5, 6, 7
  References (executor has NO interview context - be exhaustive): `include/Core/Simulation/SimulationWorkflow.h:13-60`; `src/Core/Simulation/SimulationWorkflow.cpp:5-90,154-170`; `docs/features/mos-planning.md:52-54,91-102,163-176,205-217`; `docs/dev/architecture-mos.md:31-61,71-79`; `docs/dev/sequence-diagram-mos.mermaid:20-34`; `include/MainWindow/MainWindow.h:78-81`; official Qt 5.15 `QSignalSpy` documentation `https://doc.qt.io/archives/qt-5.15/qsignalspy.html`.
  Acceptance criteria (agent-executable): `ctest --test-dir build -R mos_session --output-on-failure`; `QSignalSpy` is valid and observes exactly one notification for each accepted commit, zero for stale completions, and the expected rejection-log behavior; a reverse-completion test submits revision 2 before revision 1 and proves revision 1 cannot overwrite state or append a log; export test writes only a requested temporary artifact and proves a subsequent runtime snapshot is unchanged.
  QA scenarios (name the exact tool + invocation): happy: accepted result, accepted no-solution, tier select, and one-way export pass with one notification each, Evidence `.omo/evidence/mos-p0-qt-implementation/task-4/happy.log`; failure: invalid params, non-nested tiers, duplicate commit, and reverse completion preserve pre-command business state and produce only the specified rejection/stale evidence, Evidence `.omo/evidence/mos-p0-qt-implementation/task-4/failure-cases.log`.
   Commit: N | feat(core): no commit without explicit user request

### Todo 4 execution clarification

- `mosStateChanged` means the authoritative snapshot changed: accepted solution, accepted no-solution result, latest rejection-log append, and valid tier selection each emit exactly once. Stale or duplicate completions mutate nothing, append no log, and emit nothing.
- A replan request receives its monotonic revision before validation; an invalid newer request supersedes older pending work. The controller owns the pending immutable request and latest revision; the plain Core session owns only committed business state, selected tier, committed revision, and sequenced log.
- Todo 4 uses a synchronous `MosReplanWorker` QObject seam; no speculative `QThread`, event bus, Repository, Store, or dispatcher abstraction. The worker receives value-copied request data and returns a value-copied completion; the revision guard remains usable if measured Todo 7 evidence later requires threading.
- Session snapshots are returned by value. One-way export is controller-owned and observational: serialize the current accepted obstacle fixture with `QSaveFile` to the explicit path, changing no business state, revision, log, or signal count.

- [x] 5. 构建 DecisionView P0 组件和无固定延时的交互状态
  What to do / Must NOT do: 将 `DecisionView` 空壳改为独立 P0 页面，新增 `MosRunwayWidget`（首个 QPainter `paintEvent`，wheel zoom/pan/selection）、`MosGeneratorDialog` 和必要的参数/方案子组件；所有页面状态由 controller snapshot 驱动，UI 只保存 selected index/zoom/collapse。按 `DEC-*` 文档 ID 设置稳定 `objectName`，复用 `GlobalStyle` token；提供正常、loading（真实计算期间）、empty、error、disabled 五态，所有数据/参数/结果明确“模拟/合成”。P0 显示全部已计算档位且当前档强调；P1 控件恒禁用；不复制算法、修复集合或会话状态到 widget，不使用人工 sleep/delay，不改 HTML。
  Parallelization: Wave 3 | Blocked by: 4 | Blocks: 6, 7
  References (executor has NO interview context - be exhaustive): `include/MainWindow/DecisionView.h:8-19`; `src/MainWindow/DecisionView.cpp:3-12`; `docs/ui/pages/decision.md:12-35,58-77,119-186,370-439`; `docs/UI.md:159-161`; `include/Common/GlobalStyle.h:20-109`; Qt 5.15 `QPainter`/`QImage` docs `https://doc.qt.io/archives/qt-5.15/qpainter.html`, `https://doc.qt.io/archives/qt-5.15/qimage.html`.
  Acceptance criteria (agent-executable): `cmake --build build --target MainWindow -j2`; `bash tests/fixtures/verify_decision_contract.sh --scenario=happy` exits 0; the new page/component source contains every required P0 `DEC-*` object name and no artificial 350/500ms timer; all controls are wired to snapshot/request APIs rather than owning algorithm/session state; the QPainter widget renders only from an immutable snapshot and uses `GlobalStyle` tokens.
  QA scenarios (name the exact tool + invocation): happy: `cmake --build build --target MainWindow -j2 && bash tests/fixtures/verify_decision_contract.sh --scenario=happy` exits 0, Evidence `.omo/evidence/mos-p0-qt-implementation/task-5/happy.log`; failure: `bash tests/fixtures/verify_decision_contract.sh --scenario=missing-object` and `bash tests/fixtures/verify_decision_contract.sh --scenario=fixed-delay` each exit 1 with the missing/forbidden token named, Evidence `.omo/evidence/mos-p0-qt-implementation/task-5/failure.log`.
  Commit: N | feat(ui): no commit without explicit user request

- [x] 6. 接入 MainWindow 最小路由并新增决策页 QtTest 契约
  What to do / Must NOT do: 在 `MainWindow` 中将现有 situation workspace 与 `DecisionView` 放入最小 `QStackedWidget`/页面容器；`onNavigationChanged(2)` 切到 MOS，其他五个 index 保持当前占位行为；启动默认情况保持现有可见窗口行为。连接 controller 的单一状态通知和 DecisionView 请求，给导航按钮及 MOS 控件补稳定 object names；更新 `src/MainWindow/CMakeLists.txt`、根 `CMakeLists.txt` 和新增 UI test target。不得改 `RightPanelWidget` 的 live situation decision suggestion、`Application` 初始化占位、其他页面或全壳架构。
  Parallelization: Wave 3 | Blocked by: 4, 5 | Blocks: 7, 8
  References (executor has NO interview context - be exhaustive): `src/MainWindow/MainWindow.cpp:163-244,258-287,289-347,349-433`; `include/MainWindow/MainWindow.h:22-85`; `src/MainWindow/NavigationWidget.cpp:14-21,88-113`; `src/MainWindow/CMakeLists.txt:1-51`; `CMakeLists.txt:31-100`; `tests/simulation_workflow_ui_test.cpp:28-153,284-320`; `docs/ARCHITECTURE.md:20-38,121-133`; `docs/ui/pages/decision.md:16,58-77`.
  Acceptance criteria (agent-executable): `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`; `cmake --build build --target UXOMissionControlMosDecisionUiTest -j2`; `ctest --test-dir build -R mos_decision_ui --output-on-failure`; test opens MainWindow offscreen, clicks navigation index 2, proves DecisionView becomes visible, exercises target/parameter/replan/tier/generator/export paths, captures `QWidget::grab()` PNGs and geometry TSV at all three sizes, and proves existing four CTests remain registered and green. Object-name contract failure must list missing IDs. The test accepts `MOS_UI_SCENARIO=happy|invalid|no-solution|no-output|route-regression` and asserts each scenario's expected result.
  QA scenarios (name the exact tool + invocation): happy: `UXO_VISUAL_EVIDENCE_DIR=.omo/evidence/mos-p0-qt-implementation/task-6 UXO_VISUAL_GEOMETRY_REPORT=1 UXO_VISUAL_EVIDENCE_SIZE=1280x720 QT_QPA_PLATFORM=offscreen QT_OPENGL=software LIBGL_ALWAYS_SOFTWARE=1 MOS_UI_SCENARIO=happy ctest --test-dir build -R mos_decision_ui --output-on-failure`, repeated with `1920x1080` and `3840x2160`, records interaction and visual evidence, Evidence `.omo/evidence/mos-p0-qt-implementation/task-6/happy/`; failure: `MOS_UI_SCENARIO=invalid QT_QPA_PLATFORM=offscreen QT_OPENGL=software LIBGL_ALWAYS_SOFTWARE=1 ctest --test-dir build -R mos_decision_ui --output-on-failure`, the same full command with `MOS_UI_SCENARIO=no-solution`, the same full command with `MOS_UI_SCENARIO=no-output`, and `MOS_UI_SCENARIO=route-regression QT_QPA_PLATFORM=offscreen QT_OPENGL=software LIBGL_ALWAYS_SOFTWARE=1 ctest --test-dir build -R mos_decision_ui --output-on-failure` each exit 0 while asserting validation/error/no-file/route-regression behavior, Evidence `.omo/evidence/mos-p0-qt-implementation/task-6/failure.log`.
  Commit: N | feat(ui): no commit without explicit user request

- [x] 7. 建立 Release 性能、heartbeat、进程确定性和内存门禁
  What to do / Must NOT do: 新增 standalone `tests/mos_performance_test.cpp`，构建 target `UXOMissionControlMosPerformanceTest` 并注册 CTest `mos_performance`；使用 `QElapsedTimer`、`QProcess`、`QSignalSpy` 和 Linux process memory accounting。命令行必须支持 `--scenario=happy|stale-revision|threshold-regression|seed-mismatch|no-solution`、`--output <json>` 和 `--max-p99-ms <value>`，默认 happy 执行 Release 100 warmups + 1000 samples in 5 fresh processes，报告 p50/p95/p99/max/hash/environment；heartbeat 场景执行 100 ordered replans with 10ms heartbeat and assert max dispatch <=50ms, no stale overwrite/duplicate commit/lost input；assert core p95<=10ms, p99<=16ms, max<=33ms, click-to-render p95<=100ms/max<=200ms, temp memory/RSS limits, and 100 fresh-process identical canonical bytes. Keep worker strategy if it passes；never claim GUI-thread sync unless the stated gate passes. Do not add external benchmark libraries or weaken thresholds.
  Parallelization: Wave 4 | Blocked by: 3, 4, 6 for click-to-render | Blocks: 8
  References (executor has NO interview context - be exhaustive): `docs/features/mos-planning.md:196-232`; `docs/DEVELOPMENT.md:69-86,115-134`; `CMakeLists.txt:36-100`; Qt 5.15 docs `https://doc.qt.io/archives/qt-5.15/qelapsedtimer.html`, `https://doc.qt.io/archives/qt-5.15/qsignalspy.html`, `https://doc.qt.io/archives/qt-5.15/qtest-overview.html`.
  Acceptance criteria (agent-executable): `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`; `cmake --build build --target UXOMissionControlMosPerformanceTest -j2`; `ctest --test-dir build -R mos_performance --output-on-failure`; machine-readable evidence contains all required sample counts/percentiles/hash/environment fields and exits nonzero on any threshold violation. Offscreen limitations and X11 support caveat are recorded; no screenshot is used as proof of native display/GPU fidelity.
  QA scenarios (name the exact tool + invocation): happy: `ctest --test-dir build -R mos_performance --output-on-failure` runs the default happy scenario and five-process/100-process hashes match, Evidence `.omo/evidence/mos-p0-qt-implementation/task-7/performance.json`; expected-negative business paths: `./build/UXOMissionControlMosPerformanceTest --scenario=stale-revision --output .omo/evidence/mos-p0-qt-implementation/task-7/stale.json` and `./build/UXOMissionControlMosPerformanceTest --scenario=no-solution --output .omo/evidence/mos-p0-qt-implementation/task-7/empty.json` must exit 0 while proving stale results do not commit and legal no-solution is accepted; injected failures: `./build/UXOMissionControlMosPerformanceTest --scenario=threshold-regression --max-p99-ms 0 --output .omo/evidence/mos-p0-qt-implementation/task-7/threshold.json` and `./build/UXOMissionControlMosPerformanceTest --scenario=seed-mismatch --output .omo/evidence/mos-p0-qt-implementation/task-7/seed.json` must exit nonzero with a diagnostic JSON naming the failed assertion, Evidence `.omo/evidence/mos-p0-qt-implementation/task-7/failure/`.
  Commit: N | test(perf): no commit without explicit user request

- [x] 8. 完成全门禁、实际运行/视觉 QA 和事实文档回写
  What to do / Must NOT do: 在实现分支先执行全量 Release configure/build/CTest，再通过 WSLg/实际运行入口验证 MOS P0；使用现有 visual evidence env 在 1280x720、1920x1080、3840x2160 采集 PNG/geometry TSV，并记录 offscreen 与 actual-display 的边界。运行安全/范围扫描，确认无网络/数据库/真实设备调用、所有数值有模拟标识、JSON 无导入/回灌。只有所有证据通过后，单独更新 `docs/ARCHITECTURE.md`、`docs/UI.md`、`docs/DEVELOPMENT.md` 的 CURRENT/门禁事实，并将 REQ-007/feature 状态改为 `Implemented`；不要把当前 docs 分支用户已有变更合并进代码差异。
  Parallelization: Wave 5 | Blocked by: 1-7 | Blocks: final verification wave
  References (executor has NO interview context - be exhaustive): `docs/DEVELOPMENT.md:40-59,69-86,115-146`; `docs/ARCHITECTURE.md:18-38,73-135`; `docs/UI.md:145-173`; `docs/features/mos-planning.md:225-243`; `AGENTS.md` safety/build rules; `tests/simulation_workflow_ui_test.cpp:137-280`.
  Acceptance criteria (agent-executable): `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`; `cmake --build build --target UXOMissionControl -j2`; `ctest --test-dir build --output-on-failure`; real app launch exits/operates through the P0 workflow without external services; visual artifacts exist for all three sizes and geometry reports show no unintended overflow; `git diff --check` passes and a scope scan shows only planned source/tests/CMake/docs changes. Core docs describe only verified CURRENT facts; no source claim is based solely on HTML or a passing compile.
  QA scenarios (name the exact tool + invocation): happy: run `UXO_VISUAL_EVIDENCE_DIR=.omo/evidence/mos-p0-qt-implementation/task-8/happy UXO_VISUAL_GEOMETRY_REPORT=1 UXO_VISUAL_EVIDENCE_SIZE=1280x720 QT_QPA_PLATFORM=offscreen QT_OPENGL=software LIBGL_ALWAYS_SOFTWARE=1 MOS_UI_SCENARIO=happy ctest --test-dir build -R mos_decision_ui --output-on-failure`, repeat the complete command with `UXO_VISUAL_EVIDENCE_SIZE=1920x1080` and `UXO_VISUAL_EVIDENCE_SIZE=3840x2160`, then run `ctest --test-dir build --output-on-failure` plus the WSLg startup smoke `set -o pipefail; timeout 10 bash scripts/run_demo.sh xcb 2>&1 | tee .omo/evidence/mos-p0-qt-implementation/task-8/xcb.log; test ${PIPESTATUS[0]} -eq 124`; full target selection, valid/invalid/no-solution, tier comparison, generator apply/cancel/export and visual evidence must pass, Evidence `.omo/evidence/mos-p0-qt-implementation/task-8/happy/`; failure: `MOS_UI_SCENARIO=invalid QT_QPA_PLATFORM=offscreen ctest --test-dir build -R mos_decision_ui --output-on-failure`, `MOS_UI_SCENARIO=no-solution QT_QPA_PLATFORM=offscreen ctest --test-dir build -R mos_decision_ui --output-on-failure`, and `MOS_UI_SCENARIO=no-output QT_QPA_PLATFORM=offscreen ctest --test-dir build -R mos_decision_ui --output-on-failure` each exit 0 while asserting the expected error/no-solution/no-file behavior; `QT_QPA_PLATFORM=invalid ctest --test-dir build -R mos_decision_ui --output-on-failure` exits nonzero as an environment failure, Evidence `.omo/evidence/mos-p0-qt-implementation/task-8/failure/`.
  Commit: N | docs(verification): no commit without explicit user request

### Todos 5-8 execution clarification

- 实际证据目录命名与计划原约定存在差异，但全部证据已落盘并通过验证：
  - Todo 5 证据：`.omo/evidence/mos-p0-qt-implementation/task-5/`（含 `contract-happy.log`、`contract-fixed-delay.log`、`ctest-mos.log`、`git-diff-check.log` 及 `r2/` 子目录）。
  - Todo 6 证据：`.omo/evidence/mos-p0-qt-implementation/todo-6/`（目录名为 `todo-6` 而非 `task-6`，含 `scenarios.log`、`release-ctest-all.log`、`git-diff-check.log`）。
  - Todo 7 证据：`.omo/evidence/mos-p0-qt-implementation/task-7/r2/`（Revision 2，含 `REPORT.md`、`happy.json`、`happy-validation.log`、`stale-revision`/`no-solution`/`threshold-regression`/`seed-mismatch` 各场景 JSON 与日志、`static-scan.log`）。
  - Todo 8 证据：`.omo/evidence/mos-p0-qt-implementation/task-8/`（含 `REPORT.md`、`xcb.log`、`preflight.log`、`worktree-check.log`、`cleanup-check.log`）。
- 最终汇总证据：`.omo/evidence/mos-p0-qt-final/`（含 `REPORT.md`、Release `ctest.log` 11/11 通过、`scenarios-functional.log`、`static-scans.log`、`visual/` 24 PNG + 18 TSV 头校验 + 0 溢出）。
- 性能门禁最终结果（Revision 2，5 测量进程 / 100 预热 / 1000 样本 / 100 全新哈希进程）：core p95=2.13ms / p99=3.34ms / max=3.76ms；点击到渲染 p95=12.55ms / max=15.94ms；10ms 心跳最大派发 ≤50ms；100 全新进程规范哈希一致；RSS `postWarmupGrowth=0`。所有阈值门禁 PASS。
- xcb 官方入口烟测：`timeout 10 bash scripts/run_demo.sh xcb` exit 124（进程存活至超时，无崩溃），符合计划 F3 的存活断言语义。
- Todos 5-8 全部 `[x]`；F1-F4 仍为 `[ ]`，留待最终独立审计通过后再勾选。
- 本计划遵循 no-commit 策略，所有改动仅停留在工作树，未执行任何 git commit/stage/push。

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [x] F1. Plan compliance audit: round-2 修复仅触及 MosPlanningController/MosRunwayWidget*/MosParamsPanel/closure tests/CMakeLists；15/15 CTest 通过（round-2/build-ctest.log）；Todos 1-8 全 `[x]`；证据落盘 `.omo/evidence/mos-p0-qt-closure-final/round-2/`。
- [x] F2. Code quality review: Core/MOS 无 QWidget/QNetwork/QSql/QThread/QTimer 依赖；无 `as any`/`@ts-ignore`/空 catch；中文注释齐全；closure oracle 独立调用 planner 公共 API 未复用生产几何辅助；worker 由 controller 唯一拥有。
- [x] F3. Real runtime QA: xcb 三视口（1920x1080/1280x720/1600x900）均 `Application started successfully`（round-2/xcb-visual.log）；offscreen closure 测试 mos_ui_closure 11 slots + mos_runway_closure 13 slots 覆盖 K 标签几何/runway 行为/export 安全；xcb 截图受 WSLg 限制全黑，属基础设施非代码问题。
- [x] F4. Scope fidelity: `git status --short` 所有改动均在 MOS P0（REQ-007）范围内；无 P1/P2/UXR/真实集成/其他页面行为；未覆盖用户已有文档/归档/原型改动。

## Commit strategy
No commits, staging, rebases, merges, or pushes are part of this plan unless the user explicitly requests them. Keep implementation, tests/CMake, and post-verification documentation changes separable in the working tree; do not include the current `docs/mos-prd-normalization` dirty documentation changes in a future code commit. If the user later requests commits, inspect `git status`, `git diff`, `git log --oneline -10`, and stage only the verified task files.

## Success criteria
The implementation is complete only when all of the following are true: the pure Core planner matches the independent oracle and all fixture families; input/generator/session semantics are deterministic and atomic; stale revisions cannot mutate state; P0 DecisionView and index-2 routing pass QtTest and actual-run QA; Release performance, heartbeat, memory, and fresh-process determinism meet every Approved threshold; four pre-existing CTests remain green; visual evidence has no unintended overflow; safety labels and one-way export boundaries are preserved; core docs reflect verified CURRENT facts only; and final scope/whitespace/security audits pass.
