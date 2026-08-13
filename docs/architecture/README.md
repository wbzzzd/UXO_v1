# 架构细节子文档

本目录收录从 [ARCHITECTURE.md](../ARCHITECTURE.md) 拆出的架构细节子文档，以及历史架构设计输入。ARCHITECTURE.md 保留概览、构建目标、结构问题、TARGET 方向与演进映射；本目录承载启动装配、状态所有权、调用链等 CURRENT 实现细节，以及 NEXT 候选架构草案和历史架构草稿。

## 权威性与使用边界

- CURRENT 细节文档（`startup.md`、`state-ownership.md`、`call-chains.md`）描述当前实现事实，以源码和实际构建为准。
- `candidate-architecture.md` 是 NEXT 候选架构（Draft），不得直接指导实现。
- `architecture-mos.md`、`architecture-uxr.md` 及配套 Mermaid 图是历史架构草稿（Draft/来源资料），不代表 CURRENT 实现事实。
- 当本目录文档与 [ARCHITECTURE.md](../ARCHITECTURE.md) 冲突时，以 ARCHITECTURE.md 为准。

## 文件索引

| 文件 | 类别 | 角色 |
|------|------|------|
| [`startup.md`](startup.md) | CURRENT 细节 | 启动时序与对象装配 |
| [`state-ownership.md`](state-ownership.md) | CURRENT 细节 | 状态树与权威位置 |
| [`call-chains.md`](call-chains.md) | CURRENT 细节 | 已验证操作调用链 |
| [`candidate-architecture.md`](candidate-architecture.md) | NEXT 草案 | 候选架构（Draft），不得直接指导实现 |
| [`architecture-mos.md`](architecture-mos.md) | 历史来源 | MOS 架构设计草稿（Draft/来源资料） |
| [`architecture-uxr.md`](architecture-uxr.md) | 历史来源 | UXR 架构设计草稿（Draft/来源资料） |
| [`sequence-diagram-mos.mermaid`](sequence-diagram-mos.mermaid) | 历史来源 | MOS 时序图（配合 `architecture-mos.md`） |
| [`sequence-diagram-recognition.mermaid`](sequence-diagram-recognition.mermaid) | 历史来源 | 识别功能时序图（配合 `architecture-mos.md`） |
