# 架构基线

最后更新：2026-07-20
状态：初始草稿，待评审

本文档只记录后续开发必须遵守的架构规则。源码和 `CMakeLists.txt` 是 CURRENT 事实依据；产品范围见 `PRODUCT.md`；构建与完成门禁见 `DEVELOPMENT.md`。

## 1. CURRENT

当前运行链：

```text
main
  -> Application
     -> MainWindow
        -> DemoScenarioProvider
        -> SimulationWorkflow
        -> UI Panels
```

当前事实：

- `SimulationWorkflow` 保存模拟目标、当前选择和操作日志。
- `MainWindow` 另外保存任务和设备，并把数据复制给多个 Panel。
- `MainWindow` 负责业务命令转发和界面刷新，职责过多。
- 导航按钮会发出信号，但中心页面不会切换。
- 配置、日志、数据库、通信和模块初始化仍是占位实现。
- 当前无真实设备、网络、数据库或外部服务接入。

## 2. NEXT 架构

```text
Application
  ├── 创建 SimulationWorkflow
  └── 创建 MainWindow(workflow)

MainWindow
  ├── 把用户操作转换为 workflow 命令
  └── 从 workflow 读取状态并刷新 UI

SimulationWorkflow
  ├── targets
  ├── missions
  ├── devices
  ├── selectedTarget
  └── operationLogs
```

NEXT 不新增通用 Repository、Store、服务容器或事件总线。

## 3. 核心规则

### 3.1 单一状态所有者

- `SimulationWorkflow` 是模拟目标、任务、设备、选择和日志的唯一可变状态所有者。
- `MainWindow` 和 Panel 不保存可独立修改的业务副本。
- UI 可以使用只读展示快照，但状态变化后必须重新从 workflow 获取。

### 3.2 命令和查询

- UI 通过 workflow 命令执行选择目标、指派设备、推进任务和完成任务。
- 每个命令返回成功/失败及原因。
- 非法命令不得部分修改状态，并必须记录拒绝日志。
- workflow 状态变化后发出统一 Qt 信号；`MainWindow` 收到信号后刷新相关页面。
- NEXT 使用进程内同步调用，不引入异步消息队列。

### 3.3 依赖方向

```text
App -> MainWindow
App -> Core/Simulation
MainWindow -> Core/Data + Core/Simulation
Core/Simulation -> Core/Data
Core 不得依赖 MainWindow
```

Panel 之间不直接引用；交互信号上抛到 `MainWindow`，由 `MainWindow` 调用 workflow。

### 3.4 装配职责

- `Application` 创建 `SimulationWorkflow` 并注入 `MainWindow`。
- `MainWindow` 只负责 UI 组合、信号连接和显示刷新。
- `SimulationWorkflow` 负责业务状态和业务规则。
- `DemoScenarioProvider` 只负责构造本地模拟场景，不成为运行时状态所有者。

## 4. 模块职责

| 模块 | 职责 |
|------|------|
| `Core/Data` | `TargetInfo`、`MissionInfo`、`DeviceInfo` 等稳定数据类型 |
| `Core/Simulation` | 模拟会话状态、业务命令和操作日志 |
| `MainWindow` | 页面组合、用户输入和状态展示 |
| `App` | 生命周期和依赖装配 |
| 功能模块 | 功能获批后新增的独立工作流或适配器 |

## 5. 模拟与真实边界

- NEXT 只实现本地内存模拟，不实现持久化、网络通信或真实设备适配器。
- 只有获批功能存在真实调用方时才定义最小外部端口。
- 不提前创建 `IStorage`、`IDeviceAdapter` 等空接口或目录。
- 真实适配器必须与模拟实现分离，并经过用户明确授权。
- UI 必须明确显示“模拟”或“演示”，不得伪装成真实接入状态。

## 6. 功能扩展规则

- 新功能先有 `docs/features/<feature>.md`，状态为 `Approved` 后才能实施。
- 功能状态由对应具体 workflow 管理，不放入 UI 控件。
- 只有出现真实跨工作流用例时，才增加最小协调对象。
- UXR、MOS 和真实数据源的接口、参数及适配器在各自功能评审中决定。

## 7. 迁移顺序

1. 评审四份核心文档，确认 NEXT 范围。
2. 扩展 `SimulationWorkflow`，统一目标、任务、设备和日志状态。
3. 由 `Application` 创建 workflow 并注入 `MainWindow`。
4. 把业务命令从 `MainWindow` 收敛到 workflow。
5. 实现导航页面切换，并处理未接入的孤儿面板。
6. 完成模拟设备指派、任务执行和 UI 联动。
7. 验收完整模拟指挥环后，再评审 UXR、MOS 等功能。

## 8. 当前技术债

- 任务和设备状态存在多份副本。
- `MainWindow` 同时承担装配、业务转发和 UI 组合。
- 导航切换为空操作。
- `DecisionView`、`DeviceControlView` 等文件已编译但未接入。
- `Application` 初始化流程存在多个无行为占位函数。

上述问题的详细源码证据暂保留在 `docs/dev/current-state.md` 和 `docs/dev/architecture-boundaries.md`，本文件不重复展开。
