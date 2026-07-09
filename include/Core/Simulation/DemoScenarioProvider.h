#ifndef CORE_SIMULATION_DEMOSCENARIOPROVIDER_H
#define CORE_SIMULATION_DEMOSCENARIOPROVIDER_H

// 模拟演示场景提供者
// 仅用于本地演示，不连接真实设备、不发送真实控制命令。
// 后续接入真实数据源时，应通过接口替换，而非修改本文件。

#include "Core/Data/Types.h"

#include <QString>
#include <QVector>

namespace Core::Simulation {

// 模拟演示场景数据，包含目标、任务和设备的本地假数据
struct DemoScenario {
    QString label;                   // 场景标识，须明确标注"模拟"
    QVector<TargetInfo> targets;    // 模拟目标列表
    QVector<MissionInfo> missions;  // 模拟任务列表
    QVector<DeviceInfo> devices;     // 模拟设备列表
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
