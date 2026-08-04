// 模拟演示场景提供者实现
// 构造一份本地假数据，用于 MVP 演示流程，不连接真实设备或真实任务。

#include "Core/Simulation/DemoScenarioProvider.h"

namespace Core::Simulation {

// 创建模拟演示场景：1个目标、1个任务、2个模拟设备 + 5 条探测阶段脚本数据
// 坐标统一为本地米坐标系（0-5000 范围），不再混用经纬度。
DemoScenario DemoScenarioProvider::create()
{
    DemoScenario scenario;
    scenario.label = QStringLiteral("模拟排弹抢修演示场景");

    // 模拟目标：反跑道雷，仅用于演示（坐标已改为本地米坐标系）
    TargetInfo target;
    target.id = QStringLiteral("target-demo-001");
    target.type = TargetType::AntiRunwayMine;
    target.typeName = QStringLiteral("模拟反跑道雷");
    target.position = QVector3D(2500.0f, 2500.0f, 0.0f);
    target.depth = 0.45;
    target.confidence = 0.86;
    target.threatLevel = ThreatLevel::High;
    target.status = TargetStatus::Detected;
    target.detectTime = QDateTime::currentDateTimeUtc();
    target.updateTime = target.detectTime;
    target.remark = QStringLiteral("模拟目标，仅用于本地演示");
    scenario.targets.append(target);

    // 模拟设备1：侦察无人机，不连接真实无人机（米坐标系）
    DeviceInfo drone;
    drone.id = QStringLiteral("device-demo-drone-001");
    drone.type = DeviceType::Drone;
    drone.name = QStringLiteral("模拟侦察无人机");
    drone.manufacturer = QStringLiteral("模拟厂商");
    drone.model = QStringLiteral("SIM-UAV-1");
    drone.status = DeviceStatus::Online;
    drone.batteryLevel = 82.0;
    drone.position = QVector3D(2480.0f, 2520.0f, 50.0f);
    drone.lastOnlineTime = target.detectTime;
    drone.remark = QStringLiteral("模拟设备，不连接真实无人机");
    scenario.devices.append(drone);

    // 模拟设备2：排爆机器人，不连接真实机器人（米坐标系）
    DeviceInfo robot;
    robot.id = QStringLiteral("device-demo-robot-001");
    robot.type = DeviceType::UGV;
    robot.name = QStringLiteral("模拟排爆机器人");
    robot.manufacturer = QStringLiteral("模拟厂商");
    robot.model = QStringLiteral("SIM-UGV-1");
    robot.status = DeviceStatus::Idle;
    robot.batteryLevel = 74.0;
    robot.position = QVector3D(2520.0f, 2480.0f, 0.0f);
    robot.lastOnlineTime = target.detectTime;
    robot.remark = QStringLiteral("模拟设备，不连接真实机器人");
    scenario.devices.append(robot);

    // 模拟任务：处置任务，关联到上面的模拟目标和模拟机器人
    MissionInfo mission;
    mission.id = QStringLiteral("mission-demo-001");
    mission.type = MissionType::Disposal;
    mission.targetId = target.id;
    mission.priority = 1;
    mission.status = MissionStatus::Approved;
    mission.assigner = QStringLiteral("模拟指挥员");
    mission.assignee = QStringLiteral("模拟排爆组");
    mission.deviceId = robot.id;
    mission.planStartTime = target.detectTime;
    mission.planEndTime = target.detectTime.addSecs(1800);
    mission.createTime = target.detectTime;
    mission.updateTime = target.detectTime;
    mission.remark = QStringLiteral("模拟处置任务，仅用于演示流程");
    scenario.missions.append(mission);

    // 探测阶段脚本：5 个目标在预设视频时间点被"探测"到，驱动四区同步
    // 坐标为本地米坐标系（0-5000），红框位置/尺寸为视频画面归一化坐标（0.0-1.0）
    scenario.detectionScript = buildDetectionScript(target.detectTime);

    return scenario;
}

// 构造探测阶段脚本：5 目标在 10s/25s/42s/60s/78s 被探测到
// 类型覆盖 TargetType 主要枚举，坐标分布在 0-5000 米坐标系各区域
QVector<DetectionScriptEntry> DemoScenarioProvider::buildDetectionScript(const QDateTime &baseTime)
{
    QVector<DetectionScriptEntry> script;

    // TGT-001 @ 10s：反跑道雷，机场北跑道区域
    DetectionScriptEntry e1;
    e1.timeMs = 10000;
    e1.target.id = QStringLiteral("TGT-001");
    e1.target.type = TargetType::AntiRunwayMine;
    e1.target.typeName = QStringLiteral("模拟反跑道雷");
    e1.target.position = QVector3D(1200.0f, 1800.0f, 0.0f);
    e1.target.depth = 0.30;
    e1.target.confidence = 0.88;
    e1.target.threatLevel = ThreatLevel::High;
    e1.target.status = TargetStatus::Detected;
    e1.target.detectTime = baseTime.addMSecs(e1.timeMs);
    e1.target.updateTime = e1.target.detectTime;
    e1.target.remark = QStringLiteral("模拟探测目标，演示用");
    e1.videoBoxPos = QPointF(0.20, 0.30);
    e1.videoBoxSize = QSizeF(0.15, 0.20);
    script.append(e1);

    // TGT-002 @ 25s：航弹，机场东侧停机坪
    DetectionScriptEntry e2;
    e2.timeMs = 25000;
    e2.target.id = QStringLiteral("TGT-002");
    e2.target.type = TargetType::AirBomb;
    e2.target.typeName = QStringLiteral("模拟航弹");
    e2.target.position = QVector3D(2800.0f, 900.0f, 0.0f);
    e2.target.depth = 0.55;
    e2.target.confidence = 0.82;
    e2.target.threatLevel = ThreatLevel::Critical;
    e2.target.status = TargetStatus::Detected;
    e2.target.detectTime = baseTime.addMSecs(e2.timeMs);
    e2.target.updateTime = e2.target.detectTime;
    e2.target.remark = QStringLiteral("模拟探测目标，演示用");
    e2.videoBoxPos = QPointF(0.55, 0.20);
    e2.videoBoxSize = QSizeF(0.18, 0.22);
    script.append(e2);

    // TGT-003 @ 42s：子母弹，机场中部滑行道
    DetectionScriptEntry e3;
    e3.timeMs = 42000;
    e3.target.id = QStringLiteral("TGT-003");
    e3.target.type = TargetType::ClusterBomb;
    e3.target.typeName = QStringLiteral("模拟子母弹");
    e3.target.position = QVector3D(3700.0f, 2600.0f, 0.0f);
    e3.target.depth = 0.20;
    e3.target.confidence = 0.79;
    e3.target.threatLevel = ThreatLevel::High;
    e3.target.status = TargetStatus::Detected;
    e3.target.detectTime = baseTime.addMSecs(e3.timeMs);
    e3.target.updateTime = e3.target.detectTime;
    e3.target.remark = QStringLiteral("模拟探测目标，演示用");
    e3.videoBoxPos = QPointF(0.40, 0.55);
    e3.videoBoxSize = QSizeF(0.20, 0.18);
    script.append(e3);

    // TGT-004 @ 60s：巡航导弹残骸，机场南端
    DetectionScriptEntry e4;
    e4.timeMs = 60000;
    e4.target.id = QStringLiteral("TGT-004");
    e4.target.type = TargetType::CruiseMissile;
    e4.target.typeName = QStringLiteral("模拟巡航导弹残骸");
    e4.target.position = QVector3D(800.0f, 3400.0f, 0.0f);
    e4.target.depth = 0.10;
    e4.target.confidence = 0.91;
    e4.target.threatLevel = ThreatLevel::Critical;
    e4.target.status = TargetStatus::Detected;
    e4.target.detectTime = baseTime.addMSecs(e4.timeMs);
    e4.target.updateTime = e4.target.detectTime;
    e4.target.remark = QStringLiteral("模拟探测目标，演示用");
    e4.videoBoxPos = QPointF(0.15, 0.65);
    e4.videoBoxSize = QSizeF(0.22, 0.15);
    script.append(e4);

    // TGT-005 @ 78s：简易爆炸装置，机场西侧边缘
    DetectionScriptEntry e5;
    e5.timeMs = 78000;
    e5.target.id = QStringLiteral("TGT-005");
    e5.target.type = TargetType::IED;
    e5.target.typeName = QStringLiteral("模拟简易爆炸装置");
    e5.target.position = QVector3D(3100.0f, 4100.0f, 0.0f);
    e5.target.depth = 0.05;
    e5.target.confidence = 0.74;
    e5.target.threatLevel = ThreatLevel::Medium;
    e5.target.status = TargetStatus::Detected;
    e5.target.detectTime = baseTime.addMSecs(e5.timeMs);
    e5.target.updateTime = e5.target.detectTime;
    e5.target.remark = QStringLiteral("模拟探测目标，演示用");
    e5.videoBoxPos = QPointF(0.60, 0.70);
    e5.videoBoxSize = QSizeF(0.12, 0.16);
    script.append(e5);

    return script;
}

}
