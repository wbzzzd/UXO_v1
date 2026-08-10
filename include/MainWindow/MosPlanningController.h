#ifndef MAINWINDOW_MOSPLANNINGCONTROLLER_H
#define MAINWINDOW_MOSPLANNINGCONTROLLER_H

// MOS P0 MainWindow-library 控制器：在 plain Core 会话之上增加 revision-safe replan
// 与单向 fixture 导出。controller 拥有会话、worker 与 pending 不可变请求；不引入
// QThread/事件总线/Repository/Store/Dispatcher。所有数据均为合成本地 fixture 语义，
// 非真实设备、真实跑道或真实作业参数。

#include "Core/MOS/MosPlanningSession.h"
#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosPlanner.h"

#include <QObject>
#include <QString>
#include <QtGlobal>

#include <optional>

namespace Core::MOS {

// replan 请求：revision 由 controller 在校验前分配；obstacles/params 为值拷贝的不可变快照。
struct MosReplanRequest {
    quint64 revision{0};       // 单调递增的 revision
    MosObstacleSet obstacles;  // 合成障碍物集合
    MosRunwayParams params;    // 合成跑道参数
};

// replan 完成结果：worker 返回的值拷贝；revision 必须与 pending 请求匹配才能提交。
struct MosReplanCompletion {
    quint64 revision{0};            // 对应请求的 revision
    MosProgressiveResult result;    // 递进规划结果
};

// replan 完成处置：controller 决定是否写入会话状态。
enum class MosCompletionDisposition {
    Committed,    // 已接受并写入会话状态
    Rejected,     // 已拒绝（合法拒绝路径），仅追加拒绝日志
    IgnoredStale  // 陈旧/重复完成：业务状态不变，不追加日志，不发通知
};

// 单向 fixture 导出结果：仅用于 observational export，不改变业务状态。
struct MosExportResult {
    bool success{false};  // 是否成功写入目标路径
    QString message;      // 人类可读说明（含失败原因）
};

// 同步 worker seam：消费值拷贝请求，返回值拷贝完成；不持有或修改会话。
// Todo 7 测量证据若要求线程化，可在此处引入 QThread，revision guard 保持不变。
class MosReplanWorker : public QObject
{
    Q_OBJECT

public:
    explicit MosReplanWorker(QObject *parent = nullptr);
    ~MosReplanWorker();

public slots:
    // 同步执行 replan；完成后发出 replanCompleted。
    void replan(const MosReplanRequest &request);

signals:
    // 发出完成结果；controller 据此调用 completeReplan。
    void replanCompleted(const MosReplanCompletion &completion);
};

// MainWindow-library 控制器：拥有 session/worker/pending request。
// 通知契约（Oracle 决议）：
//   - 最新接受或拒绝日志追加：发出恰好一次 mosStateChanged。
//   - 陈旧/重复完成：不写状态、不追加日志、不发通知。
//   - revision 在校验前分配；非法的更新请求覆盖较旧的 pending。
class MosPlanningController : public QObject
{
    Q_OBJECT

public:
    explicit MosPlanningController(QObject *parent = nullptr);
    ~MosPlanningController();

    // 当前会话快照（按值返回）。
    MosPlanningSnapshot snapshot() const;

    // 返回内部 worker 指针（生命周期由 controller 持有）。
    MosReplanWorker *worker();

    // 是否存在 pending 请求未完成。
    bool isReplanning() const;

    // 发起 replan：分配新 revision、记录 pending 请求、发出 replanRequested 与
    // replanActivityChanged(true)，并同步触发 worker.replan。返回分配的 revision。
    // 非法输入同样分配 revision 并覆盖较旧的 pending。
    quint64 requestReplan(const MosObstacleSet &obstacles,
                          const MosRunwayParams &params);

    // 替换障碍物与参数但不执行规划：更新会话障碍物/参数、清除已有结果。
    // 发出 mosStateChanged，不发出 replanActivityChanged（无规划活动）。
    void replaceObstacles(const MosObstacleSet &obstacles,
                          const MosRunwayParams &params);

    // 处理 worker 完成：revision 匹配 pending 则按 result.accepted 提交或拒绝，
    //   并发出 mosStateChanged 与 replanActivityChanged(false)；
    // 陈旧/重复返回 IgnoredStale，不写状态、不追加日志、不发通知。
    MosCompletionDisposition completeReplan(const MosReplanCompletion &completion);

    // 切换档位；越界返回 false；有效切换写入会话并发出 mosStateChanged。
    bool selectTier(int tierIndex);

    // 单向导出：将当前已接受 fixture 序列化到显式目标路径（通过 QSaveFile），
    // 不改业务状态/revision/日志/通知计数。
    MosExportResult exportFixture(const QString &targetPath) const;

signals:
    // 发起 replan 时发出（携带 pending 不可变请求副本）。
    void replanRequested(const MosReplanRequest &request);

    // 权威快照变化时发出一次：接受、合法无解、最新拒绝日志追加、有效档位切换。
    void mosStateChanged();

    // replan 活动状态变化：true 表示进入 pending，false 表示完成或被覆盖。
    void replanActivityChanged(bool active);

private:
    MosPlanningSession m_session;             // plain Core 会话
    MosReplanWorker m_worker;                 // 同步 worker（值拥有）
    std::optional<MosReplanRequest> m_pending; // pending 不可变请求
    quint64 m_nextRevision{1};                // 下一个分配的 revision
};

} // namespace Core::MOS

// 让 QSignalSpy/QVariant 识别值类型；运行时尚需 qRegisterMetaType。
Q_DECLARE_METATYPE(Core::MOS::MosReplanRequest)
Q_DECLARE_METATYPE(Core::MOS::MosReplanCompletion)

#endif // MAINWINDOW_MOSPLANNINGCONTROLLER_H
