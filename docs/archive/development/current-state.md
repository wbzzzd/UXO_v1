# 当前工程状态

最后更新：2026-07-20

## 当前事实

- 项目是 Qt 5 / CMake / C++17 桌面客户端，目标程序名为 `UXOMissionControl`。
- 当前 CMake 目标纳入 `src/App/`、`src/MainWindow/`、`include/App/`、`include/MainWindow/` 和 `include/Core/Data/Types.h` 的核心文件。
- `src/App/main.cpp` 会创建 `App::Application`，调用 `initialize()`，调用 `app.run()` 显示主窗口，然后进入 `app.exec()`。
- `Application::initialize()` 当前调用的配置、日志、数据库、通信、模块初始化函数都是占位返回成功。
- `MainWindow` 及其主界面区域已有初步实现，启动后由 `Application::run()` 创建并显示。
- `Core::Simulation::DemoScenarioProvider` 提供本地模拟演示场景，包含目标、任务和模拟设备。
- `Core::Simulation::SimulationWorkflow` 是当前模拟状态闭环的核心：在内存中维护目标列表、当前选中的目标，以及一个按时间顺序追加的操作日志。
- 目标在内存中按 `Detected -> Confirmed -> Disposing -> Disposed` 流转，状态变更由工作流方法触发并写入有序操作日志，过程不持久化。
- 主窗口左侧的模拟工作流面板是当前唯一可交互的流程入口，提供目标选择与状态推进操作；决策建议面板和设备状态面板当前只读展示，不发起真实控制。
- `config/system.json` 和 `config/devices.json` 存在，可作为后续模拟配置输入。

## 文档与代码差异

- SRS、SDD、UI 设计文档和 `PROJECT_STRUCTURE.md` 描述的是目标系统和规划结构。
- `PROJECT_STRUCTURE.md` 中提到的多个模块、驱动、插件、测试、部署脚本和配置文件当前并不存在。
- 后续开发必须以源码、CMake 和实际构建结果作为当前事实来源。

## 当前主要缺口

- 没有真实设备通信、数据库持久化、算法识别或排弹控制实现；所有流程均停留在内存模拟与占位阶段。
- 当前共有 4 个 CTest 用例覆盖启动可见性、模拟场景提供器和 `SimulationWorkflow` 的功能与 UI 行为，但更完整的单元、集成和 UI 自动化测试体系仍缺失。
- 已知问题（P2·不阻塞）：1280×720 窗口最小尺寸下 `DecisionSuggestionPanel` 底部约 5px 溢出，真实部署走 4K 三屏不触发该边界；MVP 以默认 1920×1080 演示为准，视觉修复留待后续。MVP 阶段 5 验收已通过（构建 + CTest 4/4 + 安全检查）。
- 文档机制刚开始向 opencode + oh-my-openagent 适配。
