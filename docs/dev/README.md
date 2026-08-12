# docs/dev 保留草稿索引

本文分类列出 `docs/dev/` 保留的全部文件与子目录。本目录所有内容均为历史决策、`Draft` 草稿、来源或设计输入，不得作为当前实现依据。已归档文件的分类和历史路径见 [`../archive/catalog.md`](../archive/catalog.md)。

> 原合并 PRD 已归档并拆分为 [`prd-mos.md`](prd-mos.md) 与 [`prd-uxr.md`](prd-uxr.md)。旧核心基线不再保留在本目录，归档路径见第 4 节。

关联基线：[`../PRODUCT.md`](../PRODUCT.md) · [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../UI.md`](../UI.md) · [`../DEVELOPMENT.md`](../DEVELOPMENT.md) · [`../../AGENTS.md`](../../AGENTS.md)

## 1. 已接受的历史工作流决策

| 路径 | 说明 |
|------|------|
| [`decisions/`](decisions/) | 历史决策记录目录 |
| [`decisions/0001-opencode-omo-workflow.md`](decisions/0001-opencode-omo-workflow.md) | 采用 opencode + oh-my-openagent 工作流的决策，状态"已接受"。当前工作流权威是 [`../../AGENTS.md`](../../AGENTS.md) 与 `.opencode/`、`.omo/rules/` 配置；本文件保留为历史依据，不再单独驱动实现 |

## 2. UXR/MOS 草稿与图（Draft/来源/设计输入）

以下文件均为 `Draft` 或来源/设计输入，不得直接指导实现。原合并增量 PRD 已拆分为独立的 MOS PRD 与 UXR PRD。

| 文件 | 说明 |
|------|------|
| [`prd-mos.md`](prd-mos.md) | MOS 规划 PRD 草稿（从原合并 PRD 拆分，`Draft`/来源资料） |
| [`prd-uxr.md`](prd-uxr.md) | 未爆弹识别 PRD 草稿（从原合并 PRD 拆分，`Draft`/来源资料） |
| [`architecture-mos.md`](architecture-mos.md) | MOS 架构草稿（`Draft`/来源资料） |
| [`architecture-uxr.md`](architecture-uxr.md) | UXR 架构草稿（`Draft`/来源资料） |
| [`sequence-diagram-mos.mermaid`](sequence-diagram-mos.mermaid) | MOS 时序图草稿 |
| [`sequence-diagram-recognition.mermaid`](sequence-diagram-recognition.mermaid) | 识别时序图草稿 |

MOS 功能设计见 [`../features/mos-planning.md`](../features/mos-planning.md)（P0 `Implemented`；P1/P2 `Draft`），关联需求 REQ-007 见 [`../PRODUCT.md`](../PRODUCT.md) 第 9.3 节（P0 `Implemented`；P1/P2 `Draft`）。P0 于 2026-08-03 获批，并于 2026-08-04 完成实现与验证；该事实不授权真实接入。来源 PRD [`prd-mos.md`](prd-mos.md) 与架构草稿 [`architecture-mos.md`](architecture-mos.md) 仍保持 `Draft`/来源资料，不直接指导后续实现。MOS/UXR 架构草稿分别归入各自功能的评审链路。

## 3. UI 设计参考（已归档）

以下来源文档已不在 `docs/dev/`，仅保留在归档区供追溯。当前 UI 设计以 [`../UI.md`](../UI.md) 和 [`../ui/pages/decision.md`](../ui/pages/decision.md) 为准。

| 文件 | 说明 |
|------|------|
| [`../archive/ui/mos-ui-design-brief.md`](../archive/ui/mos-ui-design-brief.md) | MOS UI 设计简报，历史来源 |
| [`../archive/ui/product-design-mos.md`](../archive/ui/product-design-mos.md) | MOS 产品设计，历史来源 |

## 4. 已归档的旧基线

以下五份旧基线已移入 `docs/archive/core-baselines/`，不再属于 `docs/dev/` 当前来源集合。它们仅用于追溯，当前工程事实和门禁以四份核心基线、源码和实际验证为准。

| 文件 | 说明 |
|------|------|
| [`../archive/core-baselines/current-state.md`](../archive/core-baselines/current-state.md) | 旧工程事实快照，已被 [`../ARCHITECTURE.md`](../ARCHITECTURE.md) 取代 |
| [`../archive/core-baselines/build-and-run.md`](../archive/core-baselines/build-and-run.md) | 旧构建说明，已被 [`../DEVELOPMENT.md`](../DEVELOPMENT.md) 取代 |
| [`../archive/core-baselines/mvp-scope.md`](../archive/core-baselines/mvp-scope.md) | 旧 MVP 范围快照，已被 [`../PRODUCT.md`](../PRODUCT.md) 取代 |
| [`../archive/core-baselines/architecture-boundaries.md`](../archive/core-baselines/architecture-boundaries.md) | 旧架构边界快照，已被 [`../ARCHITECTURE.md`](../ARCHITECTURE.md) 取代 |
| [`../archive/core-baselines/simulation-policy.md`](../archive/core-baselines/simulation-policy.md) | 旧模拟策略快照，安全边界以 [`../../AGENTS.md`](../../AGENTS.md) 为准 |

## 5. 实现前置条件

任何 UXR/MOS 草稿或图要进入实现，必须同时满足两项前提：关联的 `REQ-NNN` 在 [`../PRODUCT.md`](../PRODUCT.md) 第 9 节状态为 `Approved`，且对应 [`../features/<feature>.md`](../features/) 自身状态为 `Approved`。`Draft` 需求或 `Draft` 功能设计不得进入 `.omo/plans/` 执行计划。
