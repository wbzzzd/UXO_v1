#ifndef CORE_SIMULATION_DEMOSCENARIOPROVIDER_H
#define CORE_SIMULATION_DEMOSCENARIOPROVIDER_H

// 模拟演示场景提供者
// 仅用于本地演示，不连接真实设备、不发送真实控制命令。
// 后续接入真实数据源时，应通过接口替换，而非修改本文件。

#include "Core/Data/Types.h"

#include <QString>
#include <QVector>
#include <QPointF>
#include <QSizeF>
#include <QDateTime>

namespace Core::Simulation {

// 无人机航点（经纬度 + 高度 + 航向）
// 用于 DroneTelemetrySimulator 沿航线插值输出 GPS 遥测
struct DroneWaypoint {
    double lat = 0.0;       // 纬度（度）
    double lng = 0.0;       // 经度（度）
    double alt = 0.0;       // 高度（米）
    double heading = 0.0;   // 航向角（度，0=北，顺时针）
};

// 检测数据条目（内部使用，驱动检测时机）
// videoPositionMs 是视频位置，用于 DetectionSimulator 内部驱动检测时机，
// 但不在 DetectionResult 公开接口中暴露时间点概念
struct DetectionEntry {
    qint64 videoPositionMs = 0;        // 视频位置（毫秒），内部驱动检测时机
    TargetType type = TargetType::Unknown;  // 目标类型
    QString typeName;                  // 目标类型名称
    QPointF videoBoxPos;               // 红框归一化位置 (0.0-1.0)
    QSizeF videoBoxSize;               // 红框归一化尺寸 (0.0-1.0)
    double confidence = 0.0;           // 置信度
};

// 机场区域边界（经纬度）
// 用于 TacticalMapWidget 卫星底图坐标映射和目标坐标推算
struct AirportBounds {
    double north = 0.0;   // 北边界纬度
    double south = 0.0;   // 南边界纬度
    double west = 0.0;    // 西边界经度
    double east = 0.0;    // 东边界经度
};

// 模拟演示场景数据
// 包含设备、任务以及无人机航线、检测数据、机场边界的本地假数据
struct DemoScenario {
    QString label;                                  // 场景标识，须明确标注"模拟"
    QVector<TargetInfo> targets;                    // 模拟目标列表（初始占位，探测目标由检测模拟器动态生成）
    QVector<MissionInfo> missions;                  // 模拟任务列表
    QVector<DeviceInfo> devices;                     // 模拟设备列表
    QVector<DroneWaypoint> droneRoute;              // 无人机航线航点序列
    QVector<DetectionEntry> detections;             // 检测数据条目
    AirportBounds airportBounds;                    // 机场区域边界
};

// 模拟演示场景提供者
// 提供 MVP 阶段所需的本地模拟数据，所有数据均为假数据，不涉及真实设备或真实任务。
class DemoScenarioProvider
{
public:
    // 创建并返回一个模拟演示场景
    static DemoScenario create();
};

}

#endif
