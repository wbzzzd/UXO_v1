// MOS P0 MainWindow-library 控制器实现：在 plain Core 会话之上增加 revision-safe replan
// 与单向 fixture 导出。controller 拥有会话、worker 与 pending 不可变请求；不引入
// QThread/事件总线/Repository/Store/Dispatcher。所有数据均为合成本地 fixture 语义，
// 非真实设备、真实跑道或真实作业参数。

#include "MainWindow/MosPlanningController.h"

#include <QFileInfo>
#include <QIODevice>
#include <QMetaType>
#include <QSaveFile>

namespace Core::MOS {

namespace {

// 逐位比对 worker 完成结果与控制器重算结果。
// 比对项：accepted、reason、tier count、每档 repairedIds/矩形字段/估算字段。
// 规划器确定性可复现，使用精确 double 比较（无 epsilon）。
bool completionMatchesRecompute(const MosProgressiveResult &worker,
                                const MosProgressiveResult &recomputed)
{
    if (worker.accepted != recomputed.accepted) return false;
    if (worker.reason != recomputed.reason) return false;
    if (worker.tiers.size() != recomputed.tiers.size()) return false;

    for (int t = 0; t < worker.tiers.size(); ++t) {
        const auto &w = worker.tiers[t];
        const auto &r = recomputed.tiers[t];
        if (w.repairedIds != r.repairedIds) return false;

        const auto &wr = w.rectangle;
        const auto &rr = r.rectangle;
        if (wr.valid != rr.valid) return false;
        if (wr.reason != rr.reason) return false;
        if (wr.xStart != rr.xStart) return false;
        if (wr.xEnd != rr.xEnd) return false;
        if (wr.yStart != rr.yStart) return false;
        if (wr.yEnd != rr.yEnd) return false;
        if (wr.length != rr.length) return false;
        if (wr.width != rr.width) return false;
        if (wr.area != rr.area) return false;

        const auto &we = w.estimate;
        const auto &re = r.estimate;
        if (we.totalBackfillVolume != re.totalBackfillVolume) return false;
        if (we.backfillHours != re.backfillHours) return false;
        if (we.uxoHours != re.uxoHours) return false;
        if (we.totalHours != re.totalHours) return false;
        if (we.difficulty != re.difficulty) return false;
    }
    return true;
}

} // namespace

// === MosReplanWorker ===

MosReplanWorker::MosReplanWorker(QObject *parent)
    : QObject(parent)
{
    // MOC 在 namespace 内记录信号参数名为非限定短名；必须用短名注册 metatype，
    // 否则 QSignalSpy 经 QVariant 取值得到默认构造结果（revision=0）。此处注册
    // 覆盖独立构造的 worker 测试。勿与 controller 的全名注册视作重复而删除。
    qRegisterMetaType<MosReplanRequest>("MosReplanRequest");
    qRegisterMetaType<MosReplanCompletion>("MosReplanCompletion");
}

MosReplanWorker::~MosReplanWorker() = default;

void MosReplanWorker::replan(const MosReplanRequest &request)
{
    // 同步执行：拷贝请求值，调用纯规划器，发出恰好一次完成通知。
    MosReplanCompletion completion;
    completion.revision = request.revision;
    completion.result = MosPlanner::planProgressive(request.obstacles, request.params);
    emit replanCompleted(completion);
}

// === MosPlanningController ===

MosPlanningController::MosPlanningController(QObject *parent)
    : QObject(parent)
{
    // 注册值类型，确保 QVariant/QSignalSpy 在直连下也能识别。
    // 全名注册覆盖 controller 侧 spy；短名注册匹配 MOC 在 namespace 内记录的参数名。
    qRegisterMetaType<MosReplanRequest>("Core::MOS::MosReplanRequest");
    qRegisterMetaType<MosReplanCompletion>("Core::MOS::MosReplanCompletion");
    qRegisterMetaType<MosReplanRequest>("MosReplanRequest");
    qRegisterMetaType<MosReplanCompletion>("MosReplanCompletion");

    // 发起请求 -> worker 同步执行（直连）。
    connect(this, &MosPlanningController::replanRequested,
            &m_worker, &MosReplanWorker::replan, Qt::DirectConnection);
    // worker 完成 -> 控制器处置（直连 lambda）。
    connect(&m_worker, &MosReplanWorker::replanCompleted, this,
            [this](const MosReplanCompletion &completion) { completeReplan(completion); },
            Qt::DirectConnection);
}

MosPlanningController::~MosPlanningController() = default;

MosPlanningSnapshot MosPlanningController::snapshot() const
{
    return m_session.snapshot();
}

MosReplanWorker *MosPlanningController::worker()
{
    return &m_worker;
}

bool MosPlanningController::isReplanning() const
{
    return m_pending.has_value();
}

quint64 MosPlanningController::requestReplan(const MosObstacleSet &obstacles,
                                             const MosRunwayParams &params)
{
    // revision 在校验前分配；非法输入同样覆盖较旧的 pending。
    const quint64 revision = m_nextRevision++;

    MosReplanRequest request;
    request.revision = revision;
    request.obstacles = obstacles;
    request.params = params;
    m_pending = request;  // 替换 pending 请求

    emit replanActivityChanged(true);
    emit replanRequested(request);  // 同步触发 worker.replan -> completeReplan

    return revision;
}

void MosPlanningController::replaceObstacles(const MosObstacleSet &obstacles,
                                              const MosRunwayParams &params)
{
    m_session.replaceObstacles(obstacles, params);
    emit mosStateChanged();
}

MosCompletionDisposition MosPlanningController::completeReplan(const MosReplanCompletion &completion)
{
    // 陈旧守卫（最先）：无 pending 或 revision 不匹配，业务不变、不追加日志、不发通知。
    if (!m_pending.has_value() || m_pending->revision != completion.revision) {
        return MosCompletionDisposition::IgnoredStale;
    }

    // 取出 pending 请求的障碍物/参数（值拷贝），再清除 pending。
    const MosReplanRequest pendingRequest = *m_pending;
    m_pending.reset();
    emit replanActivityChanged(false);

    // 合法拒绝路径：worker 已判定 accepted=false，直接追加拒绝日志。
    // 重算一致性校验只对 accepted=true 的完成结果执行（拒绝结果无几何可重算）。
    if (!completion.result.accepted) {
        m_session.rejectReplan(completion.revision, completion.result.reason,
                               completion.result.message);
        emit mosStateChanged();
        return MosCompletionDisposition::Rejected;
    }

    // 重算守卫：从 worker 完成结果提取档位 ID 方案，用 supplied-tier 重载重新规划，
    // 与 worker 完成结果逐位比对。
    //   - 重算自身拒绝（supplied 档位结构非法，如非嵌套/未知 ID/末档不全）：
    //     按 重算的 reason/message 拒绝（防 worker 上报了结构非法却 accepted=true 的结果）。
    //   - 重算接受但与 worker 完成结果逐位不等：CompletionMismatch（防几何/估算被篡改）。
    QVector<MosTierPlan> suppliedTiers;
    suppliedTiers.reserve(completion.result.tiers.size());
    for (const auto &tier : completion.result.tiers)
        suppliedTiers.append(MosTierPlan{tier.repairedIds});

    const MosProgressiveResult recomputed = MosPlanner::planProgressive(
        pendingRequest.obstacles, pendingRequest.params, suppliedTiers);

    if (!recomputed.accepted) {
        // 重算判定 supplied 档位结构非法：用重算的具体原因拒绝（原子拒绝，不提交）
        m_session.rejectReplan(completion.revision, recomputed.reason, recomputed.message);
        emit mosStateChanged();
        return MosCompletionDisposition::Rejected;
    }

    if (!completionMatchesRecompute(completion.result, recomputed)) {
        // 结构合法但几何/估算/档位 ID 与重算不一致：视为篡改/陈旧
        m_session.rejectReplan(completion.revision,
                               MosPlannerReason::CompletionMismatch,
                               QStringLiteral("重算结果与 worker 完成结果不一致"));
        emit mosStateChanged();
        return MosCompletionDisposition::Rejected;
    }

    m_session.commitReplan(completion.revision, pendingRequest.obstacles,
                           pendingRequest.params, completion.result);
    emit mosStateChanged();
    return MosCompletionDisposition::Committed;
}

bool MosPlanningController::selectTier(int tierIndex)
{
    // 仅在 session 报告有效切换时发一次通知；越界/同档位无通知。
    if (!m_session.selectTier(tierIndex)) {
        return false;
    }
    emit mosStateChanged();
    return true;
}

MosExportResult MosPlanningController::exportFixture(const QString &targetPath) const
{
    // 单向观测导出：按值快照，序列化已提交障碍物，写入显式路径。
    // 不改业务状态/revision/日志/通知计数。
    // 安全策略：仅允许本地 fixture 的显式绝对 .json 路径；拒绝无提交快照、空/相对路径、
    // 非 .json 后缀、符号链接目标及已存在的非普通文件目标（目录/设备/管道等）。
    const MosPlanningSnapshot snap = m_session.snapshot();

    // 守卫 1：必须有已接受的提交快照。hasResult 表示已 commit，result.accepted 二次确认。
    if (!snap.hasResult || !snap.result.accepted) {
        return {false, QStringLiteral("本地 fixture 导出被拒：无已接受的提交快照")};
    }

    // 守卫 2：目标路径必须非空。
    if (targetPath.isEmpty()) {
        return {false, QStringLiteral("本地 fixture 导出被拒：目标路径为空")};
    }

    const QFileInfo targetInfo(targetPath);

    // 守卫 3：目标路径必须为绝对路径，避免相对路径随 CWD 漂移写到任意位置。
    if (targetInfo.isRelative()) {
        return {false, QStringLiteral("本地 fixture 导出被拒：目标路径必须为绝对路径：%1").arg(targetPath)};
    }

    // 守卫 4：目标路径必须以 .json 结尾（大小写不敏感），明确本地 fixture 语义。
    if (targetInfo.suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) != 0) {
        return {false, QStringLiteral("本地 fixture 导出被拒：目标路径必须以 .json 结尾：%1").arg(targetPath)};
    }

    // 守卫 5：已存在的目标不得为符号链接（不论是否悬空），避免写穿或替换到任意路径。
    if (targetInfo.isSymLink()) {
        return {false, QStringLiteral("本地 fixture 导出被拒：目标已存在为符号链接：%1").arg(targetPath)};
    }

    // 守卫 6：已存在的目标必须是普通文件，拒绝目录/设备/管道等非普通文件。
    if (targetInfo.exists() && !targetInfo.isFile()) {
        return {false, QStringLiteral("本地 fixture 导出被拒：目标已存在且非普通文件：%1").arg(targetPath)};
    }

    const QByteArray payload = serializeObstacleSetBytes(snap.obstacles);

    QSaveFile file(targetPath);
    file.setDirectWriteFallback(false);  // 显式禁用直接写回退，强制原子替换语义
    if (!file.open(QIODevice::WriteOnly)) {
        return {false, QStringLiteral("无法打开导出文件：%1").arg(targetPath)};
    }
    if (file.write(payload) != payload.size()) {
        return {false, QStringLiteral("写入导出文件失败：%1").arg(targetPath)};
    }
    if (!file.commit()) {
        return {false, QStringLiteral("提交导出文件失败：%1").arg(targetPath)};
    }
    return {true, QStringLiteral("已导出 fixture 至 %1").arg(targetPath)};
}

} // namespace Core::MOS
