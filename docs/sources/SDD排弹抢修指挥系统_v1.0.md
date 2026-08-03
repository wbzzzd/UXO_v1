# 排弹抢修指挥系统 软件设计说明书

**文档编号**：SDD-UXO-2024-001

**版本**：V1.0

**日期**：2024年

**关联文档**：SRS-UXO-2024-001 软件需求规格说明书

---

## 目录

1. [引言](#1-引言)
2. [系统架构设计](#2-系统架构设计)
3. [层次结构设计](#3-层次结构设计)
4. [模块设计](#4-模块设计)
5. [数据结构设计](#5-数据结构设计)
6. [接口设计](#6-接口设计)
7. [关键算法设计](#7-关键算法设计)
8. [界面设计](#8-界面设计)
9. [错误处理设计](#9-错误处理设计)

---

## 1. 引言

### 1.1 目的

本文档在SRS的基础上，详细描述排弹抢修指挥系统的软件架构、模块划分、数据结构、接口定义及关键算法设计，为开发团队提供具体的技术实现指导。

### 1.2 范围

本文档覆盖以下设计内容：

- 系统整体架构设计（C/S架构、4层框架）
- 各软件模块的详细设计
- 核心数据结构定义
- 模块间接口定义
- 关键算法实现方案
- 用户界面布局设计

### 1.3 参考资料

| 文档名称 | 文档编号 | 说明 |
|----------|----------|------|
| 排弹方案0919 | - | 系统总体设计方案 |
| SRS-UXO-2024-001 | 软件需求规格说明书 | 系统需求定义 |

---

## 2. 系统架构设计

### 2.1 整体架构

本系统采用C/S（客户端/服务器）架构，按照逻辑架构统一设计的原则，对软件进行4层框架设计。

```
┌─────────────────────────────────────────────────────────────────┐
│                         客户端应用层                             │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐    │
│  │态势呈现 │ │设备控制 │ │决策支持 │ │数据管理 │ │系统管理 │    │
│  │ 界面   │ │ 界面   │ │ 界面   │ │ 界面   │ │ 界面   │    │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘    │
├───────┼───────────┼───────────┼───────────┼───────────┼─────────┤
│       │           │           │           │           │          │
│  ┌────┴───────────┴───────────┴───────────┴───────────┴────┐   │
│  │                      插件层                                │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐     │   │
│  │  │处置列表  │ │统计分析  │ │系统配置  │ │信息提示  │     │   │
│  │  │  插件   │ │  插件   │ │  插件   │ │  插件   │     │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘     │   │
│  └──────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                       通信服务层                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │数据采集服务 │ │态势分发服务 │ │指令下发服务 │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
├─────────────────────────────────────────────────────────────────┤
│                       数据处理层                                  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐               │
│  │点云处理 │ │图像处理 │ │雷达处理 │ │AI推理  │               │
│  │ 服务   │ │ 服务   │ │ 服务   │ │ 服务   │               │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘               │
├─────────────────────────────────────────────────────────────────┤
│                       硬件抽象层                                  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐               │
│  │无人机   │ │探地雷达 │ │红外热像 │ │机械臂   │               │
│  │ 控制   │ │ 控制   │ │ 控制   │ │ 控制   │               │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         服务器端                                  │
│  ┌─────────────────┐  ┌─────────────────┐                      │
│  │ 数据处理服务器  │  │ 数据存储服务器  │                      │
│  │ - 实时数据处理  │  │ - 态势数据持久化 │                      │
│  │ - AI推理计算   │  │ - 历史数据管理   │                      │
│  │ - 态势融合    │  │ - 预案存储      │                      │
│  └─────────────────┘  └─────────────────┘                      │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 技术选型

| 层级 | 技术选型 | 说明 |
|------|----------|------|
| **应用框架** | Qt 5.15 LTS | 成熟稳定的跨平台C++框架，支持Linux/国产操作系统 |
| **3D渲染** | Qt 3D / OSG | 三维战场态势可视化 |
| **AI推理** | TensorFlow Lite | 目标识别模型推理 |
| **数据库** | PostgreSQL + Redis | 结构化数据+缓存 |
| **通信协议** | MQTT + WebSocket | 实时数据传输 |
| **消息队列** | ZeroMQ | 高性能进程间通信 |

**跨平台兼容性说明**：
- 开发环境：Ubuntu 20.04 LTS
- 目标部署：银河麒麟 V10 / 统信UOS V20
- 由于Qt和所有选型组件均为跨平台设计，代码可零修改部署至国产操作系统

### 2.3 部署架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        指挥方舱（室内）                          │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                   排弹抢修指挥席终端                     │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐      │    │
│  │  │主显示屏 │ │副显示屏 │ │控制终端 │ │操作终端 │      │    │
│  │  │(态势)  │ │(视频)  │ │(指挥)  │ │(处置)  │      │    │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘      │    │
│  └─────────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────────┤
│                        网络基础设施                              │
│              (光纤 + 无线网桥 + 移动网络)                       │
├─────────────────────────────────────────────────────────────────┤
│                      场站基础设施（室外）                        │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐               │
│  │无人机   │ │无人机   │ │排爆     │ │探测     │               │
│  │+机巢   │ │+机巢   │ │机器人   │ │设备     │               │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        塔台机房（室内）                          │
│  ┌─────────────────┐  ┌─────────────────┐                      │
│  │ 数据处理服务器  │  │ 数据存储服务器  │                      │
│  │ (迎达RS6228H)  │  │ (迎达RS6228H)  │                      │
│  │ 安装麒麟/UOS   │  │ 安装麒麟/UOS   │                      │
│  └─────────────────┘  └─────────────────┘                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. 层次结构设计

### 3.1 四层框架定义

#### 3.1.1 感知层

感知层是系统的基础，负责对机场及周边区域进行全方位、高精度的侦察。

| 组件 | 功能 | 数据输出 |
|------|------|----------|
| 红外热成像系统 | 目标热特征捕捉 | 红外图像流 |
| 3D激光扫描系统 | 三维点云采集 | 激光点云 |
| 广角相机 | 光学图像采集 | 可见光图像 |
| 车载探地雷达 | 地下目标探测 | 雷达反射信号 |
| 磁力探测系统 | 金属目标定位 | 磁场强度数据 |

#### 3.1.2 硬件层

硬件层分为传输层和处理层：

**传输层**：
- 高速无线通信链路：远距离、高带宽数据传输
- 光纤通信：室内固定设备间通信
- MQTT消息总线：设备控制指令传输

**处理层**：
- 数据预处理与校准
- 多源数据融合
- 隐患识别与分类
- 隐患信息标注与传输

#### 3.1.3 决策层

决策层是系统的指挥中心：

| 组件 | 功能 |
|------|------|
| 交互式操作平台 | 态势显示、指令下达 |
| 任务规划器 | 任务分配、优先级排序 |
| 路径规划算法 | 最优路径计算、避障 |
| 自主决策逻辑 | 离线预案调用 |
| 应急预案管理 | 弹药数据库、处置预案 |

#### 3.1.4 执行层

执行层负责执行决策层下达的指令：

| 设备 | 功能 |
|------|------|
| 无人设备 | 侦察、处置任务执行 |
| 定向能量武器 | 电子引信目标处置 |
| 液压拆解系统 | 机械引信目标拆解 |

---

## 4. 模块设计

### 4.1 模块架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                     排弹抢修指挥系统                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    应用层模块                            │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │   │
│  │  │S_main   │ │S_situation│ │S_decision│ │S_device │        │   │
│  │  │主模块   │ │态势模块 │ │决策模块 │ │设备模块 │        │   │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    核心层模块                            │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │   │
│  │  │C_data   │ │C_algo   │ │C_comm   │ │C_storage│        │   │
│  │  │数据模块 │ │算法模块 │ │通信模块 │ │存储模块 │        │   │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    驱动层模块                            │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │   │
│  │  │D_lidar  │ │D_radar  │ │D_infrared│ │D_arm   │        │   │
│  │  │激光雷达 │ │探地雷达 │ │红外热像 │ │机械臂   │        │   │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 核心模块详细设计

#### 4.2.1 S_main 主模块

```cpp
// 模块：S_main
// 功能：应用程序主入口，系统初始化和生命周期管理

class MainModule : public QMainWindow {
    // 功能：系统主界面
    // - 创建主窗口布局
    // - 初始化各子模块
    // - 管理系统退出
};

class SystemInitializer {
    // 功能：系统初始化
    // - 加载配置文件
    // - 连接数据库
    // - 初始化通信服务
    // - 加载插件模块
};

class System lifecycleManager {
    // 功能：系统生命周期管理
    // - 系统启动/停止
    // - 运行状态监控
    // - 异常处理
};
```

#### 4.2.2 S_situation 态势模块

```cpp
// 模块：S_situation
// 功能：战场态势呈现和交互

class SituationDisplay : public QWidget {
    // 功能：态势显示主界面
    // - 三维地图渲染
    // - 隐患标注显示
    // - 态势信息面板
};

class ThreeDMapWidget : public QWidget {
    // 功能：三维地图组件
    // - Qt 3D场景渲染
    // - 地图缩放/旋转/平移
    // - 目标选中交互
};

class SituationFusion {
    // 功能：态势融合
    // - 多源数据融合
    // - 三维模型构建
    // - 态势实时更新
};

class AnnotationManager {
    // 功能：标注管理
    // - 隐患标注创建/更新/删除
    // - 标注样式管理
    // - 标注查询定位
};
```

#### 4.2.3 S_decision 决策模块

```cpp
// 模块：S_decision
// 功能：任务规划和决策支持

class MissionPlanner {
    // 功能：任务规划器
    // - 隐患威胁评估
    // - 任务分配算法
    // - 优先级排序
};

class PathPlanner {
    // 功能：路径规划器
    // - A*路径算法
    // - 殉爆安全距离计算
    // - 动态避障重规划
};

class EmergencyPlanManager {
    // 功能：应急预案管理器
    // - 离线弹药数据库
    // - 预案检索匹配
    // - 预案调用执行
};

class AutoDecisionEngine {
    // 功能：自主决策引擎
    // - 规则引擎
    // - 决策逻辑执行
    // - 离线模式切换
};
```

#### 4.2.4 S_device 设备模块

```cpp
// 模块：S_device
// 功能：设备控制和管理

class DeviceManager {
    // 功能：设备管理器
    // - 设备发现注册
    // - 设备状态监控
    // - 设备故障诊断
};

class DroneController {
    // 功能：无人机控制
    // - 起飞/降落控制
    // - 航线规划执行
    // - 状态监控
};

class RobotArmController {
    // 功能：机械臂控制
    // - 关节控制
    // - 末端执行器控制
    // - 力控模式
};

class SensorInterface {
    // 功能：传感器接口
    // - 红外热像仪接口
    // - 激光雷达接口
    // - 探地雷达接口
};
```

#### 4.2.5 C_data 数据模块

```cpp
// 模块：C_data
// 功能：数据模型定义和管理

struct TargetInfo {
    QString id;              // 目标ID
    TargetType type;        // 类型（未爆导弹/集束弹药/反跑道雷）
    QVector3D position;     // 位置
    double depth;           // 埋深
    QVector3D attitude;     // 姿态角
    double confidence;      // 置信度
    ThreatLevel threatLevel; // 威胁等级
    QDateTime detectTime;   // 发现时间
};

struct MissionInfo {
    QString id;              // 任务ID
    MissionType type;       // 任务类型
    QString targetId;        // 关联目标ID
    MissionStatus status;   // 任务状态
    QDateTime createTime;   // 创建时间
    QString assignee;       // 执行人
};

class TargetDatabase {
    // 功能：目标数据库
    // - 目标CRUD操作
    // - 目标查询统计
    // - 目标轨迹追踪
};
```

#### 4.2.6 C_algo 算法模块

```cpp
// 模块：C_algo
// 功能：核心算法实现

class PointCloudProcessor {
    // 功能：点云处理
    // - 去噪滤波
    // - 点云配准
    // - 特征提取
};

class ImageRecognizer {
    // 功能：图像识别
    // - 深度学习推理
    // - 目标检测/分类
    // - 特征比对
};

class RadarSignalProcessor {
    // 功能：雷达信号处理
    // - 信号滤波
    // - 目标检测
    // - 目标分类
};

class DataFusionEngine {
    // 功能：数据融合引擎
    // - 时空对齐
    // - 特征融合
    // - 目标关联
};
```

#### 4.2.7 C_comm 通信模块

```cpp
// 模块：C_comm
// 功能：通信服务

class MqttService {
    // 功能：MQTT通信服务
    // - 连接管理
    // - 主题订阅发布
    // - 消息路由
};

class DataCollectionService {
    // 功能：数据采集服务
    // - 传感器数据接收
    // - 数据缓冲
    // - 数据分发
};

class CommandDispatchService {
    // 功能：指令下发服务
    // - 指令封装
    // - 下发确认
    // - 重传机制
};

class WebSocketService {
    // 功能：WebSocket服务
    // - 前端通信
    // - 数据推送
    // - 心跳检测
};
```

#### 4.2.8 C_storage 存储模块

```cpp
// 模块：C_storage
// 功能：数据持久化

class DatabaseManager {
    // 功能：数据库管理器
    // - 连接池管理
    // - 事务处理
    // - SQL执行
};

class CacheManager {
    // 功能：缓存管理器
    // - Redis缓存操作
    // - 热点数据缓存
    // - 缓存失效策略
};

class FileStorageService {
    // 功能：文件存储服务
    // - 大文件存储
    // - 点云文件存储
    // - 图像文件存储
};
```

---

## 5. 数据结构设计

### 5.1 核心数据模型

#### 5.1.1 目标信息表 (t_target)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| id | VARCHAR(64) | 主键 | PK |
| target_type | VARCHAR(32) | 类型 | NOT NULL |
| longitude | DOUBLE | 经度 | NOT NULL |
| latitude | DOUBLE | 纬度 | NOT NULL |
| altitude | DOUBLE | 高度/埋深 | |
| position_x | DOUBLE | X坐标 | |
| position_y | DOUBLE | Y坐标 | |
| position_z | DOUBLE | Z坐标 | |
| heading | DOUBLE | 朝向角 | |
| pitch | DOUBLE | 俯仰角 | |
| roll | DOUBLE | 横滚角 | |
| confidence | DOUBLE | 置信度 | 0-1 |
| threat_level | INT | 威胁等级 | 1-5 |
| status | INT | 状态 | 0-4 |
| image_path | VARCHAR(256) | 图像路径 | |
| radar_data_path | VARCHAR(256) | 雷达数据路径 | |
| detect_time | TIMESTAMP | 发现时间 | |
| update_time | TIMESTAMP | 更新时间 | |
| remark | TEXT | 备注 | |

#### 5.1.2 任务信息表 (t_mission)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| id | VARCHAR(64) | 主键 | PK |
| mission_type | VARCHAR(32) | 任务类型 | NOT NULL |
| target_id | VARCHAR(64) | 关联目标ID | FK |
| priority | INT | 优先级 | 1-5 |
| status | INT | 状态 | 0-6 |
| assigner | VARCHAR(64) | 指派人 | |
| assignee | VARCHAR(64) | 执行人 | |
| plan_start_time | TIMESTAMP | 计划开始时间 | |
| plan_end_time | TIMESTAMP | 计划结束时间 | |
| actual_start_time | TIMESTAMP | 实际开始时间 | |
| actual_end_time | TIMESTAMP | 实际结束时间 | |
| result | INT | 执行结果 | |
| create_time | TIMESTAMP | 创建时间 | |
| update_time | TIMESTAMP | 更新时间 | |

#### 5.1.3 设备信息表 (t_device)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| id | VARCHAR(64) | 主键 | PK |
| device_type | VARCHAR(32) | 设备类型 | NOT NULL |
| device_name | VARCHAR(128) | 设备名称 | |
| manufacturer | VARCHAR(128) | 制造商 | |
| model | VARCHAR(64) | 型号 | |
| serial_number | VARCHAR(64) | 序列号 | |
| status | INT | 状态 | 0-4 |
| battery_level | DOUBLE | 电池电量 | 0-100 |
| position_x | DOUBLE | X坐标 | |
| position_y | DOUBLE | Y坐标 | |
| position_z | DOUBLE | Z坐标 | |
| last_online_time | TIMESTAMP | 最后上线时间 | |
| create_time | TIMESTAMP | 创建时间 | |

#### 5.1.4 告警记录表 (t_alarm)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| id | VARCHAR(64) | 主键 | PK |
| alarm_type | VARCHAR(32) | 告警类型 | NOT NULL |
| alarm_level | INT | 告警级别 | 1-5 |
| target_id | VARCHAR(64) | 关联目标ID | FK |
| device_id | VARCHAR(64) | 关联设备ID | FK |
| message | TEXT | 告警消息 | |
| is_handled | BOOLEAN | 是否处理 | |
| handle_time | TIMESTAMP | 处理时间 | |
| handle_result | TEXT | 处理结果 | |
| create_time | TIMESTAMP | 创建时间 | |

#### 5.1.5 操作日志表 (t_operation_log)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| id | VARCHAR(64) | 主键 | PK |
| operator | VARCHAR(64) | 操作人 | |
| operation_type | VARCHAR(32) | 操作类型 | |
| target_id | VARCHAR(64) | 操作对象ID | |
| operation_detail | TEXT | 操作详情 | |
| before_value | JSON | 操作前值 | |
| after_value | JSON | 操作后值 | |
| ip_address | VARCHAR(64) | IP地址 | |
| create_time | TIMESTAMP | 操作时间 | |

### 5.2 实时数据结构

#### 5.2.1 传感器数据

```cpp
struct SensorDataHeader {
    QString deviceId;        // 设备ID
    QString sensorType;      // 传感器类型
    QDateTime timestamp;     // 时间戳
    int dataSize;           // 数据大小
};

struct PointCloudData {
    SensorDataHeader header;
    QVector<QVector3D> points;
    QVector<float> intensities;
    QVector<int> classifications;
};

struct ImageData {
    SensorDataHeader header;
    cv::Mat image;          // 图像数据
    double latitude;        // 拍摄位置
    double longitude;
    double altitude;
    double heading;
};

struct RadarData {
    SensorDataHeader header;
    QVector<QVector<float>> radarMatrix;
    double centerFrequency;
    double bandwidth;
    double maxDepth;
};
```

#### 5.2.2 态势数据

```cpp
struct SituationUpdate {
    QString updateId;
    QDateTime timestamp;
    QVector<TargetInfo> targets;
    QVector<MissionInfo> missions;
    QVector<DeviceStatus> devices;
    QVector3D observerPosition;
    QVector3D observerAttitude;
};

struct ThreeDMapData {
    QVector<TriangleMesh> terrainMesh;
    QVector<PointCloudData> obstacles;
    QVector<AnnotationData> annotations;
    QDateTime buildTime;
};
```

---

## 6. 接口设计

### 6.1 内部接口

#### 6.1.1 模块间通信接口

```cpp
// 态势模块 -> 决策模块
interface ISituationDecisionBridge {
    // 功能：态势信息传递给决策模块
    // 输入：态势更新数据
    // 输出：决策建议
    DecisionSuggestion onSituationUpdate(SituationUpdate situation);
};

// 决策模块 -> 执行模块
interface IDecisionExecutionBridge {
    // 功能：决策指令下发给执行模块
    // 输入：处置指令
    // 输出：执行结果
    ExecutionResult onCommandDispatch(Command command);
};

// 数据模块 -> 各业务模块
interface IDataAccessInterface {
    // 目标数据访问
    TargetInfo getTargetById(QString targetId);
    QVector<TargetInfo> queryTargets(TargetQuery query);

    // 任务数据访问
    MissionInfo getMissionById(QString missionId);
    bool createMission(MissionInfo mission);
    bool updateMissionStatus(QString missionId, MissionStatus status);

    // 告警数据访问
    QVector<AlarmInfo> queryAlarms(TimeRange range, AlarmQuery query);
    bool handleAlarm(QString alarmId, HandleResult result);
};
```

#### 6.1.2 服务间通信协议

| 服务对 | 协议 | 端口 | 数据格式 |
|--------|------|------|----------|
| 客户端-服务器 | WebSocket | 8080 | JSON |
| 客户端-数据服务 | gRPC | 9090 | Protobuf |
| 服务器-MQTT Broker | MQTT | 1883 | JSON |
| 数据处理-存储 | ZeroMQ | 5555 | Binary |
| AI推理服务 | HTTP | 8501 | JSON |

### 6.2 硬件接口

#### 6.2.1 无人机接口

```cpp
// 无人机控制接口
interface IDroneInterface {
    // 连接无人机
    bool connect(QString deviceId);
    bool disconnect();

    // 飞行控制
    bool takeOff();
    bool land();
    bool goToPosition(double lat, double lon, double alt);
    bool hover();
    bool returnToHome();

    // 任务控制
    bool startMission(QString missionId);
    bool pauseMission();
    bool resumeMission();
    bool abortMission();

    // 状态查询
    DroneStatus getStatus();
    QVector3D getPosition();
    double getBatteryLevel();
};

// 无人机状态
struct DroneStatus {
    bool isConnected;
    bool isFlying;
    DroneMode mode;
    QVector3D position;
    double heading;
    double speed;
    double batteryLevel;
    double altitude;
    int signalStrength;
    QString errorMessage;
};
```

#### 6.2.2 探地雷达接口

```cpp
// 探地雷达接口
interface IGprInterface {
    // 设备控制
    bool connect(QString deviceId);
    bool disconnect();
    bool setFrequency(double centerFrequency);
    bool setDepthRange(double maxDepth);

    // 数据采集
    bool startScan();
    bool stopScan();
    RadarData getScanData();

    // 状态查询
    GprStatus getStatus();
};

// 雷达状态
struct GprStatus {
    bool isConnected;
    bool isScanning;
    double centerFrequency;
    double currentDepth;
    int traceCount;
    QString errorMessage;
};
```

#### 6.2.3 红外热像仪接口

```cpp
// 红外热像仪接口
interface IInfraredCameraInterface {
    // 设备控制
    bool connect(QString deviceId);
    bool disconnect();
    bool setGainMode(GainMode mode);
    bool setPalette(PaletteType palette);

    // 图像采集
    bool captureImage();
    ImageData getLatestImage();
    bool startVideoStream();
    bool stopVideoStream();

    // 温度测量
    double getTemperatureAt(double x, double y);
    QVector<TemperaturePoint> getTemperatureMap();

    // 状态查询
    CameraStatus getStatus();
};
```

#### 6.2.4 机械臂接口

```cpp
// 机械臂接口
interface IRobotArmInterface {
    // 连接控制
    bool connect(QString deviceId);
    bool disconnect();
    bool enable();
    bool disable();

    // 关节控制
    bool moveJoints(QVector<double> angles);
    QVector<double> getJointAngles();

    // 末端控制
    bool moveEndEffector(QVector3D position, QVector3D orientation);
    EndEffectorStatus getEndEffectorStatus();

    // 工具控制
    bool selectTool(int toolId);
    bool activateTool();
    bool deactivateTool();

    // 力控模式
    bool enableForceControl();
    bool disableForceControl();
    bool setForceLimit(QVector3D force, QVector3D torque);
};

// 末端执行器状态
struct EndEffectorStatus {
    QVector3D position;
    QVector3D orientation;
    QVector3D force;
    QVector3D torque;
    int currentToolId;
    bool isActive;
};
```

### 6.3 外部接口

#### 6.3.1 地图服务接口

```cpp
// 地图服务接口
interface IMapServiceInterface {
    // 底图获取
    QImage getMapTile(double lon, double lat, int zoom);
    QImage getAerialImage(double lon, double lat, double width, double height);

    // 地理编码
    QString coordinateToAddress(double lon, double lat);
    QPointF addressToCoordinate(QString address);

    // 路径规划
    QVector<QPointF> routePlanning(QPointF start, QPointF end, RouteType type);
};
```

---

## 7. 关键算法设计

### 7.1 多源数据融合算法

```cpp
// 多源数据融合算法
class MultiSourceFusionAlgorithm {
public:
    // 输入：多源传感器数据
    struct FusionInput {
        PointCloudData::Ptr lidarData;      // 激光点云
        ImageData::Ptr infraredData;         // 红外图像
        ImageData::Ptr opticalData;          // 可见光图像
        RadarData::Ptr radarData;            // 雷达数据
        MagnetData::Ptr magnetData;          // 磁力数据
    };

    // 输出：融合后的目标列表
    struct FusionOutput {
        QVector<FusedTarget> targets;
        ThreeDMapData::Ptr mapData;
        double processingTime;
    };

    // 融合步骤
    // 1. 时空对齐 - 将不同传感器的数据对齐到统一时空坐标系
    void temporalAlignment(FusionInput& input);

    // 2. 特征提取 - 从各传感器数据中提取目标特征
    void featureExtraction(FusionInput& input, FeatureData& features);

    // 3. 目标关联 - 关联不同传感器的同一目标
    void targetAssociation(FeatureData& features, AssociationResult& result);

    // 4. 状态估计 - 估计目标的位置、速度、类型等状态
    void stateEstimation(AssociationResult& result, FusionOutput& output);

    // 5. 地图更新 - 更新三维战场地图
    void mapUpdate(FusionOutput& output);
};
```

### 7.2 路径规划算法

```cpp
// 路径规划算法
class PathPlanningAlgorithm {
public:
    // A*算法实现
    struct PathNode {
        QVector2D position;
        double gCost;    // 从起点到当前节点的实际代价
        double hCost;    // 从当前节点到终点的估计代价
        double fCost;    // 总代价 gCost + hCost
        PathNode* parent;
    };

    // 输入：起点、终点、障碍物地图
    struct PathPlanningInput {
        QVector2D startPos;
        QVector2D goalPos;
        QVector<Obstacle> obstacles;
        double safeDistance;    // 安全距离
    };

    // 输出：最优路径
    struct PathPlanningOutput {
        QVector<QVector2D> path;
        double totalDistance;
        double estimatedTime;
        bool pathFound;
    };

    // 主路径规划函数
    PathPlanningOutput planPath(const PathPlanningInput& input);

    // 殉爆安全链计算
    double calculateSafeDistance(TargetInfo target, QString targetType);

    // 动态避障重规划
    PathPlanningOutput replan(const PathPlanningInput& input, QVector<Obstacle> newObstacles);

private:
    // 启发式函数（欧几里得距离）
    double heuristic(QVector2D a, QVector2D b);

    // 检查碰撞
    bool checkCollision(QVector2D pos, const QVector<Obstacle>& obstacles);
};
```

### 7.3 目标识别算法

```cpp
// AI目标识别算法接口
class TargetRecognitionAlgorithm {
public:
    // 图像识别
    struct ImageRecognitionResult {
        QVector<DetectedObject> objects;
        cv::Mat resultImage;
        double processingTime;
    };

    // 雷达信号识别
    struct RadarRecognitionResult {
        QVector<RadarTarget> targets;
        double processingTime;
    };

    // 可见光图像识别
    ImageRecognitionResult recognizeOptical(const cv::Mat& image);

    // 红外图像识别
    ImageRecognitionResult recognizeInfrared(const cv::Mat& image);

    // 雷达信号识别
    RadarRecognitionResult recognizeRadar(const RadarData& data);

    // 综合识别（融合多源）
    TargetInfo::Ptr comprehensiveRecognize(
        const ImageRecognitionResult& optical,
        const ImageRecognitionResult& infrared,
        const RadarRecognitionResult& radar
    );
};
```

---

## 8. 界面设计

### 8.1 主界面布局

```
┌─────────────────────────────────────────────────────────────────────────┐
│  排弹抢修指挥系统 V1.0                  [用户名] 2024-XX-XX  状态: ●在线 │
├────┬──────────────────────────────────┬───────────────────────────────┤
│    │                                  │                               │
│ 导 │        主视图区域                 │       三维态势地图            │
│ 航 │                                  │                               │
│ 菜 │   ┌────────┬────────┬────────┐   │   ● 目标标注                  │
│ 单 │   │ 视频1 │ 视频2 │ 视频3  │   │   ■ 设备位置                  │
│    │   │ UAV1  │ UAV2  │ Robot  │   │   ═ 作业路径                   │
│    │   └────────┴────────┴────────┘   │                               │
│ ○态势│   [1][2][3][4][全屏]          │   ┌─────────────────────┐     │
│ ○探测│   ┌──────────────┬────────┐  │   │     设备状态        │     │
│ ○决策│   │  目标列表    │ 告警   │  │   │  UAV1 ●  UAV2 ○    │     │
│ ○设备│   │ T-01 高      │ ⚠ 告警 │  │   │  Robot ●  雷达 ○    │     │
│ ○统计│   │ T-02 中      │ ● 提示 │  │   └─────────────────────┘     │
│ ○配置│   ├──────────────┴────────┤  │                               │
│    │   │   探测控制/操作记录     │  │   ┌─────────────────────┐     │
│    │   │ [起飞][降落] 高度:50m  │  │   │     决策建议         │     │
│    │   │ 10:30 抵近目标 T-001  │  │   │  建议: 聚能引爆       │     │
├────┴───┴─────────────────────────┴──┴───────────────────────────────┤
│  在线:5 │ 目标:12 │ 告警:2 │ CPU:45% │ 内存:62% │ 网络:稳定            │
└─────────────────────────────────────────────────────────────────────────┘
```

**布局说明：**
- **顶部标题栏**（32px）：系统名称、用户名、时间、在线状态
- **左侧导航栏**（80px）：态势、探测、决策、设备、统计、配置
- **中间主视图区域**（60%宽度）：
  - 视频流分屏区：4路视频，可切换/全屏
  - 信息面板：目标列表、告警信息、探测控制、操作记录
- **右侧面板**（30%宽度）：
  - 三维态势地图（可缩放/切换）
  - 设备状态卡片
  - 决策建议
- **底部状态栏**（28px）：在线设备数、目标数、告警数、系统资源、网络状态

### 8.2 态势显示界面

```
┌─────────────────────────────────────────────────────────────────────────┐
│ 态势监视                                              [全屏] [截图] [录屏] │
├─────────────────────────────────────────────────────────────────────────┤
│ ┌─────────────────────────────────────────────────────────┐ ┌──────────┐ │
│ │                                                         │ │ 目标列表  │ │
│ │                   三维战场态势地图                       │ │          │ │
│ │                                                         │ │ ◎T-001   │ │
│ │              [跑道三维模型+隐患标注]                    │ │ ◎T-002   │ │
│ │                                                         │ │ ◎T-003   │ │
│ │                                                         │ │ ◎T-004   │ │
│ │                                                         │ │ ...      │ │
│ │                                                         │ │          │ │
│ └─────────────────────────────────────────────────────────┘ │          │ │
│ ┌─────────────────────────────────────────────────────────┐ │          │ │
│ │  工具条: [旋转][平移][缩放][复位][标注][测量][框选][多点] │ │ [查看详情]│ │
│ └─────────────────────────────────────────────────────────┘ └──────────┘ │
├─────────────────────────────────────────────────────────────────────────┤
│  颜色图例: ●未爆导弹  ●集束弹药  ●反跑道雷  ●已排除  ○待确认            │
└─────────────────────────────────────────────────────────────────────────┘
```

### 8.3 探测控制界面

```
┌─────────────────────────────────────────────────────────────────────────┐
│  全域探测控制                                                          │
├─────────────────────────────────────────────────────────────────────────┤
│ ┌──────────────────────────┐  ┌──────────────────────────────────────┐ │
│ │     无人机1控制          │  │         传感器状态                   │ │
│ │  ┌────────────────────┐  │  │  ┌────────────────────────────────┐  │ │
│ │  │ [飞行状态指示]     │  │  │  │ 红外热像: ● 运行  温度: 25°C   │  │ │
│ │  │                    │  │  │  │激光雷达: ● 运行  点云: 120Hz  │  │ │
│ │  │  高度: 50m         │  │  │  │广角相机: ● 运行  分辨率: 4K   │  │ │
│ │  │  速度: 10m/s        │  │  │  │探地雷达: ● 运行  深度: 1.5m   │  │ │
│ │  │  电量: 85%          │  │  │  │磁力探测: ● 运行  灵敏度: 高   │  │ │
│ │  └────────────────────┘  │  │  └────────────────────────────────┘  │ │
│ │  [起飞][降落][悬停][返航] │  │                                      │ │
│ └──────────────────────────┘  │  ┌────────────────────────────────┐  │ │
│ ┌──────────────────────────┐  │  │         实时视频               │  │ │
│ │     无人机2控制          │  │  │  ┌──────────────────────────┐  │  │ │
│ │  ┌────────────────────┐  │  │  │  │                          │  │  │ │
│ │  │ [飞行状态指示]     │  │  │  │  │      [红外/可见光]       │  │  │ │
│ │  │                    │  │  │  │  │                          │  │  │ │
│ │  │  高度: 80m         │  │  │  │  │                          │  │  │ │
│ │  │  速度: 8m/s        │  │  │  │  └──────────────────────────┘  │  │ │
│ │  │  电量: 72%         │  │  │  └────────────────────────────────┘  │ │
│ │  └────────────────────┘  │  │                                      │ │
│ │  [起飞][降落][悬停][返航] │  │  [开始探测] [停止探测] [数据回放]     │ │
│ └──────────────────────────┘  └──────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

### 8.4 决策处置界面

```
┌─────────────────────────────────────────────────────────────────────────┐
│  决策处置                                                              │
├─────────────────────────────────────────────────────────────────────────┤
│ ┌─────────────────────────────────┐  ┌────────────────────────────────┐│
│ │         任务列表                 │  │        处置控制                 ││
│ │  ┌───────────────────────────┐  │  │                                ││
│ │  │ ● T-003 反跑道炸弹 [紧急] │  │  │   目标: T-003                  ││
│ │  │   位置: (1234.5, 5678.9)  │  │  │   类型: 反跑道炸弹             ││
│ │  │   埋深: 0.4m              │  │  │   处置方式: [聚能引爆] [转移]  ││
│ │  │   威胁: 高                │  │  │   ┌────────────────────────┐   ││
│ │  │   状态: 待处置            │  │  │   │      [三维视图]         │   ││
│ │  │   [执行] [详情] [取消]    │  │  │   │                        │   ││
│ │  └───────────────────────────┘  │  │   └────────────────────────┘   ││
│ │  ┌───────────────────────────┐  │  │                                ││
│ │  │ ○ T-005 集束弹药 [一般]  │  │  │   [开始处置] [取消] [完成]      ││
│ │  │   位置: (1334.5, 5778.9)  │  │  │                                ││
│ │  │   埋深: 0.1m              │  │  │   处置日志:                     ││
│ │  │   威胁: 中                │  │  │   [10:30:15] 无人机抵近目标     ││
│ │  │   状态: 待分配            │  │  │   [10:30:20] 探测器扫描中      ││
│ │  │   [执行] [详情] [取消]    │  │  │   [10:30:25] 目标确认          ││
│ │  └───────────────────────────┘  │  │                                ││
│ └─────────────────────────────────┘  └────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 9. 错误处理设计

### 9.1 错误分类

| 错误级别 | 说明 | 处理方式 |
|----------|------|----------|
| 0-致命 | 系统无法运行 | 记录日志，告警，尝试恢复 |
| 1-严重 | 模块无法正常工作 | 切换到备用模块，告警 |
| 2-一般 | 功能受限但可运行 | 记录日志，降级处理 |
| 3-提示 | 轻微异常，不影响功能 | 记录日志，提示用户 |

### 9.2 错误码定义

| 错误码 | 模块 | 说明 |
|--------|------|------|
| E1001 | 系统 | 数据库连接失败 |
| E1002 | 系统 | 配置文件加载失败 |
| E2001 | 通信 | MQTT连接断开 |
| E2002 | 通信 | 数据发送超时 |
| E3001 | 设备 | 设备离线 |
| E3002 | 设备 | 设备控制失败 |
| E4001 | 算法 | AI推理超时 |
| E4002 | 算法 | 数据融合失败 |
| E5001 | 任务 | 任务执行失败 |
| E5002 | 任务 | 任务超时 |

### 9.3 异常处理策略

```cpp
// 全局异常处理
class GlobalExceptionHandler {
    // 捕获未处理异常
    void handleUncaughtException(std::exception_ptr e);

    // 记录异常日志
    void logException(const ExceptionInfo& info);

    // 告警通知
    void notifyException(const ExceptionInfo& info);

    // 尝试恢复
    bool tryRecover(const ExceptionInfo& info);
};

// 模块级错误处理
class ModuleErrorHandler {
    // 错误码到错误的映射
    static const QMap<int, QString> errorMessages;

    // 获取错误描述
    static QString getErrorMessage(int errorCode);

    // 错误恢复策略
    ErrorRecoveryStrategy getRecoveryStrategy(int errorCode);
};
```

---

## 附录A：模块依赖关系

```
S_main
├── S_situation
│   ├── C_data
│   └── C_comm
├── S_decision
│   ├── C_data
│   ├── C_algo
│   └── C_comm
├── S_device
│   ├── C_comm
│   └── D_* (设备驱动)
├── C_data
│   └── C_storage
├── C_algo
├── C_comm
│   └── C_storage
└── 插件模块
    ├── C_data
    └── C_comm
```

---

**文档结束**
