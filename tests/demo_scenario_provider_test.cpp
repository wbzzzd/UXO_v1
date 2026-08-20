// 模拟场景提供者测试
// 验证演示场景包含模拟标识、空起步目标列表（探测目标由 DetectionSimulator 动态生成）、
// 1个任务（探测阶段目标未定）、至少2个模拟设备，设备坐标为经纬度（WGS84），
// 且检测数据包含 4 个预设视频位置点的目标类型/红框/置信度。

#include "Core/Simulation/DemoScenarioProvider.h"

#include <QCoreApplication>
#include <QDebug>
#include <QtMath>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const Core::Simulation::DemoScenario scenario = Core::Simulation::DemoScenarioProvider::create();

    // 场景标识必须包含"模拟"字样，明确标注为模拟数据
    if (!scenario.label.contains(QStringLiteral("模拟"))) {
        qCritical() << "scenario label must mark simulated data";
        return 1;
    }

    // 空起步：targets 必须为空（探测目标由 DetectionSimulator 动态生成，不再预加载）
    if (!scenario.targets.isEmpty()) {
        qCritical() << "scenario targets must be empty (空起步), got" << scenario.targets.size();
        return 1;
    }

    // 必须恰好提供1个任务
    if (scenario.missions.size() != 1) {
        qCritical() << "scenario must provide exactly one mission, got" << scenario.missions.size();
        return 1;
    }

    // 探测阶段目标未定：任务 targetId 必须为空
    if (!scenario.missions.first().targetId.isEmpty()) {
        qCritical() << "mission targetId must be empty in detection phase, got"
                    << scenario.missions.first().targetId;
        return 1;
    }

    // 必须至少提供2个模拟设备
    if (scenario.devices.size() < 2) {
        qCritical() << "scenario must provide at least two simulated devices, got"
                    << scenario.devices.size();
        return 1;
    }

    // 机场边界必须有效（北 > 南，东 > 西）
    if (scenario.airportBounds.north <= scenario.airportBounds.south
        || scenario.airportBounds.east <= scenario.airportBounds.west) {
        qCritical() << "airport bounds invalid: N/S/E/W ="
                    << scenario.airportBounds.north << scenario.airportBounds.south
                    << scenario.airportBounds.east << scenario.airportBounds.west;
        return 1;
    }

    // 设备坐标为经纬度（WGS84），必须在机场边界范围内
    auto assertInBounds = [&](double lng, double lat) {
        return lng >= scenario.airportBounds.west && lng <= scenario.airportBounds.east
            && lat >= scenario.airportBounds.south && lat <= scenario.airportBounds.north;
    };
    for (const Core::DeviceInfo &d : scenario.devices) {
        // position: x=经度, y=纬度, z=高度
        if (!assertInBounds(d.position.x(), d.position.y())) {
            qCritical() << "Device" << d.id << "坐标不在机场边界范围内："
                        << d.position;
            return 1;
        }
    }

    // 无人机航线不得为空
    if (scenario.droneRoute.isEmpty()) {
        qCritical() << "drone route must not be empty";
        return 1;
    }

    // 航线为跑道方向 out-and-back：至少 3 航点（P1 -> P2 -> P1）
    if (scenario.droneRoute.size() < 3) {
        qCritical() << "out-and-back 航线至少需要 3 航点，实际" << scenario.droneRoute.size();
        return 1;
    }

    // 航点必须在机场边界内（严格内缩，不触碰边界）
    for (int i = 0; i < scenario.droneRoute.size(); ++i) {
        const auto &wp = scenario.droneRoute[i];
        if (wp.lat <= scenario.airportBounds.south || wp.lat >= scenario.airportBounds.north
            || wp.lng <= scenario.airportBounds.west || wp.lng >= scenario.airportBounds.east) {
            qCritical() << "航点" << i << "不在机场边界内：lat=" << wp.lat << "lng=" << wp.lng;
            return 1;
        }
    }

    // out-and-back 终点必须返回 P1（与起点经纬度一致）
    if (qAbs(scenario.droneRoute[2].lat - scenario.droneRoute[0].lat) > 1e-9
        || qAbs(scenario.droneRoute[2].lng - scenario.droneRoute[0].lng) > 1e-9) {
        qCritical() << "out-and-back 终点未返回 P1：起点 lat=" << scenario.droneRoute[0].lat
                    << "lng=" << scenario.droneRoute[0].lng
                    << "终点 lat=" << scenario.droneRoute[2].lat
                    << "lng=" << scenario.droneRoute[2].lng;
        return 1;
    }

    // 航点存储航向应与跑道轴向真方位角一致：
    // 真方位角公式 θ = atan2(sin(Δλ)cos(φ2), cos(φ1)sin(φ2) - sin(φ1)cos(φ2)cos(Δλ))
    // Δλ = lng2 - lng1，结果归一化到 [0,360)
    auto initialBearing = [](double lat1, double lng1, double lat2, double lng2) {
        const double phi1 = qDegreesToRadians(lat1);
        const double phi2 = qDegreesToRadians(lat2);
        const double dLambda = qDegreesToRadians(lng2 - lng1);
        const double x = qSin(dLambda) * qCos(phi2);
        const double y = qCos(phi1) * qSin(phi2)
                       - qSin(phi1) * qCos(phi2) * qCos(dLambda);
        double deg = qRadiansToDegrees(qAtan2(x, y));
        while (deg < 0.0) deg += 360.0;
        while (deg >= 360.0) deg -= 360.0;
        return deg;
    };

    // P1->P2 真方位角应与离场航向（航点0）一致（±1° 容差，考虑 0.1° 舍入）
    const double p1ToP2 = initialBearing(
        scenario.droneRoute[0].lat, scenario.droneRoute[0].lng,
        scenario.droneRoute[1].lat, scenario.droneRoute[1].lng);
    if (qAbs(p1ToP2 - scenario.droneRoute[0].heading) > 1.0) {
        qCritical() << "P1->P2 真方位角" << p1ToP2
                    << "与航点0存储航向" << scenario.droneRoute[0].heading << "偏差超 1°";
        return 1;
    }

    // P2->P1 真方位角应与返航航向（航点1）一致（±1° 容差）
    const double p2ToP1 = initialBearing(
        scenario.droneRoute[1].lat, scenario.droneRoute[1].lng,
        scenario.droneRoute[0].lat, scenario.droneRoute[0].lng);
    if (qAbs(p2ToP1 - scenario.droneRoute[1].heading) > 1.0) {
        qCritical() << "P2->P1 真方位角" << p2ToP1
                    << "与航点1存储航向" << scenario.droneRoute[1].heading << "偏差超 1°";
        return 1;
    }

    // out-and-back 终点（航点2）保持返航航向，应与航点1一致
    if (qAbs(scenario.droneRoute[2].heading - scenario.droneRoute[1].heading) > 0.01) {
        qCritical() << "out-and-back 终点航向" << scenario.droneRoute[2].heading
                    << "应与返航航向" << scenario.droneRoute[1].heading << "一致";
        return 1;
    }

    // 检测数据条目：必须恰好 4 条
    if (scenario.detections.size() != 4) {
        qCritical() << "detections must have exactly four entries, got"
                    << scenario.detections.size();
        return 1;
    }

    // 4 个视频位置点必须依次为 10s/40s/49s/66s
    const QVector<qint64> expectedTimes = {10000, 40000, 49000, 66000};
    for (int i = 0; i < 4; ++i) {
        if (scenario.detections[i].videoPositionMs != expectedTimes[i]) {
            qCritical() << "Detection entry" << i << "视频位置应为"
                        << expectedTimes[i] << "实际为"
                        << scenario.detections[i].videoPositionMs;
            return 1;
        }
    }

    // 每条检测数据：类型有效、typeName 含"模拟"、红框归一化坐标合法、置信度合法
    for (int i = 0; i < 4; ++i) {
        const auto &entry = scenario.detections[i];

        if (entry.type == Core::TargetType::Unknown) {
            qCritical() << "Detection entry" << i << "目标类型不得为 Unknown";
            return 1;
        }

        if (!entry.typeName.contains(QStringLiteral("模拟"))) {
            qCritical() << "Detection entry" << i << "typeName 必须标注模拟，实际为"
                        << entry.typeName;
            return 1;
        }

        const QPointF &bp = entry.videoBoxPos;
        const QSizeF &bs = entry.videoBoxSize;
        if (bp.x() < 0.0 || bp.x() > 1.0 || bp.y() < 0.0 || bp.y() > 1.0
            || bs.width() <= 0.0 || bs.width() > 1.0
            || bs.height() <= 0.0 || bs.height() > 1.0) {
            qCritical() << "Detection entry" << i << "红框归一化坐标越界：pos="
                        << bp << "size=" << bs;
            return 1;
        }

        if (entry.confidence < 0.0 || entry.confidence > 1.0) {
            qCritical() << "Detection entry" << i << "置信度越界：" << entry.confidence;
            return 1;
        }
    }

    return 0;
}
