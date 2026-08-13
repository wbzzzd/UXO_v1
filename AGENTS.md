# 排弹抢修指挥系统 Agent 规则

本文件是 UXO_v1 的项目级 agent 规则。opencode 原生 agent、oh-my-openagent agent 和后续子代理都必须优先遵守。

## 项目定位与文档入口

- 本仓库交付 Qt 5 / CMake / C++17 桌面客户端 `UXOMissionControl`，不交付完整外部设备和安全执行系统。
- `docs/PRODUCT.md`、`docs/ARCHITECTURE.md`、`docs/UI.md`、`docs/DEVELOPMENT.md` 是当前核心基线，分别负责产品、架构、界面和工程事实。
- CURRENT 实现事实以源码、CMake 和实际验证结果为准；具体模块及启动过程见 `docs/ARCHITECTURE.md`。
- SRS、SDD、旧 UI 设计和功能草案仅作为来源资料；`docs/sources/` 收录上游来源，`docs/prd/` 收录 Draft PRD 需求定义，`docs/architecture/` 收录从 ARCHITECTURE.md 拆出的架构细节与历史架构草稿，`docs/ddr/` 收录历史决策记录，已被核心基线取代的工程快照归档于 `docs/archive/development/`；与核心基线冲突时不得直接指导实现。
- 当前 NEXT 仍是草稿，完成对应设计评审并获得用户确认前不得实施。

## 安全边界

- 真实设备控制、排弹动作、外部通信命令、数据库写入和安全关键动作默认禁止实现或执行。
- 未经用户明确授权，只能使用本地模拟数据、只读分析或接口占位。
- 不要把模拟状态写成真实设备状态；文档和 UI 都必须明确区分模拟、占位和真实接入。
- `opencode.json` 只能使用环境变量引用 provider key。不得复制、打印、提交或复述任何明文密钥。
- `.env`、密钥、凭据文件不得提交；只允许提交 `.env.example` 这类变量名示例文件。

## 开发流程

- `AGENTS.md` 是默认常驻上下文；不要在每次任务中无差别读取全部核心文档。
- 产品范围、需求或路线任务：读取 `docs/PRODUCT.md`。
- 状态、模块、依赖或业务逻辑任务：读取相关需求及 `docs/ARCHITECTURE.md`、`docs/DEVELOPMENT.md`。
- 页面或交互任务：读取相关需求及 `docs/UI.md`、`docs/DEVELOPMENT.md`；涉及状态边界时再读 `docs/ARCHITECTURE.md`。
- 构建、测试、发布或工程流程任务：读取 `docs/DEVELOPMENT.md`。
- 功能开发：读取获批的 `docs/features/<feature>.md` 和它明确引用的核心文档章节，不遍历全部历史资料。
- `docs/PRODUCT.md` 第 9 节是项目唯一需求注册表与状态权威，唯一负责分配 `REQ-NNN` ID 和 `Draft`/`Approved`/`Implemented`/`Superseded` 状态；其他文档只能引用需求 ID，不得创建或重编号需求，也不得复制或修改需求状态。
- 功能开发与执行计划必须同时满足两项前提：关联的 `REQ-NNN` 在 `PRODUCT.md` 中状态为 `Approved`，且对应 `docs/features/<feature>.md` 自身状态为 `Approved`；`Draft` 需求或 `Draft` 功能设计不得进入实现计划。
- `TARGET`、`NEXT 草稿` 和归档资料不得直接触发实现。
- 多文件或架构性修改必须先计划，得到用户确认后再实现。
- 修改前检查 `git status --short`，不要还原、覆盖或提交用户已有改动。
- 只操作自己负责的 worktree 和分支；`main` 只接受经审核的 GitHub Pull Request，具体流程见 `docs/DEVELOPMENT.md`。
- CURRENT 事实以源码和实际构建为准；产品范围、目标架构、UI 规则和工程门禁分别以四份核心文档为准。
- 代码变更必须写中文注释；按任务粒度及时提交 git，commit message 须说明变更内容与目的。
- 每次工作会话保存记录并附总结；任务完成后在对应计划文档更新完成状态。
- 委托的子任务或 subagent 失败时，必须立即停止该执行通道并告知用户，不得自行接手执行；先检查原 session 和工作区已有进度，再续跑最小剩余步骤。

## 子代理可靠性协议

- 默认使用子代理分担实现和验证；互不冲突的文件组允许并行写入，单批建议 2–4 个独立通道，并为每个通道保留独立 `session_id`。
- 单个同步委托只包含一个文件组或一次只读审查，不得在同步 `task()` 中执行构建、CTest、截图等长命令；目标在 90 秒左右返回。
- 实现、构建、测试、截图和审查必须拆成独立阶段，状态依次记录为 `WORKING`、`ARTIFACT_READY`、`VERIFIED` 或 `FAILED`。
- 构建、测试、截图等预计超过 60 秒的命令，由子代理启动带硬超时的 tmux/后台进程并立即返回 session、PID、日志和证据路径；主 agent 每 30–60 秒轮询并向用户汇报。
- 不得把多个长同步任务放在同一次并行调用中；并行通道必须能够独立完成、独立失败和独立续跑，避免一个返回流错误取消整批任务。
- 若当前工具无法提供可观察后台会话，必须在启动前继续拆分，直到每个同步步骤只执行一个短动作。
- 90 秒没有新回传时，先用 session 状态、会话记录和 `git diff` 判断真实进度，不得直接启动重复任务；已完成的并行通道继续保留，不因单个通道失败而整体重做。
- 遇到 `stream_read_error`、`Tool execution aborted` 或返回流中断时，保留已完成改动和证据，复用原 `session_id`，只续跑未完成的最小验证或汇报步骤。
- 每个步骤完成后立即记录测试结果和证据路径；后续步骤只消费已落盘证据，不重复执行已确认工作。
- 最终回复前确认没有运行中的子任务、构建、测试或 GUI 进程，并明确报告被跳过或仍未验证的门禁。

## 构建验证

优先使用以下命令验证当前客户端构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

`scripts/build.sh` 也可用，但末尾有交互式安装确认，不适合作为自动化首选。

## 文档纪律

- `README.md` 只做入口导航。
- 四份核心文档是默认开发入口；已被核心基线取代的工程快照移至 `docs/archive/development/`，上游来源资料归入 `docs/sources/`，Draft PRD 需求定义归入 `docs/prd/`，ARCHITECTURE.md 架构细节与历史架构草稿归入 `docs/architecture/`，历史决策记录归入 `docs/ddr/`。
- `docs/features/` 只放功能增量设计；`Draft` 文档不能直接指导实现。
- `docs/archive/` 与 `docs/research/` 不进入默认开发上下文，仅在追溯历史或验证依据时读取。
- `.omo/rules/` 放项目级 OMO 规则；`.omo/plans/` 放可评审计划；`.omo/run-continuation/` 等运行态数据不得提交。
- 产品范围变更更新 `PRODUCT.md`；模块或状态边界变更更新 `ARCHITECTURE.md`；页面与交互变更更新 `UI.md`；构建、测试和完成门禁变更更新 `DEVELOPMENT.md`。

## 禁止事项

- 不要把 SRS/SDD 中尚未实现的模块写成当前代码事实。
- 不要为了让检查通过而删除失败测试或弱化验证。
- 不要使用破坏性 git 命令，除非用户明确要求。
- 不要主动提交或推送；提交必须由用户明确要求。
