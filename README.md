# 排弹抢修指挥系统

本仓库是排弹抢修指挥系统指挥席客户端的当前实现工程。现阶段重点是 Qt 5 / CMake / C++17 桌面客户端 MVP，优先完成可见、可演示、模拟数据闭环的指挥席流程。

## 当前状态

- 当前真实构建目标：`UXOMissionControl`。
- 当前主要源码：`src/App/`、`src/MainWindow/`、`include/App/`、`include/MainWindow/`、`include/Core/Data/Types.h`。
- SRS、SDD、UI 设计文档和 `PROJECT_STRUCTURE.md` 是目标/设计资料，不代表所有模块已经实现。
- 真实设备控制、排弹动作、外部通信命令和数据库写入默认不在当前 MVP 范围内。

## 开发入口

- [Agent 项目规则](./AGENTS.md)
- [OMO 适配计划](./docs/OMO_ADAPTATION_PLAN.md)
- [当前工程状态](./docs/dev/current-state.md)
- [构建与运行](./docs/dev/build-and-run.md)
- [MVP 范围](./docs/dev/mvp-scope.md)
- [架构边界](./docs/dev/architecture-boundaries.md)
- [模拟策略](./docs/dev/simulation-policy.md)

## 目标设计资料

- [SRS 软件需求规格说明书](./SRS排弹抢修指挥系统_v1.0.md)
- [SDD 软件设计说明书](./SDD排弹抢修指挥系统_v1.0.md)
- [前端 UI 详细设计文档](./docs/前端UI详细设计文档.md)
- [项目结构说明](./PROJECT_STRUCTURE.md)

## 快速构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

更多说明见 [构建与运行](./docs/dev/build-and-run.md)。
