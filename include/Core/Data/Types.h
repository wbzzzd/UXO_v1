#ifndef CORE_DATA_TYPES_H
#define CORE_DATA_TYPES_H

#include <QString>
#include <QVector>
#include <QVector3D>
#include <QRectF>
#include <QDateTime>
#include <QtMath>

namespace Core {

enum class TargetType {
    Unknown = 0,
    AirBomb,           // 航弹
    ClusterBomb,       // 子母弹
    AntiRunwayMine,    // 反跑道雷
    CruiseMissile,     // 巡航导弹
    IED,               // 简易爆炸装置
    Other              // 其他
};

enum class ThreatLevel {
    Unknown = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

enum class TargetStatus {
    Unknown = 0,
    Detected = 1,
    Confirmed = 2,
    Pending = 3,
    Disposing = 4,
    Disposed = 5,
    FalseAlarm = 6
};

struct TargetInfo {
    QString id;
    TargetType type;
    QString typeName;
    QVector3D position;
    double depth;
    QVector3D attitude;
    double confidence;
    ThreatLevel threatLevel;
    TargetStatus status;
    QString imagePath;
    QString radarDataPath;
    QDateTime detectTime;
    QDateTime updateTime;
    QString remark;

    TargetInfo()
        : type(TargetType::Unknown)
        , depth(0.0)
        , confidence(0.0)
        , threatLevel(ThreatLevel::Unknown)
        , status(TargetStatus::Unknown)
    {}
};

// 检测结果：检测模拟器输出的单次检测结果
// 接口贴合真实检测器输出，真实系统替换为 AI 推理引擎即可
struct DetectionResult {
    TargetType type;        // 目标类型
    QRectF videoRect;       // 画面红框归一化坐标 [0,1]
    double confidence;      // 置信度 0.0-1.0

    DetectionResult()
        : type(TargetType::Unknown)
        , confidence(0.0)
    {}
};

enum class MissionType {
    Unknown = 0,
    Reconnaissance,
    Disposal,
    Transfer,
    Destruction
};

enum class MissionStatus {
    Unknown = 0,
    Planned,
    PendingApproval,
    Approved,
    Executing,
    Paused,
    Completed,
    Failed,
    Cancelled
};

struct MissionInfo {
    QString id;
    MissionType type;
    QString targetId;
    int priority;
    MissionStatus status;
    QString assigner;
    QString assignee;
    QString deviceId;
    QDateTime planStartTime;
    QDateTime planEndTime;
    QDateTime actualStartTime;
    QDateTime actualEndTime;
    int result;
    QDateTime createTime;
    QDateTime updateTime;
    QString remark;

    MissionInfo()
        : type(MissionType::Unknown)
        , priority(0)
        , status(MissionStatus::Unknown)
        , result(0)
    {}
};

enum class DeviceType {
    Unknown = 0,
    Drone,
    UGV,
    RobotArm,
    GPR,
    InfraredCamera,
    Lidar,
    Magnetometer
};

enum class DeviceStatus {
    Unknown = 0,
    Offline,
    Online,
    Idle,
    Busy,
    Error,
    Maintenance
};

struct DeviceInfo {
    QString id;
    DeviceType type;
    QString name;
    QString manufacturer;
    QString model;
    QString serialNumber;
    DeviceStatus status;
    double batteryLevel;
    QVector3D position;
    QDateTime lastOnlineTime;
    QDateTime createTime;
    QString remark;

    DeviceInfo()
        : type(DeviceType::Unknown)
        , status(DeviceStatus::Unknown)
        , batteryLevel(0.0)
    {}
};

enum class AlarmLevel {
    Unknown = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

struct AlarmInfo {
    QString id;
    QString type;
    AlarmLevel level;
    QString targetId;
    QString deviceId;
    QString message;
    bool isHandled;
    QDateTime handleTime;
    QString handleResult;
    QDateTime createTime;

    AlarmInfo()
        : level(AlarmLevel::Unknown)
        , isHandled(false)
    {}
};

}

#endif
