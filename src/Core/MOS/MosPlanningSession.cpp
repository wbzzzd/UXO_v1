// MOS P0 应用会话实现：持有已提交业务状态、当前档位、已提交 revision 与顺序日志。
// 仅依赖 Qt 值类型与 Core::MOS 数据模型，不依赖 UI/网络/数据库/真实设备。
// 所有几何与估算均为合成本地 fixture 语义，非真实跑道、真实弹坑或真实作业参数。

#include "Core/MOS/MosPlanningSession.h"

namespace Core::MOS {

namespace {

// 构造一条顺序日志条目：sequence 由调用方分配并递增，时间戳取 UTC。
MosSessionLogEntry makeLogEntry(quint64 sequence,
                                quint64 revision,
                                MosSessionLogType type,
                                MosPlannerReason reason,
                                const QString &message)
{
    MosSessionLogEntry entry;
    entry.sequence = sequence;
    entry.revision = revision;
    entry.timestampUtc = QDateTime::currentDateTimeUtc();
    entry.type = type;
    entry.reason = reason;
    entry.message = message;
    return entry;
}

} // namespace

MosPlanningSnapshot MosPlanningSession::snapshot() const
{
    // 按值返回快照，调用方任意持有，不暴露内部可变状态。
    MosPlanningSnapshot snap;
    snap.obstacles = m_obstacles;
    snap.params = m_params;
    snap.result = m_result;
    snap.hasResult = m_hasResult;
    snap.selectedTier = m_selectedTier;
    snap.committedRevision = m_committedRevision;
    snap.logEntries = m_logEntries;
    return snap;
}

void MosPlanningSession::commitReplan(quint64 revision,
                                      const MosObstacleSet &obstacles,
                                      const MosRunwayParams &params,
                                      const MosProgressiveResult &result)
{
    // 写入业务状态：障碍物/参数/结果均按值拷贝。
    m_obstacles = obstacles;
    m_params = params;
    m_result = result;
    m_hasResult = true;
    m_committedRevision = revision;

    // 保留当前档位若仍在合法范围，否则回到 0。
    if (m_selectedTier >= m_result.tiers.size()) {
        m_selectedTier = 0;
    }

    m_logEntries.append(makeLogEntry(m_nextSequence++, revision,
                                     MosSessionLogType::ReplanAccepted,
                                     result.reason,
                                     QStringLiteral("已接受 replan（revision %1）").arg(revision)));
}

void MosPlanningSession::rejectReplan(quint64 revision,
                                      MosPlannerReason reason,
                                      const QString &message)
{
    // 业务字段/已提交 revision/档位选择均不变，仅追加一条拒绝日志。
    m_logEntries.append(makeLogEntry(m_nextSequence++, revision,
                                     MosSessionLogType::ReplanRejected,
                                     reason, message));
}

void MosPlanningSession::replaceObstacles(const MosObstacleSet &obstacles,
                                          const MosRunwayParams &params)
{
    m_obstacles = obstacles;
    m_params = params;
    m_result = {};
    m_hasResult = false;
    m_selectedTier = 0;
}

bool MosPlanningSession::selectTier(int tierIndex)
{
    // 无结果或越界：返回 false 且不改状态。
    if (!m_hasResult) {
        return false;
    }
    if (tierIndex < 0 || tierIndex >= m_result.tiers.size()) {
        return false;
    }
    // 同档位幂等：返回 false 且不追加日志。
    if (tierIndex == m_selectedTier) {
        return false;
    }

    m_selectedTier = tierIndex;
    m_logEntries.append(makeLogEntry(m_nextSequence++, m_committedRevision,
                                     MosSessionLogType::TierSelected,
                                     MosPlannerReason::Accepted,
                                     QStringLiteral("已切换到档位 %1").arg(tierIndex)));
    return true;
}

} // namespace Core::MOS
