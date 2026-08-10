#ifndef CORE_MOS_MOSPLANNINGSESSION_H
#define CORE_MOS_MOSPLANNINGSESSION_H

// MOS P0 应用会话：持有已提交业务状态、当前档位、已提交 revision 与顺序日志。
// 仅依赖 Qt 值类型与 Core::MOS 数据模型，不依赖 UI/网络/数据库/真实设备。
// 所有几何与估算均为合成本地 fixture 语义，非真实跑道、真实弹坑或真实作业参数。

#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosPlanner.h"

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace Core::MOS {

// 会话日志类型：仅记录本地 replan 接受/拒绝与档位切换，不写真实控制日志。
enum class MosSessionLogType {
    ReplanAccepted,  // replan 接受（合法有解或合法无解）
    ReplanRejected,  // replan 拒绝（非法/非嵌套/陈旧等）
    TierSelected     // 档位切换
};

// 顺序日志条目：sequence 单调递增，不依赖系统时钟排序。
struct MosSessionLogEntry {
    quint64 sequence{0};                                       // 顺序号
    quint64 revision{0};                                       // 关联的 replan revision
    QDateTime timestampUtc;                                    // UTC 时间戳（仅展示）
    MosSessionLogType type{MosSessionLogType::ReplanRejected}; // 类型
    MosPlannerReason reason{MosPlannerReason::Accepted};       // 规划原因码
    QString message;                                           // 人类可读说明
};

// 会话快照：调用方按值持有，不暴露内部可变状态。
struct MosPlanningSnapshot {
    MosObstacleSet obstacles;                   // 已提交障碍物集合（合成 fixture）
    MosRunwayParams params;                     // 已提交跑道参数（合成几何）
    MosProgressiveResult result;                // 已提交递进规划结果
    bool hasResult{false};                      // 是否存在已提交结果
    int selectedTier{0};                        // 当前选中档位序号
    quint64 committedRevision{0};               // 已提交 revision
    QVector<MosSessionLogEntry> logEntries;     // 顺序日志（按 sequence 升序）
};

// plain Core 会话：不继承 QObject，由 controller 按值或唯一拥有。
// 职责：保存已提交业务状态、追加顺序日志、校验档位切换；不发出任何信号。
class MosPlanningSession
{
public:
    // 返回当前快照（按值拷贝，调用方可任意持有）。
    MosPlanningSnapshot snapshot() const;

    // 提交一次 replan：写入业务状态、追加 ReplanAccepted 日志。
    // 合法有解：result.accepted=true 且 tiers 非空；
    // 合法无解：result.accepted=true 且存在 valid=false 档位（空结果）。
    // 调用方负责 revision 单调性；陈旧 revision 不应进入此接口。
    void commitReplan(quint64 revision,
                      const MosObstacleSet &obstacles,
                      const MosRunwayParams &params,
                      const MosProgressiveResult &result);

    // 拒绝一次 replan：业务状态不变，仅追加 ReplanRejected 日志。
    void rejectReplan(quint64 revision,
                      MosPlannerReason reason,
                      const QString &message);

    // 替换障碍物集合与参数，不执行规划：写入 m_obstacles/m_params，
    // 清除已有结果（m_hasResult=false），档位归零。用于生成器仅生成障碍物、
    // 等待用户手动触发 replan 的场景。不追加日志、不改 committedRevision。
    void replaceObstacles(const MosObstacleSet &obstacles,
                          const MosRunwayParams &params);

    // 切换当前档位；序号越界返回 false 且不改状态，否则追加 TierSelected 日志并返回 true。
    bool selectTier(int tierIndex);

private:
    MosObstacleSet m_obstacles;
    MosRunwayParams m_params;
    MosProgressiveResult m_result;
    bool m_hasResult{false};
    int m_selectedTier{0};
    quint64 m_committedRevision{0};
    QVector<MosSessionLogEntry> m_logEntries;
    quint64 m_nextSequence{1};
};

} // namespace Core::MOS

#endif // CORE_MOS_MOSPLANNINGSESSION_H
