# OMO 计划目录

`.omo/plans/` 用于保存可评审、可交接的工作计划。运行态 session、continuation、loop 状态和缓存不得放入版本化计划文件。

## 命名建议

```text
YYYYMMDD-topic.md
```

示例：

```text
20260707-mvp-command-seat.md
```

## 计划必须包含

- 背景和当前事实。
- 明确目标和非目标。
- 安全边界。
- 分阶段任务。
- 验收标准。
- 验证命令。
- 是否需要用户确认。

## 规则

- 多文件或架构性修改先写计划，再执行。
- MVP 计划必须区分模拟、占位和真实接入。
- 不把 `.omo/run-continuation/` 中的运行态数据复制进计划。
