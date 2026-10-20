// 轨迹/事件落库实现（REQ-015 M4E 落库 WP6）
// TrackStore 为前台门面；嵌套 Worker 经 moveToThread 运行在专属线程，
// 是全文件唯一触库点（QSqlDatabase/QSqlQuery 只出现在 Worker 内）。
// 文件末尾 #include "TrackStore.moc" 提供 Worker 的 Q_OBJECT 元对象。

#include "Core/TrackStore/TrackStore.h"

#include <QDateTime>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

#include <atomic>

#include "Core/TrackStore/TrackStoreMapping.h"

namespace Core::TrackStore {
namespace {

// 连接名计数器：同一进程内多次 start/stop 也绝不重用连接名（QSqlDatabase 全局注册表要求）
std::atomic<int> g_connectionCounter{0};

// 轨迹写入 SQL（无 PostGIS：纯属性列）
const QString kTrajectoryNoGeomSql = QStringLiteral(
    "INSERT INTO trajectory (task_id, drone_sn, gateway_sn, ts_utc, lat, lon, rel_alt, "
    "elevation, h_speed, v_speed, attitude_head, attitude_pitch, attitude_roll, mode_code, "
    "battery_pct, remain_flight_time, rtk_fixed, wind_speed, home_distance, raw) VALUES "
    "(:task_id, :drone_sn, :gateway_sn, :ts_utc, :lat, :lon, :rel_alt, :elevation, :h_speed, "
    ":v_speed, :attitude_head, :attitude_pitch, :attitude_roll, :mode_code, :battery_pct, "
    ":remain_flight_time, :rtk_fixed, :wind_speed, :home_distance, :raw)");

// 轨迹写入 SQL（PostGIS 可用：额外写 geom 点列，SRID 4326）
const QString kTrajectoryGeomSql = QStringLiteral(
    "INSERT INTO trajectory (task_id, drone_sn, gateway_sn, ts_utc, lat, lon, rel_alt, "
    "elevation, h_speed, v_speed, attitude_head, attitude_pitch, attitude_roll, mode_code, "
    "battery_pct, remain_flight_time, rtk_fixed, wind_speed, home_distance, raw, geom) VALUES "
    "(:task_id, :drone_sn, :gateway_sn, :ts_utc, :lat, :lon, :rel_alt, :elevation, :h_speed, "
    ":v_speed, :attitude_head, :attitude_pitch, :attitude_roll, :mode_code, :battery_pct, "
    ":remain_flight_time, :rtk_fixed, :wind_speed, :home_distance, :raw, "
    "ST_SetSRID(ST_MakePoint(:g_lon, :g_lat, :g_rel), 4326))");

// 事件写入 SQL
const QString kEventsSql = QStringLiteral(
    "INSERT INTO events (task_id, drone_sn, ts_utc, method, level, data, raw) VALUES "
    "(:task_id, :drone_sn, :ts_utc, :method, :level, :data, :raw)");

// 写失败告警节流窗口（毫秒）：同一表 5 秒内至多一次 qWarning + writeError
const qint64 kWarnThrottleMsecs = 5000;

} // namespace

// ---------- Worker（专属线程内，唯一触库点） ----------
// 注意：必须完整定义在门面方法之前（start()/onRawMessage() 需要完整类型做
// new/connect/invokeMethod），故本节置于文件前部。

class TrackStore::Worker : public QObject
{
    Q_OBJECT

public:
    explicit Worker(const TrackStoreConfig &config, QObject *parent = nullptr);
    ~Worker() override;

signals:
    // open 结果：ok=false 时 error 携带数据库错误文本
    void opened(bool ok, const QString &error);
    // 成功写入一行的累计计数（按表）
    void rowWritten(const QString &table, qint64 totalRows);
    // 写入/打开错误（按表节流）
    void writeError(const QString &message);

public slots:
    // 建立连接、探测 PostGIS、预编译两条 INSERT（线程 started 触发）
    void open();
    // 轨迹帧写入（门面经队列连接转投）
    void onTrajectoryMessage(const QString &topic, const QByteArray &payload,
                             const QDateTime &receivedUtc);
    // 事件帧写入（门面经队列连接转投）
    void onEventMessage(const QString &topic, const QByteArray &payload,
                        const QDateTime &receivedUtc);

private:
    // 绑定全部行键并执行；geom 模式额外绑定 :g_lon/:g_lat/:g_rel
    void writeRow(const QString &table, QSqlQuery &query, qint64 &counter,
                  const QVariantMap &row, bool bindGeom);

    TrackStoreConfig m_config;
    QString m_connectionName;
    QSqlDatabase m_db;
    QSqlQuery m_trajectoryInsert;
    QSqlQuery m_eventInsert;
    bool m_hasPostgis = false;               // PostGIS 探测结果（false=降级无 geom 模式）
    qint64 m_trajectoryRows = 0;             // 轨迹累计行数
    qint64 m_eventRows = 0;                  // 事件累计行数
    QHash<QString, qint64> m_lastWarnMsecs;  // 节流：表名 → 上次告警 epoch 毫秒
};

TrackStore::Worker::Worker(const TrackStoreConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_connectionName(QStringLiteral("uxo_trackstore_%1").arg(g_connectionCounter.fetch_add(1)))
{
}

TrackStore::Worker::~Worker()
{
    // 成员 QSqlQuery 持有连接引用，必须先重置为空查询再移除连接，
    // 否则 QSqlDatabase::removeDatabase 会因"仍有查询引用"告警并跳过移除
    m_trajectoryInsert = QSqlQuery();
    m_eventInsert = QSqlQuery();
    if (m_db.isValid()) {
        const QString name = m_db.connectionName();
        if (m_db.isOpen())
            m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }
}

void TrackStore::Worker::open()
{
    if (m_db.isValid())  // 幂等：已初始化（线程生命周期内只 open 一次）
        return;

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connectionName);
    m_db.setHostName(m_config.host);
    m_db.setPort(m_config.port);
    m_db.setDatabaseName(m_config.database);
    m_db.setUserName(m_config.user);
    m_db.setPassword(m_config.password);
    if (!m_db.open()) {
        emit opened(false, m_db.lastError().text());
        return;
    }

    // PostGIS 探测：不可用时降级为无 geom 列写入（只告警一次）
    QSqlQuery probe(m_db);
    m_hasPostgis = probe.exec(QStringLiteral("SELECT PostGIS_Version()")) && probe.next();
    if (!m_hasPostgis)
        qWarning("TrackStore: PostGIS 不可用，降级为无 geom 列写入模式");

    m_trajectoryInsert = QSqlQuery(m_db);
    m_eventInsert = QSqlQuery(m_db);
    if (!m_trajectoryInsert.prepare(m_hasPostgis ? kTrajectoryGeomSql : kTrajectoryNoGeomSql)) {
        const QString error = m_trajectoryInsert.lastError().text();
        m_db.close();
        emit opened(false, error);
        return;
    }
    if (!m_eventInsert.prepare(kEventsSql)) {
        const QString error = m_eventInsert.lastError().text();
        m_db.close();
        emit opened(false, error);
        return;
    }

    m_trajectoryRows = 0;
    m_eventRows = 0;
    emit opened(true, QString());
}

void TrackStore::Worker::onTrajectoryMessage(const QString &topic, const QByteArray &payload,
                                             const QDateTime &receivedUtc)
{
    if (!m_db.isOpen())
        return;
    const std::optional<QVariantMap> row = mapOsdToRow(topic, payload, receivedUtc);
    if (!row)
        return;  // 无效帧（主题不合规/非 JSON/未定位）静默丢弃
    writeRow(QStringLiteral("trajectory"), m_trajectoryInsert, m_trajectoryRows, *row, true);
}

void TrackStore::Worker::onEventMessage(const QString &topic, const QByteArray &payload,
                                        const QDateTime &receivedUtc)
{
    if (!m_db.isOpen())
        return;
    const std::optional<QVariantMap> row = mapEventToRow(topic, payload, receivedUtc);
    if (!row)
        return;  // 无效帧（主题不合规/非 JSON/无 method）静默丢弃
    writeRow(QStringLiteral("events"), m_eventInsert, m_eventRows, *row, false);
}

void TrackStore::Worker::writeRow(const QString &table, QSqlQuery &query, qint64 &counter,
                                  const QVariantMap &row, bool bindGeom)
{
    // 逐键绑定：行键与占位符一一对应（无效 QVariant 即 SQL NULL）
    for (auto it = row.constBegin(); it != row.constEnd(); ++it)
        query.bindValue(QStringLiteral(":") + it.key(), it.value());
    if (bindGeom && m_hasPostgis) {
        query.bindValue(QStringLiteral(":g_lon"), row.value(QStringLiteral("lon")));
        query.bindValue(QStringLiteral(":g_lat"), row.value(QStringLiteral("lat")));
        query.bindValue(QStringLiteral(":g_rel"), row.value(QStringLiteral("rel_alt")));
    }

    if (!query.exec()) {
        // 节流上报：同一表 5 秒内至多一次 qWarning + writeError，避免刷屏
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - m_lastWarnMsecs.value(table, 0) >= kWarnThrottleMsecs) {
            m_lastWarnMsecs.insert(table, now);
            qWarning("TrackStore 写入 %s 失败: %s",
                     qPrintable(table), qPrintable(query.lastError().text()));
            emit writeError(query.lastError().text());
        }
        return;
    }

    ++counter;
    emit rowWritten(table, counter);
}

// ---------- 门面（前台线程） ----------

bool TrackStore::isEnabledByEnv()
{
    // 全限定名调用 TrackStoreConfig.h 的自由函数，避免递归解析到本静态成员
    return Core::TrackStore::isEnabledByEnv();
}

TrackStore::TrackStore(QObject *parent)
    : TrackStore(fromEnv(), parent)
{
}

TrackStore::TrackStore(const TrackStoreConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
}

TrackStore::~TrackStore()
{
    // 与 stop() 相同的拆除序列，但析构期绝不发信号
    if (!m_thread)
        return;
    m_thread->quit();
    if (!m_thread->wait(3000))
        qWarning("TrackStore 工作线程 3 秒内未退出（析构拆除）");
    m_thread = nullptr;
    m_worker = nullptr;
    m_active.store(false);
}

void TrackStore::start()
{
    if (m_thread)  // 幂等：已在运行
        return;

    QThread *thread = new QThread();          // 无父对象，finished 时 deleteLater
    Worker *worker = new Worker(m_config);    // 无父对象，随后移入工作线程
    worker->moveToThread(thread);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(worker, &Worker::opened, this, &TrackStore::onWorkerOpened);
    connect(worker, &Worker::rowWritten, this, &TrackStore::rowWritten);
    connect(worker, &Worker::writeError, this, &TrackStore::storeError);
    connect(thread, &QThread::started, worker, &Worker::open);

    m_thread = thread;
    m_worker = worker;
    thread->start();
}

void TrackStore::stop()
{
    if (!m_thread)  // 幂等：未在运行
        return;
    m_thread->quit();
    if (!m_thread->wait(3000))
        qWarning("TrackStore 工作线程 3 秒内未退出");
    m_thread = nullptr;
    m_worker = nullptr;
    if (m_active.exchange(false))
        emit openedChanged(false);
}

bool TrackStore::isActive() const
{
    return m_active.load();
}

void TrackStore::onRawMessage(const QString &topic, const QByteArray &payload)
{
    if (!m_active.load())
        return;
    // 接收时刻只打一次戳，供 ts 回退使用
    const QDateTime receivedUtc = QDateTime::currentDateTimeUtc();
    if (topic.endsWith(QStringLiteral("/osd"))) {
        // m_worker 为空时 invokeMethod 安全返回 false（stop 竞态窗口）
        QMetaObject::invokeMethod(m_worker, "onTrajectoryMessage", Qt::QueuedConnection,
                                  Q_ARG(QString, topic), Q_ARG(QByteArray, payload),
                                  Q_ARG(QDateTime, receivedUtc));
    } else if (topic.endsWith(QStringLiteral("/events"))) {
        QMetaObject::invokeMethod(m_worker, "onEventMessage", Qt::QueuedConnection,
                                  Q_ARG(QString, topic), Q_ARG(QByteArray, payload),
                                  Q_ARG(QDateTime, receivedUtc));
    }
    // 其他主题后缀：忽略（与数据源订阅契约一致）
}

void TrackStore::onWorkerOpened(bool ok, const QString &error)
{
    m_active.store(ok);
    emit openedChanged(ok);
    if (!ok)
        emit storeError(error);
}

} // namespace Core::TrackStore

#include "TrackStore.moc"
