#include "Common/MockDataGenerator.h"
#include <QRandomGenerator>
#include <QDateTime>

namespace Common {

static const QStringList TYPE_NAMES = {
    "航弹", "子母弹", "反跑道雷", "巡航导弹", "IED", "其他"
};

static const QStringList DEVICE_NAMES = {
    "侦察无人机01", "侦察无人机02", "排爆机器人01", "排爆机器人02", "探地雷达01"
};

static const QStringList DEVICE_MODELS = {
    "X-100", "X-200", "R-200", "R-300", "GPR-X1"
};

QVector<Core::TargetInfo> MockDataGenerator::generateTargets(int count)
{
    QVector<Core::TargetInfo> targets;
    auto *rng = QRandomGenerator::global();

    for (int i = 0; i < count; ++i) {
        Core::TargetInfo target;
        target.id = QString("TGT-%1").arg(i + 1, 4, 10, QChar('0'));
        target.type = randomTargetType();
        target.typeName = TYPE_NAMES[static_cast<int>(target.type)];
        target.threatLevel = randomThreatLevel();
        target.status = Core::TargetStatus::Detected;
        target.confidence = 0.7 + rng->bounded(0.3);
        target.depth = rng->bounded(2.0);

        double baseX = 1000.0 + rng->bounded(2000.0);
        double baseZ = 1000.0 + rng->bounded(2000.0);
        target.position = QVector3D(baseX, -target.depth, baseZ);

        target.detectTime = QDateTime::currentDateTime().addSecs(-rng->bounded(3600));
        target.updateTime = QDateTime::currentDateTime();

        targets.append(target);
    }

    return targets;
}

QVector<Core::MissionInfo> MockDataGenerator::generateMissions(const QVector<Core::TargetInfo>& targets, int count)
{
    QVector<Core::MissionInfo> missions;
    auto *rng = QRandomGenerator::global();

    for (int i = 0; i < qMin(count, targets.size()); ++i) {
        Core::MissionInfo mission;
        mission.id = QString("MSN-%1").arg(i + 1, 4, 10, QChar('0'));
        mission.type = Core::MissionType::Disposal;
        mission.targetId = targets[i].id;
        mission.priority = rng->bounded(3);
        mission.status = randomMissionStatus();
        mission.deviceId = QString("DEV-00%1").arg(rng->bounded(5) + 1);
        mission.createTime = QDateTime::currentDateTime().addSecs(-rng->bounded(7200));
        mission.updateTime = QDateTime::currentDateTime();

        missions.append(mission);
    }

    return missions;
}

QVector<Core::DeviceInfo> MockDataGenerator::generateDevices(int count)
{
    QVector<Core::DeviceInfo> devices;
    auto *rng = QRandomGenerator::global();

    for (int i = 0; i < count; ++i) {
        Core::DeviceInfo device;
        device.id = QString("DEV-00%1").arg(i + 1);
        device.type = randomDeviceType();
        device.name = DEVICE_NAMES[i % DEVICE_NAMES.size()];
        device.manufacturer = "某厂商";
        device.model = DEVICE_MODELS[i % DEVICE_MODELS.size()];
        device.status = randomDeviceStatus();
        device.batteryLevel = rng->bounded(100);
        device.position = QVector3D(1500.0 + rng->bounded(500), 0, 1500.0 + rng->bounded(500));
        device.lastOnlineTime = QDateTime::currentDateTime().addSecs(-rng->bounded(300));
        device.createTime = QDateTime::currentDateTime().addDays(-rng->bounded(365));

        devices.append(device);
    }

    return devices;
}

Core::TargetType MockDataGenerator::randomTargetType()
{
    auto *rng = QRandomGenerator::global();
    return static_cast<Core::TargetType>(rng->bounded(6));
}

Core::ThreatLevel MockDataGenerator::randomThreatLevel()
{
    auto *rng = QRandomGenerator::global();
    return static_cast<Core::ThreatLevel>(rng->bounded(3) + 1);
}

Core::TargetStatus MockDataGenerator::randomTargetStatus()
{
    auto *rng = QRandomGenerator::global();
    return static_cast<Core::TargetStatus>(rng->bounded(4) + 1);
}

Core::MissionStatus MockDataGenerator::randomMissionStatus()
{
    auto *rng = QRandomGenerator::global();
    return static_cast<Core::MissionStatus>(rng->bounded(8));
}

Core::DeviceStatus MockDataGenerator::randomDeviceStatus()
{
    auto *rng = QRandomGenerator::global();
    return static_cast<Core::DeviceStatus>(rng->bounded(6));
}

Core::DeviceType MockDataGenerator::randomDeviceType()
{
    auto *rng = QRandomGenerator::global();
    return static_cast<Core::DeviceType>(rng->bounded(4) + 1);
}

}
