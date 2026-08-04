// 模拟场景提供者测试
// 验证演示场景包含模拟标识、1个目标、1个任务、至少2个模拟设备，任务引用正确关联到目标，
// 坐标统一为本地米坐标系（0-5000），且探测阶段脚本包含 5 个预设时间点的目标数据。

#include "Core/Simulation/DemoScenarioProvider.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const Core::Simulation::DemoScenario scenario = Core::Simulation::DemoScenarioProvider::create();

    // 场景标识必须包含"模拟"字样，明确标注为模拟数据
    if (!scenario.label.contains(QStringLiteral("模拟"))) {
        qCritical() << "Then the demo scenario label clearly marks simulated data";
        return 1;
    }

    // 必须恰好提供1个目标（空起步推迟到阶段4，本阶段保留预加载）
    if (scenario.targets.size() != 1) {
        qCritical() << "Then the demo scenario provides exactly one target";
        return 1;
    }

    // 模拟工作流必须从"已发现"状态开始
    if (scenario.targets.first().status != Core::TargetStatus::Detected) {
        qCritical() << "Then the demo target starts at Detected";
        return 1;
    }

    // 必须恰好提供1个任务
    if (scenario.missions.size() != 1) {
        qCritical() << "Then the demo scenario provides exactly one mission";
        return 1;
    }

    // 必须至少提供2个模拟设备
    if (scenario.devices.size() < 2) {
        qCritical() << "Then the demo scenario provides at least two simulated devices";
        return 1;
    }

    // 任务必须引用到正确的目标
    if (scenario.missions.first().targetId != scenario.targets.first().id) {
        qCritical() << "Then the demo mission references the demo target";
        return 1;
    }

    // 坐标修复：所有目标和设备坐标必须在本地米坐标系（0-5000）范围内，
    // 不再混用经纬度（经纬度典型值 108.9/34.2 会远小于 0 或超出 5000）
    auto assertMeterCoords = [](const QVector3D &pos) {
        return pos.x() >= 0.0f && pos.x() <= 5000.0f
            && pos.y() >= 0.0f && pos.y() <= 5000.0f;
    };
    for (const Core::TargetInfo &t : scenario.targets) {
        if (!assertMeterCoords(t.position)) {
            qCritical() << "Target" << t.id << "坐标不在本地米坐标系（0-5000）范围："
                        << t.position;
            return 1;
        }
    }
    for (const Core::DeviceInfo &d : scenario.devices) {
        if (!assertMeterCoords(d.position)) {
            qCritical() << "Device" << d.id << "坐标不在本地米坐标系（0-5000）范围："
                        << d.position;
            return 1;
        }
    }

    // 探测阶段脚本：必须恰好 5 条
    if (scenario.detectionScript.size() != 5) {
        qCritical() << "Then the detection script provides exactly five entries, got"
                    << scenario.detectionScript.size();
        return 1;
    }

    // 5 个时间点必须依次为 10s/25s/42s/60s/78s
    const QVector<qint64> expectedTimes = {10000, 25000, 42000, 60000, 78000};
    for (int i = 0; i < 5; ++i) {
        if (scenario.detectionScript[i].timeMs != expectedTimes[i]) {
            qCritical() << "Detection script entry" << i << "时间点应为"
                        << expectedTimes[i] << "实际为"
                        << scenario.detectionScript[i].timeMs;
            return 1;
        }
    }

    // 5 个目标 ID 必须依次为 TGT-001..TGT-005，且均为 Detected 状态
    for (int i = 0; i < 5; ++i) {
        const QString expectedId = QStringLiteral("TGT-%1").arg(i + 1, 3, 10, QChar('0'));
        if (scenario.detectionScript[i].target.id != expectedId) {
            qCritical() << "Detection script entry" << i << "目标 ID 应为"
                        << expectedId << "实际为"
                        << scenario.detectionScript[i].target.id;
            return 1;
        }
        if (scenario.detectionScript[i].target.status != Core::TargetStatus::Detected) {
            qCritical() << "Detection script entry" << i << "目标状态必须为 Detected";
            return 1;
        }
        // 脚本目标坐标也必须是米坐标系
        if (!assertMeterCoords(scenario.detectionScript[i].target.position)) {
            qCritical() << "Detection script entry" << i << "目标坐标不在米坐标系范围";
            return 1;
        }
        // 红框归一化坐标必须在 0.0-1.0 范围
        const QPointF &bp = scenario.detectionScript[i].videoBoxPos;
        const QSizeF &bs = scenario.detectionScript[i].videoBoxSize;
        if (bp.x() < 0.0 || bp.x() > 1.0 || bp.y() < 0.0 || bp.y() > 1.0
            || bs.width() <= 0.0 || bs.width() > 1.0
            || bs.height() <= 0.0 || bs.height() > 1.0) {
            qCritical() << "Detection script entry" << i << "红框归一化坐标越界";
            return 1;
        }
    }

    return 0;
}
