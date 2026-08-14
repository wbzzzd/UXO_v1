# 归档目录清单

本表是归档文件的长期台账，不承担当前需求、架构或 UI 状态管理。每一行代表一次归档记录；目录层级后续可以重组，但不得丢失原始路径、归档时间和归档原因。

| 记录 ID | 类别 | 原始路径 | 归档路径 | 归档时间 | 归档原因 | 当前替代权威 | 保留方式 |
|----------|------|----------|----------|----------|----------|--------------|----------|
| ARC-001 | 核心基线快照 | `docs/dev/current-state.md` | [`core-baselines/current-state.md`](core-baselines/current-state.md) | 2026-07-30 | 已被当前架构基线、源码和实际验证取代 | `docs/ARCHITECTURE.md`、源码和实际验证 | Git rename，字节不变 |
| ARC-002 | 核心基线快照 | `docs/dev/build-and-run.md` | [`core-baselines/build-and-run.md`](core-baselines/build-and-run.md) | 2026-07-30 | 已被当前工程开发基线取代 | `docs/DEVELOPMENT.md` | Git rename，字节不变 |
| ARC-003 | 核心基线快照 | `docs/dev/mvp-scope.md` | [`core-baselines/mvp-scope.md`](core-baselines/mvp-scope.md) | 2026-07-30 | 已被当前产品与需求基线取代 | `docs/PRODUCT.md` | Git rename，字节不变 |
| ARC-004 | 核心基线快照 | `docs/dev/architecture-boundaries.md` | [`core-baselines/architecture-boundaries.md`](core-baselines/architecture-boundaries.md) | 2026-07-30 | 已被当前架构基线取代 | `docs/ARCHITECTURE.md` | Git rename，字节不变 |
| ARC-005 | 核心基线快照 | `docs/dev/simulation-policy.md` | [`core-baselines/simulation-policy.md`](core-baselines/simulation-policy.md) | 2026-07-30 | 模拟安全边界已由项目规则和核心基线统一维护 | `AGENTS.md` 和四份核心基线 | Git rename，字节不变 |
| ARC-006 | 需求来源 | `docs/dev/incremental-prd-uxo-recognition-and-mos.md` | [`requirements/incremental-prd-uxo-recognition-and-mos.md`](requirements/incremental-prd-uxo-recognition-and-mos.md) | 2026-07-30 | 合并需求已拆分为独立 UXR/MOS Draft，旧合并版本不再作为 active 输入 | `docs/requirements/`（REQ-007 等） | Git rename，字节不变 |
| ARC-007 | 架构来源 | `docs/dev/architecture-uxo-recognition-and-mos.md` | [`architecture/architecture-uxo-recognition-and-mos.md`](architecture/architecture-uxo-recognition-and-mos.md) | 2026-07-30 | UXR/MOS 架构职责已拆分为独立 Draft，旧合并稿仅作历史追溯 | `architecture-mos.md`、`architecture-uxr.md`、`docs/ARCHITECTURE.md` | Git rename，字节不变 |
| ARC-008 | 架构来源 | `docs/dev/class-diagram-uxo-mos.mermaid` | [`architecture/class-diagram-uxo-mos.mermaid`](architecture/class-diagram-uxo-mos.mermaid) | 2026-07-30 | 混合类图尚未拆分，暂移出 active 架构输入避免误用 | 独立架构 Draft 和 `docs/ARCHITECTURE.md` | Git rename，字节不变 |
| ARC-009 | UI 来源 | `docs/dev/product-design-mos.md` | [`ui/product-design-mos.md`](ui/product-design-mos.md) | 2026-07-30 | PRD 与功能增量已承担需求职责，旧产品设计稿与当前 UI 契约存在分歧 | `docs/UI.md`、`docs/ui/pages/decision.md` | Git rename，字节不变 |
| ARC-010 | UI 来源 | `docs/dev/mos-ui-design-brief.md` | [`ui/mos-ui-design-brief.md`](ui/mos-ui-design-brief.md) | 2026-07-30 | 早期 UI 设计简报已被当前 UI 页面契约替代 | `docs/UI.md`、`docs/ui/pages/decision.md` | Git rename，字节不变 |
| ARC-011 | 需求来源 | `docs/prd/prd-mos.md` | [`requirements/prd-mos.md`](requirements/prd-mos.md) | 2026-08-13 | PRD 与 requirements/REQ-007.md 及 features/mos-planning.md 内容重叠，prd/ 目录不再独立维护 | `docs/requirements/REQ-007.md`、`docs/features/mos-planning.md` | Git rename，字节不变 |
| ARC-012 | 需求来源 | `docs/prd/prd-uxr.md` | [`requirements/prd-uxr.md`](requirements/prd-uxr.md) | 2026-08-13 | UXR 未注册 REQ，PRD 仅作 Draft 来源；prd/ 目录不再独立维护 | `docs/requirements/`（UXR 未注册） | Git rename，字节不变 |

归档时间使用 `YYYY-MM-DD`。归档原因必须说明“为什么退出 active 文档链路”，不能只写“历史文件”。所有移动均应保持文件字节不变；归档文件仅作历史资料，不得写成 CURRENT、Approved 或 Implemented 事实。
