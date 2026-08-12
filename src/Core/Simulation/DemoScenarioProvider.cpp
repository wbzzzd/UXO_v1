// 模拟演示场景提供者实现
// 构造一份本地假数据，用于无人机探测态势演示，不连接真实设备或真实任务。
// 坐标系：经纬度（WGS84），机场为沈阳于洪全胜机场 (ZYSY)

#include "Core/Simulation/DemoScenarioProvider.h"

namespace Core::Simulation {

// 创建模拟演示场景：2个模拟设备 + 1个模拟任务 + 无人机航线 + 检测数据 + 机场边界
// 所有坐标为经纬度（WGS84），目标坐标由检测模拟器触发时动态推算
DemoScenario DemoScenarioProvider::create()
{
    DemoScenario scenario;
    scenario.label = QStringLiteral("模拟无人机探测态势演示场景");

    // 机场区域边界：沈阳于洪全胜机场 (ZYSY)
    // 约 4km × 3.5km 范围，覆盖跑道和主要设施区域
    scenario.airportBounds.north = 41.840;
    scenario.airportBounds.south = 41.805;
    scenario.airportBounds.west = 123.278;
    scenario.airportBounds.east = 123.320;

    // === 无人机航线（跑道方向 out-and-back 巡航）===
    // 像素->WGS84 标定：卫星底图 2000×1800 北朝上，机场边界
    //   N41.840/S41.805/W123.278/E123.320 线性映射整幅图像（左上=西北角）：
    //   lng = 123.278 + (px/2000)×0.042；lat = 41.840 - (py/1800)×0.035。
    //   跑道像素端点 (1240,245)->(41.83524,123.30404)、(755,1400)->(41.81278,123.29386)；
    //   两端点距最近边界均 >500m，整条直线均在 AirportBounds 内，无需额外内缩余量。
    // 航向约定：0=北、顺时针；各航点航向取离开该航点航段的真方位角。
    //   跑道轴向 P1->P2 方位 ≈198.7°（偏南南西），返航 P2->P1 ≈18.7°（偏北北东）。
    // 高度固定 300m；out-and-back 共 3 航点/2 航段，由 DroneTelemetrySimulator 在 96s 内均分。
    scenario.droneRoute = {
        {41.83524, 123.30404, 300.0, 198.7},  // P1 跑道一端，离场飞向 P2（方位≈198.7°）
        {41.81278, 123.29386, 300.0,  18.7},  // P2 跑道另一端，返航飞回 P1（方位≈18.7°）
        {41.83524, 123.30404, 300.0,  18.7},  // 返回 P1（out-and-back 终点），保持返航航向
    };

    // === 检测数据条目（内部用视频位置驱动检测时机）===
    // videoPositionMs 是视频播放位置，DetectionSimulator 到达该位置时触发检测
    // 公开接口只输出 DetectionResult{类型, 红框, 置信度}，不暴露时间点
    // 红框位置/尺寸为视频画面归一化坐标（0.0-1.0），基于视频帧分析标注
    scenario.detections = {
        { 5000, TargetType::AntiRunwayMine, QStringLiteral("模拟反跑道雷"),
            QPointF(0.20, 0.30), QSizeF(0.15, 0.20), 0.88},
        {20000, TargetType::AirBomb, QStringLiteral("模拟航弹"),
            QPointF(0.55, 0.20), QSizeF(0.18, 0.22), 0.82},
        {40000, TargetType::ClusterBomb, QStringLiteral("模拟子母弹"),
            QPointF(0.40, 0.55), QSizeF(0.20, 0.18), 0.79},
        {65000, TargetType::IED, QStringLiteral("模拟简易爆炸装置"),
            QPointF(0.15, 0.65), QSizeF(0.22, 0.15), 0.91},
    };

    // === 模拟设备（用于设备资源条显示）===
    const QDateTime now = QDateTime::currentDateTimeUtc();

    // 模拟设备1：侦察无人机 UAV-1
    DeviceInfo drone;
    drone.id = QStringLiteral("UAV-1");
    drone.type = DeviceType::Drone;
    drone.name = QStringLiteral("侦察无人机");
    drone.manufacturer = QStringLiteral("模拟厂商");
    drone.model = QStringLiteral("SIM-UAV-1");
    drone.status = DeviceStatus::Online;
    drone.batteryLevel = 82.0;
    drone.position = QVector3D(123.300f, 41.822f, 300.0f);  // x=lng, y=lat, z=alt
    drone.lastOnlineTime = now;
    drone.remark = QStringLiteral("模拟设备，不连接真实无人机");
    scenario.devices.append(drone);

    // 模拟设备2：排爆机器人
    DeviceInfo robot;
    robot.id = QStringLiteral("UGV-1");
    robot.type = DeviceType::UGV;
    robot.name = QStringLiteral("排爆机器人");
    robot.manufacturer = QStringLiteral("模拟厂商");
    robot.model = QStringLiteral("SIM-UGV-1");
    robot.status = DeviceStatus::Idle;
    robot.batteryLevel = 74.0;
    robot.position = QVector3D(123.300f, 41.822f, 0.0f);
    robot.lastOnlineTime = now;
    robot.remark = QStringLiteral("模拟设备，不连接真实机器人");
    scenario.devices.append(robot);

    // === 模拟任务（占位，探测阶段不执行）===
    MissionInfo mission;
    mission.id = QStringLiteral("MSN-001");
    mission.type = MissionType::Reconnaissance;
    mission.targetId = QString();  // 探测阶段目标未定
    mission.priority = 1;
    mission.status = MissionStatus::Planned;
    mission.assigner = QStringLiteral("模拟指挥员");
    mission.assignee = QStringLiteral("模拟排爆组");
    mission.deviceId = drone.id;
    mission.planStartTime = now;
    mission.planEndTime = now.addSecs(1800);
    mission.createTime = now;
    mission.updateTime = now;
    mission.remark = QStringLiteral("模拟侦察任务，仅用于演示流程");
    scenario.missions.append(mission);

    return scenario;
}

}
