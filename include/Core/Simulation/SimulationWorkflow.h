#ifndef CORE_SIMULATION_SIMULATIONWORKFLOW_H
#define CORE_SIMULATION_SIMULATIONWORKFLOW_H

#include "Core/Data/Types.h"

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace Core::Simulation {

// 模拟操作类型仅用于本地演示日志。
enum class SimulationOperationType {
    TargetSelected,
    StatusChanged,
    ActionRejected,
    TargetDetected  // 探测阶段新目标被"探测"到 (阶段4 探测脚本驱动)
};

// 内存日志使用序号保证顺序，不依赖系统时钟排序。
struct SimulationOperationLogEntry {
    quint64 sequence{0};
    QDateTime timestampUtc;
    SimulationOperationType type{SimulationOperationType::ActionRejected};
    QString targetId;
    Core::TargetStatus beforeStatus{Core::TargetStatus::Unknown};
    Core::TargetStatus afterStatus{Core::TargetStatus::Unknown};
    QString message;
};

// 管理模拟目标副本、选择状态及进程内操作日志。
class SimulationWorkflow
{
public:
    void reset(const QVector<Core::TargetInfo> &targets);
    bool selectTarget(const QString &targetId);
    bool requestSelectedTargetStatus(Core::TargetStatus requestedStatus);

    // 探测阶段驱动: 运行时插入 AI 检测新目标并追加探测日志
    // target 为新探测到的目标; 追加一条 "[AI] 探测到目标 TGT-XXX" 日志
    void addTarget(const Core::TargetInfo &target);

    // 探测页 [拒绝]: 选中目标判定为误报 (仅 Detected 状态允许流转到 FalseAlarm)
    // 成功追加 "[AI] 目标 XXX 判定为误报" 日志并返回 true;
    // 无选中/状态不允许时追加拒绝日志并返回 false
    bool markSelectedTargetFalseAlarm();

    bool hasSelectedTarget() const;
    QString selectedTargetId() const;
    const Core::TargetInfo *selectedTarget() const;
    const QVector<Core::TargetInfo> &targets() const;
    const QVector<SimulationOperationLogEntry> &logEntries() const;

    static QString simulationStatusText(Core::TargetStatus status);

private:
    Core::TargetInfo *findTarget(const QString &targetId);
    const Core::TargetInfo *findTarget(const QString &targetId) const;
    void appendLog(SimulationOperationType type,
                   const QString &targetId,
                   Core::TargetStatus beforeStatus,
                   Core::TargetStatus afterStatus,
                   const QString &message,
                   const QDateTime &timestampUtc);

    QVector<Core::TargetInfo> m_targets;
    QString m_selectedTargetId;
    QVector<SimulationOperationLogEntry> m_logEntries;
    quint64 m_nextSequence{1};
};

}

#endif
