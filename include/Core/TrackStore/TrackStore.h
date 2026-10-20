#ifndef CORE_TRACKSTORE_TRACKSTORE_H
#define CORE_TRACKSTORE_TRACKSTORE_H

// 轨迹/事件落库门面（REQ-015 M4E 落库 WP6）
// 前台 QObject 门面：接收原始 MQTT 报文并转投后台线程的 Worker（唯一触库点）。
// 本头文件不包含任何 <QSql*> 头，UI/调用方只需 QtCore。

#include <QByteArray>
#include <QObject>
#include <QString>

#include <atomic>

#include "Core/TrackStore/TrackStoreConfig.h"

class QThread;

namespace Core::TrackStore {

class TrackStore : public QObject
{
    Q_OBJECT

public:
    // 总开关（委托 TrackStoreConfig.h 的同名自由函数）
    static bool isEnabledByEnv();

    // 默认配置取自环境变量
    explicit TrackStore(QObject *parent = nullptr);
    explicit TrackStore(const TrackStoreConfig &config, QObject *parent = nullptr);
    ~TrackStore() override;

    // 后台线程生命周期（幂等）
    void start();
    void stop();

    // Worker 是否已成功打开连接
    bool isActive() const;

signals:
    // 打开状态翻转（成功/失败/停止）
    void openedChanged(bool active);
    // 每成功写入一行的累计计数（按表）
    void rowWritten(const QString &table, qint64 totalRows);
    // 写入/打开错误（经节流）
    void storeError(const QString &message);

public slots:
    // 原始 MQTT 报文入口：按主题后缀分发 /osd → 轨迹、/events → 事件，其余忽略
    void onRawMessage(const QString &topic, const QByteArray &payload);

private slots:
    // Worker open 结果回收
    void onWorkerOpened(bool ok, const QString &error);

private:
    // 后台线程内的数据库 Worker（嵌套类，定义在 .cpp；唯一触库点）
    class Worker;
    Worker *m_worker = nullptr;
    QThread *m_thread = nullptr;
    TrackStoreConfig m_config;
    std::atomic_bool m_active{false};
};

} // namespace Core::TrackStore

#endif // CORE_TRACKSTORE_TRACKSTORE_H
