# 排弹抢修指挥系统 项目结构文档

**版本**: V1.0.0

**日期**: 2024年

**技术栈**: Qt 5.15 LTS + CMake

---

## 1. 项目概述

排弹抢修指挥系统（UXOMissionControl）是一款基于Qt框架开发的智能化排爆管理软件，采用C/S架构，分为客户端和服务器端。系统提供全域探测、目标识别、三维态势呈现、智能决策和排爆处置等功能。

## 2. 技术架构

| 层级 | 组件 | 技术选型 |
|------|------|----------|
| 应用框架 | Qt 5.15 LTS | 跨平台C++框架 |
| 3D渲染 | Qt 3D | 三维态势可视化 |
| 通信框架 | MQTT + WebSocket | 实时数据传输 |
| 消息队列 | ZeroMQ | 进程间通信 |
| 数据库 | PostgreSQL + Redis | 数据持久化 |
| AI推理 | TensorFlow Lite | 目标识别 |

## 3. 目录结构

```
UXO_v1/
├── CMakeLists.txt                    # 根级CMake配置
├── README.md                         # 项目说明文档
├── SRS排弹抢修指挥系统_v1.0.md       # 软件需求规格说明书
├── SDD排弹抢修指挥系统_v1.0.md       # 软件设计说明书
│
├── include/                          # 公共头文件目录
│   ├── App/                          # 应用程序入口
│   │   └── Application.h
│   ├── MainWindow/                   # 主窗口
│   │   ├── MainWindow.h
│   │   ├── SituationView.h           # 态势显示视图
│   │   ├── DeviceControlView.h        # 设备控制视图
│   │   ├── DecisionView.h            # 决策处置视图
│   │   └── StatusBarWidget.h          # 状态栏组件
│   │
│   ├── Modules/                      # 功能模块
│   │   ├── SituationModule/           # 态势管理模块
│   │   │   ├── SituationManager.h
│   │   │   ├── ThreeDMapRenderer.h
│   │   │   └── AnnotationManager.h
│   │   │
│   │   ├── DetectionModule/          # 全域探测模块
│   │   │   ├── DetectionController.h
│   │   │   ├── SensorManager.h
│   │   │   └── DataPreprocessor.h
│   │   │
│   │   ├── ClassificationModule/     # 目标分类模块
│   │   │   ├── TargetClassifier.h
│   │   │   ├── ImageRecognizer.h
│   │   │   └── DataFusionEngine.h
│   │   │
│   │   ├── DecisionModule/           # 决策逻辑模块
│   │   │   ├── MissionPlanner.h
│   │   │   ├── PathPlanner.h
│   │   │   └── EmergencyPlanManager.h
│   │   │
│   │   └── DisposalModule/           # 排爆处置模块
│   │       ├── DisposalController.h
│   │       └── EffectEvaluator.h
│   │
│   ├── Core/                         # 核心模块
│   │   ├── Data/                     # 数据模型
│   │   │   ├── Types.h               # 基础数据类型
│   │   │   ├── AirportData.h        # 机场数据结构
│   │   │   ├── TargetInfo.h
│   │   │   ├── MissionInfo.h
│   │   │   ├── DeviceInfo.h
│   │   │   └── AlarmInfo.h
│   │   │
│   │   ├── 3D/                       # 3D渲染模块
│   │   │   └── AirportSceneFactory.h # 机场场景工厂
│   │   │
│   │   ├── Comm/                     # 通信服务
│   │   │   ├── MqttService.h
│   │   │   ├── WebSocketService.h
│   │   │   └── DataCollectionService.h
│   │   │
│   │   ├── Algo/                     # 算法模块
│   │   │   ├── PointCloudProcessor.h
│   │   │   ├── RadarSignalProcessor.h
│   │   │   └── TargetTracker.h
│   │   │
│   │   └── Storage/                  # 存储模块
│   │       ├── DatabaseManager.h
│   │       └── CacheManager.h
│   │
│   ├── Drivers/                      # 硬件驱动层
│   │   ├── DroneInterface.h
│   │   ├── GprInterface.h
│   │   ├── InfraredCameraInterface.h
│   │   ├── LidarInterface.h
│   │   └── RobotArmInterface.h
│   │
│   └── Common/                       # 公共组件
│       ├── Constants.h
│       ├── Types.h
│       ├── Logger.h
│       └── Utils.h
│
├── src/                             # 源代码目录（与include对应）
│   ├── App/
│   │   ├── Application.cpp
│   │   └── main.cpp
│   │
│   ├── MainWindow/
│   │   ├── MainWindow.cpp
│   │   ├── SituationView.cpp
│   │   ├── DeviceControlView.cpp
│   │   ├── DecisionView.cpp
│   │   └── StatusBarWidget.cpp
│   │
│   ├── Modules/
│   │   ├── SituationModule/
│   │   ├── DetectionModule/
│   │   ├── ClassificationModule/
│   │   ├── DecisionModule/
│   │   └── DisposalModule/
│   │
│   ├── Core/
│   │   ├── Data/
│   │   ├── 3D/
│   │   ├── Comm/
│   │   ├── Algo/
│   │   └── Storage/
│   │
│   ├── Drivers/
│   │
│   └── Common/
│
├── plugins/                         # 插件目录
│   ├──处置列表插件/
│   ├──统计分析插件/
│   ├──系统配置插件/
│   └──信息提示插件/
│
├── resources/                       # 资源文件目录
│   ├── icons/                        # 图标资源
│   ├── styles/                       # 样式表
│   ├── fonts/                        # 字体文件
│   ├── models/                       # 3D模型文件
│   └── shaders/                      # 着色器文件
│
├── config/                          # 配置文件目录
│   ├── system.json                  # 系统配置
│   ├── devices.json                 # 设备配置
│   ├── map.json                      # 地图配置
│   └── algorithms.json              # 算法配置
│
├── data/                           # 数据目录
│   ├── models/                      # 训练模型
│   ├── targets/                     # 目标特征库
│   └── plans/                      # 应急预案
│
├── docs/                           # 开发文档
│   ├── api/                        # API接口文档
│   ├── protocol/                   # 通信协议文档
│   └── manual/                     # 用户手册
│
├── scripts/                        # 脚本目录
│   ├── build.sh                    # 构建脚本
│   ├── deploy.sh                   # 部署脚本
│   └── setup.sh                    # 环境配置脚本
│
├── tests/                          # 测试目录
│   ├── unit/                       # 单元测试
│   ├── integration/                # 集成测试
│   └── system/                     # 系统测试
│
└── build/                          # 构建输出目录（不提交）
```

## 4. 模块说明

### 4.1 应用入口层 (App)

应用程序入口模块，负责系统初始化和生命周期管理。

| 文件 | 功能 |
|------|------|
| Application.h/cpp | 系统主应用类，管理全局配置和资源 |
| main.cpp | 程序入口点 |

### 4.2 主窗口层 (MainWindow)

主窗口及界面组件模块。

| 文件 | 功能 |
|------|------|
| MainWindow.h/cpp | 主窗口框架，包含菜单栏、工具栏、状态栏 |
| SituationView.h/cpp | 态势显示视图，三维地图渲染区域 |
| DeviceControlView.h/cpp | 设备控制视图，无人机/机器人控制面板 |
| DecisionView.h/cpp | 决策处置视图，任务规划和执行界面 |
| StatusBarWidget.h/cpp | 状态栏组件，显示系统状态和告警信息 |

### 4.3 功能模块层 (Modules)

#### 态势管理模块 (SituationModule)
- 负责三维战场态势的呈现和交互
- 包含地图渲染、标注管理、图层控制等功能

#### 全域探测模块 (DetectionModule)
- 负责多传感器的控制和数据采集
- 包含传感器管理、数据预处理、飞行控制等功能

#### 目标分类模块 (ClassificationModule)
- 负责目标识别和分类
- 包含图像识别、数据融合、特征提取等功能

#### 决策逻辑模块 (DecisionModule)
- 负责任务规划和决策支持
- 包含任务分配、路径规划、应急预案管理等功能

#### 排爆处置模块 (DisposalModule)
- 负责排爆处置控制
- 包含处置指令下发、效果评估等功能

### 4.4 核心层 (Core)

#### 数据模型 (Data)
- TargetInfo: 目标信息结构
- MissionInfo: 任务信息结构
- DeviceInfo: 设备信息结构
- AlarmInfo: 告警信息结构

#### 通信服务 (Comm)
- MqttService: MQTT通信服务
- WebSocketService: WebSocket通信服务
- DataCollectionService: 数据采集服务

#### 算法模块 (Algo)
- PointCloudProcessor: 点云处理算法
- RadarSignalProcessor: 雷达信号处理
- TargetTracker: 目标跟踪算法

#### 存储模块 (Storage)
- DatabaseManager: 数据库管理
- CacheManager: 缓存管理

### 4.5 驱动层 (Drivers)

硬件设备抽象接口层：

| 接口 | 功能 |
|------|------|
| DroneInterface | 无人机控制接口 |
| GprInterface | 探地雷达接口 |
| InfraredCameraInterface | 红外热像仪接口 |
| LidarInterface | 激光雷达接口 |
| RobotArmInterface | 机械臂控制接口 |

### 4.6 公共组件 (Common)

| 文件 | 功能 |
|------|------|
| Constants.h | 系统常量定义 |
| Types.h | 类型定义 |
| Logger.h | 日志组件 |
| Utils.h | 工具函数 |

## 5. 构建说明

### 5.1 环境要求

- CMake >= 3.16
- Qt 5.15 LTS
- C++17 兼容编译器
- Ubuntu 20.04 LTS (开发环境)
- 银河麒麟 V10 / 统信UOS V20 (部署环境)

### 5.2 构建步骤

```bash
# 创建构建目录
mkdir build && cd build

# 运行CMake配置
cmake ..

# 编译
make -j$(nproc)

# 安装
make install
```

### 5.3 依赖项

- Qt 5.15 Core, Widgets, Gui, 3D*, Network, WebSockets, Sql, Charts
- ZeroMQ (可选)
- PostgreSQL (可选)
- Redis (可选)

## 6. 开发规范

### 6.1 命名规范

- 类名: PascalCase (如: `MainWindow`)
- 方法名: camelCase (如: `processData`)
- 变量名: camelCase (如: `targetList`)
- 常量: UPPER_SNAKE_CASE (如: `MAX_TARGET_COUNT`)

### 6.2 代码组织

- 头文件放在 `include/` 目录
- 源文件放在 `src/` 目录
- 同名头文件和源文件放在同一模块目录

### 6.3 Git提交规范

```
<type>(<scope>): <subject>

Types:
- feat: 新功能
- fix: 修复bug
- docs: 文档更新
- style: 代码格式
- refactor: 重构
- test: 测试
- chore: 构建/工具
```

### 6.4 代码注释规范

#### 6.4.1 文件头注释
每个 `.cpp` 和 `.h` 文件开头必须包含文件头注释：

```cpp
/**
 * @file 文件名.h / 文件名.cpp
 * @brief 简要描述文件功能
 * @details 详细描述（可选）
 * @author 作者名
 * @date 创建日期 YYYY-MM-DD
 * @version 版本号
 */
```

#### 6.4.2 类注释
每个类定义前必须包含类注释：

```cpp
/**
 * @brief 类名简要描述
 * @details 类详细功能描述
 */
class ClassName : public QObject
{
    Q_OBJECT
};
```

#### 6.4.3 方法注释
公共方法必须包含注释：

```cpp
/**
 * @brief 方法功能简要描述
 * @param 参数名 参数描述
 * @return 返回值描述
 * @note 注意事项（可选）
 * @see 相关方法/类（可选）
 */
ReturnType methodName(ParamType param);
```

#### 6.4.4 注释示例

```cpp
/**
 * @file MainWindow.h
 * @brief 主窗口类定义
 * @details 负责系统主界面的创建、布局管理、菜单栏、工具栏、状态栏的初始化
 * @author 开发团队
 * @date 2024-01-01
 * @version 1.0.0
 */

/**
 * @brief 目标信息数据结构
 * @details 存储单个目标的所有属性信息，包括位置、类型、威胁等级等
 */
struct TargetInfo {
    QString id;                    ///< 目标唯一标识符
    TargetType type;               ///< 目标类型
    QVector3D position;            ///< 目标位置坐标
    ThreatLevel threatLevel;       ///< 威胁等级
};
```

#### 6.4.5 注释符号规范

| 符号 | 用途 | 示例 |
|------|------|------|
| `///` | Doxygen 单行注释 | `/// @brief 描述` |
| `/** */` | Doxygen 多行注释 | `/** @brief 描述 */` |
| `//` | 行内普通注释 | `// TODO: 后续优化` |
| `///<` | 成员变量注释 | `QString name; ///< 名称` |
| `//!` | TODO 标记 | `//! TODO: 实现xxx` |
| `//!<` | FIXME 标记 | `//!< FIXME: 修复xxx` |

#### 6.4.6 TODO/FIXME 标记规范
- `//! TODO: 说明需要完成的工作`
- `//! FIXME: 说明需要修复的问题`
- 格式: `//! TODO/FIXME: 描述 @author 作者 @date 日期`

#### 6.4.7 注释检查
- 上线前必须移除所有 `//! TODO` 标记
- 所有 `//! FIXME` 必须已修复或确认已知问题
- 关键逻辑必须有注释说明

---

## 7. 维护说明

| 维护人 | 角色 | 职责 |
|--------|------|------|
| [待补充] | 项目经理 | 项目管理、需求把控 |
| [待补充] | 技术负责人 | 架构设计、技术决策 |
| [待补充] | 开发工程师 | 功能开发、模块实现 |
| [待补充] | 测试工程师 | 测试用例、缺陷管理 |

---

**文档结束**
