# 排弹抢修指挥系统

本仓库是排弹抢修指挥系统指挥席客户端的当前实现工程。现阶段重点是 Qt 5 / CMake / C++17 桌面客户端，并重新建立可维护的产品、架构、UI 与工程基线。

## 当前状态

- 当前真实构建目标：`UXOMissionControl`。
- 当前主要源码：`src/App/`、`src/MainWindow/`、`include/App/`、`include/MainWindow/`、`include/Core/Data/Types.h`。
- SRS、SDD、旧 UI 设计和功能草案是需求输入，不代表所有模块已经实现。
- 真实设备控制、排弹动作、外部通信命令和数据库写入默认不在当前 MVP 范围内。

## 当前核心文档

- [Agent 项目规则](./AGENTS.md)
- [产品与需求基线](./docs/PRODUCT.md)
- [软件架构基线](./docs/ARCHITECTURE.md)
- [UI 规范与扩展契约](./docs/UI.md)
- [工程开发基线](./docs/DEVELOPMENT.md)
- [功能增量设计规则](./docs/features/README.md)
- [文档地图与阅读路线](./docs/README.md)

## 来源资料

- [SRS 软件需求规格说明书](./SRS排弹抢修指挥系统_v1.0.md)
- [SDD 软件设计说明书](./SDD排弹抢修指挥系统_v1.0.md)
- [前端 UI 详细设计文档](./docs/前端UI详细设计文档.md)
- [研究资料](./docs/research/)
- [历史文档归档说明](./docs/archive/README.md)

## 快速构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

更多说明见 [工程开发基线](./docs/DEVELOPMENT.md)。
