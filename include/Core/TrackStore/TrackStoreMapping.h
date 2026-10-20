#ifndef CORE_TRACKSTORE_TRACKSTOREMAPPING_H
#define CORE_TRACKSTORE_TRACKSTOREMAPPING_H

// MQTT 报文 → 数据库行 的纯函数映射（REQ-015 M4E 落库 WP6）
// 无副作用、不触库：OSD 轨迹帧与 events 事件帧各映射为一个 QVariantMap 行，
// 行键与 SQL 绑定占位符一一对应；无效帧返回 std::nullopt 由上层静默丢弃。

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

#include <optional>

namespace Core::TrackStore {

// 从主题提取无人机 SN：thing/product/<SN>/{osd|events}，不合规返回空串
QString droneSnFromTopic(const QString &topic);

// OSD 轨迹帧 → trajectory 行（20 个键）；无效帧返回 std::nullopt
std::optional<QVariantMap> mapOsdToRow(const QString &topic, const QByteArray &payload,
                                       const QDateTime &receivedUtc);

// events 事件帧 → events 行（7 个键）；无效帧返回 std::nullopt
std::optional<QVariantMap> mapEventToRow(const QString &topic, const QByteArray &payload,
                                         const QDateTime &receivedUtc);

} // namespace Core::TrackStore

#endif // CORE_TRACKSTORE_TRACKSTOREMAPPING_H
