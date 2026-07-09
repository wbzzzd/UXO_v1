# OMO 适配计划

状态：`[规划文档已完成]` `[仓库级适配已完成]` `[全局 OMO 安装未开始]`

本文档供后续继续开发 UXO_v1 的 agent 使用。在开始 MVP 功能开发之前，先按本文档完成仓库级 opencode + oh-my-openagent 适配。目标是在保留 opencode 原生兼容性的同时，让项目具备稳定的项目记忆、安全边界和可复用工作流。

## 目的

状态：`[已完成]`

让后续开发可以通过 opencode 和 oh-my-openagent 进行，并且具备：

- 稳定的项目级上下文。
- 清晰的安全边界。
- 可重复执行的命令。
- 可交接、可恢复的计划机制。

本文档只覆盖仓库级适配。不覆盖全局 oh-my-openagent 安装、provider 账号配置、Team Mode 调优、MCP 深度接入或 MVP 功能实现。

## 当前仓库事实

状态：`[已完成]`

- 项目根目录：`/home/ubuntu/UXO_v1`。
- 当前应用：Qt 5 / CMake / C++17 桌面客户端，程序名为 `UXOMissionControl`。
- 当前实际纳入构建的代码主要集中在 `src/App/`、`src/MainWindow/`、`include/App/`、`include/MainWindow/` 和 `include/Core/Data/Types.h`。
- 现有文档包括 `README.md`、`PROJECT_STRUCTURE.md`、`SRS排弹抢修指挥系统_v1.0.md`、`SDD排弹抢修指挥系统_v1.0.md`、UI 设计文档和 `docs/前端UI详细设计文档.md`。
- `docs/快速开始.md` 是前期粗略添加的启动说明，后续适配时应删除、替换或拆分。不要把它作为长期文档模型。
- `PROJECT_STRUCTURE.md`、SRS 和 SDD 描述的是目标架构，不代表其中每个模块都已经在代码中实现。
- `.omo/` 当前包含已版本化的规则、计划、模板目录；`run-continuation/` 和 `ralph-loop.local.md` 是运行态数据，已通过 `.gitignore` 忽略。
- `CMakeLists.txt` 存在用户已有修改。除非用户明确要求，否则不要还原、覆盖或纳入无关提交。

## 关键安全警告

状态：`[已识别]` `[配置已脱敏]` `[用户当前选择暂不轮换旧 key]`

`opencode.json` 曾经包含明文 provider API key，当前已改为环境变量引用。后续 agent 不得复制、打印、引用、写入文档或提交任何密钥值。

在提交任何 opencode 配置变更前，必须完成：

1. 尊重用户当前决策：暂不轮换旧 provider key。
2. 保持 `opencode.json` 使用环境变量引用，例如 `{env:TENCENT_MAAS_API_KEY}`。
3. 检查暂存 diff，确认没有任何密钥材料。
4. 禁止提交 `.env` 文件。只允许使用 `.env.example` 记录变量名。

如果后续 agent 在工具输出中看到了密钥值，不得在聊天、文档、提交信息、issue 或 PR 中复述。

## 目标文档机制

状态：`[已规划]` `[已实施]`

采用分层文档模型：

1. `README.md`：面向人的简短入口和导航，不承载长篇开发规则。
2. `AGENTS.md`：opencode 和 oh-my-openagent agent 的常驻项目规则。
3. `opencode.json`：项目配置、instructions、references 和权限边界。
4. `docs/dev/`：当前工程事实、MVP 边界、构建运行说明和安全规则。
5. `.opencode/commands/`：可重复执行的项目级 slash commands。
6. `.opencode/skills/`：基础机制稳定后再添加的懒加载项目技能。
7. `.omo/`：仅存放项目级 OMO 规则、计划模板和未来计划文件。运行态数据必须忽略。

## 目标文件布局

状态：`[已规划]` `[已创建]`

适配阶段创建或更新以下文件：

```text
AGENTS.md
opencode.json
.gitignore
.opencode/commands/analyze-current.md
.opencode/commands/plan-mvp.md
.opencode/commands/verify-build.md
.opencode/commands/review-safety.md
.omo/rules/project-safety.md
.omo/rules/documentation-truth.md
.omo/plans/README.md
.omo/templates/mvp-plan-template.md
docs/dev/current-state.md
docs/dev/build-and-run.md
docs/dev/mvp-scope.md
docs/dev/architecture-boundaries.md
docs/dev/simulation-policy.md
docs/dev/decisions/0001-opencode-omo-workflow.md
```

如果用户后续缩小范围，不要盲目创建所有可选文件。最小可用集合是：`AGENTS.md`、已脱敏的 `opencode.json`、`docs/dev/current-state.md`、`docs/dev/build-and-run.md`、`docs/dev/mvp-scope.md` 和 `.opencode/commands/verify-build.md`。

## 非目标

状态：`[已定义]`

- 不在本适配任务中实现 MVP 功能。
- 不接入真实设备、真实通信、数据库或安全关键动作。
- 未经源码验证，不假设 SRS/SDD 中的模块已经存在。
- 除非用户明确要求，不在本仓库任务中安装全局 oh-my-openagent。
- 暂不调优 provider 模型、Team Mode、MCP server 或长循环任务。
- 不触碰无关用户改动，例如已有的 `CMakeLists.txt` 修改。

## 有序工作阶段

状态：`[已规划]` `[已完成]`

### 阶段 1：基线盘点

状态：`[已完成]`

目标：在编辑前确认真实当前状态。

操作：

- 检查 `git status --short`。
- 阅读 `README.md`、`opencode.json`、`.gitignore`、`CMakeLists.txt`、`scripts/build.sh`、`src/App/main.cpp`、`src/App/Application.cpp` 和现有文档。
- 确认 `docs/快速开始.md` 是否仍然存在，以及应删除、替换还是拆分。
- 记录哪些文件是用户已有改动。

验收标准：

- 后续编辑不会影响无关脏文件。
- 当前源码事实来自实际文件，而不是记忆或推测。
- 已识别 `opencode.json` 中的密钥问题，但没有复制密钥值。

### 阶段 2：密钥安全的 opencode 配置

状态：`[已完成]`

目标：让 `opencode.json` 可提交、可维护，并适合项目开发。

操作：

- 将明文 provider key 替换为环境变量引用。
- 添加或保留 `$schema`。
- 在相关文档存在后添加 `instructions`。
- 考虑为 SRS、SDD 和 UI 设计文档添加 `references`。
- 对破坏性 shell 命令和密钥文件保持保守权限。

验收标准：

- `git diff -- opencode.json` 不包含明文密钥。
- 配置仍然是有效 JSON。
- 不依赖 OMO 特有能力时，项目仍可被 opencode 原生打开。

### 阶段 3：项目 Agent 规则

状态：`[已完成]`

目标：创建根目录 `AGENTS.md`，让 opencode 原生 agent 和 OMO agent 都能遵守。

必须覆盖：

- 项目身份和当前实现状态。
- 安全边界：设备和危险动作默认使用模拟。
- 文档真实性规则：源码事实优先于目标设计文档。
- MVP 优先级：先做可见的指挥席闭环，再扩展大架构。
- 构建验证命令。
- Git 和脏工作区纪律。
- provider key 和环境文件的密钥处理规则。

验收标准：

- `AGENTS.md` 足够简洁，适合常驻上下文。
- 明确告诉 agent 不能假设什么。
- 引导后续工作先计划再实现。

### 阶段 4：当前事实文档

状态：`[已完成]`

目标：把分散文档整理成小型 `docs/dev/` 知识库。

创建：

- `docs/dev/current-state.md`：当前代码真实实现到哪里。
- `docs/dev/build-and-run.md`：已验证的配置、构建和运行命令。
- `docs/dev/mvp-scope.md`：MVP 目标、非目标和模拟优先规则。
- `docs/dev/architecture-boundaries.md`：App、MainWindow、Core、未来 Modules、未来 Drivers 的边界。
- `docs/dev/simulation-policy.md`：模拟数据与真实设备集成必须如何隔离。

验收标准：

- 这些文档明确区分当前状态和目标状态。
- 不大段复制 SRS/SDD。
- 足够短，便于 agent 快速读取。

### 阶段 5：OMO 项目结构

状态：`[已完成]`

目标：准备仓库级 OMO 使用方式，同时避免提交运行态状态。

操作：

- 添加 `.omo/rules/` 存放项目专用规则。
- 添加 `.omo/plans/README.md`，说明计划文件命名和评审方式。
- 如有必要，添加 `.omo/templates/` 存放可复用计划模板。
- 更新 `.gitignore`，忽略 `.omo/run-continuation/`、循环状态、session 日志和缓存等运行态文件。

验收标准：

- `.omo/run-continuation/` 未被暂存。
- 项目规则与运行态产物分开版本化。
- 后续 agent 知道 MVP 计划文件应放在哪里。

### 阶段 6：opencode 命令

状态：`[已完成]`

目标：把高频工作流显式化、命令化。

创建初始命令：

- `.opencode/commands/analyze-current.md`：只分析代码和文档差距，不编辑。
- `.opencode/commands/plan-mvp.md`：基于当前事实和模拟优先约束生成 MVP 计划。
- `.opencode/commands/verify-build.md`：运行已验证的 CMake 配置和构建命令。
- `.opencode/commands/review-safety.md`：检查真实设备命令风险、密钥泄漏和文档真实性问题。

验收标准：

- 命令使用 opencode 原生命令格式。
- 命令不依赖全局 OMO 安装。
- 命令强化先计划后实现的工作流。

### 阶段 7：快速开始文档清理

状态：`[已完成]`

目标：用新的文档机制替换粗略的快速开始说明。

操作：

- 决定删除 `docs/快速开始.md`，或把其中有用事实合并到 `docs/dev/build-and-run.md`。
- 更新 `README.md`，指向新的文档入口。
- 避免在多个地方重复维护构建说明，除非明确指定一个 canonical 文件。

验收标准：

- 只有一个权威构建/运行文档。
- README 是导航页，不是长篇指南。
- 没有文档把目标架构写成当前实现。

### 阶段 8：交接与验证

状态：`[已完成]`

目标：留下干净、可恢复的状态，供后续 MVP 规划使用。

操作：

- 运行当前环境可用的文档检查。
- 只有在相关文件变更或用户要求时，运行 CMake 配置/构建。
- 检查 `git diff` 和 `git status --short`。
- 确认 tracked diff 中没有密钥值。
- 只有在文档机制被接受后，才在 `.omo/plans/` 下创建或更新 MVP 计划文件。

验收标准：

- 所有预期适配文件存在。
- 运行态 `.omo` 文件被忽略或未暂存。
- `opencode.json` 已密钥安全。
- 下一个 agent 可以从 `AGENTS.md` 和 `docs/dev/` 开始 MVP 规划。

## 验证门禁

状态：`[已定义]` `[本轮已执行]`

编辑前：

- 验证工作区状态。
- 验证 `opencode.json` 是否仍包含明文 key。
- 验证 `.omo/run-continuation/` 是 tracked 还是 untracked。

提交或交接前：

- 运行 `git diff --check`。
- 做密钥导向的 diff review：检查暂存和未暂存 diff 是否包含 provider key 形态或明文 API key 字符串。
- 确认 `.env` 文件没有被暂存。
- 确认无关 `CMakeLists.txt` 改动没有被纳入。
- 确认 README 链接指向真实文件。
- 确认 opencode command 文件使用有效 frontmatter。

必要时使用的构建验证命令：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

## 原子提交策略

状态：`[已定义]` `[未执行]`

除非用户明确要求，否则不要提交。若用户要求提交，应使用小而可审查的提交：

1. `[已完成待提交]` `docs: add OMO adaptation plan`
   - 只包含本文档。
2. `[已完成待提交]` `chore: sanitize opencode project config`
   - 已将 provider key 配置迁移为环境变量引用；用户当前选择暂不轮换旧 key。
3. `[已完成待提交]` `docs: add agent project rules`
   - 添加 `AGENTS.md` 和核心 `docs/dev/` 文件。
4. `[已完成待提交]` `chore: add OMO project structure`
   - 添加 `.omo/rules/`、`.omo/plans/README.md`、模板和 `.gitignore` 更新。
5. `[已完成待提交]` `chore: add opencode workflow commands`
   - 添加 `.opencode/commands/` 文件。
6. `[已完成待提交]` `docs: replace quick-start with canonical dev docs`
   - 删除或拆分 `docs/快速开始.md`，并更新 README 链接。

每次提交前必须检查 staged diff。除非用户明确纳入提交范围，否则不要包含当前 `CMakeLists.txt` 用户改动。

## 后续 Agent 交接清单

状态：`[已定义]`

- `[待执行]` 先读本文档。
- `[待执行]` 如果 `AGENTS.md` 已存在，读取它。
- `[待执行]` 读取 `opencode.json`，但不得复述密钥值。
- `[待执行]` 编辑前检查 `git status --short`。
- `[持续遵守]` 将 SRS/SDD 当作目标需求，不当作已实现代码。
- `[持续遵守]` 将全局 OMO 安装与仓库级适配分开。
- `[持续遵守]` 不提交 `.omo/run-continuation/` 和其他运行态文件。
- `[持续遵守]` 提交、安装全局工具或触碰真实设备流程前必须询问用户。

## 下一步建议

状态：`[仓库级适配已完成]` `[MVP 计划已创建]` `[等待用户确认执行]`

本轮已经完成阶段 1 到阶段 8 的仓库级适配，并创建 `.omo/plans/20260707-mvp-command-seat.md` 作为 MVP 指挥席闭环计划。下一个 agent 应先读取 `AGENTS.md`、`docs/dev/`、`.omo/plans/README.md` 和该 MVP 计划文件，再等待用户确认是否进入实现。

用户当前选择暂不轮换旧 key；后续运行 opencode 时，本机仍需设置 `TENCENT_MAAS_API_KEY` 环境变量。
