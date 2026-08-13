# Draft PRD 需求定义

本目录收录内部撰写的 Draft PRD（产品需求定义）文档。PRD 定义产品做什么，是需求定义文档，位于功能增量设计的上游。关系为：PRD（需求定义）-> [PRODUCT.md](../PRODUCT.md) 注册 REQ-NNN -> [features/](../features/) 实现设计。

## 权威性与使用边界

- PRD 是 Draft 设计输入，不代表 Approved 需求。
- 需求 ID 与状态的唯一权威是 [PRODUCT.md](../PRODUCT.md) 第 9 节。
- PRD 不得直接指导实现；进入实现计划前，关联需求必须在 PRODUCT.md 中为 `Approved`，且对应功能设计为 `Approved`。
- 与核心基线冲突时，以核心基线为准。

## 与 sources/ 的区别

[`sources/`](../sources/) 收录利益相关方早期提交的原始上游产物（SRS、SDD、排弹方案等）；本目录收录内部 PM 撰写的 Draft PRD，是需求定义的工作产物。

## 文件索引

| 文件 | 角色 | 状态 |
|------|------|------|
| [`prd-mos.md`](prd-mos.md) | MOS 功能 PRD 草稿（V2.1） | Draft |
| [`prd-uxr.md`](prd-uxr.md) | UXR 功能 PRD 草稿（V1.0） | Draft |
