#include "Core/Simulation/SimulationWorkflow.h"

namespace Core::Simulation {

void SimulationWorkflow::reset(const QVector<Core::TargetInfo> &targets)
{
    // 重置表示新的本地演示会话，不保留选择和历史日志。
    m_targets = targets;
    m_selectedTargetId.clear();
    m_logEntries.clear();
    m_nextSequence = 1;
}

bool SimulationWorkflow::selectTarget(const QString &targetId)
{
    Core::TargetInfo *target = findTarget(targetId);
    if (target == nullptr) {
        appendLog(SimulationOperationType::ActionRejected,
                  targetId,
                  Core::TargetStatus::Unknown,
                  Core::TargetStatus::Unknown,
                  QStringLiteral("[模拟] 操作被拒绝：模拟目标 %1 不存在").arg(targetId),
                  QDateTime::currentDateTimeUtc());
        return false;
    }

    // 重复选择保持幂等，避免点击事件产生重复日志。
    if (m_selectedTargetId == targetId) {
        return true;
    }

    m_selectedTargetId = targetId;
    appendLog(SimulationOperationType::TargetSelected,
              targetId,
              target->status,
              target->status,
              QStringLiteral("[模拟] 已选择目标 %1").arg(targetId),
              QDateTime::currentDateTimeUtc());
    return true;
}

bool SimulationWorkflow::requestSelectedTargetStatus(Core::TargetStatus requestedStatus)
{
    Core::TargetInfo *target = findTarget(m_selectedTargetId);
    if (target == nullptr) {
        appendLog(SimulationOperationType::ActionRejected,
                  QString(),
                  Core::TargetStatus::Unknown,
                  Core::TargetStatus::Unknown,
                  QStringLiteral("[模拟] 操作被拒绝：未选择模拟目标"),
                  QDateTime::currentDateTimeUtc());
        return false;
    }

    const Core::TargetStatus currentStatus = target->status;
    const bool isAllowed =
        (currentStatus == Core::TargetStatus::Detected
         && requestedStatus == Core::TargetStatus::Confirmed)
        || (currentStatus == Core::TargetStatus::Confirmed
            && requestedStatus == Core::TargetStatus::Disposing)
        || (currentStatus == Core::TargetStatus::Disposing
            && requestedStatus == Core::TargetStatus::Disposed);

    if (!isAllowed) {
        appendLog(
            SimulationOperationType::ActionRejected,
            target->id,
            currentStatus,
            requestedStatus,
            QStringLiteral("[模拟] 操作被拒绝：目标 %1 不能从%2变更为%3")
                .arg(target->id,
                     simulationStatusText(currentStatus),
                     simulationStatusText(requestedStatus)),
            QDateTime::currentDateTimeUtc());
        return false;
    }

    const QDateTime timestampUtc = QDateTime::currentDateTimeUtc();
    target->status = requestedStatus;
    target->updateTime = timestampUtc;
    appendLog(SimulationOperationType::StatusChanged,
              target->id,
              currentStatus,
              requestedStatus,
              QStringLiteral("[模拟] 目标 %1：%2 -> %3")
                  .arg(target->id,
                       simulationStatusText(currentStatus),
                       simulationStatusText(requestedStatus)),
              timestampUtc);
    return true;
}

bool SimulationWorkflow::hasSelectedTarget() const
{
    return selectedTarget() != nullptr;
}

QString SimulationWorkflow::selectedTargetId() const
{
    return m_selectedTargetId;
}

const Core::TargetInfo *SimulationWorkflow::selectedTarget() const
{
    return findTarget(m_selectedTargetId);
}

const QVector<Core::TargetInfo> &SimulationWorkflow::targets() const
{
    return m_targets;
}

const QVector<SimulationOperationLogEntry> &SimulationWorkflow::logEntries() const
{
    return m_logEntries;
}

QString SimulationWorkflow::simulationStatusText(Core::TargetStatus status)
{
    switch (status) {
    case Core::TargetStatus::Detected:
        return QStringLiteral("已发现");
    case Core::TargetStatus::Confirmed:
        return QStringLiteral("已确认");
    case Core::TargetStatus::Disposing:
        return QStringLiteral("处置中");
    case Core::TargetStatus::Disposed:
        return QStringLiteral("已完成");
    default:
        return QStringLiteral("不支持的模拟状态");
    }
}

Core::TargetInfo *SimulationWorkflow::findTarget(const QString &targetId)
{
    for (Core::TargetInfo &target : m_targets) {
        if (target.id == targetId) {
            return &target;
        }
    }
    return nullptr;
}

const Core::TargetInfo *SimulationWorkflow::findTarget(const QString &targetId) const
{
    for (const Core::TargetInfo &target : m_targets) {
        if (target.id == targetId) {
            return &target;
        }
    }
    return nullptr;
}

void SimulationWorkflow::appendLog(SimulationOperationType type,
                                   const QString &targetId,
                                   Core::TargetStatus beforeStatus,
                                   Core::TargetStatus afterStatus,
                                   const QString &message,
                                   const QDateTime &timestampUtc)
{
    SimulationOperationLogEntry entry;
    entry.sequence = m_nextSequence++;
    entry.timestampUtc = timestampUtc;
    entry.type = type;
    entry.targetId = targetId;
    entry.beforeStatus = beforeStatus;
    entry.afterStatus = afterStatus;
    entry.message = message;
    m_logEntries.append(entry);
}

}
