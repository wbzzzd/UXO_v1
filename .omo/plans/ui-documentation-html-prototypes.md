# ui-documentation-html-prototypes - Work Plan

## TL;DR (For humans)
**What you'll get:** 一套可在仓库中长期维护的 UI 详细设计：六个系统页面都有完整视觉图、可点击原型、逐控件功能说明、异常状态和关键跨页流程。

**Why this approach:** 页面说明、原型和自动检查共享稳定控件编号；评审者既能先看整体效果，也能追踪每个按钮的行为。每页只保留一张权威截图，其他尺寸由自动检查覆盖，避免图片基线失控。

**What it will NOT do:** 不修改 Qt 客户端，不连接真实设备或外部系统，不把设计原型声明为已实现功能，也不改变任何需求状态。

**Effort:** Large
**Risk:** Medium - 六个页面的视觉与功能定义需要保持一致，同时现有核心文档已有未提交修改。
**Decisions to sanity-check:** 六页信息架构只作为 TARGET 草案；权威截图统一使用 1920×1080；精确三屏 4K 方案留待单独需求确认。

Your next move: 批准本计划后开始制作。Full execution detail follows below.

---

> TL;DR (machine): Large/Medium；交付 TARGET/Draft Markdown 规格、六页静态 HTML 原型、Playwright 契约测试和六张权威整体截图。

## Scope
### Must have
- `docs/UI.md` 保持 UI 权威入口；`docs/ui/` 只保存下位详细规格和原型。
- 六个页面：态势、探测、决策、设备、统计、配置。
- 每页包含整体设计、区域说明、稳定控件 ID、功能/状态/反馈/键盘/安全/CURRENT 对照表。
- 原生 HTML/CSS/JS 原型，使用固定本地模拟数据；页面和动作均明确标注 TARGET/Draft 与模拟语义。
- 隔离的 Node + Playwright 工具链，验证链接、ID 覆盖、键盘操作、危险控件缺失和布局。
- 每页提交一张 1920×1080 权威整体截图；1280×720、1920×1080、3840×2160 运行布局检查。
- 保留所有用户现有修改，不覆盖或回退工作树已有 hunks。
### Must NOT have (guardrails, anti-slop, scope boundaries)
- 不修改 C++、Qt、CMake、业务状态或页面路由。
- 不实现真实设备控制、真实处置、紧急停止、外部通信、数据库、持久化、认证、UXR 或 MOS。
- 不把原型证据解释为 Qt 验证、REQ Approved/Implemented 或 CURRENT 事实。
- 不引入前端框架、后端、网络请求、浏览器存储或外部运行时资源。
- 不创建与 `docs/UI.md` 竞争的根级设计权威文档。
- 不定义尚无依据的三屏窗口分配，不维护组合爆炸式截图矩阵。

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: TDD；Node 校验脚本 + `@playwright/test`，先写页面/控件/状态契约再实现原型。
- Evidence classes: `documentation-structure`、`prototype-behavior`、`prototype-visual`、`CURRENT-Qt` 分开记录；前三类不得推导第四类。
- Commands: `npm ci --prefix docs/ui/prototypes`；`npm --prefix docs/ui/prototypes run validate`；`npm --prefix docs/ui/prototypes test`；`npm --prefix docs/ui/prototypes run snapshots:check`；`git diff --check`。
- Evidence: <attemptDir>/task-<N>-ui-documentation-html-prototypes.<ext> (attemptDir = currentAttemptDir from 'omo ulw-loop status --json', .omo/evidence/ulw/<session>/<goalId>/a<attempt>; outside ulw-loop use .omo/evidence/)

## Execution strategy
### Parallel execution waves
1. Wave 1：文档契约、工具链、设计系统。
2. Wave 2：应用壳与六个页面；六页在共享壳稳定后并行。
3. Wave 3：跨页流程、截图、核心文档集成。
4. Wave 4：四路最终审查。

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| 1 | - | 2,3 | - |
| 2 | 1 | 4-12 | 3 |
| 3 | 1 | 4-10 | 2 |
| 4 | 2,3 | 5-11 | - |
| 5-10 | 4 | 11,12 | 彼此并行 |
| 11 | 5-10 | 12 | - |
| 12 | 5-11 | 13 | - |
| 13 | 12 | F1-F4 | - |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [ ] 1. 建立 UI 设计评审契约与覆盖注册表
  What to do / Must NOT do: 新建 `docs/features/ui-documentation-system.md`（Draft）和 `docs/ui/README.md`，定义 CURRENT/TARGET/Prototype/Qt 四层证据、六页清单、页面/控件/流程稳定 ID 规则和安全禁区；只引用 REQ-001..006，不复制或修改状态。
  Parallelization: Wave 1 | Blocked by: - | Blocks: 2,3
  References: `AGENTS.md` 安全边界与开发流程；`docs/PRODUCT.md:161-203`；`docs/UI.md:27-166`；`docs/features/README.md:12-45`；`src/MainWindow/NavigationWidget.cpp:14-20`。
  Acceptance criteria: 文档明确“设计评审工件，不授权 Qt 实现”；注册表恰好声明六页；所有页面 ID 唯一；真实控制、外部通信、持久化均列为排除项。
  QA scenarios: 正常路径运行 `rg -n '^(状态|artifact-status): (Approved|Implemented)$|REQ-[0-9]+.*(Approved|Implemented)' docs/features/ui-documentation-system.md docs/ui/README.md` 并要求无匹配（退出 1）；失败路径把功能文档复制为 `${TMPDIR:-/tmp}/ui-doc-contract-negative.md`、追加 `状态: Implemented`，再对该临时文件运行同一正则并要求匹配（退出 0），随后删除临时文件；Evidence `<attemptDir>/task-1-ui-documentation-html-prototypes.txt`。
  Commit: Y | `docs(ui): establish draft documentation contract`

- [ ] 2. 建立隔离的 Node 与 Playwright 契约工具链
  What to do / Must NOT do: 在 `docs/ui/prototypes/` 添加锁定版本的 `package.json`/lockfile、Playwright 配置、静态服务、`scripts/validate-docs.mjs`、测试骨架和 manifest；在 `.gitignore` 仅放行 `docs/ui/prototypes/**/*.html`，忽略 node_modules/report/test-results。不得使用 `.opencode/package.json`。
  Parallelization: Wave 1 | Blocked by: 1 | Blocks: 4-12 | Can parallelize with: 3
  References: `.gitignore:34-36`；`docs/DEVELOPMENT.md:115-134`。
  Acceptance criteria: 校验器能拒绝缺页、坏链接、重复 ID、控件表与 `data-testid` 不一致、缺少 TARGET/模拟标识；网络请求在测试中被阻止。
  QA scenarios: `npm --prefix docs/ui/prototypes run validate` 正常通过；临时测试夹具故意制造重复 ID 后非零退出且随后清理；Evidence `<attemptDir>/task-2-ui-documentation-html-prototypes.txt`。
  Commit: Y | `docs(ui): add prototype validation toolchain`

- [ ] 3. 定义设计系统、信息密度和组件状态
  What to do / Must NOT do: 新建 `docs/ui/design-system.md`、`assets/css/tokens.css`、`components.css`，定义色彩、字体、间距、边框、图标、焦点、表格、Tab、按钮、输入、抽屉、对话框、状态条及 normal/hover/focus/selected/disabled/loading/empty/error；CURRENT 值与 TARGET 变更必须分栏。不得在组件 CSS 中散落未定义常量。
  Parallelization: Wave 1 | Blocked by: 1 | Blocks: 4-10 | Can parallelize with: 2
  References: `docs/UI.md:156-166`；`src/Common/GlobalStyle.cpp`；`include/Common/GlobalStyle.h`；旧 `docs/前端UI详细设计文档.md` 仅作来源，不作权威。
  Acceptance criteria: 每个原型组件可追踪到 token 和 Markdown 组件条目；颜色不是状态唯一线索；焦点可见；所有危险样式有文字限定。
  QA scenarios: Playwright 组件状态页检查 token 使用和可见焦点；故意缺 token 的测试夹具被校验器拒绝；Evidence `<attemptDir>/task-3-ui-documentation-html-prototypes.html`。
  Commit: Y | `docs(ui): define design system`

- [ ] 4. 定义并实现共享应用壳与交互规则
  What to do / Must NOT do: 新建 `docs/ui/application-shell.md`、`interaction-rules.md`、`prototypes/index.html`、`assets/css/shell.css`、`assets/js/app.js`、`fixtures.js`；实现六项导航、标题/会话区、主工作区、详情区、状态区、全局 TARGET/Draft/本地模拟标识、URL 页面状态和可复位本地固定数据。不得使用 localStorage、网络或后端。
  Parallelization: Wave 2 | Blocked by: 2,3 | Blocks: 5-11
  References: `docs/UI.md:27-48`；`docs/UI.md:70-105`；`docs/ARCHITECTURE.md:63-120`；`src/MainWindow/NavigationWidget.cpp:14-101`。
  Acceptance criteria: 六项导航顺序与 CURRENT 一致；刷新恢复确定性默认状态；Tab/对话框/抽屉支持键盘、Escape、焦点恢复和可访问名称；无紧急停止及空行为工具按钮。
  QA scenarios: `npm --prefix docs/ui/prototypes exec playwright test -- tests/shell.spec.js tests/interactions.spec.js` 覆盖点击、键盘和非法 URL 回落；Evidence `<attemptDir>/task-4-ui-documentation-html-prototypes.zip`。
  Commit: Y | `docs(ui): build application shell prototype`

- [ ] 5. 设计态势页面整体图与控件规格
  What to do / Must NOT do: 新建 `pages/situation.md`、页面模块和测试；包含模拟会话总览、态势画布、目标/任务/设备摘要、告警队列和活动时间线；可用控件仅限图层显示、视角复位、缩放、实体选择、筛选和详情抽屉，不提供设备命令。
  Parallelization: Wave 2 | Blocked by: 4 | Blocks: 11,12 | Can parallelize with: 6-10
  References: `docs/UI.md:95-105`；`docs/PRODUCT.md:58-75`；`docs/ARCHITECTURE.md:63-93`。
  Acceptance criteria: 文档区域图和控件表覆盖页面全部 `data-testid`；normal/loading/empty/error/disabled 可切换；选择实体只改变原型显示状态。
  QA scenarios: Playwright 点击实体打开详情；无实体时控件禁用并显示原因；Evidence `<attemptDir>/task-5-ui-documentation-html-prototypes.png`。
  Commit: Y | `docs(ui): specify situation page`

- [ ] 6. 设计探测页面整体图与控件规格
  What to do / Must NOT do: 新建 `pages/detection.md`、页面模块和测试；提供模拟目标表、搜索、类型/威胁/状态筛选、排序、证据 Tab、详情抽屉和“模拟确认”演示；不得声称控制传感器或调用 AI。
  Parallelization: Wave 2 | Blocked by: 4 | Blocks: 11,12 | Can parallelize with: 5,7-10
  References: `docs/UI.md:70-93`；`docs/PRODUCT.md:114-123`；`docs/ARCHITECTURE.md:94-120`。
  Acceptance criteria: 每个表头、筛选器、行操作和确认控件均有功能、状态、反馈、键盘和 CURRENT 映射；未选择目标时确认被拒绝并解释。
  QA scenarios: Playwright 搜索/筛选/选中/确认 happy path；无选择和空结果 failure path；Evidence `<attemptDir>/task-6-ui-documentation-html-prototypes.png`。
  Commit: Y | `docs(ui): specify detection page`

- [ ] 7. 设计决策页面整体图与控件规格
  What to do / Must NOT do: 新建 `pages/decision.md`、页面模块和测试；展示已选目标、依据、资源约束、候选方案比较、风险与原型内草案选择；只读决策支持，不提供执行/处置/下发按钮。
  Parallelization: Wave 2 | Blocked by: 4 | Blocks: 11,12 | Can parallelize with: 5,6,8-10
  References: `docs/UI.md:95-105`；`docs/PRODUCT.md:79-90,121-123`；`docs/ARCHITECTURE.md:122-132`。
  Acceptance criteria: 方案信息明确为模拟固定数据；选择方案只产生原型草案状态；无执行语义控件；无目标时呈现明确空状态。
  QA scenarios: Playwright 比较并选择草案；缺少目标时禁用且焦点不丢失；Evidence `<attemptDir>/task-7-ui-documentation-html-prototypes.png`。
  Commit: Y | `docs(ui): specify decision page`

- [ ] 8. 设计设备页面整体图与控件规格
  What to do / Must NOT do: 新建 `pages/devices.md`、页面模块和测试；展示模拟设备清单、类型/状态筛选、详情、可用性说明和“选择用于模拟指派”；不得出现驾驶、飞行、武器/载荷或设备控制台操作。
  Parallelization: Wave 2 | Blocked by: 4 | Blocks: 11,12 | Can parallelize with: 5-7,9,10
  References: `docs/UI.md:70-83,95-105`；`docs/PRODUCT.md:116-126,148-157`；`docs/ARCHITECTURE.md:85-93`。
  Acceptance criteria: 设备可用/忙碌/离线原因同时用文字和状态标识；不可用设备不能选择；全部交互有控件目录条目。
  QA scenarios: Playwright 选择可用设备；选择忙碌设备被拒绝并显示原因；Evidence `<attemptDir>/task-8-ui-documentation-html-prototypes.png`。
  Commit: Y | `docs(ui): specify devices page`

- [ ] 9. 设计统计页面整体图与控件规格
  What to do / Must NOT do: 新建 `pages/statistics.md`、页面模块和测试；只展示固定本地会话指标、图表/表格切换、当前会话筛选及状态示例；不得暗示历史数据库、导出、回放或持久化。
  Parallelization: Wave 2 | Blocked by: 4 | Blocks: 11,12 | Can parallelize with: 5-8,10
  References: `docs/PRODUCT.md:58-60,88`；`docs/UI.md:143-154`；`docs/DEVELOPMENT.md:126-134`。
  Acceptance criteria: 每个指标定义口径和固定数据来源；空/错误状态不伪造历史结果；图表有文字/表格等价信息。
  QA scenarios: Playwright 切换图表/表格并核对同值；空会话时显示空状态而非零值误导；Evidence `<attemptDir>/task-9-ui-documentation-html-prototypes.png`。
  Commit: Y | `docs(ui): specify statistics page`

- [ ] 10. 设计配置页面整体图与控件规格
  What to do / Must NOT do: 新建 `pages/configuration.md`、页面模块和测试；显示只读运行/模拟事实和临时展示选项（密度、对比度、评审布局），刷新即复位；不得提供保存/应用、凭据、集成、场景重置或持久配置。
  Parallelization: Wave 2 | Blocked by: 4 | Blocks: 11,12 | Can parallelize with: 5-9
  References: `docs/UI.md:52-68,143-166`；`docs/PRODUCT.md:124-126,187-203`；`docs/ARCHITECTURE.md:36-61`。
  Acceptance criteria: 临时设置均标明“仅原型预览”；刷新恢复默认值；不存在密码/API key/网络端点字段。
  QA scenarios: Playwright 修改密度和对比度后验证视觉状态；刷新后复位；扫描无 credential/integration 控件；Evidence `<attemptDir>/task-10-ui-documentation-html-prototypes.png`。
  Commit: Y | `docs(ui): specify configuration page`

- [ ] 11. 定义关键跨页模拟流程与拒绝路径
  What to do / Must NOT do: 新建 `workflows/target-review.md`、`simulated-assignment.md`、`simulated-execution.md`、`exceptions.md` 和流程测试；每步记录来源页、控件 ID、状态变化、可见反馈、焦点和本地日志。流程只演示目标审阅、候选比较、模拟设备资格/草案指派和模拟进度，不建立真实业务引擎。
  Parallelization: Wave 3 | Blocked by: 5-10 | Blocks: 12
  References: `docs/PRODUCT.md:62-77,174-193`；`docs/UI.md:115-154`；`docs/ARCHITECTURE.md:134-181`。
  Acceptance criteria: happy path 与无目标、无可用设备、非法指派、冲突状态均可由固定 fixture 重现；页面间状态不矛盾；文档明确对应需求仍为 Draft。
  QA scenarios: `npm --prefix docs/ui/prototypes exec playwright test -- tests/workflows.spec.js` 执行成功和拒绝流程；Evidence `<attemptDir>/task-11-ui-documentation-html-prototypes.zip`。
  Commit: Y | `docs(ui): document simulated review workflows`

- [ ] 12. 生成六张确定性整体截图并验证多尺寸布局
  What to do / Must NOT do: 为六页各生成一张 `images/<page>/overview-1920x1080.png`，manifest 记录页面/URL/状态/视口/选择器；固定 Chromium、zh-CN、UTC、DPR=1、时钟、数据、字体和动画。1280×720 与 3840×2160 只做溢出和遮挡检查，失败截图进入忽略的测试证据，不提交额外 golden。
  Parallelization: Wave 3 | Blocked by: 5-11 | Blocks: 13
  References: `docs/UI.md:48,143-166`；`docs/DEVELOPMENT.md:115-134`。
  Acceptance criteria: 恰好六张权威 PNG，均被对应 Markdown 引用；连续两次生成 SHA-256 相同；三个视口无非预期滚动、遮挡或文字溢出；不声称验证 Qt。
  QA scenarios: `npm --prefix docs/ui/prototypes run snapshots:check` 正常通过；修改 fixture 后检测到 mismatch；Evidence `<attemptDir>/task-12-ui-documentation-html-prototypes.txt`。
  Commit: Y | `docs(ui): add canonical page snapshots`

- [ ] 13. 集成核心文档入口并完成文档门禁
  What to do / Must NOT do: 在保留现有 hunks 的前提下，为 `docs/UI.md` 添加详细规格入口和“六个 Qt 页面尚未实现”声明，为 `docs/DEVELOPMENT.md` 添加文档验证命令；需要时只在 `docs/features/README.md` 添加索引。不得改 PRODUCT 需求状态或 ARCHITECTURE 实现事实。
  Parallelization: Wave 3 | Blocked by: 12 | Blocks: F1-F4
  References: `docs/UI.md:6-12,128-166`；`docs/DEVELOPMENT.md:104-160`；`docs/features/README.md:1-45`；初始 `git status --short`。
  Acceptance criteria: 从 `docs/UI.md` 可到达所有页面、流程、原型和图片；所有相对链接有效；`git diff` 显示用户既有修改未被回退；文档明确测试证明范围。
  QA scenarios: `npm --prefix docs/ui/prototypes run validate`、`git diff --check`、逐段 `read` CURRENT/TARGET 用词；Evidence `<attemptDir>/task-13-ui-documentation-html-prototypes.md`。
  Commit: Y | `docs(ui): integrate detailed design documentation`

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [ ] F1. Plan compliance audit
  Tool/steps: Oracle 只读审查本计划和最终 diff，并运行 `npm --prefix docs/ui/prototypes run validate`；逐项核对 13 个 todo 的交付物、Must have/Must NOT have 和六页覆盖注册表。判定：命令退出 0 且没有缺失页面/控件/流程 ID、没有未完成 todo 才可返回 `APPROVE`，否则返回 `REJECT` 和文件位置。
- [ ] F2. Code quality review
  Tool/steps: Oracle 读取全部新增 HTML/CSS/JS/Node 脚本，运行 `npm --prefix docs/ui/prototypes test` 和 `npm --prefix docs/ui/prototypes run snapshots:check`；检查重复 token、不可访问控件、非确定性、网络/存储调用和误导性文案。判定：两命令退出 0 且无高/中严重度问题才可 `APPROVE`。
- [ ] F3. Real manual QA
  Tool/steps: 加载 `visual-qa` 与 `playwright`，实际打开六页，在 1280×720、1920×1080、3840×2160 执行主鼠标/键盘路径并保存证据截图；检查空白画布、溢出、遮挡、焦点、中文裁切和全局/动作模拟标识。判定：六页三视口均可操作且无上述缺陷才可 `APPROVE`。
- [ ] F4. Scope fidelity
  Tool/steps: Oracle 运行 `git status --short`、`git diff --name-only`、`git diff --check`，读取 `git diff -- docs/PRODUCT.md docs/ARCHITECTURE.md docs/UI.md docs/DEVELOPMENT.md docs/features/README.md` 并与初始脏路径对照。判定：无 C++/Qt/CMake 改动、无需求状态变化、无真实控制语义、无用户 hunk 回退且 diff-check 退出 0 才可 `APPROVE`；任一不满足即 `REJECT`。

## Commit strategy

- 只在用户明确要求提交后创建 commit；默认仅保留工作树改动。
- 工具链、设计系统/应用壳、每个页面、流程、截图和基线集成分别形成原子提交候选。
- 对当前已修改的核心文档使用逐 hunk 审查，只包含本任务新增内容，不重写用户现有修改。

## Success criteria

- 六个页面均有完整 1920×1080 整体图、可打开原型和逐控件规格。
- 页面内所有交互元素都有唯一 ID，且 Markdown、HTML 与 Playwright 三方覆盖一致。
- 关键正常与拒绝流程可用固定本地数据演示；页面和动作明确标注 TARGET/Draft/本地模拟。
- 六张权威截图可确定性复现；三种视口无布局溢出或文本遮挡。
- 所有链接、manifest、控件覆盖与安全禁区自动校验通过。
- 不修改 Qt/C++，不改变 REQ 状态，不连接外部系统，不覆盖用户既有改动。

---

## 实际完成情况（2026-07-23）

> 用户中途将范围从原 13 项 todo（含 Playwright 契约测试、验证工具链、跨页流程、多视口检查）收窄为"方案 B：提纲挈领"--以最短路径产出六页 HTML 原型 + 六张 1920×1080 截图 + 一份态势页详细清单 + 六页一览规格。原 todo 1-13 中绝大部分子项未按原规格执行，下面仅记录实际交付物，不勾选原 checkbox。

### 已交付

- 六页 HTML 原型：`docs/ui/prototypes/{situation,detection,decision,devices,statistics,configuration}/index.html`（单文件内联 CSS+JS，本地模拟 fixture）。
- 六张 1920×1080 权威截图：`docs/ui/images/<page>/overview-1920x1080.png`，全部通过 `look_at` 视觉验证。
- 态势页详细控件清单：`docs/ui/pages/situation.md`（648 行，逐控件规格）。
- 六页一览表：`docs/ui/pages/index.md`（每页一段：定位、关键区域、控件 ID 前缀、原型路径、截图路径）。
- 通用设计系统：`docs/ui/design-system.md`（标题与前言已改为六页通用，去掉了"态势页面"前缀）。
- 通用应用壳规格：`docs/ui/application-shell.md`（前言已改为六页通用，去掉了"态势页面试点使用"表述）。
- 总览入口：`docs/ui/README.md` 重写为六页总览，含范围表、证据边界、安全边界、ID 约定、状态词汇、三视口规则、后续任务与完成条件。
- 上级链接修复：`pages/situation.md` 第 4 行相对路径 `README.md` -> `../README.md`。
- `.gitignore` 新增规则：`docs/ui/prototypes/*/node_modules/` 与 `package-lock.json`。
- 截图脚本：`prototypes/situation/screenshot.js`（单页）与 `screenshot-all.js`（六页批量），临时 `screenshot-fix.js` 已移除。

### 未执行（按原计划属后续任务）

- Task 2：隔离 Node + Playwright 契约工具链（`scripts/validate-docs.mjs`、契约测试骨架、manifest）。
- Task 3 子项：`assets/css/tokens.css`、`components.css` 拆分（当前 token 内联在 HTML 与 `design-system.md` 中）。
- Task 4 子项：`prototypes/index.html` 共享壳、`interaction-rules.md`、`assets/js/app.js`、`fixtures.js`。
- Task 6-10 子项：探测/决策/设备/统计/配置五页的 `pages/<page>.md` 详细逐控件清单（当前只有 `pages/index.md` 一览规格 + HTML 原型）。
- Task 11：跨页流程文档（`workflows/*.md`）与流程测试。
- Task 12 子项：1280×720 与 3840×2160 视口检查、SHA-256 确定性校验、manifest。
- Task 13：`docs/UI.md` 与 `docs/DEVELOPMENT.md` 的入口集成与文档门禁命令。
- Final verification wave F1-F4：未执行。

### 安全与边界遵守

- 未修改 C++/Qt/CMake，未改动 6 个用户预存脏文件（`AGENTS.md`、`docs/ARCHITECTURE.md`、`docs/DEVELOPMENT.md`、`docs/PRODUCT.md`、`docs/UI.md`、`docs/features/README.md`）。
- 未连接真实设备、未实现排爆动作、未写入数据库、未接入外部通信。
- 所有原型操作均标注"模拟"或"演示"；危险控件（紧急停止、设备控制、排爆执行）在原型中禁用或省略。
- node_modules 与 package-lock 已通过 `.gitignore` 排除，不会进入提交。

### 用户当前状态

用户已批准"方案 B"文档整改，对六页原型与截图表示满意。后续是否补做上述未执行项，等待用户决策。本轮工作无进行中的子任务、构建、测试或 GUI 进程。

