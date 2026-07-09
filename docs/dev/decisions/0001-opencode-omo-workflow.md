# 0001: 采用 opencode + oh-my-openagent 工作流

状态：已接受

日期：2026-07-07

## 背景

UXO_v1 当前有较多目标设计文档，但实际代码仍处于 Qt/CMake 桌面客户端骨架阶段。后续开发需要避免 agent 把目标架构误认为已实现代码，也需要建立安全边界和可复用工作流。

## 决策

项目采用 opencode 原生兼容作为底座，并在仓库级适配 oh-my-openagent 工作流。

具体做法：

- 使用 `AGENTS.md` 作为常驻项目规则。
- 使用 `opencode.json` 注入项目文档和保守权限。
- 使用 `docs/dev/` 记录当前工程事实、MVP 边界和模拟策略。
- 使用 `.opencode/commands/` 固化高频工作流。
- 使用 `.omo/rules/`、`.omo/plans/` 和 `.omo/templates/` 支撑 OMO 的计划和交接机制。

## 后果

- 不使用 OMO 的开发者仍可通过 opencode 原生规则和命令工作。
- 使用 OMO 的 agent 可以获得更清晰的项目记忆、计划入口和安全边界。
- 全局 OMO 安装、Team Mode 和 MCP 深度集成推迟到仓库级文档机制稳定之后。
