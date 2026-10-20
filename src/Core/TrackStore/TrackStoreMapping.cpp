// MQTT 报文 → 数据库行 的纯函数映射实现（REQ-015 M4E 落库 WP6）
// 帧有效性规则与 MqttTelemetrySource 对齐：坐标缺失/非数值、MSDK 未定位
// 哨兵 (0,0)（|lat|<=1e-7 且 |lon|<=1e-7）整帧丢弃；数值提取只接受
// 整数值 JSON number 落整数列（非整数记 SQL NULL），double 列原样透传。

#include "Core/TrackStore/TrackStoreMapping.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <cmath>
#include <limits>

namespace Core::TrackStore {
namespace {

// 信封 timestamp（毫秒 epoch）→ UTC 时间；非整数毫秒或超 qint64 范围回退接收时刻
QDateTime tsFromEnvelope(const QJsonObject &envelope, const QDateTime &receivedUtc)
{
    const QJsonValue value = envelope.value(QStringLiteral("timestamp"));
    if (value.isDouble()) {
        const double raw = value.toDouble();
        if (std::floor(raw) == raw && raw >= static_cast<double>(std::numeric_limits<qint64>::min())
            && raw <= static_cast<double>(std::numeric_limits<qint64>::max())) {
            return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(raw), Qt::UTC);
        }
    }
    return receivedUtc.toUTC();
}

// 整数列取值：仅接受整数值 JSON number 且在 int 范围内，否则 SQL NULL
QVariant intValueOrNull(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble())
        return QVariant();
    const double raw = value.toDouble();
    if (std::floor(raw) != raw || raw < static_cast<double>(std::numeric_limits<int>::min())
        || raw > static_cast<double>(std::numeric_limits<int>::max())) {
        return QVariant();
    }
    return QVariant(static_cast<int>(raw));
}

// double 列取值：接受任意 JSON number，否则 SQL NULL
QVariant doubleValueOrNull(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? QVariant(value.toDouble()) : QVariant();
}

} // namespace

QString droneSnFromTopic(const QString &topic)
{
    const QStringList segments = topic.split(QLatin1Char('/'));
    if (segments.size() != 4)
        return QString();
    if (segments.at(0) != QStringLiteral("thing") || segments.at(1) != QStringLiteral("product"))
        return QString();
    if (segments.at(2).isEmpty())
        return QString();
    if (segments.at(3) != QStringLiteral("osd") && segments.at(3) != QStringLiteral("events"))
        return QString();
    return segments.at(2);
}

std::optional<QVariantMap> mapOsdToRow(const QString &topic, const QByteArray &payload,
                                       const QDateTime &receivedUtc)
{
    const QString droneSn = droneSnFromTopic(topic);
    if (droneSn.isEmpty())
        return std::nullopt;

    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (!document.isObject())
        return std::nullopt;
    const QJsonObject envelope = document.object();
    const QJsonObject data = envelope.value(QStringLiteral("data")).toObject();

    // 帧有效性：与 MqttTelemetrySource 相同的定位判据
    const QJsonValue latValue = data.value(QStringLiteral("latitude"));
    const QJsonValue lonValue = data.value(QStringLiteral("longitude"));
    if (!latValue.isDouble() || !lonValue.isDouble())
        return std::nullopt;
    const double lat = latValue.toDouble();
    const double lon = lonValue.toDouble();
    if (std::abs(lat) <= 1e-7 && std::abs(lon) <= 1e-7)
        return std::nullopt;

    const QJsonObject battery = data.value(QStringLiteral("battery")).toObject();
    const QJsonObject positionState = data.value(QStringLiteral("position_state")).toObject();
    const QJsonValue gatewayValue = envelope.value(QStringLiteral("gateway"));
    const QJsonValue elevationValue = data.value(QStringLiteral("elevation"));

    QVariantMap row;
    row.insert(QStringLiteral("task_id"), QVariant());  // 一期无任务域，恒 SQL NULL
    row.insert(QStringLiteral("drone_sn"), droneSn);
    row.insert(QStringLiteral("gateway_sn"),
               gatewayValue.isString() ? gatewayValue.toString() : QVariant());
    row.insert(QStringLiteral("ts_utc"), tsFromEnvelope(envelope, receivedUtc));
    row.insert(QStringLiteral("lat"), QVariant(lat));
    row.insert(QStringLiteral("lon"), QVariant(lon));
    row.insert(QStringLiteral("rel_alt"), data.value(QStringLiteral("height")).isDouble()
                                                ? QVariant(data.value(QStringLiteral("height")).toDouble())
                                                : QVariant());
    row.insert(QStringLiteral("elevation"),
               elevationValue.isDouble() ? QVariant(elevationValue.toDouble()) : QVariant());
    row.insert(QStringLiteral("h_speed"), doubleValueOrNull(data, QStringLiteral("horizontal_speed")));
    row.insert(QStringLiteral("v_speed"), doubleValueOrNull(data, QStringLiteral("vertical_speed")));
    row.insert(QStringLiteral("attitude_head"), doubleValueOrNull(data, QStringLiteral("attitude_head")));
    row.insert(QStringLiteral("attitude_pitch"), doubleValueOrNull(data, QStringLiteral("attitude_pitch")));
    row.insert(QStringLiteral("attitude_roll"), doubleValueOrNull(data, QStringLiteral("attitude_roll")));
    row.insert(QStringLiteral("mode_code"), intValueOrNull(data, QStringLiteral("mode_code")));
    row.insert(QStringLiteral("battery_pct"), intValueOrNull(battery, QStringLiteral("capacity_percent")));
    row.insert(QStringLiteral("remain_flight_time"),
               intValueOrNull(battery, QStringLiteral("remain_flight_time")));
    row.insert(QStringLiteral("rtk_fixed"), intValueOrNull(positionState, QStringLiteral("is_fixed")));
    row.insert(QStringLiteral("wind_speed"), doubleValueOrNull(data, QStringLiteral("wind_speed")));
    row.insert(QStringLiteral("home_distance"), doubleValueOrNull(data, QStringLiteral("home_distance")));
    row.insert(QStringLiteral("raw"), QString::fromUtf8(payload));
    return row;
}

std::optional<QVariantMap> mapEventToRow(const QString &topic, const QByteArray &payload,
                                         const QDateTime &receivedUtc)
{
    const QString droneSn = droneSnFromTopic(topic);
    if (droneSn.isEmpty())
        return std::nullopt;

    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (!document.isObject())
        return std::nullopt;
    const QJsonObject envelope = document.object();

    const QString method = envelope.value(QStringLiteral("method")).toString();
    if (method.isEmpty())
        return std::nullopt;

    // 级别取 data.list[] 中整数 level 的最大值（字符串/非整数不计入；无有效值记 SQL NULL）
    const QJsonObject dataObject = envelope.value(QStringLiteral("data")).toObject();
    const QJsonArray list = dataObject.value(QStringLiteral("list")).toArray();
    QVariant level;
    for (const QJsonValue &item : list) {
        const QJsonValue itemLevel = item.toObject().value(QStringLiteral("level"));
        if (!itemLevel.isDouble())
            continue;
        const double raw = itemLevel.toDouble();
        if (std::floor(raw) != raw || raw < static_cast<double>(std::numeric_limits<int>::min())
            || raw > static_cast<double>(std::numeric_limits<int>::max())) {
            continue;
        }
        const int value = static_cast<int>(raw);
        if (!level.isValid() || value > level.toInt())
            level = QVariant(value);
    }

    QVariantMap row;
    row.insert(QStringLiteral("task_id"), QVariant());  // 一期无任务域，恒 SQL NULL
    row.insert(QStringLiteral("drone_sn"), droneSn);
    row.insert(QStringLiteral("ts_utc"), tsFromEnvelope(envelope, receivedUtc));
    row.insert(QStringLiteral("method"), method);
    row.insert(QStringLiteral("level"), level);
    row.insert(QStringLiteral("data"),
               QString::fromUtf8(QJsonDocument(dataObject).toJson(QJsonDocument::Compact)));
    row.insert(QStringLiteral("raw"), QString::fromUtf8(payload));
    return row;
}

} // namespace Core::TrackStore
