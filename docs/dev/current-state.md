# 当前工程状态

最后更新：2026-07-07

## 当前事实

- 项目是 Qt 5 / CMake / C++17 桌面客户端，目标程序名为 `UXOMissionControl`。
- 当前 CMake 目标只纳入 `src/App/`、`src/MainWindow/`、`include/App/`、`include/MainWindow/` 和 `include/Core/Data/Types.h` 的核心文件。
- `src/App/main.cpp` 会创建 `App::Application`，调用 `initialize()`，调用 `app.run()` 显示主窗口，然后进入 `app.exec()`。
- `Application::initialize()` 当前调用的配置、日志、数据库、通信、模块初始化函数都是占位返回成功。
- `MainWindow` 和几个主界面区域已有初步实现，当前入口已通过 `Application::run()` 创建并显示 `MainWindow`。
- `Core::Simulation::DemoScenarioProvider` 已提供一个本地模拟演示场景，包含 1 个目标、1 个任务和 2 个模拟设备。
- `config/system.json` 和 `config/devices.json` 存在，可作为后续模拟配置输入。

## 文档与代码差异

- SRS、SDD、UI 设计文档和 `PROJECT_STRUCTURE.md` 描述的是目标系统和规划结构。
- `PROJECT_STRUCTURE.md` 中提到的多个模块、驱动、插件、测试、部署脚本和配置文件当前并不存在。
- 后续开发必须以源码、CMake 和实际构建结果作为当前事实来源。

## 当前主要缺口

- 主窗口已可在启动路径中创建和显示，最小模拟数据模型已存在，后续仍需接入 UI 展示闭环。
- 没有真实设备通信、数据库持久化、算法识别或排弹控制实现。
- 已有阶段 1 启动可见性测试 `tests/startup_visible_test.cpp` 和阶段 2 模拟场景测试 `tests/demo_scenario_provider_test.cpp`，但更完整的单元、集成和 UI 自动化测试体系仍缺失。
- 文档机制刚开始向 opencode + oh-my-openagent 适配。
