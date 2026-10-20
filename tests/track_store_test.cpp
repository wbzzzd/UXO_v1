// TrackStore 映射层单测（REQ-015 M4E 落库 WP6）
// 覆盖：主题 SN 提取合法/非法用例、OSD 轨迹行 20 键全字段提取与缺省键 SQL NULL、
// ts_utc 信封时间戳与回退、无效帧丢弃（非 JSON/缺坐标/(0,0) 未定位哨兵）、
// events 行映射（method/level/data/raw）、混合级别取最大整数、启用开关默认关闭。
// 用 QTEST_GUILESS_MAIN：纯 QCoreApplication，无 QPA 平台插件依赖（与 WP3 单测一致）。

#include <QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Core/TrackStore/TrackStoreConfig.h"
#include "Core/TrackStore/TrackStoreMapping.h"

namespace {

// 信封 → 紧凑 JSON 字节流（与真实 MQTT payload 形态一致）
QByteArray toPayload(const QJsonObject &envelope)
{
    return QJsonDocument(envelope).toJson(QJsonDocument::Compact);
}

// 构造含全部映射键的 OSD 信封（data + battery/position_state 嵌套 + 信封元数据）
QByteArray fullOsdPayload()
{
    QJsonObject data;
    data.insert(QStringLiteral("latitude"), 39.9);
    data.insert(QStringLiteral("longitude"), 116.4);
    data.insert(QStringLiteral("height"), 120.5);
    data.insert(QStringLiteral("elevation"), 55.5);
    data.insert(QStringLiteral("horizontal_speed"), 12.5);
    data.insert(QStringLiteral("vertical_speed"), -1.5);
    data.insert(QStringLiteral("attitude_head"), 88.0);
    data.insert(QStringLiteral("attitude_pitch"), -2.5);
    data.insert(QStringLiteral("attitude_roll"), 3.5);
    data.insert(QStringLiteral("mode_code"), 2);
    QJsonObject battery;
    battery.insert(QStringLiteral("capacity_percent"), 87);
    battery.insert(QStringLiteral("remain_flight_time"), 1500);
    data.insert(QStringLiteral("battery"), battery);
    QJsonObject positionState;
    positionState.insert(QStringLiteral("is_fixed"), 1);
    data.insert(QStringLiteral("position_state"), positionState);
    data.insert(QStringLiteral("wind_speed"), 4.2);
    data.insert(QStringLiteral("home_distance"), 640.0);

    QJsonObject envelope;
    envelope.insert(QStringLiteral("tid"), QStringLiteral("tid-value"));
    envelope.insert(QStringLiteral("bid"), QStringLiteral("bid-value"));
    envelope.insert(QStringLiteral("timestamp"), 1760000000123.0);
    envelope.insert(QStringLiteral("gateway"), QStringLiteral("GW-SN-001"));
    envelope.insert(QStringLiteral("method"), QStringLiteral("update"));
    envelope.insert(QStringLiteral("data"), data);
    return toPayload(envelope);
}

} // namespace

class TrackStoreTest : public QObject
{
    Q_OBJECT

private slots:
    // 环境隔离：总开关默认关闭
    void initTestCase();

    // 主题 SN 提取：合法 osd/events 与五种非法形态
    void droneSnFromTopicCases();

    // OSD 轨迹行：全字段提取，逐键断言 20 个键
    void mapOsdToRowFullField();

    // OSD 轨迹行：可选键缺失 → SQL NULL（无效 QVariant）
    void mapOsdToRowMissingOptionalKeys();

    // OSD 轨迹行：信封无 timestamp 时 ts_utc 回退接收时刻
    void mapOsdToRowTimestampFallback();

    // OSD 轨迹行：无效帧丢弃
    void mapOsdToRowInvalidFrames();

    // events 行：method/level/data/raw 四键断言
    void mapEventToRowHms();

    // events 行：混合级别 ["0",1,2.5,2] 只计整数取最大 → 2
    void mapEventToRowMixedLevels();

    // events 行：device_exit_hms（data 为空对象）level 记 NULL、data 为 "{}"
    void mapEventToRowDeviceExitHms();

    // events 行：非 JSON payload 丢弃
    void mapEventToRowInvalidFrames();

    // 开关：UXO_TRACKSTORE 未设置时 isEnabledByEnv() 为 false
    void isEnabledByEnvUnset();
};

void TrackStoreTest::initTestCase()
{
    // 隔离宿主环境：确保总开关默认关闭用例不受外部影响
    qunsetenv("UXO_TRACKSTORE");
}

void TrackStoreTest::droneSnFromTopicCases()
{
    using Core::TrackStore::droneSnFromTopic;
    // 合法：osd 与 events 两种后缀
    QCOMPARE(droneSnFromTopic(QStringLiteral("thing/product/7KABC2D00A00BC/osd")),
             QStringLiteral("7KABC2D00A00BC"));
    QCOMPARE(droneSnFromTopic(QStringLiteral("thing/product/7KABC2D00A00BC/events")),
             QStringLiteral("7KABC2D00A00BC"));
    // 非法：前缀错误
    QVERIFY(droneSnFromTopic(QStringLiteral("sys/product/7KABC2D00A00BC/osd")).isEmpty());
    // 非法：3 段
    QVERIFY(droneSnFromTopic(QStringLiteral("thing/product/7KABC2D00A00BC")).isEmpty());
    // 非法：5 段
    QVERIFY(droneSnFromTopic(QStringLiteral("thing/product/a/7KABC2D00A00BC/osd")).isEmpty());
    // 非法：SN 为空
    QVERIFY(droneSnFromTopic(QStringLiteral("thing/product//osd")).isEmpty());
    // 非法：非 osd/events 后缀（网关 state 拓扑）
    QVERIFY(droneSnFromTopic(QStringLiteral("thing/product/GW001/state")).isEmpty());
}

void TrackStoreTest::mapOsdToRowFullField()
{
    const QByteArray payload = fullOsdPayload();
    const auto row = Core::TrackStore::mapOsdToRow(
        QStringLiteral("thing/product/7KABC2D00A00BC/osd"), payload, QDateTime());
    QVERIFY(row.has_value());
    QCOMPARE(row->size(), 20);

    // 恒空列与标识列
    QVERIFY(!row->value(QStringLiteral("task_id")).isValid());
    QCOMPARE(row->value(QStringLiteral("drone_sn")).toString(), QStringLiteral("7KABC2D00A00BC"));
    QCOMPARE(row->value(QStringLiteral("gateway_sn")).toString(), QStringLiteral("GW-SN-001"));

    // ts_utc 取信封 timestamp（毫秒 epoch，整数表示）
    QCOMPARE(row->value(QStringLiteral("ts_utc")).toDateTime(),
             QDateTime::fromMSecsSinceEpoch(1760000000123, Qt::UTC));

    // 位置与高度
    QCOMPARE(row->value(QStringLiteral("lat")).toDouble(), 39.9);
    QCOMPARE(row->value(QStringLiteral("lon")).toDouble(), 116.4);
    QCOMPARE(row->value(QStringLiteral("rel_alt")).toDouble(), 120.5);
    QCOMPARE(row->value(QStringLiteral("elevation")).toDouble(), 55.5);

    // 速度与姿态
    QCOMPARE(row->value(QStringLiteral("h_speed")).toDouble(), 12.5);
    QCOMPARE(row->value(QStringLiteral("v_speed")).toDouble(), -1.5);
    QCOMPARE(row->value(QStringLiteral("attitude_head")).toDouble(), 88.0);
    QCOMPARE(row->value(QStringLiteral("attitude_pitch")).toDouble(), -2.5);
    QCOMPARE(row->value(QStringLiteral("attitude_roll")).toDouble(), 3.5);

    // 整数列（含嵌套 battery/position_state）
    QVERIFY(row->value(QStringLiteral("mode_code")).isValid());
    QCOMPARE(row->value(QStringLiteral("mode_code")).toInt(), 2);
    QVERIFY(row->value(QStringLiteral("battery_pct")).isValid());
    QCOMPARE(row->value(QStringLiteral("battery_pct")).toInt(), 87);
    QVERIFY(row->value(QStringLiteral("remain_flight_time")).isValid());
    QCOMPARE(row->value(QStringLiteral("remain_flight_time")).toInt(), 1500);
    QVERIFY(row->value(QStringLiteral("rtk_fixed")).isValid());
    QCOMPARE(row->value(QStringLiteral("rtk_fixed")).toInt(), 1);

    // 其余 double 列与原始报文
    QCOMPARE(row->value(QStringLiteral("wind_speed")).toDouble(), 4.2);
    QCOMPARE(row->value(QStringLiteral("home_distance")).toDouble(), 640.0);
    QCOMPARE(row->value(QStringLiteral("raw")).toString(), QString::fromUtf8(payload));
}

void TrackStoreTest::mapOsdToRowMissingOptionalKeys()
{
    // 最小合法帧：仅经纬度；其余可选键缺失一律记 SQL NULL
    QJsonObject data;
    data.insert(QStringLiteral("latitude"), 39.9);
    data.insert(QStringLiteral("longitude"), 116.4);
    QJsonObject envelope;
    envelope.insert(QStringLiteral("data"), data);
    const QByteArray payload = toPayload(envelope);

    const auto row = Core::TrackStore::mapOsdToRow(
        QStringLiteral("thing/product/7KABC2D00A00BC/osd"), payload, QDateTime());
    QVERIFY(row.has_value());
    QCOMPARE(row->size(), 20);

    QVERIFY(!row->value(QStringLiteral("task_id")).isValid());
    QVERIFY(!row->value(QStringLiteral("gateway_sn")).isValid());
    QVERIFY(!row->value(QStringLiteral("rel_alt")).isValid());
    QVERIFY(!row->value(QStringLiteral("elevation")).isValid());
    QVERIFY(!row->value(QStringLiteral("h_speed")).isValid());
    QVERIFY(!row->value(QStringLiteral("v_speed")).isValid());
    QVERIFY(!row->value(QStringLiteral("attitude_head")).isValid());
    QVERIFY(!row->value(QStringLiteral("attitude_pitch")).isValid());
    QVERIFY(!row->value(QStringLiteral("attitude_roll")).isValid());
    QVERIFY(!row->value(QStringLiteral("mode_code")).isValid());
    QVERIFY(!row->value(QStringLiteral("battery_pct")).isValid());
    QVERIFY(!row->value(QStringLiteral("remain_flight_time")).isValid());
    QVERIFY(!row->value(QStringLiteral("rtk_fixed")).isValid());
    QVERIFY(!row->value(QStringLiteral("wind_speed")).isValid());
    QVERIFY(!row->value(QStringLiteral("home_distance")).isValid());
    // 必备键仍然有效
    QCOMPARE(row->value(QStringLiteral("drone_sn")).toString(), QStringLiteral("7KABC2D00A00BC"));
    QCOMPARE(row->value(QStringLiteral("lat")).toDouble(), 39.9);
    QCOMPARE(row->value(QStringLiteral("lon")).toDouble(), 116.4);
    QCOMPARE(row->value(QStringLiteral("raw")).toString(), QString::fromUtf8(payload));
}

void TrackStoreTest::mapOsdToRowTimestampFallback()
{
    // 信封无 timestamp：ts_utc 回退接收时刻（UTC）
    QJsonObject data;
    data.insert(QStringLiteral("latitude"), 39.9);
    data.insert(QStringLiteral("longitude"), 116.4);
    QJsonObject envelope;
    envelope.insert(QStringLiteral("data"), data);

    const QDateTime receivedUtc =
        QDateTime::fromMSecsSinceEpoch(1750001234567, Qt::UTC);
    const auto row = Core::TrackStore::mapOsdToRow(
        QStringLiteral("thing/product/7KABC2D00A00BC/osd"), toPayload(envelope), receivedUtc);
    QVERIFY(row.has_value());
    QCOMPARE(row->value(QStringLiteral("ts_utc")).toDateTime(), receivedUtc.toUTC());
}

void TrackStoreTest::mapOsdToRowInvalidFrames()
{
    const QString topic = QStringLiteral("thing/product/7KABC2D00A00BC/osd");
    const QDateTime receivedUtc = QDateTime::fromMSecsSinceEpoch(1750001234567, Qt::UTC);

    // 非 JSON payload
    QVERIFY(!Core::TrackStore::mapOsdToRow(topic, QByteArrayLiteral("not-json"), receivedUtc)
                 .has_value());
    // data 缺 latitude（未定位）
    {
        QJsonObject data;
        data.insert(QStringLiteral("longitude"), 116.4);
        QJsonObject envelope;
        envelope.insert(QStringLiteral("data"), data);
        QVERIFY(
            !Core::TrackStore::mapOsdToRow(topic, toPayload(envelope), receivedUtc).has_value());
    }
    // MSDK 未定位哨兵 (0,0)：整帧丢弃
    {
        QJsonObject data;
        data.insert(QStringLiteral("latitude"), 0.0);
        data.insert(QStringLiteral("longitude"), 0.0);
        QJsonObject envelope;
        envelope.insert(QStringLiteral("data"), data);
        QVERIFY(
            !Core::TrackStore::mapOsdToRow(topic, toPayload(envelope), receivedUtc).has_value());
    }
    // 主题不合规（SN 为空）：同样丢弃
    QVERIFY(!Core::TrackStore::mapOsdToRow(QStringLiteral("thing/product//osd"),
                                           QByteArrayLiteral("{}"), receivedUtc)
                 .has_value());
}

void TrackStoreTest::mapEventToRowHms()
{
    // 构造 hms 事件：data.list 一条 level=2 记录
    QJsonObject item;
    item.insert(QStringLiteral("code"), QStringLiteral("battery_low"));
    item.insert(QStringLiteral("level"), 2);
    QJsonObject hmsData;
    hmsData.insert(QStringLiteral("list"), QJsonArray{item});
    QJsonObject envelope;
    envelope.insert(QStringLiteral("method"), QStringLiteral("hms"));
    envelope.insert(QStringLiteral("timestamp"), 1760000000456.0);
    envelope.insert(QStringLiteral("data"), hmsData);
    const QByteArray payload = toPayload(envelope);

    const QDateTime receivedUtc = QDateTime::fromMSecsSinceEpoch(1750001234567, Qt::UTC);
    const auto row = Core::TrackStore::mapEventToRow(
        QStringLiteral("thing/product/7KABC2D00A00BC/events"), payload, receivedUtc);
    QVERIFY(row.has_value());
    QCOMPARE(row->size(), 7);

    QVERIFY(!row->value(QStringLiteral("task_id")).isValid());
    QCOMPARE(row->value(QStringLiteral("drone_sn")).toString(), QStringLiteral("7KABC2D00A00BC"));
    QCOMPARE(row->value(QStringLiteral("ts_utc")).toDateTime(),
             QDateTime::fromMSecsSinceEpoch(1760000000456, Qt::UTC));
    QCOMPARE(row->value(QStringLiteral("method")).toString(), QStringLiteral("hms"));
    QVERIFY(row->value(QStringLiteral("level")).isValid());
    QCOMPARE(row->value(QStringLiteral("level")).toInt(), 2);
    // data 为信封 data 对象的紧凑序列化
    QCOMPARE(row->value(QStringLiteral("data")).toString(),
             QString::fromUtf8(QJsonDocument(hmsData).toJson(QJsonDocument::Compact)));
    // raw 为原始报文逐字节
    QCOMPARE(row->value(QStringLiteral("raw")).toString(), QString::fromUtf8(payload));
}

void TrackStoreTest::mapEventToRowMixedLevels()
{
    // 混合级别 ["0",1,2.5,2]：字符串与非整数不计，整数取最大 → 2
    QJsonObject itemStr;
    itemStr.insert(QStringLiteral("level"), QStringLiteral("0"));
    QJsonObject itemOne;
    itemOne.insert(QStringLiteral("level"), 1);
    QJsonObject itemFrac;
    itemFrac.insert(QStringLiteral("level"), 2.5);
    QJsonObject itemTwo;
    itemTwo.insert(QStringLiteral("level"), 2);
    QJsonObject hmsData;
    hmsData.insert(QStringLiteral("list"),
                   QJsonArray{itemStr, itemOne, itemFrac, itemTwo});
    QJsonObject envelope;
    envelope.insert(QStringLiteral("method"), QStringLiteral("hms"));
    envelope.insert(QStringLiteral("data"), hmsData);

    const auto row = Core::TrackStore::mapEventToRow(
        QStringLiteral("thing/product/7KABC2D00A00BC/events"), toPayload(envelope), QDateTime());
    QVERIFY(row.has_value());
    QVERIFY(row->value(QStringLiteral("level")).isValid());
    QCOMPARE(row->value(QStringLiteral("level")).toInt(), 2);
}

void TrackStoreTest::mapEventToRowDeviceExitHms()
{
    // HMS 解除通知：data 为空对象 → level 无有效值记 SQL NULL，data 序列化为 "{}"
    QJsonObject envelope;
    envelope.insert(QStringLiteral("method"), QStringLiteral("device_exit_hms"));
    envelope.insert(QStringLiteral("data"), QJsonObject());
    const QByteArray payload = toPayload(envelope);

    const auto row = Core::TrackStore::mapEventToRow(
        QStringLiteral("thing/product/7KABC2D00A00BC/events"), payload, QDateTime());
    QVERIFY(row.has_value());
    QCOMPARE(row->size(), 7);
    QCOMPARE(row->value(QStringLiteral("method")).toString(), QStringLiteral("device_exit_hms"));
    QVERIFY(!row->value(QStringLiteral("level")).isValid());
    QCOMPARE(row->value(QStringLiteral("data")).toString(), QStringLiteral("{}"));
    QCOMPARE(row->value(QStringLiteral("raw")).toString(), QString::fromUtf8(payload));
}

void TrackStoreTest::mapEventToRowInvalidFrames()
{
    const QString topic = QStringLiteral("thing/product/7KABC2D00A00BC/events");
    const QDateTime receivedUtc = QDateTime::fromMSecsSinceEpoch(1750001234567, Qt::UTC);

    // 非 JSON payload
    QVERIFY(!Core::TrackStore::mapEventToRow(topic, QByteArrayLiteral("not-json"), receivedUtc)
                 .has_value());
    // method 缺失：丢弃
    {
        QJsonObject envelope;
        envelope.insert(QStringLiteral("data"), QJsonObject());
        QVERIFY(
            !Core::TrackStore::mapEventToRow(topic, toPayload(envelope), receivedUtc).has_value());
    }
}

void TrackStoreTest::isEnabledByEnvUnset()
{
    // initTestCase 已 unset；未设置时总开关必须为 false（默认不落库）
    QVERIFY(!Core::TrackStore::isEnabledByEnv());
}

QTEST_GUILESS_MAIN(TrackStoreTest)

#include "track_store_test.moc"
