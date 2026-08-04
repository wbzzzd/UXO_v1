#ifndef CORE_SIMULATION_DEMOSCENARIOPROVIDER_H
#define CORE_SIMULATION_DEMOSCENARIOPROVIDER_H

// 模拟演示场景提供者
// 仅用于本地演示，不连接真实设备、不发送真实控制命令。
// 后续接入真实数据源时，应通过接口替换，而非修改本文件。

#include "Core/Data/Types.h"

#include <QString>
#include <QVector>
#include <QPointF>
#include <QSizeF>
#include <QDateTime>

namespace Core::Simulation {

// 探测阶段脚本条目：视频时间点 + 目标信息 + 红框在视频画面中的归一化位置/尺寸
// 归一化坐标范围 0.0-1.0，相对于视频画面左上角。
struct DetectionScriptEntry {
    qint64 timeMs;          // 视频时间点（毫秒），目标在该时刻被"探测"到
    TargetInfo target;
    QPointF videoBoxPos;    // 红框在视频画面中的归一化位置 (0.0-1.0)
    QSizeF videoBoxSize;    // 红框归一化尺寸 (0.0-1.0)

    DetectionScriptEntry()
        : timeMs(0)
    {}
};

// 模拟演示场景数据，包含目标、任务、设备以及探测阶段脚本的本地假数据
struct DemoScenario {
    QString label;                                  // 场景标识，须明确标注"模拟"
    QVector<TargetInfo> targets;                    // 模拟目标列表
    QVector<MissionInfo> missions;                  // 模拟任务列表
    QVector<DeviceInfo> devices;                     // 模拟设备列表
    QVector<DetectionScriptEntry> detectionScript;  // 探测阶段脚本数据
};

// 模拟演示场景提供者
// 提供 MVP 阶段所需的本地模拟数据，所有数据均为假数据，不涉及真实设备或真实任务。
class DemoScenarioProvider
{
public:
    // 创建并返回一个模拟演示场景
    static DemoScenario create();

private:
    // 构造探测阶段脚本：5 目标在 10s/25s/42s/60s/78s 被探测到
    static QVector<DetectionScriptEntry> buildDetectionScript(const QDateTime &baseTime);
};

}

#endif
