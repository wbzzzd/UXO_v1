# docs/dev 保留草稿索引

本文分类列出五份旧基线（`current-state.md`、`build-and-run.md`、`mvp-scope.md`、`architecture-boundaries.md`、`simulation-policy.md`）迁入 [`../archive/development/`](../archive/development/) 之后，`docs/dev/` 保留的全部文件与子目录。本目录所有内容均为历史决策、`Draft` 草稿、来源或设计输入，不得作为当前实现依据。

关联基线：[`../PRODUCT.md`](../PRODUCT.md) · [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../UI.md`](../UI.md) · [`../DEVELOPMENT.md`](../DEVELOPMENT.md) · [`../../AGENTS.md`](../../AGENTS.md)

## 1. 已接受的历史工作流决策

| 路径 | 说明 |
|------|------|
| [`decisions/`](decisions/) | 历史决策记录目录 |
| [`decisions/0001-opencode-omo-workflow.md`](decisions/0001-opencode-omo-workflow.md) | 采用 opencode + oh-my-openagent 工作流的决策，状态"已接受"。当前工作流权威是 [`../../AGENTS.md`](../../AGENTS.md) 与 `.opencode/`、`.omo/rules/` 配置；本文件保留为历史依据，不再单独驱动实现 |

## 2. UXR/MOS 草稿与图（Draft/来源/设计输入）

以下文件均为 `Draft` 或来源/设计输入，不得直接指导实现。

| 文件 | 说明 |
|------|------|
| [`incremental-prd-uxo-recognition-and-mos.md`](incremental-prd-uxo-recognition-and-mos.md) | UXR/MOS 合并增量 PRD 草稿（Draft/来源输入） |
| [`architecture-uxo-recognition-and-mos.md`](architecture-uxo-recognition-and-mos.md) | UXO 识别与 MOS 架构草稿 |
| [`class-diagram-uxo-mos.mermaid`](class-diagram-uxo-mos.mermaid) | UXO/MOS 类图草稿 |
| [`sequence-diagram-mos.mermaid`](sequence-diagram-mos.mermaid) | MOS 时序图草稿 |
| [`sequence-diagram-recognition.mermaid`](sequence-diagram-recognition.mermaid) | 识别时序图草稿 |

## 3. UI 拥有文件（本索引不管理）

以下文件物理位于 `docs/dev/`，但由 UI 设计流程拥有，变更走 [`../ui/`](../ui/) 与 [`../UI.md`](../UI.md) 流程，不在开发草稿轨道内。

| 文件 | 说明 |
|------|------|
| [`mos-ui-design-brief.md`](mos-ui-design-brief.md) | MOS UI 设计简报，UI 拥有 |
| [`product-design-mos.md`](product-design-mos.md) | MOS 产品设计，UI 拥有 |

## 4. 已迁出文件

五份旧基线已通过 `git mv` 字节不变地迁入 [`../archive/development/`](../archive/development/)，本目录不再保留，详见 [`../archive/README.md`](../archive/README.md)。

## 5. 实现前置条件

任何 UXR/MOS 草稿或图要进入实现，必须同时满足两项前提：关联的 `REQ-NNN` 在 [`../PRODUCT.md`](../PRODUCT.md) 第 9 节状态为 `Approved`，且对应 [`../features/<feature>.md`](../features/) 自身状态为 `Approved`。`Draft` 需求或 `Draft` 功能设计不得进入 `.omo/plans/` 执行计划。
