# 历史文档归档说明

本目录保存已被核心基线取代、不再直接指导当前开发的历史工程快照。归档文件仅作为历史依据和需求来源，不具备当前事实权威。

与 [`../sources/`](../sources/) 的区别：本目录收录已被内部基线取代的工程快照；`sources/` 收录利益相关方早期提交的原始上游输入，未被内部基线"取代"，仅不再具备当前事实权威。

## 归档权威与不可覆盖规则

归档文件保留原始字节和 Git 追溯关系，但不得覆盖以下当前核心基线：

- [`../PRODUCT.md`](../PRODUCT.md)
- [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- [`../UI.md`](../UI.md)
- [`../DEVELOPMENT.md`](../DEVELOPMENT.md)
- [`../../AGENTS.md`](../../AGENTS.md)

当归档内容与上述核心基线冲突时，以核心基线为准。

## development/ 目录

`development/` 保存本轮文档重整中从 `docs/dev/` 迁入的五份已被取代的工程基线快照。迁入通过 `git mv` 完成，文件字节未改动，Git 历史连续可追溯。

| 原始路径 | 归档路径 | 当前替代权威 |
| --- | --- | --- |
| `docs/dev/current-state.md` | [`development/current-state.md`](development/current-state.md) | `../PRODUCT.md`、`../ARCHITECTURE.md` |
| `docs/dev/build-and-run.md` | [`development/build-and-run.md`](development/build-and-run.md) | `../DEVELOPMENT.md` |
| `docs/dev/mvp-scope.md` | [`development/mvp-scope.md`](development/mvp-scope.md) | `../PRODUCT.md` |
| `docs/dev/architecture-boundaries.md` | [`development/architecture-boundaries.md`](development/architecture-boundaries.md) | `../ARCHITECTURE.md` |
| `docs/dev/simulation-policy.md` | [`development/simulation-policy.md`](development/simulation-policy.md) | `../../AGENTS.md`、`../ARCHITECTURE.md`、`../DEVELOPMENT.md` |

归档快照中的构建命令、模块清单、状态机描述和模拟策略仅供追溯历史决策使用；当前实施以核心基线和源码为准。

## OMO 适配计划

[`OMO_ADAPTATION_PLAN.md`](OMO_ADAPTATION_PLAN.md) 是 opencode + oh-my-openagent 适配计划的早期草案，记录了文档基线建立前的工作流适配设想。该计划已完成并被后续工作流取代，从 `docs/` 根目录归档至本目录，文件字节与 Git 追溯保留不变。

| 原始路径 | 归档路径 | 当前替代权威 |
| --- | --- | --- |
| `docs/OMO_ADAPTATION_PLAN.md` | [`OMO_ADAPTATION_PLAN.md`](OMO_ADAPTATION_PLAN.md) | `../../AGENTS.md`、`../DEVELOPMENT.md`、`.opencode/`、`.omo/rules/` |

归档文件保留原始正文，仅作为历史工作流快照，不更新其内部历史表述。当前工作流权威以 [`../../AGENTS.md`](../../AGENTS.md)、[`../DEVELOPMENT.md`](../DEVELOPMENT.md) 及 `.opencode/`、`.omo/rules/` 为准。

## 本阶段保留原位的类别

本轮除上述五份工程基线快照和 OMO 适配计划外，以下类别未移动，仍在原位：

- UI 文档：`docs/UI.md`、`docs/ui/`、`docs/dev/mos-ui-design-brief.md`、`docs/dev/product-design-mos.md`。
- DDR 决策记录：`docs/ddr/`。
- 研究资料：`docs/research/`。
- `docs/dev/` 中其余 Draft 草稿与来源资料，包括 `docs/dev/decisions/`。

注：原 `docs/前端UI详细设计文档.md` 已迁入 [`../sources/`](../sources/)，不再保留原位。这些保留类别的权威状态以其自身文档为准，不因本归档动作而改变。

## 归档操作约束

归档操作不得删除历史内容，不得改写迁入文件的字节，不得破坏 Git 追溯关系。
