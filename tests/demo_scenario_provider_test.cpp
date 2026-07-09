// 模拟场景提供者测试
// 验证演示场景包含模拟标识、1个目标、1个任务、至少2个模拟设备，且任务引用正确关联到目标。

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

    // 必须恰好提供1个目标
    if (scenario.targets.size() != 1) {
        qCritical() << "Then the demo scenario provides exactly one target";
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

    return 0;
}
