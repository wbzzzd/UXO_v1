// REQ-014 统计页控制器实现：controller/worker 双对象 + 真 QThread。
// - controller（UI 线程）：revision 自增派发快照请求，陈旧结果（revision 不匹配）丢弃；
// - worker（m_workerThread）：懒建并独占 DataPlatformApi/VocabularyCache，
//   快照任务 = 健康检查 → 词表加载 → 五实体全量分页拉取 → 纯聚合 → 补全元数据；
//   列表任务 = 按 resource 直接转发对应分页 API；
// - 全部跨线程交互均为值语义排队信号（StatisticsSnapshot/StatisticsListPage）。
// 线程纪律见 docs/features/statistics-data-platform.md §6。

#include "MainWindow/StatisticsController.h"

#include "MainWindow/StatisticsAggregation.h"

#include <QMetaType>

namespace {

// ApiErrorCode → 契约 snake_case 字符串（§8 全部 10 契约码 + 4 客户端内部码）。
QString apiErrorCodeToString(Integration::ApiErrorCode code)
{
    switch (code) {
    case Integration::ApiErrorCode::InvalidEnum:
        return QStringLiteral("invalid_enum");
    case Integration::ApiErrorCode::OutOfRange:
        return QStringLiteral("out_of_range");
    case Integration::ApiErrorCode::ForeignKeyViolation:
        return QStringLiteral("foreign_key_violation");
    case Integration::ApiErrorCode::InvalidFormat:
        return QStringLiteral("invalid_format");
    case Integration::ApiErrorCode::NotNull:
        return QStringLiteral("not_null");
    case Integration::ApiErrorCode::DuplicateId:
        return QStringLiteral("duplicate_id");
    case Integration::ApiErrorCode::NotFound:
        return QStringLiteral("not_found");
    case Integration::ApiErrorCode::PaginationInvalid:
        return QStringLiteral("pagination_invalid");
    case Integration::ApiErrorCode::InternalError:
        return QStringLiteral("internal_error");
    case Integration::ApiErrorCode::DatabaseUnavailable:
        return QStringLiteral("database_unavailable");
    case Integration::ApiErrorCode::NetworkError:
        return QStringLiteral("network_error");
    case Integration::ApiErrorCode::Timeout:
        return QStringLiteral("timeout");
    case Integration::ApiErrorCode::InvalidResponse:
        return QStringLiteral("invalid_response");
    case Integration::ApiErrorCode::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown"); // 兜底：防枚举外值
}

// 分页全量拉取：pageSize=100 逐页追加，直到某页为空或已收满 meta.total；
// 计数口径以本地收到的行数为准（聚合只消费 out 行），total 仅作终止条件。
template <typename Entity, typename Filter>
bool fetchAllPages(Integration::DataPlatformApi *api,
                   bool (Integration::DataPlatformApi::*listMethod)(const Filter &,
                                                                    const Integration::PageQuery &,
                                                                    Integration::PagedResult<Entity> *,
                                                                    Integration::ApiError *),
                   QVector<Entity> *out, Integration::ApiError *error)
{
    Integration::PageQuery pageQuery;
    pageQuery.pageSize = 100; // 契约 §7 page_size 上限
    int fetched = 0;
    while (true) {
        Integration::PagedResult<Entity> result;
        if (!(api->*listMethod)(Filter(), pageQuery, &result, error))
            return false;
        out->append(result.items);
        fetched += result.items.size();
        if (result.items.isEmpty() || fetched >= result.total)
            break;
        ++pageQuery.page;
    }
    return true;
}

} // namespace

// ---- StatisticsController（UI 线程侧）----

StatisticsController::StatisticsController(QObject *parent)
    : QObject(parent)
{
    // 跨线程队列连接传值类型：先注册元类型（头文件底部已 Q_DECLARE_METATYPE）。
    qRegisterMetaType<StatisticsSnapshot>();
    qRegisterMetaType<StatisticsListPage>();
    qRegisterMetaType<StatisticsController::ListRequest>();

    // worker 无父对象、归属 m_workerThread（deleteLater 经线程 finished 回收）。
    m_worker = new StatisticsWorker;
    m_worker->moveToThread(&m_workerThread);

    // 请求下发（值语义跨线程，队列连接）
    connect(this, &StatisticsController::snapshotRequested, m_worker,
            &StatisticsWorker::runSnapshotTask, Qt::QueuedConnection);
    connect(this, &StatisticsController::listRequested, m_worker,
            &StatisticsWorker::runListTask, Qt::QueuedConnection);
    // 快照结果回 controller：revision 守卫在槽内执行（陈旧结果不向 UI 转发）
    connect(m_worker, &StatisticsWorker::snapshotFinished, this,
            &StatisticsController::onSnapshotFinished, Qt::QueuedConnection);
    connect(m_worker, &StatisticsWorker::snapshotFailed, this,
            &StatisticsController::onSnapshotFailed, Qt::QueuedConnection);
    // 列表结果无 revision：worker 信号接 controller 信号直接转发 UI（仍为队列连接）
    connect(m_worker, &StatisticsWorker::listFinished, this,
            &StatisticsController::listReady, Qt::QueuedConnection);
    connect(m_worker, &StatisticsWorker::listFailed, this,
            &StatisticsController::listFetchFailed, Qt::QueuedConnection);
    // 线程收尾：事件循环退出后回收 worker
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}

StatisticsController::~StatisticsController()
{
    // 先 quit 再 wait：worker 内全部 API 调用为短超时同步调用，wait 上界可预期
    //（快照与列表请求共用 worker 事件循环 FIFO 串行，无并发阻塞源）。
    m_workerThread.quit();
    m_workerThread.wait();
}

void StatisticsController::requestSnapshot()
{
    ++m_revision;
    emit snapshotRequested(m_revision);
}

void StatisticsController::requestList(const StatisticsController::ListRequest &request)
{
    emit listRequested(request);
}

void StatisticsController::onSnapshotFinished(int revision, const StatisticsSnapshot &snapshot)
{
    if (revision != m_revision)
        return; // 陈旧快照：期间已发起更新的请求，丢弃不转发
    emit snapshotReady(snapshot);
}

void StatisticsController::onSnapshotFailed(int revision, const QString &message)
{
    if (revision != m_revision)
        return;
    emit snapshotFetchFailed(message);
}

// ---- StatisticsWorker（worker 线程侧）----

void StatisticsWorker::runSnapshotTask(int revision)
{
    // 懒建适配对象：本线程创建、本线程使用（QNAM 线程亲和纪律，§6）。
    if (!m_api) {
        m_api = new Integration::DataPlatformApi;
        m_vocabularyCache = new Integration::VocabularyCache;
    }

    // 1) 健康检查：传输/形状失败或未就绪（status!=ok 或 database!=connected）即整体失败。
    Integration::HealthStatus health;
    Integration::ApiError error;
    if (!m_api->health(&health, &error)) {
        emit snapshotFailed(revision, formatApiError(QStringLiteral("health"), error));
        return;
    }
    if (health.status != QStringLiteral("ok") || health.database != QStringLiteral("connected")) {
        // 健康形状但未就绪（典型 503）：合成 DatabaseUnavailable 单条错误，走统一报文口径。
        Integration::ApiErrorItem item;
        item.code = Integration::ApiErrorCode::DatabaseUnavailable;
        item.message =
            QStringLiteral("status=%1, database=%2").arg(health.status, health.database);
        error.httpStatus = health.httpStatus;
        error.items.clear();
        error.items.append(item);
        emit snapshotFailed(revision, formatApiError(QStringLiteral("health"), error));
        return;
    }

    // 2) 词表加载：DataPlatformApi 的 HttpClient 为私有成员且无访问器，
    //    词表缓存用独立 HttpClient（本线程栈上临时对象）。
    //    load 内部先行 clear，每次快照重载（幂等），保证标签随正典演进。
    Integration::HttpClient vocabularyClient;
    if (!m_vocabularyCache->load(&vocabularyClient, &error)) {
        emit snapshotFailed(revision, formatApiError(QStringLiteral("vocabularies"), error));
        return;
    }

    // 3) 五实体全量分页拉取（顺序：目标 → 设备 → 任务 → 告警 → 检测记录）；
    //    任一步失败即整体失败，报文携带首个失败资源。
    StatisticsFullEntityDump dump;
    if (!fetchAllPages(m_api, &Integration::DataPlatformApi::listTargets, &dump.targets, &error)) {
        emit snapshotFailed(revision, formatApiError(QStringLiteral("targets"), error));
        return;
    }
    if (!fetchAllPages(m_api, &Integration::DataPlatformApi::listDevices, &dump.devices, &error)) {
        emit snapshotFailed(revision, formatApiError(QStringLiteral("devices"), error));
        return;
    }
    if (!fetchAllPages(m_api, &Integration::DataPlatformApi::listMissions, &dump.missions, &error)) {
        emit snapshotFailed(revision, formatApiError(QStringLiteral("missions"), error));
        return;
    }
    if (!fetchAllPages(m_api, &Integration::DataPlatformApi::listAlarms, &dump.alarms, &error)) {
        emit snapshotFailed(revision, formatApiError(QStringLiteral("alarms"), error));
        return;
    }
    if (!fetchAllPages(m_api, &Integration::DataPlatformApi::listDetectionRecords,
                       &dump.detectionRecords, &error)) {
        emit snapshotFailed(revision, formatApiError(QStringLiteral("detection-records"), error));
        return;
    }

    // 4) 纯聚合 + 补全快照元数据（拉取完成时刻、词表副本），整体值拷贝回投 UI 线程。
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    StatisticsSnapshot snapshot =
        aggregateStatistics(dump, m_vocabularyCache->vocabularies(), nowUtc);
    snapshot.fetchedAtUtc = nowUtc;
    snapshot.vocabularies = m_vocabularyCache->vocabularies();
    emit snapshotFinished(revision, snapshot);
}

void StatisticsWorker::runListTask(const StatisticsController::ListRequest &request)
{
    // 懒建适配对象（与快照任务同纪律；本任务只用 m_api，缓存一并建齐保持两入口一致）。
    if (!m_api) {
        m_api = new Integration::DataPlatformApi;
        m_vocabularyCache = new Integration::VocabularyCache;
    }

    StatisticsListPage page;
    page.resource = request.resource;
    page.page = request.page.page;

    Integration::ApiError error;
    if (request.resource == QStringLiteral("targets")) {
        Integration::PagedResult<Core::TargetInfo> result;
        if (!m_api->listTargets(request.targetFilter, request.page, &result, &error)) {
            emit listFailed(request.resource, formatApiError(request.resource, error));
            return;
        }
        page.total = result.total;
        page.targets = result.items;
    } else if (request.resource == QStringLiteral("devices")) {
        Integration::PagedResult<Core::DeviceInfo> result;
        if (!m_api->listDevices(request.deviceFilter, request.page, &result, &error)) {
            emit listFailed(request.resource, formatApiError(request.resource, error));
            return;
        }
        page.total = result.total;
        page.devices = result.items;
    } else if (request.resource == QStringLiteral("missions")) {
        Integration::PagedResult<Core::MissionInfo> result;
        if (!m_api->listMissions(request.missionFilter, request.page, &result, &error)) {
            emit listFailed(request.resource, formatApiError(request.resource, error));
            return;
        }
        page.total = result.total;
        page.missions = result.items;
    } else if (request.resource == QStringLiteral("alarms")) {
        Integration::PagedResult<Core::AlarmInfo> result;
        if (!m_api->listAlarms(request.alarmFilter, request.page, &result, &error)) {
            emit listFailed(request.resource, formatApiError(request.resource, error));
            return;
        }
        page.total = result.total;
        page.alarms = result.items;
    } else if (request.resource == QStringLiteral("detection-records")) {
        Integration::PagedResult<Integration::DetectionRecordData> result;
        if (!m_api->listDetectionRecords(request.detectionRecordFilter, request.page, &result, &error)) {
            emit listFailed(request.resource, formatApiError(request.resource, error));
            return;
        }
        page.total = result.total;
        page.detectionRecords = result.items;
    } else {
        // 未知资源：不携带 HTTP 语义的直接错误（调用方参数防线）
        emit listFailed(request.resource, QStringLiteral("未知资源: %1").arg(request.resource));
        return;
    }

    emit listFinished(page);
}

QString StatisticsWorker::formatApiError(const QString &resource,
                                         const Integration::ApiError &error) const
{
    // 报文口径："<resource>: HTTP <status> [field/]code[ message]"，取首条错误项
    //（§8 判断以 field+code 为准；message 仅展示。400 聚合多条时取首条即可定位资源）。
    QString text = QStringLiteral("%1: HTTP %2").arg(resource).arg(error.httpStatus);
    if (!error.items.isEmpty()) {
        const Integration::ApiErrorItem &item = error.items.first();
        QString detail;
        if (!item.field.isEmpty())
            detail = item.field + QStringLiteral("/");
        detail += apiErrorCodeToString(item.code);
        if (!item.message.isEmpty())
            detail += QStringLiteral(" ") + item.message;
        text += QStringLiteral(" ") + detail;
    }
    return text;
}
