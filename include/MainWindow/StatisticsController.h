#ifndef MAINWINDOW_STATISTICSCONTROLLER_H
#define MAINWINDOW_STATISTICSCONTROLLER_H

// REQ-014 统计页控制器：MainWindow 层首个真 QThread 控制器。
// 模式沿用 MosPlanningController（include/MainWindow/MosPlanningController.h）：
//   - 值拷贝进/出：快照请求仅带 revision，列表请求按值携带过滤器；
//   - revision 守卫：陈旧快照完成结果在 controller 侧丢弃，不向 UI 转发；
//   - worker 与 controller 同头文件，worker 仅经信号槽交互、永不共享对象指针。
// 线程纪律（docs/features/statistics-data-platform.md §6）：
//   - StatisticsWorker 活在 m_workerThread，懒建并独占 HttpClient/DataPlatformApi/
//     VocabularyCache（与 IntegrationHttpClientAdapter 同纪律，规避 QNAM 线程亲和）；
//   - 全部 HTTP/解析/聚合在 worker 线程；UI 线程只接收值类型信号
//     （StatisticsSnapshot/StatisticsListPage，均已 Q_DECLARE_METATYPE）；
//   - 快照与列表请求共用 worker 事件循环 FIFO 串行（同步 API 不可中途取消；
//     陈旧快照结果经 revision 丢弃）；
//   - 析构先 quit 再 wait：所有 API 调用为短超时同步调用，wait 上界可预期。

#include "Core/Data/Types.h"
#include "Integration/DataPlatformApi.h"
#include "Integration/VocabularyCache.h"
#include "MainWindow/StatisticsSnapshot.h"

#include <QDateTime>
#include <QObject>
#include <QThread>
#include <QVector>

class StatisticsWorker;

class StatisticsController : public QObject {
    Q_OBJECT

public:
    // 列表请求参数（值类型；filter 为契约 §7 对应实体的过滤结构体，
    // pageSize 固定 20 由调用方/UI 层约定）。
    struct ListRequest {
        QString resource;  // targets / devices / missions / alarms / detection-records
        Integration::TargetFilter targetFilter;
        Integration::DeviceFilter deviceFilter;
        Integration::MissionFilter missionFilter;
        Integration::AlarmFilter alarmFilter;
        Integration::DetectionRecordFilter detectionRecordFilter;
        Integration::PageQuery page;
    };

    explicit StatisticsController(QObject *parent = nullptr);
    ~StatisticsController() override;  // quit + wait 回收线程

    StatisticsController(const StatisticsController &) = delete;
    StatisticsController &operator=(const StatisticsController &) = delete;

public slots:
    // 触发一次全量快照拉取（进页加载/手动刷新）。revision 自增；
    // 仅最新 revision 的结果经 snapshotReady/snapshotFetchFailed 转发。
    void requestSnapshot();
    // 触发一次明细分页查询。结果串行回投，无 revision（拉取期间 UI 禁用相关控件）。
    void requestList(const StatisticsController::ListRequest &request);

signals:
    // 转发给 worker（值语义跨线程队列连接）
    void snapshotRequested(int revision);
    void listRequested(const StatisticsController::ListRequest &request);
    // 回投 UI 线程
    void snapshotReady(const StatisticsSnapshot &snapshot);
    void snapshotFetchFailed(const QString &message);
    void listReady(const StatisticsListPage &page);
    void listFetchFailed(const QString &resource, const QString &message);

private slots:
    void onSnapshotFinished(int revision, const StatisticsSnapshot &snapshot);
    void onSnapshotFailed(int revision, const QString &message);

private:
    int m_revision = 0;  // 仅 UI 线程（controller 所属线程）读写
    QThread m_workerThread;
    StatisticsWorker *m_worker = nullptr;  // 归属 m_workerThread，deleteLater 回收
};

// 线程工作者：worker 线程内懒建并独占数据平台适配对象。
// 对外仅暴露信号槽；不得在主线程直接调用其任何方法。
class StatisticsWorker : public QObject {
    Q_OBJECT

public slots:
    void runSnapshotTask(int revision);
    void runListTask(const StatisticsController::ListRequest &request);

signals:
    void snapshotFinished(int revision, const StatisticsSnapshot &snapshot);
    void snapshotFailed(int revision, const QString &message);
    void listFinished(const StatisticsListPage &page);
    void listFailed(const QString &resource, const QString &message);

private:
    // 失败报文口径：首个失败的 resource + 端点/契约错误语义（ApiError 首条 items
    // 或 HTTP 状态）；快照拉取任一步骤失败即整体失败。
    QString formatApiError(const QString &resource, const Integration::ApiError &error) const;

    // 以下对象仅在 worker 线程创建与访问（首次任务执行时懒建）。
    Integration::DataPlatformApi *m_api = nullptr;
    Integration::VocabularyCache *m_vocabularyCache = nullptr;
};

Q_DECLARE_METATYPE(StatisticsSnapshot)
Q_DECLARE_METATYPE(StatisticsListPage)
Q_DECLARE_METATYPE(StatisticsController::ListRequest)

#endif // MAINWINDOW_STATISTICSCONTROLLER_H
