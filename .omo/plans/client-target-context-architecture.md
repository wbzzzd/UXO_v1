# client-target-context-architecture - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 一份同时说明长期客户端边界、当前实现、下一阶段草案和演进差距的架构基线。读者可以直接看出客户端与外部信息系统、外部安全执行系统之间的责任关系，而不会把历史协议或尚未实现的接口误写成事实。

**Why this approach:** 产品基线决定长期方向，源码/CMake/测试决定 CURRENT，六项 Draft 需求只决定 NEXT 候选架构。三层严格分开，旧 SDD 和 DDR 只提供经过 PRODUCT 边界过滤后的设计依据。

**What it will NOT do:** 不选择通信协议、数据库、端口、部署方式或硬件驱动；不设计或实现真实设备控制和安全执行链；不移动或归档旧文档。

**Effort:** Short
**Risk:** Medium - 最大风险是把方向性外部责任误写成已批准接口，或让长期 TARGET 与 Draft NEXT 再次混淆。
**Decisions to sanity-check:** 外部系统只按责任命名；地图和历史能力不假设已有独立服务；客户端只提交任务意图，真实校验与执行始终属于外部安全边界。

Your next move: 启动执行，或先运行高精度双重计划审查。完整执行细节如下。

---

> TL;DR (machine): Short documentation-only rewrite; medium semantic risk; one ARCHITECTURE.md baseline with long-term TARGET context, logical layers, Draft NEXT, and two-stage gaps.

## Scope
### Must have
- `docs/ARCHITECTURE.md` 开头明确区分长期 `TARGET`、实现事实 `CURRENT`、未批准 `NEXT`。
- 一张长期 TARGET 客户端上下文图，包含指挥席用户、客户端边界、外部信息责任与外部安全执行责任。
- 一张长期 TARGET 客户端逻辑分层图，包含界面交互、应用/业务编排、客户端权威状态、本地模拟数据源、未来外部适配边界。
- 一张责任与概念数据方向表，每个参与方和箭头均可追溯到 PRODUCT；不支持的方向必须省略或标为未确定。
- 保留 CURRENT 模块、启动、状态、调用链和半成品事实，不把目标设计写成当前实现。
- 现有第 7-9 节重构为 `NEXT 候选架构（Draft）`，需求仍由 PRODUCT 第 9 节管理。
- 分别说明 `CURRENT -> NEXT` 和 `NEXT -> 长期 TARGET` 差距。
- 明确客户端不直接控制设备、不拥有真实安全执行链，未来外部接入必须单独批准。

### Must NOT have (guardrails, anti-slop, scope boundaries)
- 不修改 `docs/PRODUCT.md`、`docs/UI.md`、`docs/DEVELOPMENT.md`、`AGENTS.md`、源码、测试、CMake 或需求状态。
- 不移动、删除、改写或归档 SRS、SDD、旧 UI、`docs/dev/`、DDR 或 research 文件。
- 不继承旧 SDD 的插件层、通信服务层、数据处理层、HAL 或服务器部署作为客户端既定架构。
- 不写 MQTT、ZeroMQ、WebSocket、gRPC、HTTP、端口、数据库、表结构、厂商、驱动或具体 API。
- 不增加 UXR/MOS 模块，不把 `REQ-001` 至 `REQ-006` 从 `Draft` 提升为实施依据。
- 不使用具体类名定义长期 TARGET 层；现有类名只允许保留在 CURRENT 或 NEXT 候选事实中。
- 不把地图、历史或审计能力写成已确定外部服务合同；只能描述未指定的信息责任或待后续评审边界。
- 不宣称 Mermaid 已渲染验证；当前环境没有 `mmdc` 或 `markdownlint`。

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: none - documentation-only change; no C++ behavior changes. Use deterministic Markdown structure checks plus semantic source review.
- Evidence: outside ulw-loop, write task receipts under `.omo/evidence/client-target-context-architecture/` as `task-N-*.md` or `.txt`, replacing `N` with the todo number.
- Baseline check: capture `git status --short` and `git diff -- docs/ARCHITECTURE.md` before editing so pre-existing user changes remain distinguishable.
- Structural checks: `git diff --check -- docs/ARCHITECTURE.md`; a Node script must verify balanced fenced blocks and every repository-relative Markdown link target exists.
- Mermaid limitation: run `command -v mmdc`; current expected result is unavailable. Do not install dependencies. Record `[blocked] Mermaid parser unavailable`; perform balanced-fence and agent READ review, and do not claim render validation.
- Semantic checks: agent READ comparison against PRODUCT must account for every external actor/arrow, preserve CURRENT, mark NEXT Draft/unapproved, mark TARGET direction-only, and preserve safety approval gates.

## Execution strategy
### Parallel execution waves
> Target 5-8 todos per wave. Fewer than 3 (except the final) means you under-split.
- Wave 1 is sequential because every todo edits the same document and later headings depend on the final section structure.
- Final verification runs after all five document steps and may execute independent checks in parallel.

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| 1 | none | 2, 3, 4, 5 | none |
| 2 | 1 | 3, 4, 5 | none |
| 3 | 2 | 4, 5 | none |
| 4 | 3 | 5 | none |
| 5 | 4 | final verification | none |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [x] 1. Reframe the architecture baseline around TARGET, CURRENT, and NEXT
  What to do / Must NOT do: Update the document role and section navigation so long-term TARGET is product-derived, CURRENT is source/CMake/test-derived, and NEXT is Draft. Define the three terms once. Plan the final section order before moving text. Preserve every existing CURRENT statement and diagram byte-for-byte except heading numbers and links required by the new order. Remove the current implication that TARGET equals NEXT or that NEXT is ready to implement. Do not add external actors or logical layers yet.
  Parallelization: Wave 1 | Blocked by: none | Blocks: 2, 3, 4, 5
  References (executor has NO interview context - be exhaustive): `docs/ARCHITECTURE.md:1-10`, `docs/ARCHITECTURE.md:12-132`, `docs/ARCHITECTURE.md:134-183`, `docs/PRODUCT.md:34-38`, `docs/PRODUCT.md:161-193`, `AGENTS.md:23-31`.
  Acceptance criteria (agent-executable): The opening text explicitly defines three layers; all CURRENT content formerly in sections 2-6 remains present; existing NEXT text is labeled Draft/unapproved; no sentence says NEXT will be implemented after mere document editing. `git diff --check -- docs/ARCHITECTURE.md` exits 0.
  QA scenarios (name the exact tool + invocation): Happy - use Read on the full document and write a heading/order inventory to `.omo/evidence/client-target-context-architecture/task-1-section-map.md`. Failure - compare all pre-edit CURRENT table rows and Mermaid blocks against post-edit content; any missing fact or changed CURRENT semantics fails and is recorded in the same receipt.
  Commit: N | no commit without explicit user authorization

- [x] 2. Add the long-term TARGET client context and responsibility boundary
  What to do / Must NOT do: Add a direction-only client context section. Include the command-seat user; external detection/target information; external recognition/analysis outputs; an unspecified external situation-information responsibility; and an external task-execution/safety responsibility. Label arrows as conceptual information/result/task-intent flows. Show real validation/action outside the client. Omit a separate history/audit service contract; describe persistence/history responsibility as undetermined and separately approval-gated in the boundary table. Explicitly state these actors/interfaces are not CURRENT and are not approved contracts. Do not import old SDD layers, protocols, deployment, servers, drivers, or direct control commands.
  Parallelization: Wave 1 | Blocked by: 1 | Blocks: 3, 4, 5
  References (executor has NO interview context - be exhaustive): `docs/PRODUCT.md:50-60`, `docs/PRODUCT.md:62-105`, `docs/PRODUCT.md:195-203`, `docs/UI.md:14-25`, `SDD排弹抢修指挥系统_v1.0.md:55-154` (source-only; use only to identify candidate external responsibilities), `.omo/drafts/client-target-context-architecture.md:19-45`.
  Acceptance criteria (agent-executable): Every actor and arrow has a cited PRODUCT responsibility; user is shown as primary actor; no raw sensor stream, DB write, safety command, direct device command, or protocol traffic is shown; external safety/execution owns real action; context is labeled long-term TARGET/direction-only/not implemented. `git diff --check -- docs/ARCHITECTURE.md` exits 0.
  QA scenarios (name the exact tool + invocation): Happy - create `.omo/evidence/client-target-context-architecture/task-2-actor-trace.md` mapping every actor/arrow to PRODUCT lines. Failure - search the new TARGET section for protocol/vendor/port/DB/driver terms and inspect every hit; unsupported concepts fail. Record command output and semantic verdict in the receipt.
  Commit: N | no commit without explicit user authorization

- [x] 3. Add the long-term TARGET client logical architecture and principles
  What to do / Must NOT do: Add a responsibility-level logical-layer diagram/table with presentation/interaction, client workflow/application orchestration, authoritative client business/session state, local simulation data source, and future external adapter boundary. Keep external safety/execution outside all client layers. Define principles: UI never accesses external systems directly; one client business-state authority; simulation and future external adapters remain distinguishable; external data freshness/disconnection/degradation must be visible; external access is separately approval-gated. Do not name concrete TARGET classes, repositories, stores, event buses, protocols, or services.
  Parallelization: Wave 1 | Blocked by: 2 | Blocks: 4, 5
  References (executor has NO interview context - be exhaustive): `docs/PRODUCT.md:54-105`, `docs/PRODUCT.md:195-203`, `docs/ARCHITECTURE.md:63-93`, `docs/ARCHITECTURE.md:151-171`, `docs/ddr/DDR-006-产品视角架构评估.md:39-83` (risk rationale only), `docs/ddr/DDR-007-代码架构之道.md:35-88` (safety-independence rationale only), `docs/dev/architecture-boundaries.md:15-28` (source-only boundary hints).
  Acceptance criteria (agent-executable): Layers are responsibilities rather than concrete modules; local simulation is inside the client data-source boundary; future adapters are explicitly absent/unapproved; external safety execution remains outside; no DDR process/socket/channel prescriptions are adopted. `git diff --check -- docs/ARCHITECTURE.md` exits 0.
  QA scenarios (name the exact tool + invocation): Happy - write layer-to-PRODUCT responsibility mapping to `.omo/evidence/client-target-context-architecture/task-3-layer-trace.md`. Failure - inspect the TARGET section for concrete class/API/protocol/deployment claims; any unsupported claim fails and is listed in the receipt.
  Commit: N | no commit without explicit user authorization

- [x] 4. Reframe the existing design as NEXT candidate architecture and add two-stage gaps
  What to do / Must NOT do: Rename the existing requirement mapping, component diagram, and rules as `NEXT 候选架构（Draft）`. Preserve REQ-001..006 and architecture responsibilities. Replace “批准后要实施” with wording that NEXT cannot enter implementation until PRODUCT requirement and feature-design approval gates pass. Keep the existing CURRENT-to-NEXT gap table. Add a concise NEXT-to-long-term-TARGET gap table covering only PRODUCT-supported omissions: external information intake, long-term decision support, external execution feedback boundary, history/persistence boundary, and undecided user/permission model. Mark each as requiring later requirements/design approval, not as committed work.
  Parallelization: Wave 1 | Blocked by: 3 | Blocks: 5
  References (executor has NO interview context - be exhaustive): `docs/ARCHITECTURE.md:134-183`, `docs/PRODUCT.md:54-105`, `docs/PRODUCT.md:161-193`, `docs/PRODUCT.md:209-211`, `AGENTS.md:28-31`, `docs/features/README.md:12-45`.
  Acceptance criteria (agent-executable): All six REQ IDs appear once in the NEXT mapping and retain their titles/responsibilities; NEXT is explicitly Draft/unapproved; CURRENT-to-NEXT facts remain; NEXT-to-TARGET rows are grounded in PRODUCT and contain no commitment or protocol choice. `git diff --check -- docs/ARCHITECTURE.md` exits 0.
  QA scenarios (name the exact tool + invocation): Happy - write an ID/title/responsibility comparison and two-gap-table review to `.omo/evidence/client-target-context-architecture/task-4-next-gap-check.md`. Failure - any wording that equates TARGET with NEXT, treats Draft as approved, or adds a seventh requirement fails and is recorded.
  Commit: N | no commit without explicit user authorization

- [x] 5. Run whole-document structural and semantic QA
  What to do / Must NOT do: Re-read the complete final architecture document, repair broken headings/links/fences, and remove duplicate or contradictory definitions. Verify no file outside `docs/ARCHITECTURE.md` was touched by execution. Do not install Mermaid/Markdown tooling and do not claim parser/render validation. Preserve the dirty worktree's pre-existing changes.
  Parallelization: Wave 1 | Blocked by: 4 | Blocks: final verification
  References (executor has NO interview context - be exhaustive): `docs/ARCHITECTURE.md` complete file, `docs/PRODUCT.md:34-38`, `docs/PRODUCT.md:50-105`, `docs/PRODUCT.md:161-203`, `AGENTS.md:5-20`, pre-edit evidence from Todo 1.
  Acceptance criteria (agent-executable): `git diff --check -- docs/ARCHITECTURE.md` exits 0; a Node check reports balanced Markdown fences and all repository-relative link targets exist; six REQ IDs remain consistent; semantic checklist passes; `command -v mmdc` absence is reported as `[blocked] render validation`, not hidden; final diff contains no changes outside the planned architecture file attributable to this execution.
  QA scenarios (name the exact tool + invocation): Happy - save commands, outputs, final heading inventory, and semantic checklist to `.omo/evidence/client-target-context-architecture/task-5-final-qa.md`. Failure - deliberately treat any unbalanced fence, missing target, unsupported actor/arrow, safety-boundary weakening, or unreported Mermaid limitation as a blocking failure and correct before completion.
  Commit: N | no commit without explicit user authorization

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [x] F1. Plan compliance audit - confirmed by independent Oracle review.
- [x] F2. Documentation quality review - confirmed by independent Oracle review.
- [x] F3. Render/structure QA - fences/links/diff clean; Mermaid render remains explicitly blocked because `mmdc` is unavailable.
- [x] F4. Scope fidelity - confirmed by independent Oracle review against PRODUCT and pre-edit dirty-worktree evidence.

## Commit strategy
- Do not commit during execution because the user has not explicitly requested a commit.
- Keep all changes in the working tree for review.
- If the user later authorizes a commit, create one atomic documentation commit containing only `docs/ARCHITECTURE.md` with message `docs: add long-term client target architecture`; do not include pre-existing unrelated changes.

## Success criteria
- `docs/ARCHITECTURE.md` provides one clear long-term TARGET context view and one client logical-layer view.
- External participants and conceptual flows are traceable to PRODUCT and do not imply existing/approved contracts.
- Real device validation/action and safety chain remain outside the client repository boundary.
- CURRENT architecture facts and diagrams remain accurate.
- Existing REQ-001..006 responsibilities are preserved under an explicitly Draft NEXT candidate architecture.
- Separate CURRENT-to-NEXT and NEXT-to-TARGET gaps are present and non-prescriptive.
- No protocols, ports, databases, vendors, drivers, UXR/MOS modules, deployment topology, archive moves, or source/code changes are introduced.
- `git diff --check` and local link/fence checks pass.
- Mermaid render validation is either performed with an already-installed parser or explicitly reported blocked; no new dependency is installed.
