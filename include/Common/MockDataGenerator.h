#ifndef COMMON_MOCKDATAGENERATOR_H
#define COMMON_MOCKDATAGENERATOR_H

#include <QVector>
#include <QString>
#include "Core/Data/Types.h"

namespace Common {

class MockDataGenerator
{
public:
    static QVector<Core::TargetInfo> generateTargets(int count = 10);
    static QVector<Core::MissionInfo> generateMissions(const QVector<Core::TargetInfo>& targets, int count = 5);
    static QVector<Core::DeviceInfo> generateDevices(int count = 5);

private:
    static Core::TargetType randomTargetType();
    static Core::ThreatLevel randomThreatLevel();
    static Core::TargetStatus randomTargetStatus();
    static Core::MissionStatus randomMissionStatus();
    static Core::DeviceStatus randomDeviceStatus();
    static Core::DeviceType randomDeviceType();
};

}

#endif
