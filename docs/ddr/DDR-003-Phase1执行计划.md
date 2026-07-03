# DDR-003: Phase 1 详细执行计划

| 字段 | 值 |
|------|-----|
| **状态** | 讨论中 |
| **日期** | 2026-06-24 |
| **参与人** | 产品经理（许清楚）、架构师（高见远）、主理人（齐活林） |

---

## 阅读说明

本文档对每个改造项均采用统一格式：

```
### 改造项 X: [名称]

**问题诊断**（为什么现在有问题）
  - 具体文件 + 行号
  - 问题描述
  - 造成的影响

**改造方案**（怎么改）
  - 新增/修改文件清单
  - 改动内容
  - 关键代码结构

**验收标准**
  - 具体可测的条件
```

---

## 总览：三线并行 + 汇合

```
线1: HAL + 仿真设备层          线2: 前端数据层 + UI 骨架       线3: MQTT 通信层
┌──────────────────┐          ┌──────────────────┐          ┌──────────────────┐
│ 改造项1: HAL接口  │          │ 改造项4: 数据Store│          │ 改造项7: MQTT服务 │
│ 改造项2: 仿真设备  │          │ 改造项5: 视图切换  │          │ 改造项8: SensorSim│
│ 改造项3: 数据录制  │          │ 改造项6: UI补全   │          │                  │
└────────┬─────────┘          └────────┬─────────┘          └────────┬─────────┘
         │                             │                             │
         └─────────────┬───────────────┘                             │
                       ▼                                             │
              改造项9: Application 装配  ◄────────────────────────────┘
                       │
                       ▼
              改造项10: 端到端闭环验证
```

---

## 线 1：HAL + 仿真设备层

### 改造项 1: HAL 接口定义

**问题诊断**

当前代码中设备数据完全由 `MockDataGenerator` 硬编码生成，没有任何硬件抽象：

- `src/Common/MockDataGenerator.cpp`：`generateTargets()` 用 `QRandomGenerator` 随机生成目标，`generateDevices()` 随机生成设备状态。这些数据与真实传感器数据毫无关系——真实传感器会持续输出红外图像、点云、雷达信号，而不是一次性生成一组 TargetInfo。
- `include/MainWindow/MainWindow.cpp` `loadMockData()`：直接调用 `MockDataGenerator::generateTargets(15)`，数据 push 到 Widget 私有成员后就不再变化。
- **造成的影响**：上层模块（DetectionModule、ClassificationModule 等）无法对接任何数据源。没有接口抽象，真实硬件到位后需要重写所有数据接入代码。

**改造方案**

新增 `include/HAL/` 目录，定义设备抽象接口继承树：

```
include/HAL/
  IDevice.h            — 所有设备的基类（生命周期管理、状态查询、诊断）
  ISensor.h            — 传感器基类（采样控制、校准）
  IActuator.h          — 执行器基类（运动控制、任务管理）
  ICamera.h            — 红外/可见光相机（曝光、增益、温度范围）
  ILidar.h             — 激光雷达（扫描角度、分辨率、量程）
  IGpr.h               — 探地雷达（频率、深度范围、测线）
  IMagnetometer.h      — 磁力探测（灵敏度、零场校准）
  IDrone.h             — 无人机（起降、云台、返航）
  IUGV.h               — 排爆机器人（机械臂、夹持）
  SensorData.h         — 统一数据容器（热图像/点云/GPR/磁场/位置）
  DeviceManager.h      — 设备注册/查询/批量管理
  DeviceFactory.h      — 工厂模式创建设备（Simulation/Hardware/Replay 三种模式）
```

接口设计要点：
- `IDevice` 继承 `QObject`，通过 Qt 信号推送数据（`dataReady`、`stateChanged`、`errorOccurred`）
- `ISensor` 增加 `dataReady(QString deviceId, SensorData data)` 信号
- `IActuator` 增加 `positionUpdated`、`taskStateChanged` 信号
- `DeviceFactory` 支持 `Mode::Simulation` / `Mode::Hardware` / `Mode::Replay` 三种创建模式

**验收标准**
- [ ] IDevice/ISensor/IActuator 及 5 个子接口可编译通过
- [ ] DeviceManager 可注册/注销/查询设备
- [ ] DeviceFactory 可根据 config 创建 Simulation 模式设备

---

### 改造项 2: 仿真设备层

**问题诊断**

- `src/Common/MockDataGenerator.cpp`：生成的是静态的 `TargetInfo` 列表，不是传感器原始数据流。真实开发需要的是"传感器持续输出原始数据（红外帧、点云、GPR 信号），由 DetectionModule 处理后转为 TargetInfo"。
- 当前没有任何故障注入能力——无法测试"传感器离线""数据丢包""通信延迟"等场景。
- **造成的影响**：业务模块（DetectionModule 等）无法在真实数据流模式下开发和测试。

**改造方案**

新增 `src/HAL/Sim/` 目录，为每类设备实现仿真：

```
src/HAL/Sim/
  SimDevice.h          — 仿真设备公共接口（场景脚本、故障注入、噪声、延迟、回放）
  SimCamera.cpp        — 实现 ICamera，用 QTimer 定时生成合成热图像
  SimLidar.cpp         — 实现 ILidar，生成合成点云
  SimGpr.cpp           — 实现 IGpr，生成合成 A-Scan 信号
  SimMagnetometer.cpp  — 实现 IMagnetometer，生成磁场异常
  SimDrone.cpp         — 实现 IDrone，模拟飞行轨迹和遥测
  SimUGV.cpp           — 实现 IUGV，模拟机械臂运动
  FaultTypes.h         — 故障类型常量定义
```

仿真设备六项核心能力：
1. **数据回放** — `loadRecordedData(path)` 加载预录数据按时间戳重放
2. **场景脚本** — `loadScenario(path)` 用 JSON 定义行为序列
3. **故障注入** — `injectFault(type, probability)` 按概率模拟设备故障
4. **延迟模拟** — `setLatency(ms)` 模拟真实通信延迟
5. **噪声注入** — `setNoiseLevel(0.0~1.0)` 添加高斯噪声
6. **边界场景** — `triggerLowBattery()` 等触发低电量/信号丢失等场景

**验收标准**
- [ ] SimCamera 启动后以 10Hz 输出合成热图像帧
- [ ] SimDrone 可执行 takeoff → moveTo → land 完整轨迹，位置信号按 10Hz 更新
- [ ] 对 SimCamera 注入 `DataDropout` 故障后，数据流出现间歇性中断
- [ ] 对 SimDrone 设置 200ms 延迟后，指令响应延迟可观测

---

### 改造项 3: 数据录制与回放

**问题诊断**

当前完全没有数据录制机制。这意味着：
- 将来硬件到位后，每次调试都需要硬件在线，无法离线复现问题
- 无法做回归测试——没有固定的测试数据集
- **造成的影响**：开发效率低，bug 难以复现，测试不可重复

**改造方案**

新增 `include/HAL/DataRecorder.h` 和 `src/HAL/DataRecorder.cpp`：

- DataRecorder 作为 SensorBus 的旁路监听者（不增加主数据流延迟）
- 订阅所有 ISensor 的 `dataReady` 信号和 IActuator 的 `positionUpdated`/`telemetryUpdated` 信号
- 写入 `.scenario` 二进制文件（MessagePack 编码）
- DataPlayer 可加载 `.scenario` 文件，通过 SimDevice 回放

文件格式：
```
Header (16B): Magic "UXOS" + Version + DeviceCount + StartTime
Device Directory: [deviceId, deviceType, model] × N
Data Frames: [Timestamp, deviceId, dataType, payload] × 反复
Markers: [timestamp, label] × 反复
Footer: TotalFrames + CRC32
```

**验收标准**
- [ ] 录制 60 秒 SimCamera + SimDrone 数据，生成 .scenario 文件
- [ ] DataPlayer 加载该文件，回放数据与录制时一致（时间戳对齐）
- [ ] 回放过程中可暂停/继续/seek 到指定时间点

---

## 线 2：前端数据层 + UI 骨架

### 改造项 4: FrontendDataStore 数据管理层

**问题诊断**

当前数据散落在各 Widget 私有成员中，存在三个严重问题：

1. **数据孤岛**：`LeftPanelWidget` 持有 `m_targets`，`RightPanelWidget` 持有 `m_devices`，`AlertPanel` 持有 `m_alarms`——各 Widget 互不知道对方的数据，无法联动。
   - 位置：`include/MainWindow/LeftPanelWidget.h` 的 `QVector<Core::TargetInfo> m_targets`
   - 位置：`include/MainWindow/RightPanelWidget.h` 的 `QVector<Core::DeviceInfo> m_devices`

2. **一次性推送**：`MainWindow::loadMockData()` 调用一次 `MockDataGenerator::generateTargets(15)` 后，数据再也不会变化。无法模拟"传感器持续探测发现新目标"的场景。
   - 位置：`src/MainWindow/MainWindow.cpp` `loadMockData()` 方法

3. **无变更通知**：Widget 之间通过 MainWindow 中转信号，没有统一的数据变更通知机制。当一个目标的状态变更时（如从 Detected → Disposing），需要手动调用多个 Widget 的更新方法。
   - 位置：`MainWindow::onTargetSelected()` 中手动调用 `m_rightPanel->setTarget()`

**造成的影响**：MQTT 数据接入后，无法将数据变更广播到所有相关 Widget。数据流无法"活起来"。

**改造方案**

新增 `include/MainWindow/DataStore/` 目录：

```
include/MainWindow/DataStore/
  FrontendDataStore.h   — 单例，管理四个子 Store
  TargetStore.h         — 目标数据（add/update/remove + signals）
  MissionStore.h        — 任务数据
  DeviceStore.h         — 设备数据（含状态/位置/电量）
  AlarmStore.h          — 告警数据（含确认/忽略操作）
```

每个 Store 是 QObject，数据变更时发射信号：
- `TargetStore`: `targetAdded(TargetInfo)` / `targetUpdated(TargetInfo)` / `targetRemoved(QString id)`
- `DeviceStore`: `deviceRegistered(DeviceInfo)` / `deviceStatusChanged(QString id, DeviceStatus)` / `devicePositionUpdated(QString id, QVector3D)`
- `AlarmStore`: `alarmAdded(AlarmInfo)` / `alarmAcknowledged(QString id)`
- `MissionStore`: `missionCreated(MissionInfo)` / `missionStatusChanged(QString id, MissionStatus)`

Widget 改造：
- Widget 不再持有数据私有成员，改为持有 Store 指针
- Widget 在构造时 connect Store 的信号到自己的刷新槽
- 渲染时从 Store 查询当前数据

MainWindow::loadMockData() 改造：
```
旧: MockDataGenerator::generateTargets() → leftPanel->setTargets()
新: dataStore->targetStore()->add(target)  → targetStore 发射 targetAdded → leftPanel 自动刷新
```

**验收标准**
- [ ] FrontendDataStore 单例可全局访问
- [ ] 向 TargetStore 添加一个目标后，LeftPanelWidget 自动显示新行
- [ ] 向 DeviceStore 更新设备位置后，SituationView 中的设备标记自动移动
- [ ] 向 AlarmStore 添加告警后，AlertPanel 和 StatusBarWidget 同时更新

---

### 改造项 5: NavigationWidget 视图切换

**问题诊断**

- `src/MainWindow/NavigationWidget.cpp`：点击导航项时只 `qDebug()` 输出日志，发射 `navigationChanged(int index)` 信号。
- `src/MainWindow/MainWindow.cpp` `onNavigationChanged()`：收到信号后只 `qDebug()`，没有切换中央区域的视图。
- **造成的影响**：6 个导航项（态势总览/探测控制/目标分类/任务决策/设备控制/历史回放）中只有"态势总览"可见，其余 5 个完全无法访问。用户看到的系统只有 1/6 的功能。

**改造方案**

- MainWindow 中央区域改为 `QStackedWidget`
- 每个导航项对应一个页面：
  - 0 态势总览 → 当前 RightPanelWidget（含 SituationView）
  - 1 探测控制 → DetectionControlPanel（增强版）
  - 2 目标分类 → ClassificationView（新建，展示分类结果）
  - 3 任务决策 → DecisionView（改造项 6 实现）
  - 4 设备控制 → DeviceControlView（标注"未实现"）
  - 5 历史回放 → HistoryView（新建，时间轴）
- `onNavigationChanged(int index)` 改为 `m_stackedWidget->setCurrentIndex(index)`

**验收标准**
- [ ] 点击 6 个导航项，中央区域切换到对应视图
- [ ] 未实现的视图显示"功能开发中"占位提示
- [ ] 视图切换不影响各 Store 中的数据状态

---

### 改造项 6: DecisionView + LeftPanel 筛选/排序/批量

**问题诊断（DecisionView）**

- `src/MainWindow/DecisionView.cpp`：构造函数只初始化 nullptr，连 `setupUi()` 都没调用。是一个完全没有 UI 的 Widget。
- `include/MainWindow/DecisionView.h`：声明了 `setupUi()` 但无实现。
- **造成的影响**：SRS 中"任务决策"是核心功能（任务规划、路径规划、应急预案），但对应的视图完全不存在。

**问题诊断（LeftPanel 筛选/排序/批量）**

- `src/MainWindow/LeftPanelWidget.cpp`：搜索栏只做行隐藏（`setRowHidden`），O(n×m) 遍历。筛选按钮（按类型/威胁等级/状态）是空壳，点击无反应。
- `BatchOperationBar` 的 checkbox 选中逻辑未与 LeftPanel 联动——`BatchOperationBar` 始终显示"已选中 0 个目标"。
- **造成的影响**：当目标数量增多时，指挥员无法有效筛选和管理目标。批量操作完全不可用。

**改造方案**

DecisionView 实现：
```
布局：
  ┌─────────────┬───────────────────────────────┐
  │ 任务列表     │  任务详情                       │
  │ (QListWidget)│  - 目标信息（类型/位置/威胁）    │
  │              │  - 分配设备                     │
  │              │  - 优先级                       │
  │              │  - 路径规划结果（可视化）         │
  │              │  - 操作按钮（审批/启动/取消）     │
  └─────────────┴───────────────────────────────┘
数据源：MissionStore + TargetStore
```

LeftPanel 改造：
- 筛选：弹出对话框，按 TargetType / ThreatLevel / TargetStatus / 时间范围筛选
- 排序：表头点击，按 威胁等级 / 检测时间 / 置信度 排序
- 批量：每行添加 QCheckBox，选中后 BatchOperationBar 实时更新计数，"批量分配"按钮可创建批量任务

**验收标准**
- [ ] DecisionView 显示任务列表，点击任务显示详情
- [ ] LeftPanel 筛选对话框可按类型+威胁等级组合筛选
- [ ] 表头点击可切换排序方向
- [ ] 选中 3 个目标后 BatchOperationBar 显示"已选中 3 个目标"
- [ ] 点击"批量分配"后在 MissionStore 中创建 3 条任务

---

## 线 3：MQTT 通信层

### 改造项 7: MqttService 实现

**问题诊断**

- `src/App/Application.cpp` `initializeCommunication()`：直接 `return true`，没有任何通信实现。
- `CMakeLists.txt`：`find_package(ZeroMQ QUIET)` 找到后未使用。
- SRS 4.2.1 定义了 5 个内部接口（I-001~I-005），其中 3 个使用 MQTT（态势发布 I-003、指令下发 I-004）。
- **造成的影响**：系统完全孤立，无法收发任何数据。所有模块之间没有通信通道。

**改造方案**

新增 `include/Core/Comm/MqttService.h` 和 `src/Core/Comm/MqttService.cpp`：

```cpp
class MqttService : public QObject {
    // 连接管理
    bool connectToBroker(const QString &host, int port);
    bool isConnected();
    void disconnect();

    // 发布/订阅
    void publish(const QString &topic, const QByteArray &payload);
    void subscribe(const QString &topic);
    void unsubscribe(const QString &topic);

signals:
    void messageReceived(const QString &topic, const QByteArray &payload);
    void connectionStateChanged(bool connected);
};
```

MQTT Topic 设计：
```
uxo/sensor/{deviceId}/data     — 传感器数据（热图像/点云/GPR/磁场）
uxo/device/{deviceId}/status   — 设备状态（位置/电量/姿态）
uxo/device/{deviceId}/command  — 设备控制指令
uxo/targets/added              — 新目标发现
uxo/targets/updated            — 目标状态更新
uxo/alarms/new                 — 新告警
uxo/missions/command           — 任务指令
```

依赖：Qt MQTT 模块（Qt 5.15 内置 `Qt5::Mqtt`）

**验收标准**
- [ ] MqttService 可连接本地 MQTT Broker（mosquitto）
- [ ] 发布消息到 topic 后，订阅方可收到
- [ ] 断线后自动重连，重连成功发射 `connectionStateChanged(true)`
- [ ] 离线模式下消息缓存，恢复连接后补发

---

### 改造项 8: SensorSim 模拟数据源

**问题诊断**

当前 MockDataGenerator 是在 C++ 进程内直接生成数据，无法模拟"外部传感器通过网络发送数据"的真实场景。
- 位置：`src/Common/MockDataGenerator.cpp` — 在 MainWindow 进程内直接调用
- **造成的影响**：无法验证 MQTT 通信链路、无法测试网络延迟/丢包、无法模拟多设备并发数据流。

**改造方案**

新建 `scripts/sensor_sim/` 目录，用 Python 实现独立的模拟数据源进程：

```
scripts/sensor_sim/
  sim_main.py          — 主入口，加载场景配置
  sim_camera.py        — 模拟红外相机，通过 MQTT 发布热图像数据
  sim_drone.py         — 模拟无人机，发布位置/电量/姿态
  sim_gpr.py           — 模拟探地雷达
  scenarios/
    runway_bomb.json   — 场景脚本：跑道发现未爆航弹
    cluster_field.json — 场景脚本：集束弹药散布区域
```

工作方式：
1. 读取场景脚本 JSON（定义目标位置/类型/出现时间）
2. 按时间轴通过 MQTT 发布模拟传感器数据
3. 可配置发布频率、延迟、丢包率

依赖：`paho-mqtt` Python 库

**验收标准**
- [ ] 启动 sim_main.py 后，MQTT Broker 上可看到 `uxo/sensor/+/data` 主题消息
- [ ] 运行 "runway_bomb" 场景，60 秒内发布 5 个模拟目标
- [ ] 可配置 200ms 延迟和 5% 丢包率

---

## 汇合点

### 改造项 9: Application::initializeModules() 装配

**问题诊断**

- `src/App/Application.cpp`：`initializeDatabase()`、`initializeCommunication()`、`initializeModules()` 全部 `return true` 空实现。
- **造成的影响**：系统启动后没有任何模块被初始化，所有基础设施都不存在。

**改造方案**

重写 `Application::initializeModules()`：

```cpp
bool Application::initializeModules() {
    // 1. 加载配置
    QJsonObject config = loadJsonConfig(m_configPath);
    QString mode = config.value("mode").toString("simulation");

    // 2. 创建设备工厂与管理器（线1）
    m_deviceFactory = HAL::DeviceFactory::create(config, this);
    m_deviceManager = new HAL::DeviceManager(this);
    for (const auto &devCfg : config["devices"].toArray()) {
        auto *device = m_deviceFactory->createDevice(...);
        m_deviceManager->registerDevice(device);
    }

    // 3. 创建 Core 业务模块（依赖 DeviceManager）
    m_detectionModule = new DetectionModule(m_deviceManager, this);
    m_decisionModule = new DecisionModule(this);
    // ...

    // 4. 创建 FrontendDataStore（线2）
    m_dataStore = new FrontendDataStore(this);

    // 5. 创建 MQTT 服务并连接 Adapter（线3）
    m_mqttService = new MqttService(this);
    m_mqttAdapter = new MqttAdapter(m_mqttService, m_dataStore, this);

    // 6. 创建 UI，注入 Store
    m_mainWindow = new MainWindow(m_dataStore, m_deviceManager, ...);

    // 7. 旁路数据录制
    m_dataRecorder = new HAL::DataRecorder(this);
    for (auto *sensor : m_deviceManager->allSensors())
        m_dataRecorder->subscribeSensor(sensor);

    return true;
}
```

**验收标准**
- [ ] 应用启动后，DeviceManager 中有 5 个仿真传感器 + 2 个仿真执行器
- [ ] FrontendDataStore 中有初始数据（来自仿真设备）
- [ ] MQTT Service 连接到本地 Broker
- [ ] MainWindow 显示数据，数据来自 Store 而非 MockDataGenerator

---

### 改造项 10: 端到端闭环验证

**问题诊断**

当前系统没有任何端到端数据流验证。所有数据是静态的、进程内的。
- **造成的影响**：无法确认"传感器 → 通信 → 存储 → 业务逻辑 → UI"全链路是否通畅。

**改造方案**

编写集成测试脚本，验证完整数据流：

```
SensorSim (Python)
  → MQTT Broker (mosquitto)
    → MqttService (C++)
      → MqttAdapter (解析 JSON → 调用 Store::add)
        → FrontendDataStore (发射 targetAdded 信号)
          → LeftPanelWidget (新增一行)
          → SituationView (新增一个标注点)
```

测试场景：
1. 启动 mosquitto MQTT Broker
2. 启动排弹系统主程序
3. 启动 SensorSim，加载 "runway_bomb" 场景
4. 验证：60 秒内 UI 上出现 5 个新目标标注点
5. 验证：目标列表自动新增 5 行
6. 验证：StatusBar 告警计数更新

**验收标准**
- [ ] SensorSim 发布一条目标数据 → 3 秒内 UI 上出现对应标注
- [ ] 连续发布 100 条目标数据 → 无丢失、无崩溃、帧率 ≥30fps
- [ ] SensorSim 停止发布 → UI 保持最后状态，不崩溃
- [ ] MQTT Broker 断开 → UI 显示"通信中断"告警 → Broker 恢复 → 自动重连

---

## 执行顺序与依赖关系

```
Day 1-3 (并行):
  线1: 改造项1 (HAL接口)     ← 无依赖
  线2: 改造项4 (DataStore)   ← 无依赖
  线3: 改造项7 (MQTT服务)    ← 无依赖

Day 4-7 (并行):
  线1: 改造项2 (仿真设备)    ← 依赖改造项1
  线2: 改造项5 (视图切换)    ← 依赖改造项4
       改造项6 (UI补全)      ← 依赖改造项4
  线3: 改造项8 (SensorSim)   ← 依赖改造项7

Day 8-10 (并行):
  线1: 改造项3 (数据录制)    ← 依赖改造项2
  汇合: 改造项9 (装配)       ← 依赖改造项1+4+7

Day 11-12:
  改造项10 (端到端验证)      ← 依赖改造项9
```

## 里程碑

| 天数 | 里程碑 | 可演示内容 |
|------|--------|-----------|
| D+3 | M1: 接口就绪 | HAL 接口可编译；DataStore 单例可用；MQTT 可收发消息 |
| D+7 | M2: 仿真就绪 | SimCamera 输出热图像；6 个导航视图可切换；SensorSim 发布数据 |
| D+10 | M3: 装配完成 | Application 启动后所有模块初始化；数据从 Store 流向 UI |
| D+12 | M4: 闭环验证 | SensorSim → MQTT → Store → UI 全链路通畅 |

---

## 待确认事项

1. **MQTT Broker 选型**：开发环境用 mosquitto，生产环境是否需要 EMQX？
2. **国产 OS 验证时机**：PM 建议本周在麒麟/统信上编译验证 Qt 3D，是否现在就做？
3. **殉爆安全链算法**：是否需要在本阶段就开始预研，还是等 DecisionModule 搭建后再启动？
4. **SQLite vs PostgreSQL**：开发阶段用 SQLite，是否需要现在就定义 PostgreSQL 迁移路径？

---

## 关联

- DDR-001: 项目现状评审结论
- DDR-002: 硬件缺位下的开发策略
