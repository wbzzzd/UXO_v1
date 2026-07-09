---
description: 基于当前事实生成 MVP 工作计划
agent: plan
---

请为 UXO_v1 生成 MVP 工作计划。只输出计划，不修改文件。

必须依据：

- `AGENTS.md`
- `docs/dev/current-state.md`
- `docs/dev/mvp-scope.md`
- `docs/dev/simulation-policy.md`
- 当前源码和 CMake 文件

计划必须包含：

- 当前事实。
- MVP 目标和非目标。
- 模拟数据范围。
- 阶段任务。
- 每阶段验收标准。
- 需要用户确认的真实设备或安全边界问题。

禁止：

- 不要实施代码。
- 不要接入真实设备。
- 不要假设 SRS/SDD 模块已经存在。
