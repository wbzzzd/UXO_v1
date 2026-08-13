# 排弹抢修指挥系统

本仓库是排弹抢修指挥系统指挥席客户端的当前实现工程。现阶段重点是 Qt 5 / CMake / C++17 桌面客户端，并重新建立可维护的产品、架构、UI 与工程基线。

## 当前状态

- 当前真实构建目标：`UXOMissionControl`。
- 当前主要源码：`src/App/`、`src/MainWindow/`、`include/App/`、`include/MainWindow/`、`include/Core/Data/Types.h`、`src/Core/MOS/`、`include/Core/MOS/`；MOS MainWindow 源码导航：`src/MainWindow/MosPlanningController.cpp`、`MosRunwayWidget.cpp`、`MosParamsPanel.cpp`、`MosGeneratorDialog.cpp`、`DecisionView.cpp`。
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

## 来源资料与设计输入

原始上游来源资料归入 [`docs/sources/`](./docs/sources/README.md)，Draft PRD 需求定义归入 [`docs/prd/`](./docs/prd/README.md)，架构细节子文档与历史架构草稿归入 [`docs/architecture/`](./docs/architecture/README.md)。来源资料与 Draft PRD 仅作溯源与证据引用，不代表 CURRENT 实现事实或 Approved 需求。

- [来源资料索引](./docs/sources/README.md)
- [Draft PRD 需求定义](./docs/prd/README.md)
- [架构细节与历史草稿](./docs/architecture/README.md)
- [研究资料](./docs/research/)
- [历史文档归档说明](./docs/archive/README.md)
- [历史文档归档目录](./docs/archive/catalog.md)

## 快速构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

更多说明见 [工程开发基线](./docs/DEVELOPMENT.md)。
