# DDR-004: 多 AI Agent 并行开发架构调整

| 字段 | 值 |
|------|-----|
| **状态** | 已确认 |
| **日期** | 2026-06-24 |
| **参与人** | 架构师（高见远）、主理人（齐活林） |
| **关联** | DDR-003（Phase 1 执行计划，本决策修订其执行策略） |

---

## 背景

用户澄清团队配置为"单人 + 多个 AI Agent 并行开发"。AI Agent 并行的痛点与人类团队不同：无法实时沟通、文件级冲突难自动 merge、上下文隔离导致接口对不上。需要评估当前架构是否支持多 Agent 并行，并给出修改方案。

---

## 问题诊断：当前架构的并行障碍

### 冲突点 A：MainWindow.cpp（严重）

4 个改造项需修改同一文件：
- 改造项4（FrontendDataStore 集成）
- 改造项5（NavigationWidget 添加）
- 改造项6（DecisionView + LeftPanel 添加）
- 改造项9（initializeModules 装配）

C++ 的 merge conflict 极难自动解决（涉及 include、成员变量、初始化列表、信号槽）。

### 冲突点 B：CMakeLists.txt（中等）

几乎所有改造项都需修改顶层 CMakeLists.txt 添加源文件。

### 冲突点 C：改造项2 与改造项8 文件重叠（严重）

仿真设备层和 SensorSim 都在 `src/sim/` 目录，SensorSim.cpp 依赖 SimCamera.h，两个 Agent 同时工作必然冲突。

### 冲突点 D：共享类型定义（编译依赖）

SensorData、DeviceError 等类型嵌在 HAL 接口头文件中，改造项1 没完成时其他 Agent 无法编译。

### 冲突点 E：改造项6 依赖改造项4（数据依赖）

DecisionView 需要 `#include "FrontendDataStore.h"`，改造项4 没完成时改造项6 编译不过。

---

## 决策：5 项架构修改

### 修改 1：提取共享类型到独立头文件（解决冲突点 D）

新增 `include/core/` 目录，将所有跨模块共享的类型提取为独立头文件：

```
include/core/
  ├── CoreTypes.h        — DeviceMode, DeviceError, DeviceStatus 等枚举
  ├── SensorData.h       — SensorData 结构（热图像/点云/GPR/磁场/位置）
  ├── TargetTypes.h      — TargetInfo, AlertInfo, DetectionResult 等业务类型
  ├── GeoTypes.h         — GeoPosition, BoundingBox 等地理类型
  └── IDataProvider.h    — 数据提供者抽象接口
```

这些头文件不依赖任何其他模块，在 Batch 0 一次性定义并冻结。

### 修改 2：拆分 MainWindow God Object（解决冲突点 A）

将 MainWindow 从"包含所有 UI 组件的 God Object"重构为"外壳 + 组件注册机制"：

```
src/ui/
  ├── MainWindow.cpp/h          — 外壳：只创建布局容器、菜单、工具栏
  ├── UiComponentRegistry.h     — 组件注册机制
  ├── navigation/
  │   └── NavigationWidget.cpp/h  — 改造项5 独占
  ├── decision/
  │   ├── DecisionView.cpp/h      — 改造项6 独占
  │   └── LeftPanel.cpp/h         — 改造项6 独占
  └── data/
      └── DataStoreWidget.cpp/h   — 改造项4 的 UI 桥接层
```

**UiComponentRegistry 设计**：各 UI 组件在自己的构造函数中通过 Registry 注册自己，不需要修改 MainWindow.cpp。

```cpp
class UiComponentRegistry {
public:
    static UiComponentRegistry& instance();
    void registerWidget(const QString& area, QWidget* widget);
    QLayout* getArea(const QString& areaName);
signals:
    void componentRegistered(const QString& area, QWidget* widget);
};
```

MainWindow.cpp 只做：创建空布局容器 → 调用 `UiComponentRegistry::instance().initialize(this)`。

### 修改 3：模块化 CMakeLists.txt（解决冲突点 B）

顶层 CMakeLists.txt 在 Batch 0 一次性写好所有 `add_subdirectory()`，后续不再修改：

```cmake
add_subdirectory(src/sim)
add_subdirectory(src/recording)
add_subdirectory(src/data)
add_subdirectory(src/mqtt)
add_subdirectory(src/ui/navigation)
add_subdirectory(src/ui/decision)
add_subdirectory(tests/e2e)
```

每个子目录的 CMakeLists.txt 由该模块的 Agent 独立维护。

### 修改 4：合并改造项2 和改造项8（解决冲突点 C）

SensorSim 本质是仿真设备的编排器，合并为一个改造项，由同一个 Agent 开发：

```
src/sim/
  ├── SimCamera.cpp/h ... SimUGV.cpp/h   — 原 改造项2
  ├── SensorSim.cpp/h                     — 原 改造项8
  ├── ScenarioScript.cpp/h                — 原 改造项8
  └── DeviceFactory.cpp/h
```

### 修改 5：引入 IDataProvider 接口解耦 UI 与数据层（解决冲突点 E）

```cpp
// include/core/IDataProvider.h
class IDataProvider {
public:
    virtual ~IDataProvider() = default;
    virtual QList<TargetInfo> getTargets() const = 0;
    virtual QList<AlertInfo> getAlerts() const = 0;
    virtual void subscribeToUpdates(QObject* receiver) = 0;
};
```

FrontendDataStore 实现 `IDataProvider`，DecisionView 只依赖 `IDataProvider*`。改造项6 的 Agent 只需 `include/core/IDataProvider.h`（Batch 0 已定义），不需等改造项4 完成。

---

## 接口契约机制

### 契约文档位置

`docs/contracts/` 目录，每个模块一个契约文件：

```
docs/contracts/
  ├── README.md                    — 总览：模块列表、依赖关系、并行批次
  ├── 00-core-types.md             — 共享类型契约（Batch 0 产出并冻结）
  ├── 01-hal-interfaces.md         — HAL 接口契约
  ├── 02-sim-devices.md            — 仿真设备层契约
  ├── 03-recording.md              — 录制回放契约
  ├── 04-data-store.md             — FrontendDataStore 契约
  ├── 05-navigation.md             — NavigationWidget 契约
  ├── 06-decision-view.md          — DecisionView 契约
  ├── 07-mqtt-service.md           — MqttService 契约
  └── 09-assembly.md               — 装配契约
```

### 每个契约文件包含

| 章节 | 说明 |
|------|------|
| 文件所有权 | 本 Agent 负责创建/修改的文件列表 + 不可修改的文件列表 |
| 依赖项 | 本模块只能 include 的头文件清单 |
| 公开 API | 其他模块可能依赖的接口签名（含 C++ 头文件片段） |
| 类型定义 | 本模块定义的共享类型 |
| 行为契约 | 线程安全、信号发射时机、性能保证 |
| 集成点 | 如何注册到 MainWindow、如何注入依赖 |

### 冻结规则

Batch 0 产出的契约文件在后续批次中**冻结，不可修改**。如发现契约有误，Agent 不能自行修改，需向主理人报告。

---

## 修订后的执行批次

### Batch 0：基础设施 + 契约（1 Agent，串行）

**目标**：建立共享类型、拆分 MainWindow、编写所有契约文件

**文件范围**：
- `include/core/CoreTypes.h` / `SensorData.h` / `TargetTypes.h` / `GeoTypes.h` / `IDataProvider.h`
- `src/ui/MainWindow.cpp/h`（重构为外壳 + UiComponentRegistry）
- `src/ui/UiComponentRegistry.h`
- `CMakeLists.txt`（顶层，添加所有子目录）
- `docs/contracts/*.md`（所有契约文件）

**完成后冻结**：`include/core/` 和 `docs/contracts/` 下的所有文件

**工作量**：约 1-2 天（只写头文件和契约，不写实现）

### Batch 1：第一批并行（4 Agents）

前置条件：Batch 0 完成

| Agent | 改造项 | 文件范围 | 依赖契约 |
|-------|--------|---------|---------|
| Agent A | 1. HAL 接口 | `include/hal/*.h` | 00-core-types.md |
| Agent B | 7. MqttService | `src/mqtt/*.cpp/h` | 00-core-types.md |
| Agent C | 5. NavigationWidget | `src/ui/navigation/*.cpp/h` | 00-core-types.md |
| Agent D | 3. 数据录制回放 | `src/recording/*.cpp/h` | 00-core-types.md |

**冲突分析**：文件不重叠 ✅ | CMake 各自维护子目录 ✅ | MainWindow 无人修改 ✅ | 无编译依赖 ✅

### Batch 2：第二批并行（3 Agents）

前置条件：Batch 1 完成（特别是 HAL 接口）

| Agent | 改造项 | 文件范围 | 依赖契约 |
|-------|--------|---------|---------|
| Agent E | 2+8. 仿真设备 + SensorSim | `src/sim/*.cpp/h` | 00, 01-hal-interfaces.md |
| Agent F | 4. FrontendDataStore | `src/data/*.cpp/h` | 00, 01-hal-interfaces.md |
| Agent G | 6. DecisionView + LeftPanel | `src/ui/decision/*.cpp/h` | 00, 04-data-store.md（仅 IDataProvider 接口） |

**冲突分析**：文件不重叠 ✅ | 改造项6 通过 IDataProvider 解耦，不依赖改造项4 实现 ✅ | 改造项2+8 合并消除内部冲突 ✅

### Batch 3：集成装配（1 Agent）

前置条件：Batch 2 全部完成

| Agent | 改造项 | 文件范围 |
|-------|--------|---------|
| Agent H | 9. initializeModules | `Application.cpp/h` |

### Batch 4：验证（1 Agent）

前置条件：Batch 3 完成

| Agent | 改造项 | 文件范围 |
|-------|--------|---------|
| Agent I | 10. 端到端验证 | `tests/e2e/*.cpp/h` |

---

## 并行度统计

| 指标 | 修改前 | 修改后 |
|------|--------|--------|
| 最大并行度 | 2-3 | 4 |
| 冲突点 | 5 个 | 0 个（批次内） |
| 人工 merge 概率 | 高（MainWindow.cpp 必然冲突） | 低 |
| Batch 0 额外成本 | 无 | 1-2 天（1 Agent） |
| 净收益 | — | 用 1-2 天成本换 7 个改造项无冲突并行 |

---

## 对 DDR-003 的修订

1. **改造项2 和改造项8 合并**为一个任务（仿真设备 + SensorSim）
2. **新增 Batch 0**：提取共享类型 + 拆分 MainWindow + 编写契约
3. **FrontendDataStore 实现 IDataProvider 接口**
4. **引入 UiComponentRegistry**：用注册模式代替直接在 MainWindow 中 new 组件
5. **执行顺序调整**：Batch 0 → Batch 1（4并行）→ Batch 2（3并行）→ Batch 3 → Batch 4

---

## 理由

- Batch 0 看似"额外成本"，但它是并行的前提：共享类型提取让 4 个 Agent 在 Batch 1 就能并行；MainWindow 拆分让 3 个 Agent 在 Batch 2 不冲突；契约文档让 Agent 间接口对得上。
- 5 项修改都不是"过度设计"——UiComponentRegistry、IDataProvider、模块化 CMake 都是工业界标准实践，即使单人开发也有长期价值。
