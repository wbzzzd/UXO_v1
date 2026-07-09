// 模拟演示场景提供者实现
// 构造一份本地假数据，用于 MVP 演示流程，不连接真实设备或真实任务。

#include "Core/Simulation/DemoScenarioProvider.h"

namespace Core::Simulation {

// 创建模拟演示场景：1个目标、1个任务、2个模拟设备
DemoScenario DemoScenarioProvider::create()
{
    DemoScenario scenario;
    scenario.label = QStringLiteral("模拟排弹抢修演示场景");

    // 模拟目标：反跑道雷，仅用于演示
    TargetInfo target;
    target.id = QStringLiteral("target-demo-001");
    target.type = TargetType::AntiRunwayMine;
    target.typeName = QStringLiteral("模拟反跑道雷");
    target.position = QVector3D(108.9321f, 34.2345f, 0.0f);
    target.depth = 0.45;
    target.confidence = 0.86;
    target.threatLevel = ThreatLevel::High;
    target.status = TargetStatus::Confirmed;
    target.detectTime = QDateTime::currentDateTimeUtc();
    target.updateTime = target.detectTime;
    target.remark = QStringLiteral("模拟目标，仅用于本地演示");
    scenario.targets.append(target);

    // 模拟设备1：侦察无人机，不连接真实无人机
    DeviceInfo drone;
    drone.id = QStringLiteral("device-demo-drone-001");
    drone.type = DeviceType::Drone;
    drone.name = QStringLiteral("模拟侦察无人机");
    drone.manufacturer = QStringLiteral("模拟厂商");
    drone.model = QStringLiteral("SIM-UAV-1");
    drone.status = DeviceStatus::Online;
    drone.batteryLevel = 82.0;
    drone.position = QVector3D(108.9318f, 34.2342f, 50.0f);
    drone.lastOnlineTime = target.detectTime;
    drone.remark = QStringLiteral("模拟设备，不连接真实无人机");
    scenario.devices.append(drone);

    // 模拟设备2：排爆机器人，不连接真实机器人
    DeviceInfo robot;
    robot.id = QStringLiteral("device-demo-robot-001");
    robot.type = DeviceType::UGV;
    robot.name = QStringLiteral("模拟排爆机器人");
    robot.manufacturer = QStringLiteral("模拟厂商");
    robot.model = QStringLiteral("SIM-UGV-1");
    robot.status = DeviceStatus::Idle;
    robot.batteryLevel = 74.0;
    robot.position = QVector3D(108.9320f, 34.2339f, 0.0f);
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

    return scenario;
}

}
