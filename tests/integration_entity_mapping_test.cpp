// REQ-012 W3 实体映射单元测试（TDD RED 阶段：实现为占位，本套件预期失败）。
// 语义锚点：
//   1. toJson 输出完整资源表示（契约 §1.7：全部字段必现、可空字段显式 null），
//      可直接用作 POST/PUT 请求体；
//   2. fromJson 按响应形状解析：全键在场、未知字段忽略、UUID/时间戳规范化；
//   3. 整数严格性：字符串数字 "12" 与非整数 1.5 拒收；Qt5 QJsonValue 数字统一 double，
//      12 与 12.0 客户端不可区分（契约 §1.3 对 12.0 字面量的拒绝由服务端执行）；
//   4. 数值可空字段 toJson 恒输出数值、fromJson null→0/0.0（唯一例外 missions.result
//      0↔null）；必填字段 fromJson null 拒收；
//   5. battery_level 实体为 double，toJson 先四舍五入再校验 0–100；
//   6. 词表对齐（契约表 4-1）：8 个 Core 枚举 + detection review 共 9 组 61 个 code
//      逐一对齐、双向越界拒收；
//   7. Qt5 精度边界：QDateTime 毫秒精度（.123456→.123 截断）、QVector3D 分量为 float
//      （坐标断言按 1e-5 相对容差比较）；
//   8. 纯逻辑测试不发起网络请求。

#include "Integration/EntityMapping.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include <QTime>
#include <QVector3D>
#include <QtTest>

namespace {

// 键排序后的逗号拼接，用于完整资源表示的键集断言
QStringList sortedKeyList(const QJsonObject &json)
{
    QStringList keys = json.keys();
    keys.sort();
    return keys;
}

// float 分量经 JSON double 往返后按 1e-5 相对容差比较
void verifyFloatRoundTrip(double actual, double expected, const char *what)
{
    const double tolerance = 1e-5 * qMax(1.0, qAbs(expected));
    QVERIFY2(qAbs(actual - expected) <= tolerance,
             QString(QStringLiteral("%1 应为 %2，实际 %3（容差 %4）"))
                 .arg(QString::fromUtf8(what))
                 .arg(expected)
                 .arg(actual)
                 .arg(tolerance)
                 .toUtf8()
                 .constData());
}

// 负例表条目：键 + 非法值。显式 JSON null 必须写 QJsonValue(QJsonValue::Null)，
// 默认构造的 QJsonValue{} 是 Undefined，不是 null。
struct JsonCase {
    QString key;
    QJsonValue value;
};

// 解析原始 JSON 字符串为对象，非对象或解析失败直接致命退出
QJsonObject parseFixtureJson(const char *raw)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(raw), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qFatal("测试夹具 JSON 解析失败：%s", error.errorString().toUtf8().constData());
    }
    return doc.object();
}

// 契约 §6.1 示例原文（targets，17 键）
QJsonObject validTargetJson()
{
    return parseFixtureJson(R"({"id":"3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90","type":1,"longitude":116.397128,"latitude":39.916527,"altitude":43.5,"depth":0,"heading":null,"pitch":null,"roll":null,"confidence":0.87,"threat_level":3,"status":1,"image_path":"/assets/targets/3f2a9c1e.jpg","radar_data_path":null,"detect_time":"2026-10-04T08:30:00Z","update_time":"2026-10-04T08:30:00Z","remark":"跑道东侧疑似航弹"})");
}

// 按 §6.2 字段表同形自构（devices，14 键）
QJsonObject validDeviceJson()
{
    return parseFixtureJson(R"({"id":"5c1d7a3e-9f2b-4e86-a4d1-7b8c9e0f1a2b","type":2,"name":"2 号排爆机器人","manufacturer":null,"model":"UXO-UGV-01","serial_number":"SN-2026-0042","status":3,"battery_level":88,"longitude":116.397,"latitude":39.916,"altitude":41.2,"last_online_time":null,"create_time":"2026-10-04T08:00:00Z","remark":null})");
}

// 按 §6.3 字段表同形自构（missions，16 键）
QJsonObject validMissionJson()
{
    return parseFixtureJson(R"({"id":"6e8b1c3a-5d24-4f17-9a68-3b0d5c7e9f14","type":2,"target_id":"3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90","priority":2,"status":3,"assigner":"指挥员甲","assignee":null,"device_id":"5c1d7a3e-9f2b-4e86-a4d1-7b8c9e0f1a2b","plan_start":"2026-10-04T10:00:00Z","plan_end":"2026-10-04T12:00:00Z","actual_start":null,"actual_end":null,"result":null,"create_time":"2026-10-04T09:00:00Z","update_time":"2026-10-04T09:05:00Z","remark":null})");
}

// 契约 §6.4 示例原文（alarms，10 键）
QJsonObject validAlarmJson()
{
    return parseFixtureJson(R"({"id":"9d4c8f2b-1e3a-4b7c-8f5d-0a2b6c4e8d10","type":"BATTERY_LOW","level":2,"target_id":null,"device_id":"5c1d7a3e-9f2b-4e86-a4d1-7b8c9e0f1a2b","message":"2 号机电量低于 20%","is_handled":false,"handle_time":null,"handle_result":null,"create_time":"2026-10-04T09:12:30Z"})");
}

// 契约 §6.5 示例+响应回显（detection-records，9 键）
QJsonObject validDetectionJson()
{
    return parseFixtureJson(R"({"id":"b7e2f6a4-3c1d-4a9e-8f5b-2d4e6f8a0c2e","analyzed_at":"2026-10-04T09:30:15Z","has_anomaly":true,"class_name":"aircraft-bombs","confidence":0.92,"raw_result":{"boxes":[{"x":120,"y":84,"w":57,"h":43}],"scores":{"aircraft-bombs":0.92,"background":0.05,"rockets":0.02}},"target_id":null,"review":0,"reviewed_at":null})");
}

// 与 validTargetJson 同一实体的 Core 表示样本（heading/pitch/roll 用姿态角承载）
Core::TargetInfo sampleTarget()
{
    Core::TargetInfo target;
    target.id = QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90");
    target.type = Core::TargetType::AirBomb;
    target.typeName = QStringLiteral("航弹"); // 本地展示字段，契约无对应键
    target.position = QVector3D(116.397128f, 39.916527f, 43.5f);
    target.depth = 0.0;
    target.attitude = QVector3D(12.5f, -3.25f, 0.0f);
    target.confidence = 0.87;
    target.threatLevel = Core::ThreatLevel::High;
    target.status = Core::TargetStatus::Detected;
    target.imagePath = QStringLiteral("/assets/targets/3f2a9c1e.jpg");
    target.radarDataPath = QString();
    target.detectTime = QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0), Qt::UTC);
    target.updateTime = QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0), Qt::UTC);
    target.remark = QStringLiteral("跑道东侧疑似航弹");
    return target;
}

int targetTypeOf(const Core::TargetInfo &entity) { return static_cast<int>(entity.type); }
int threatLevelOf(const Core::TargetInfo &entity) { return static_cast<int>(entity.threatLevel); }
int targetStatusOf(const Core::TargetInfo &entity) { return static_cast<int>(entity.status); }
int missionTypeOf(const Core::MissionInfo &entity) { return static_cast<int>(entity.type); }
int missionStatusOf(const Core::MissionInfo &entity) { return static_cast<int>(entity.status); }
int deviceTypeOf(const Core::DeviceInfo &entity) { return static_cast<int>(entity.type); }
int deviceStatusOf(const Core::DeviceInfo &entity) { return static_cast<int>(entity.status); }
int alarmLevelOf(const Core::AlarmInfo &entity) { return static_cast<int>(entity.level); }
int detectionReviewOf(const Integration::DetectionRecordData &entity) { return entity.review; }

// 词表对齐通用检查：0..maxCode 双向逐一对齐，越界值双向拒收
template <typename Entity, typename FromJson, typename ToJson, typename Extract>
void checkEnumRange(const QJsonObject &fixture, const QString &key, int maxCode,
                    FromJson fromJson, ToJson toJson, Extract extract, const char *ctx)
{
    for (int code = 0; code <= maxCode; ++code) {
        QJsonObject json = fixture;
        json[key] = code;
        Entity entity;
        QVERIFY2(fromJson(json, &entity),
                 QString(QStringLiteral("%1：合法 code %2 应通过 fromJson"))
                     .arg(QString::fromUtf8(ctx))
                     .arg(code)
                     .toUtf8()
                     .constData());
        QCOMPARE(extract(entity), code);
        QJsonObject out;
        QVERIFY2(toJson(entity, &out),
                 QString(QStringLiteral("%1：合法 code %2 应通过 toJson"))
                     .arg(QString::fromUtf8(ctx))
                     .arg(code)
                     .toUtf8()
                     .constData());
        QCOMPARE(out.value(key).toInt(), code);
    }
    const int badCodes[] = { maxCode + 1, -1 };
    for (const int bad : badCodes) {
        QJsonObject json = fixture;
        json[key] = bad;
        Entity entity;
        QVERIFY2(!fromJson(json, &entity),
                 QString(QStringLiteral("%1：越界 code %2 应被 fromJson 拒收"))
                     .arg(QString::fromUtf8(ctx))
                     .arg(bad)
                     .toUtf8()
                     .constData());
    }
}

} // namespace

class IntegrationEntityMappingTest : public QObject
{
    Q_OBJECT

private slots:
    void targetFromJsonParsesAllContractFields();
    void targetToJsonEmitsCompleteResourceRepresentation();
    void targetFromJsonRejectsInvalidFields();
    void deviceRoundTripAndValidation();
    void missionRoundTripAndValidation();
    void alarmRoundTripAndValidation();
    void detectionRecordRoundTripAndValidation();
    void uuidParsingAndNormalization();
    void timestampParsingAndNormalization();
    void enumVocabularyRangeAlignment();
};

// === (1) targets fromJson：全部 17 个契约字段逐一断言 ===
void IntegrationEntityMappingTest::targetFromJsonParsesAllContractFields()
{
    Core::TargetInfo t;
    QVERIFY2(Integration::targetFromJson(validTargetJson(), &t),
             QStringLiteral("合法 targets 响应应通过 fromJson").toUtf8().constData());
    QCOMPARE(t.id, QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90"));
    QCOMPARE(static_cast<int>(t.type), 1);
    QVERIFY2(t.typeName.isEmpty(),
             QStringLiteral("词表 name_cn 回填属 W4，本层不应回填 type_name").toUtf8().constData());
    verifyFloatRoundTrip(t.position.x(), 116.397128, "经度 longitude");
    verifyFloatRoundTrip(t.position.y(), 39.916527, "纬度 latitude");
    verifyFloatRoundTrip(t.position.z(), 43.5, "高度 altitude");
    QCOMPARE(t.depth, 0.0);
    // 轴向：x=heading、y=pitch、z=roll；null → 0
    QCOMPARE(t.attitude.x(), 0.0f);
    QCOMPARE(t.attitude.y(), 0.0f);
    QCOMPARE(t.attitude.z(), 0.0f);
    QCOMPARE(t.confidence, 0.87);
    QCOMPARE(static_cast<int>(t.threatLevel), 3);
    QCOMPARE(static_cast<int>(t.status), 1);
    QCOMPARE(t.imagePath, QStringLiteral("/assets/targets/3f2a9c1e.jpg"));
    QVERIFY2(t.radarDataPath.isEmpty(), QStringLiteral("radar_data_path null 应映射为空串").toUtf8().constData());
    QCOMPARE(t.detectTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0), Qt::UTC));
    QCOMPARE(t.updateTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0), Qt::UTC));
    QCOMPARE(t.remark, QStringLiteral("跑道东侧疑似航弹"));
}

// === (2) targets toJson：完整资源表示 + 往返保真 ===
void IntegrationEntityMappingTest::targetToJsonEmitsCompleteResourceRepresentation()
{
    QJsonObject out;
    QVERIFY2(Integration::targetToJson(sampleTarget(), &out),
             QStringLiteral("合法实体应通过 toJson").toUtf8().constData());
    QCOMPARE(sortedKeyList(out).join(QLatin1Char(',')),
              QStringLiteral("altitude,confidence,depth,detect_time,heading,id,image_path,latitude,longitude,pitch,radar_data_path,remark,roll,status,threat_level,type,update_time"));
    QCOMPARE(out.value(QStringLiteral("id")).toString(), QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90"));
    QCOMPARE(out.value(QStringLiteral("type")).toInt(), 1);
    verifyFloatRoundTrip(out.value(QStringLiteral("longitude")).toDouble(), 116.397128, "输出经度 longitude");
    verifyFloatRoundTrip(out.value(QStringLiteral("latitude")).toDouble(), 39.916527, "输出纬度 latitude");
    verifyFloatRoundTrip(out.value(QStringLiteral("altitude")).toDouble(), 43.5, "输出高度 altitude");
    verifyFloatRoundTrip(out.value(QStringLiteral("heading")).toDouble(), 12.5, "输出 heading（attitude.x）");
    verifyFloatRoundTrip(out.value(QStringLiteral("pitch")).toDouble(), -3.25, "输出 pitch（attitude.y）");
    verifyFloatRoundTrip(out.value(QStringLiteral("roll")).toDouble(), 0.0, "输出 roll（attitude.z）");
    verifyFloatRoundTrip(out.value(QStringLiteral("depth")).toDouble(), 0.0, "输出 depth");
    verifyFloatRoundTrip(out.value(QStringLiteral("confidence")).toDouble(), 0.87, "输出 confidence");
    QCOMPARE(out.value(QStringLiteral("threat_level")).toInt(), 3);
    QCOMPARE(out.value(QStringLiteral("status")).toInt(), 1);
    QCOMPARE(out.value(QStringLiteral("image_path")).toString(), QStringLiteral("/assets/targets/3f2a9c1e.jpg"));
    QVERIFY2(out.value(QStringLiteral("radar_data_path")).isNull(),
             QStringLiteral("空串 radar_data_path 应输出 null").toUtf8().constData());
    QCOMPARE(out.value(QStringLiteral("detect_time")).toString(), QStringLiteral("2026-10-04T08:30:00Z"));
    QCOMPARE(out.value(QStringLiteral("update_time")).toString(), QStringLiteral("2026-10-04T08:30:00Z"));
    QCOMPARE(out.value(QStringLiteral("remark")).toString(), QStringLiteral("跑道东侧疑似航弹"));
    QVERIFY2(!out.contains(QStringLiteral("type_name")),
             QStringLiteral("type_name 为本地展示字段，toJson 不应输出").toUtf8().constData());

    // 往返：fromJson → toJson 后仍为 17 键完整表示，null→0→0 语义
    Core::TargetInfo t2;
    QVERIFY2(Integration::targetFromJson(validTargetJson(), &t2),
             QStringLiteral("往返前置：fromJson 应通过").toUtf8().constData());
    QJsonObject rt;
    QVERIFY2(Integration::targetToJson(t2, &rt),
             QStringLiteral("往返：toJson 应通过").toUtf8().constData());
    QCOMPARE(sortedKeyList(rt).join(QLatin1Char(',')),
              QStringLiteral("altitude,confidence,depth,detect_time,heading,id,image_path,latitude,longitude,pitch,radar_data_path,remark,roll,status,threat_level,type,update_time"));
    QVERIFY2(rt.value(QStringLiteral("heading")).isDouble() && rt.value(QStringLiteral("heading")).toDouble() == 0.0,
             QStringLiteral("heading null→0→0：应输出数值 0").toUtf8().constData());
    QVERIFY2(rt.value(QStringLiteral("pitch")).isDouble() && rt.value(QStringLiteral("pitch")).toDouble() == 0.0,
             QStringLiteral("pitch null→0→0：应输出数值 0").toUtf8().constData());
    QVERIFY2(rt.value(QStringLiteral("roll")).isDouble() && rt.value(QStringLiteral("roll")).toDouble() == 0.0,
             QStringLiteral("roll null→0→0：应输出数值 0").toUtf8().constData());
    QVERIFY2(rt.value(QStringLiteral("radar_data_path")).isNull(),
             QStringLiteral("往返后 radar_data_path 仍应为 null").toUtf8().constData());
    verifyFloatRoundTrip(rt.value(QStringLiteral("longitude")).toDouble(), 116.397128, "往返经度 longitude");
}

// === (3) targets 负例：非法值、缺键、toJson 拒发 ===
void IntegrationEntityMappingTest::targetFromJsonRejectsInvalidFields()
{
    // 正向对照：夹具与样本本身必须合法（否则负例结论不成立）
    Core::TargetInfo ok;
    QVERIFY2(Integration::targetFromJson(validTargetJson(), &ok),
             QStringLiteral("正向对照：夹具应通过 fromJson").toUtf8().constData());
    QJsonObject okOut;
    QVERIFY2(Integration::targetToJson(sampleTarget(), &okOut),
             QStringLiteral("正向对照：样本应通过 toJson").toUtf8().constData());

    const JsonCase cases[] = {
        { QStringLiteral("type"), QStringLiteral("1") },
        { QStringLiteral("type"), 1.5 },
        { QStringLiteral("type"), true },
        { QStringLiteral("type"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("type"), 12 },
        { QStringLiteral("type"), -1 },
        { QStringLiteral("threat_level"), 5 },
        { QStringLiteral("threat_level"), QStringLiteral("3") },
        { QStringLiteral("status"), 7 },
        { QStringLiteral("status"), 1.5 },
        { QStringLiteral("longitude"), 180.5 },
        { QStringLiteral("longitude"), -180.5 },
        { QStringLiteral("longitude"), QStringLiteral("116.4") },
        { QStringLiteral("longitude"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("latitude"), 90.5 },
        { QStringLiteral("latitude"), -90.5 },
        { QStringLiteral("latitude"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("confidence"), 1.5 },
        { QStringLiteral("confidence"), -0.1 },
        { QStringLiteral("confidence"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("id"), QStringLiteral("not-a-uuid") },
        { QStringLiteral("id"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("id"), 42 },
        { QStringLiteral("detect_time"), QStringLiteral("2026-13-01T00:00:00Z") },
        { QStringLiteral("detect_time"), QStringLiteral("not-a-timestamp") },
        { QStringLiteral("detect_time"), QStringLiteral("") },
        { QStringLiteral("detect_time"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("detect_time"), 42 },
        { QStringLiteral("update_time"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("depth"), QStringLiteral("0") },
        { QStringLiteral("depth"), true },
        { QStringLiteral("heading"), QStringLiteral("12.5") },
        { QStringLiteral("altitude"), true },
        { QStringLiteral("remark"), 42 },
        { QStringLiteral("image_path"), 42 },
        { QStringLiteral("radar_data_path"), false },
    };
    for (const JsonCase &c : cases) {
        QJsonObject json = validTargetJson();
        json[c.key] = c.value;
        Core::TargetInfo t;
        QVERIFY2(!Integration::targetFromJson(json, &t),
                 QString(QStringLiteral("targets 非法值应拒收：%1")).arg(c.key).toUtf8().constData());
    }

    // 响应缺任一契约键应拒收（§1.7 响应不缺字段）
    const QStringList targetKeys = validTargetJson().keys();
    for (const QString &key : targetKeys) {
        QJsonObject json = validTargetJson();
        json.remove(key);
        Core::TargetInfo t;
        QVERIFY2(!Integration::targetFromJson(json, &t),
                 QString(QStringLiteral("响应缺键应拒收：%1")).arg(key).toUtf8().constData());
    }

    // toJson 拒发：实体侧非法值
    {
        Core::TargetInfo t = sampleTarget();
        t.id = QStringLiteral("not-a-uuid");
        QJsonObject out;
        QVERIFY2(!Integration::targetToJson(t, &out),
                 QStringLiteral("非法 UUID 应被 toJson 拒发").toUtf8().constData());
    }
    {
        Core::TargetInfo t = sampleTarget();
        t.type = static_cast<Core::TargetType>(12);
        QJsonObject out;
        QVERIFY2(!Integration::targetToJson(t, &out),
                 QStringLiteral("越界 type 序数应被 toJson 拒发").toUtf8().constData());
    }
    {
        Core::TargetInfo t = sampleTarget();
        t.confidence = 1.2;
        QJsonObject out;
        QVERIFY2(!Integration::targetToJson(t, &out),
                 QStringLiteral("confidence 越界应被 toJson 拒发").toUtf8().constData());
    }
    {
        Core::TargetInfo t = sampleTarget();
        t.position = QVector3D(200.0f, 39.916527f, 43.5f);
        QJsonObject out;
        QVERIFY2(!Integration::targetToJson(t, &out),
                 QStringLiteral("经度越界应被 toJson 拒发").toUtf8().constData());
    }
    {
        Core::TargetInfo t = sampleTarget();
        t.detectTime = QDateTime(); // 无效时间
        QJsonObject out;
        QVERIFY2(!Integration::targetToJson(t, &out),
                 QStringLiteral("无效 detectTime 应被 toJson 拒发").toUtf8().constData());
    }
}

// === (4) devices 往返与校验 ===
void IntegrationEntityMappingTest::deviceRoundTripAndValidation()
{
    // 正向：14 字段逐一断言
    Core::DeviceInfo d;
    QVERIFY2(Integration::deviceFromJson(validDeviceJson(), &d),
             QStringLiteral("合法 devices 响应应通过 fromJson").toUtf8().constData());
    QCOMPARE(d.id, QStringLiteral("5c1d7a3e-9f2b-4e86-a4d1-7b8c9e0f1a2b"));
    QCOMPARE(static_cast<int>(d.type), 2);
    QCOMPARE(d.name, QStringLiteral("2 号排爆机器人"));
    QVERIFY2(d.manufacturer.isEmpty(), QStringLiteral("manufacturer null 应映射为空串").toUtf8().constData());
    QCOMPARE(d.model, QStringLiteral("UXO-UGV-01"));
    QCOMPARE(d.serialNumber, QStringLiteral("SN-2026-0042"));
    QCOMPARE(static_cast<int>(d.status), 3);
    QCOMPARE(d.batteryLevel, 88.0);
    verifyFloatRoundTrip(d.position.x(), 116.397, "设备经度 longitude");
    verifyFloatRoundTrip(d.position.y(), 39.916, "设备纬度 latitude");
    verifyFloatRoundTrip(d.position.z(), 41.2, "设备高度 altitude");
    QVERIFY2(!d.lastOnlineTime.isValid(), QStringLiteral("last_online_time null 应映射为无效时间").toUtf8().constData());
    QCOMPARE(d.createTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(8, 0, 0), Qt::UTC));
    QVERIFY2(d.remark.isEmpty(), QStringLiteral("remark null 应映射为空串").toUtf8().constData());

    QJsonObject out;
    QVERIFY2(Integration::deviceToJson(d, &out),
             QStringLiteral("合法设备实体应通过 toJson").toUtf8().constData());
    QCOMPARE(sortedKeyList(out).join(QLatin1Char(',')),
              QStringLiteral("altitude,battery_level,create_time,id,last_online_time,latitude,longitude,manufacturer,model,name,remark,serial_number,status,type"));
    QVERIFY2(out.value(QStringLiteral("battery_level")).isDouble() && out.value(QStringLiteral("battery_level")).toDouble() == 88.0,
             QStringLiteral("battery_level 应输出数值 88.0").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("manufacturer")).isNull(), QStringLiteral("空串 manufacturer 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("last_online_time")).isNull(), QStringLiteral("无效 last_online_time 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("remark")).isNull(), QStringLiteral("空串 remark 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("name")).isString(), QStringLiteral("name 应输出字符串").toUtf8().constData());

    // battery_level 为 double：toJson 先四舍五入再校验 0–100
    {
        Core::DeviceInfo d2;
        QJsonObject o;
        QVERIFY(Integration::deviceFromJson(validDeviceJson(), &d2));
        d2.batteryLevel = 87.5;
        QVERIFY2(Integration::deviceToJson(d2, &o), QStringLiteral("87.5 应四舍五入为 88 并通过").toUtf8().constData());
        QCOMPARE(o.value(QStringLiteral("battery_level")).toInt(), 88);
    }
    {
        Core::DeviceInfo d2;
        QJsonObject o;
        QVERIFY(Integration::deviceFromJson(validDeviceJson(), &d2));
        d2.batteryLevel = 87.4;
        QVERIFY2(Integration::deviceToJson(d2, &o), QStringLiteral("87.4 应四舍五入为 87 并通过").toUtf8().constData());
        QCOMPARE(o.value(QStringLiteral("battery_level")).toInt(), 87);
    }
    {
        Core::DeviceInfo d2;
        QJsonObject o;
        QVERIFY(Integration::deviceFromJson(validDeviceJson(), &d2));
        d2.batteryLevel = 100.4;
        QVERIFY2(Integration::deviceToJson(d2, &o), QStringLiteral("100.4 应四舍五入为 100 并通过").toUtf8().constData());
        QCOMPARE(o.value(QStringLiteral("battery_level")).toInt(), 100);
    }
    {
        Core::DeviceInfo d2;
        QJsonObject o;
        QVERIFY(Integration::deviceFromJson(validDeviceJson(), &d2));
        d2.batteryLevel = 101.0;
        QVERIFY2(!Integration::deviceToJson(d2, &o), QStringLiteral("101.0 越界应被拒发").toUtf8().constData());
    }
    {
        Core::DeviceInfo d2;
        QJsonObject o;
        QVERIFY(Integration::deviceFromJson(validDeviceJson(), &d2));
        d2.batteryLevel = -1.0;
        QVERIFY2(!Integration::deviceToJson(d2, &o), QStringLiteral("-1.0 越界应被拒发").toUtf8().constData());
    }

    // 数值/可空字段全 null：fromJson 接受并归零，toJson 数值字段恒输出数值
    {
        QJsonObject json = validDeviceJson();
        const char *nullKeys[] = { "manufacturer", "model", "serial_number", "battery_level",
                                   "longitude", "latitude", "altitude", "last_online_time", "remark" };
        for (const char *k : nullKeys)
            json[QString::fromLatin1(k)] = QJsonValue(QJsonValue::Null);
        Core::DeviceInfo dn;
        QVERIFY2(Integration::deviceFromJson(json, &dn),
                 QStringLiteral("可空字段全 null 应通过 fromJson").toUtf8().constData());
        QCOMPARE(dn.batteryLevel, 0.0);
        QCOMPARE(dn.position.x(), 0.0f);
        QCOMPARE(dn.position.y(), 0.0f);
        QCOMPARE(dn.position.z(), 0.0f);
        QVERIFY2(!dn.lastOnlineTime.isValid(), QStringLiteral("null last_online_time 应为无效时间").toUtf8().constData());
        QVERIFY2(dn.manufacturer.isEmpty() && dn.model.isEmpty() && dn.serialNumber.isEmpty() && dn.remark.isEmpty(),
                 QStringLiteral("null 字符串字段应为空串").toUtf8().constData());
        QJsonObject on;
        QVERIFY2(Integration::deviceToJson(dn, &on),
                 QStringLiteral("全 null 实体应通过 toJson").toUtf8().constData());
        QVERIFY2(on.value(QStringLiteral("battery_level")).isDouble() && on.value(QStringLiteral("battery_level")).toDouble() == 0.0,
                 QStringLiteral("数值可空字段 toJson 恒输出数值").toUtf8().constData());
        QVERIFY2(on.value(QStringLiteral("longitude")).isDouble(), QStringLiteral("经度应输出数值").toUtf8().constData());
        QVERIFY2(on.value(QStringLiteral("latitude")).isDouble(), QStringLiteral("纬度应输出数值").toUtf8().constData());
        QVERIFY2(on.value(QStringLiteral("altitude")).isDouble(), QStringLiteral("高度应输出数值").toUtf8().constData());
    }

    // 必填字符串 name 空串：接受且原样输出 ""（而非 null）
    {
        QJsonObject json = validDeviceJson();
        json[QStringLiteral("name")] = QStringLiteral("");
        Core::DeviceInfo d3;
        QVERIFY2(Integration::deviceFromJson(json, &d3),
                 QStringLiteral("必填字符串空串应通过 fromJson").toUtf8().constData());
        QVERIFY2(d3.name.isEmpty(), QStringLiteral("空串 name 应为空串").toUtf8().constData());
        QJsonObject o3;
        QVERIFY2(Integration::deviceToJson(d3, &o3),
                 QStringLiteral("空串 name 实体应通过 toJson").toUtf8().constData());
        QVERIFY2(o3.value(QStringLiteral("name")).isString() && o3.value(QStringLiteral("name")).toString().isEmpty(),
                 QStringLiteral("必填字符串输出 \"\" 而非 null").toUtf8().constData());
    }

    const JsonCase cases[] = {
        { QStringLiteral("name"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("name"), 42 },
        { QStringLiteral("battery_level"), QStringLiteral("88") },
        { QStringLiteral("battery_level"), 88.5 },
        { QStringLiteral("battery_level"), 101 },
        { QStringLiteral("battery_level"), -1 },
        { QStringLiteral("battery_level"), true },
        { QStringLiteral("create_time"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("type"), 8 },
        { QStringLiteral("status"), 7 },
        { QStringLiteral("id"), QStringLiteral("not-a-uuid") },
        { QStringLiteral("last_online_time"), QStringLiteral("not-a-timestamp") },
        { QStringLiteral("model"), 42 },
        { QStringLiteral("longitude"), 180.5 },
        { QStringLiteral("latitude"), -90.5 },
        { QStringLiteral("altitude"), QStringLiteral("41.2") },
    };
    for (const JsonCase &c : cases) {
        QJsonObject json = validDeviceJson();
        json[c.key] = c.value;
        Core::DeviceInfo dv;
        QVERIFY2(!Integration::deviceFromJson(json, &dv),
                 QString(QStringLiteral("devices 非法值应拒收：%1")).arg(c.key).toUtf8().constData());
    }

    // 14 键缺一键即拒收
    const QStringList deviceKeys = validDeviceJson().keys();
    for (const QString &key : deviceKeys) {
        QJsonObject json = validDeviceJson();
        json.remove(key);
        Core::DeviceInfo dv;
        QVERIFY2(!Integration::deviceFromJson(json, &dv),
                 QString(QStringLiteral("devices 响应缺键应拒收：%1")).arg(key).toUtf8().constData());
    }
}

// === (5) missions 往返与校验 ===
void IntegrationEntityMappingTest::missionRoundTripAndValidation()
{
    // 正向：16 字段逐一断言
    Core::MissionInfo m;
    QVERIFY2(Integration::missionFromJson(validMissionJson(), &m),
             QStringLiteral("合法 missions 响应应通过 fromJson").toUtf8().constData());
    QCOMPARE(m.id, QStringLiteral("6e8b1c3a-5d24-4f17-9a68-3b0d5c7e9f14"));
    QCOMPARE(static_cast<int>(m.type), 2);
    QCOMPARE(m.targetId, QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90"));
    QCOMPARE(m.priority, 2);
    QCOMPARE(static_cast<int>(m.status), 3);
    QCOMPARE(m.assigner, QStringLiteral("指挥员甲"));
    QVERIFY2(m.assignee.isEmpty(), QStringLiteral("assignee null 应映射为空串").toUtf8().constData());
    QCOMPARE(m.deviceId, QStringLiteral("5c1d7a3e-9f2b-4e86-a4d1-7b8c9e0f1a2b"));
    QCOMPARE(m.planStartTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(10, 0, 0), Qt::UTC));
    QCOMPARE(m.planEndTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(12, 0, 0), Qt::UTC));
    QVERIFY2(!m.actualStartTime.isValid(), QStringLiteral("actual_start null 应为无效时间").toUtf8().constData());
    QVERIFY2(!m.actualEndTime.isValid(), QStringLiteral("actual_end null 应为无效时间").toUtf8().constData());
    QCOMPARE(m.result, 0); // null → 0
    QCOMPARE(m.createTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(9, 0, 0), Qt::UTC));
    QCOMPARE(m.updateTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(9, 5, 0), Qt::UTC));
    QVERIFY2(m.remark.isEmpty(), QStringLiteral("remark null 应映射为空串").toUtf8().constData());

    QJsonObject out;
    QVERIFY2(Integration::missionToJson(m, &out),
             QStringLiteral("合法任务实体应通过 toJson").toUtf8().constData());
    QCOMPARE(sortedKeyList(out).join(QLatin1Char(',')),
              QStringLiteral("actual_end,actual_start,assignee,assigner,create_time,device_id,id,plan_end,plan_start,priority,remark,result,status,target_id,type,update_time"));
    QVERIFY2(out.value(QStringLiteral("assignee")).isNull(), QStringLiteral("空串 assignee 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("actual_start")).isNull(), QStringLiteral("无效 actual_start 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("actual_end")).isNull(), QStringLiteral("无效 actual_end 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("result")).isNull(), QStringLiteral("result 0 应输出 null（0↔null）").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("remark")).isNull(), QStringLiteral("空串 remark 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("device_id")).isString()
                 && out.value(QStringLiteral("device_id")).toString() == QStringLiteral("5c1d7a3e-9f2b-4e86-a4d1-7b8c9e0f1a2b"),
             QStringLiteral("device_id 应输出原字符串").toUtf8().constData());
    QCOMPARE(out.value(QStringLiteral("plan_start")).toString(), QStringLiteral("2026-10-04T10:00:00Z"));

    // result=3：非 0 数值原样往返
    {
        QJsonObject json = validMissionJson();
        json[QStringLiteral("result")] = 3;
        Core::MissionInfo m2;
        QVERIFY2(Integration::missionFromJson(json, &m2),
                 QStringLiteral("result=3 应通过 fromJson").toUtf8().constData());
        QCOMPARE(m2.result, 3);
        QJsonObject o2;
        QVERIFY2(Integration::missionToJson(m2, &o2),
                 QStringLiteral("result=3 应通过 toJson").toUtf8().constData());
        QCOMPARE(o2.value(QStringLiteral("result")).toInt(), 3);
    }
    // device_id=null：可空 UUID，空串 → null
    {
        QJsonObject jsonNoDevice = validMissionJson();
        jsonNoDevice[QStringLiteral("device_id")] = QJsonValue(QJsonValue::Null);
        Core::MissionInfo md;
        QVERIFY2(Integration::missionFromJson(jsonNoDevice, &md),
                 QStringLiteral("device_id null 应通过 fromJson").toUtf8().constData());
        QVERIFY2(md.deviceId.isEmpty(), QStringLiteral("device_id null 应映射为空串").toUtf8().constData());
        QJsonObject od;
        QVERIFY2(Integration::missionToJson(md, &od),
                 QStringLiteral("device_id 为空的实体应通过 toJson").toUtf8().constData());
        QVERIFY2(od.value(QStringLiteral("device_id")).isNull(), QStringLiteral("空 device_id 应输出 null").toUtf8().constData());
    }
    // plan_start/plan_end=null：可空时间，无效 → null
    {
        QJsonObject jsonNoPlan = validMissionJson();
        jsonNoPlan[QStringLiteral("plan_start")] = QJsonValue(QJsonValue::Null);
        jsonNoPlan[QStringLiteral("plan_end")] = QJsonValue(QJsonValue::Null);
        Core::MissionInfo mp;
        QVERIFY2(Integration::missionFromJson(jsonNoPlan, &mp),
                 QStringLiteral("plan 时间 null 应通过 fromJson").toUtf8().constData());
        QVERIFY2(!mp.planStartTime.isValid() && !mp.planEndTime.isValid(),
                 QStringLiteral("plan 时间 null 应为无效时间").toUtf8().constData());
        QJsonObject op;
        QVERIFY2(Integration::missionToJson(mp, &op),
                 QStringLiteral("plan 时间无效的实体应通过 toJson").toUtf8().constData());
        QVERIFY2(op.value(QStringLiteral("plan_start")).isNull(), QStringLiteral("无效 plan_start 应输出 null").toUtf8().constData());
        QVERIFY2(op.value(QStringLiteral("plan_end")).isNull(), QStringLiteral("无效 plan_end 应输出 null").toUtf8().constData());
    }

    const JsonCase cases[] = {
        { QStringLiteral("target_id"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("target_id"), QStringLiteral("not-a-uuid") },
        { QStringLiteral("priority"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("priority"), QStringLiteral("1") },
        { QStringLiteral("priority"), 1.5 },
        { QStringLiteral("result"), 3.5 },
        { QStringLiteral("result"), QStringLiteral("3") },
        { QStringLiteral("create_time"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("update_time"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("device_id"), QStringLiteral("not-a-uuid") },
        { QStringLiteral("type"), 5 },
        { QStringLiteral("status"), 9 },
        { QStringLiteral("assigner"), 42 },
        { QStringLiteral("remark"), false },
    };
    for (const JsonCase &c : cases) {
        QJsonObject json = validMissionJson();
        json[c.key] = c.value;
        Core::MissionInfo mm;
        QVERIFY2(!Integration::missionFromJson(json, &mm),
                 QString(QStringLiteral("missions 非法值应拒收：%1")).arg(c.key).toUtf8().constData());
    }

    // 16 键缺一键即拒收
    const QStringList missionKeys = validMissionJson().keys();
    for (const QString &key : missionKeys) {
        QJsonObject json = validMissionJson();
        json.remove(key);
        Core::MissionInfo mm;
        QVERIFY2(!Integration::missionFromJson(json, &mm),
                 QString(QStringLiteral("missions 响应缺键应拒收：%1")).arg(key).toUtf8().constData());
    }

    // toJson 拒发：实体侧非法 UUID
    {
        Core::MissionInfo mt;
        QVERIFY(Integration::missionFromJson(validMissionJson(), &mt));
        mt.targetId = QStringLiteral("not-a-uuid");
        QJsonObject o;
        QVERIFY2(!Integration::missionToJson(mt, &o),
                 QStringLiteral("非法 targetId 应被 toJson 拒发").toUtf8().constData());
    }
    {
        Core::MissionInfo mt;
        QVERIFY(Integration::missionFromJson(validMissionJson(), &mt));
        mt.deviceId = QStringLiteral("not-a-uuid");
        QJsonObject o;
        QVERIFY2(!Integration::missionToJson(mt, &o),
                 QStringLiteral("非法 deviceId 应被 toJson 拒发").toUtf8().constData());
    }
}

// === (6) alarms 往返与校验 ===
void IntegrationEntityMappingTest::alarmRoundTripAndValidation()
{
    // 正向：10 字段逐一断言
    Core::AlarmInfo a;
    QVERIFY2(Integration::alarmFromJson(validAlarmJson(), &a),
             QStringLiteral("合法 alarms 响应应通过 fromJson").toUtf8().constData());
    QCOMPARE(a.id, QStringLiteral("9d4c8f2b-1e3a-4b7c-8f5d-0a2b6c4e8d10"));
    QCOMPARE(a.type, QStringLiteral("BATTERY_LOW")); // alarm type 为字符串词表 code
    QCOMPARE(static_cast<int>(a.level), 2);
    QVERIFY2(a.targetId.isEmpty(), QStringLiteral("target_id null 应映射为空串").toUtf8().constData());
    QCOMPARE(a.deviceId, QStringLiteral("5c1d7a3e-9f2b-4e86-a4d1-7b8c9e0f1a2b"));
    QCOMPARE(a.message, QStringLiteral("2 号机电量低于 20%"));
    QCOMPARE(a.isHandled, false);
    QVERIFY2(!a.handleTime.isValid(), QStringLiteral("handle_time null 应为无效时间").toUtf8().constData());
    QVERIFY2(a.handleResult.isEmpty(), QStringLiteral("handle_result null 应映射为空串").toUtf8().constData());
    QCOMPARE(a.createTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(9, 12, 30), Qt::UTC));

    QJsonObject out;
    QVERIFY2(Integration::alarmToJson(a, &out),
             QStringLiteral("合法告警实体应通过 toJson").toUtf8().constData());
    QCOMPARE(sortedKeyList(out).join(QLatin1Char(',')),
              QStringLiteral("create_time,device_id,handle_result,handle_time,id,is_handled,level,message,target_id,type"));
    QVERIFY2(out.value(QStringLiteral("target_id")).isNull(), QStringLiteral("空串 target_id 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("handle_time")).isNull(), QStringLiteral("无效 handle_time 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("handle_result")).isNull(), QStringLiteral("空串 handle_result 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("is_handled")).isBool() && out.value(QStringLiteral("is_handled")).toBool() == false,
             QStringLiteral("is_handled 应输出布尔 false").toUtf8().constData());
    QCOMPARE(out.value(QStringLiteral("type")).toString(), QStringLiteral("BATTERY_LOW"));
    QCOMPARE(out.value(QStringLiteral("create_time")).toString(), QStringLiteral("2026-10-04T09:12:30Z"));

    // message=null：可空字符串，空串 → null
    {
        QJsonObject json = validAlarmJson();
        json[QStringLiteral("message")] = QJsonValue(QJsonValue::Null);
        Core::AlarmInfo am;
        QVERIFY2(Integration::alarmFromJson(json, &am),
                 QStringLiteral("message null 应通过 fromJson").toUtf8().constData());
        QVERIFY2(am.message.isEmpty(), QStringLiteral("message null 应映射为空串").toUtf8().constData());
        QJsonObject om;
        QVERIFY2(Integration::alarmToJson(am, &om),
                 QStringLiteral("message 为空的实体应通过 toJson").toUtf8().constData());
        QVERIFY2(om.value(QStringLiteral("message")).isNull(), QStringLiteral("空串 message 应输出 null").toUtf8().constData());
    }
    // type=""：必填字符串，空串接受且原样输出 ""
    {
        QJsonObject json = validAlarmJson();
        json[QStringLiteral("type")] = QStringLiteral("");
        Core::AlarmInfo at;
        QVERIFY2(Integration::alarmFromJson(json, &at),
                 QStringLiteral("type 空串应通过 fromJson").toUtf8().constData());
        QVERIFY2(at.type.isEmpty(), QStringLiteral("type 空串应为空串").toUtf8().constData());
        QJsonObject ot;
        QVERIFY2(Integration::alarmToJson(at, &ot),
                 QStringLiteral("type 空串实体应通过 toJson").toUtf8().constData());
        QVERIFY2(ot.value(QStringLiteral("type")).isString() && ot.value(QStringLiteral("type")).toString().isEmpty(),
                 QStringLiteral("必填字符串 type 输出 \"\" 而非 null").toUtf8().constData());
    }

    const JsonCase cases[] = {
        { QStringLiteral("type"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("type"), 2 },
        { QStringLiteral("level"), 5 },
        { QStringLiteral("level"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("level"), QStringLiteral("2") },
        { QStringLiteral("is_handled"), QStringLiteral("true") },
        { QStringLiteral("is_handled"), 1 },
        { QStringLiteral("is_handled"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("create_time"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("device_id"), QStringLiteral("not-a-uuid") },
        { QStringLiteral("target_id"), 42 },
        { QStringLiteral("message"), 42 },
    };
    for (const JsonCase &c : cases) {
        QJsonObject json = validAlarmJson();
        json[c.key] = c.value;
        Core::AlarmInfo al;
        QVERIFY2(!Integration::alarmFromJson(json, &al),
                 QString(QStringLiteral("alarms 非法值应拒收：%1")).arg(c.key).toUtf8().constData());
    }

    // 10 键缺一键即拒收
    const QStringList alarmKeys = validAlarmJson().keys();
    for (const QString &key : alarmKeys) {
        QJsonObject json = validAlarmJson();
        json.remove(key);
        Core::AlarmInfo al;
        QVERIFY2(!Integration::alarmFromJson(json, &al),
                 QString(QStringLiteral("alarms 响应缺键应拒收：%1")).arg(key).toUtf8().constData());
    }
}

// === (7) detection-records 往返与校验 ===
void IntegrationEntityMappingTest::detectionRecordRoundTripAndValidation()
{
    const QJsonObject fixture = validDetectionJson();

    // 正向：9 字段逐一断言
    Integration::DetectionRecordData r;
    QVERIFY2(Integration::detectionRecordFromJson(fixture, &r),
             QStringLiteral("合法 detection-records 响应应通过 fromJson").toUtf8().constData());
    QCOMPARE(r.id, QStringLiteral("b7e2f6a4-3c1d-4a9e-8f5b-2d4e6f8a0c2e"));
    QCOMPARE(r.analyzedAt.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(9, 30, 15), Qt::UTC));
    QCOMPARE(r.hasAnomaly, true);
    QCOMPARE(r.className, QStringLiteral("aircraft-bombs"));
    QCOMPARE(r.confidence, 0.92);
    QCOMPARE(r.rawResult, fixture.value(QStringLiteral("raw_result"))); // QJsonValue 深比较
    QVERIFY2(r.targetId.isEmpty(), QStringLiteral("target_id null 应映射为空串").toUtf8().constData());
    QCOMPARE(r.review, 0);
    QVERIFY2(!r.reviewedAt.isValid(), QStringLiteral("reviewed_at null 应为无效时间").toUtf8().constData());

    QJsonObject out;
    QVERIFY2(Integration::detectionRecordToJson(r, &out),
             QStringLiteral("合法检测记录实体应通过 toJson").toUtf8().constData());
    QCOMPARE(sortedKeyList(out).join(QLatin1Char(',')),
              QStringLiteral("analyzed_at,class_name,confidence,has_anomaly,id,raw_result,review,reviewed_at,target_id"));
    QCOMPARE(out.value(QStringLiteral("raw_result")), fixture.value(QStringLiteral("raw_result")));
    QVERIFY2(out.value(QStringLiteral("target_id")).isNull(), QStringLiteral("空串 target_id 应输出 null").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("reviewed_at")).isNull(), QStringLiteral("无效 reviewed_at 应输出 null").toUtf8().constData());
    QCOMPARE(out.value(QStringLiteral("review")).toInt(), 0);
    QVERIFY2(out.value(QStringLiteral("has_anomaly")).isBool() && out.value(QStringLiteral("has_anomaly")).toBool() == true,
             QStringLiteral("has_anomaly 应输出布尔 true").toUtf8().constData());
    QVERIFY2(out.value(QStringLiteral("class_name")).isString(), QStringLiteral("class_name 应输出字符串").toUtf8().constData());

    // raw_result 任意非空 JSON 值结构保真透传（含再解析往返）
    const QJsonValue rawVariants[] = {
        QJsonValue(QJsonArray{ 1, 2, 3 }),
        QJsonValue(QStringLiteral("raw-text-payload")),
        QJsonValue(0.5),
        QJsonValue(true),
    };
    for (const QJsonValue &v : rawVariants) {
        QJsonObject json = fixture;
        json[QStringLiteral("raw_result")] = v;
        Integration::DetectionRecordData r2;
        QVERIFY2(Integration::detectionRecordFromJson(json, &r2),
                 QStringLiteral("raw_result 非空载荷应通过 fromJson").toUtf8().constData());
        QCOMPARE(r2.rawResult, v);
        QJsonObject out2;
        QVERIFY2(Integration::detectionRecordToJson(r2, &out2),
                 QStringLiteral("raw_result 非空载荷应通过 toJson").toUtf8().constData());
        QCOMPARE(out2.value(QStringLiteral("raw_result")), v);
        Integration::DetectionRecordData r3;
        QVERIFY2(Integration::detectionRecordFromJson(out2, &r3),
                 QStringLiteral("raw_result 输出应可再次 fromJson").toUtf8().constData());
        QCOMPARE(r3.rawResult, v);
    }

    // 默认构造载体：空载荷对象可发，可空字段输出 null/默认值
    {
        Integration::DetectionRecordData fresh;
        fresh.id = QStringLiteral("b7e2f6a4-3c1d-4a9e-8f5b-2d4e6f8a0c2e");
        fresh.analyzedAt = QDateTime(QDate(2026, 10, 4), QTime(9, 30, 15), Qt::UTC);
        QJsonObject out3;
        QVERIFY2(Integration::detectionRecordToJson(fresh, &out3),
                 QStringLiteral("默认构造载体（空载荷）应通过 toJson").toUtf8().constData());
        QVERIFY2(out3.value(QStringLiteral("raw_result")).isObject() && out3.value(QStringLiteral("raw_result")).toObject() == QJsonObject(),
                 QStringLiteral("空载荷对象可发：raw_result 应输出空对象").toUtf8().constData());
        QVERIFY2(out3.value(QStringLiteral("class_name")).isNull(), QStringLiteral("空串 class_name 应输出 null").toUtf8().constData());
        QVERIFY2(out3.value(QStringLiteral("confidence")).isDouble() && out3.value(QStringLiteral("confidence")).toDouble() == 0.0,
                 QStringLiteral("confidence 默认应输出数值 0.0").toUtf8().constData());
        QVERIFY2(out3.value(QStringLiteral("has_anomaly")).isBool() && out3.value(QStringLiteral("has_anomaly")).toBool() == false,
                 QStringLiteral("has_anomaly 默认应输出 false").toUtf8().constData());
        QCOMPARE(out3.value(QStringLiteral("review")).toInt(), 0);
        QVERIFY2(out3.value(QStringLiteral("target_id")).isNull(), QStringLiteral("空串 target_id 应输出 null").toUtf8().constData());
        QVERIFY2(out3.value(QStringLiteral("reviewed_at")).isNull(), QStringLiteral("无效 reviewed_at 应输出 null").toUtf8().constData());
    }

    // 可空字段 null：class_name → 空串、confidence → 0.0
    {
        QJsonObject json = fixture;
        json[QStringLiteral("class_name")] = QJsonValue(QJsonValue::Null);
        json[QStringLiteral("confidence")] = QJsonValue(QJsonValue::Null);
        Integration::DetectionRecordData rn;
        QVERIFY2(Integration::detectionRecordFromJson(json, &rn),
                 QStringLiteral("可空字段 null 应通过 fromJson").toUtf8().constData());
        QVERIFY2(rn.className.isEmpty(), QStringLiteral("class_name null 应映射为空串").toUtf8().constData());
        QCOMPARE(rn.confidence, 0.0);
    }

    const JsonCase cases[] = {
        { QStringLiteral("raw_result"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("review"), 3 },
        { QStringLiteral("review"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("review"), QStringLiteral("0") },
        { QStringLiteral("confidence"), 1.5 },
        { QStringLiteral("confidence"), QStringLiteral("0.9") },
        { QStringLiteral("has_anomaly"), QStringLiteral("true") },
        { QStringLiteral("has_anomaly"), 1 },
        { QStringLiteral("has_anomaly"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("class_name"), 42 },
        { QStringLiteral("target_id"), QStringLiteral("not-a-uuid") },
        { QStringLiteral("analyzed_at"), QJsonValue(QJsonValue::Null) },
        { QStringLiteral("reviewed_at"), QStringLiteral("not-a-timestamp") },
    };
    for (const JsonCase &c : cases) {
        QJsonObject json = fixture;
        json[c.key] = c.value;
        Integration::DetectionRecordData rd;
        QVERIFY2(!Integration::detectionRecordFromJson(json, &rd),
                 QString(QStringLiteral("detection-records 非法值应拒收：%1")).arg(c.key).toUtf8().constData());
    }

    // 9 键缺一键即拒收
    const QStringList detectionKeys = fixture.keys();
    for (const QString &key : detectionKeys) {
        QJsonObject json = fixture;
        json.remove(key);
        Integration::DetectionRecordData rd;
        QVERIFY2(!Integration::detectionRecordFromJson(json, &rd),
                 QString(QStringLiteral("detection-records 响应缺键应拒收：%1")).arg(key).toUtf8().constData());
    }
}

// === (8) UUID 解析与规范化（载体：targets.id） ===
void IntegrationEntityMappingTest::uuidParsingAndNormalization()
{
    const QString canonical = QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90");
    const QString accepts[] = {
        QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90"),               // 小写规范形
        QStringLiteral("3F2A9C1E-8B47-4D62-9A35-1C7D2E5F4A90"),               // 大写十六进制
        QStringLiteral("{3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90}"),             // 花括号形式
        QStringLiteral("  3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90  "),           // 首尾空白
    };
    for (const QString &form : accepts) {
        QJsonObject json = validTargetJson();
        json[QStringLiteral("id")] = form;
        Core::TargetInfo t;
        QVERIFY2(Integration::targetFromJson(json, &t),
                 QString(QStringLiteral("UUID 形式应接受：%1")).arg(form).toUtf8().constData());
        QCOMPARE(t.id, canonical); // 入实体规范化为 36 位小写
        QJsonObject out;
        QVERIFY2(Integration::targetToJson(t, &out),
                 QStringLiteral("规范化 UUID 应通过 toJson").toUtf8().constData());
        QCOMPARE(out.value(QStringLiteral("id")).toString(), canonical);
    }

    const QString rejects[] = {
        QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a9"),                // 35 位（缺 1 位）
        QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a900"),              // 37 位（多 1 位）
        QStringLiteral("3f2a9c1e8b474d629a351c7d2e5f4a90"),                   // 无连字符
        QStringLiteral("3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4zz0"),               // 非十六进制字符
    };
    for (const QString &form : rejects) {
        QJsonObject json = validTargetJson();
        json[QStringLiteral("id")] = form;
        Core::TargetInfo t;
        QVERIFY2(!Integration::targetFromJson(json, &t),
                 QString(QStringLiteral("非法 UUID 应拒收：%1")).arg(form).toUtf8().constData());
    }
}

// === (9) 时间戳解析与规范化（载体：targets.detect_time） ===
void IntegrationEntityMappingTest::timestampParsingAndNormalization()
{
    struct TimeCase {
        QString input;
        QDateTime expected;
        QString expectedString;
    };
    const TimeCase cases[] = {
        { QStringLiteral("2026-10-04T08:30:00Z"),
          QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0), Qt::UTC),
          QStringLiteral("2026-10-04T08:30:00Z") },
        { QStringLiteral("2026-10-04T08:30:00.123Z"),
          QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0, 123), Qt::UTC),
          QStringLiteral("2026-10-04T08:30:00.123Z") },
        { QStringLiteral("2026-10-04T16:30:00.123+08:00"),
          QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0, 123), Qt::UTC),
          QStringLiteral("2026-10-04T08:30:00.123Z") },
        { QStringLiteral("2026-10-04T08:30:00.123"), // 无偏移按 UTC
          QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0, 123), Qt::UTC),
          QStringLiteral("2026-10-04T08:30:00.123Z") },
        { QStringLiteral("2026-10-04T08:30:00"), // 无偏移按 UTC
          QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0), Qt::UTC),
          QStringLiteral("2026-10-04T08:30:00Z") },
    };
    for (const TimeCase &c : cases) {
        QJsonObject json = validTargetJson();
        json[QStringLiteral("detect_time")] = c.input;
        Core::TargetInfo t;
        QVERIFY2(Integration::targetFromJson(json, &t),
                 QString(QStringLiteral("时间戳形式应接受：%1")).arg(c.input).toUtf8().constData());
        QCOMPARE(t.detectTime.toUTC(), c.expected);
        QJsonObject out;
        QVERIFY2(Integration::targetToJson(t, &out),
                 QStringLiteral("已解析时间戳应通过 toJson").toUtf8().constData());
        QCOMPARE(out.value(QStringLiteral("detect_time")).toString(), c.expectedString);
    }

    // 毫秒截断：.123456 → .123（Qt5 QDateTime 仅毫秒精度）
    {
        QJsonObject json = validTargetJson();
        json[QStringLiteral("detect_time")] = QStringLiteral("2026-10-04T08:30:00.123456Z");
        Core::TargetInfo t;
        QVERIFY2(Integration::targetFromJson(json, &t),
                 QStringLiteral("6 位小数秒应接受（截断到毫秒）").toUtf8().constData());
        QCOMPARE(t.detectTime.toUTC(), QDateTime(QDate(2026, 10, 4), QTime(8, 30, 0, 123), Qt::UTC));
        QJsonObject out;
        QVERIFY2(Integration::targetToJson(t, &out),
                 QStringLiteral("截断后的时间戳应通过 toJson").toUtf8().constData());
        QCOMPARE(out.value(QStringLiteral("detect_time")).toString(), QStringLiteral("2026-10-04T08:30:00.123Z"));
    }

    const QString invalids[] = {
        QStringLiteral("2026-13-01T00:00:00Z"), // 月份越界
        QStringLiteral("2026-10-32T00:00:00Z"), // 日期越界
        QStringLiteral("not-a-timestamp"),
        QStringLiteral(""),
    };
    for (const QString &form : invalids) {
        QJsonObject json = validTargetJson();
        json[QStringLiteral("detect_time")] = form;
        Core::TargetInfo t;
        QVERIFY2(!Integration::targetFromJson(json, &t),
                 QString(QStringLiteral("非法时间戳应拒收：%1")).arg(form).toUtf8().constData());
    }
}

// === (10) 词表对齐（契约表 4-1）：9 组 61 个 code 逐一对齐、双向越界拒收 ===
void IntegrationEntityMappingTest::enumVocabularyRangeAlignment()
{
    checkEnumRange<Core::TargetInfo>(validTargetJson(), QStringLiteral("type"), 11,
                                     Integration::targetFromJson, Integration::targetToJson,
                                     targetTypeOf, "targets.type");
    checkEnumRange<Core::TargetInfo>(validTargetJson(), QStringLiteral("threat_level"), 4,
                                     Integration::targetFromJson, Integration::targetToJson,
                                     threatLevelOf, "targets.threat_level");
    checkEnumRange<Core::TargetInfo>(validTargetJson(), QStringLiteral("status"), 6,
                                     Integration::targetFromJson, Integration::targetToJson,
                                     targetStatusOf, "targets.status");
    checkEnumRange<Core::MissionInfo>(validMissionJson(), QStringLiteral("type"), 4,
                                      Integration::missionFromJson, Integration::missionToJson,
                                      missionTypeOf, "missions.type");
    checkEnumRange<Core::MissionInfo>(validMissionJson(), QStringLiteral("status"), 8,
                                      Integration::missionFromJson, Integration::missionToJson,
                                      missionStatusOf, "missions.status");
    checkEnumRange<Core::DeviceInfo>(validDeviceJson(), QStringLiteral("type"), 7,
                                     Integration::deviceFromJson, Integration::deviceToJson,
                                     deviceTypeOf, "devices.type");
    checkEnumRange<Core::DeviceInfo>(validDeviceJson(), QStringLiteral("status"), 6,
                                     Integration::deviceFromJson, Integration::deviceToJson,
                                     deviceStatusOf, "devices.status");
    checkEnumRange<Core::AlarmInfo>(validAlarmJson(), QStringLiteral("level"), 4,
                                    Integration::alarmFromJson, Integration::alarmToJson,
                                    alarmLevelOf, "alarms.level");
    checkEnumRange<Integration::DetectionRecordData>(validDetectionJson(), QStringLiteral("review"), 2,
                                                     Integration::detectionRecordFromJson,
                                                     Integration::detectionRecordToJson,
                                                     detectionReviewOf, "detection_records.review");

    // toJson 侧越界序数拒发：逐一改写并还原，验证每实体只拒对应字段
    {
        Core::TargetInfo t;
        QVERIFY(Integration::targetFromJson(validTargetJson(), &t));
        const Core::TargetType origType = t.type;
        t.type = static_cast<Core::TargetType>(12);
        QJsonObject o;
        QVERIFY2(!Integration::targetToJson(t, &o), QStringLiteral("targets.type 越界序数应被拒发").toUtf8().constData());
        t.type = origType;
        const Core::ThreatLevel origThreat = t.threatLevel;
        t.threatLevel = static_cast<Core::ThreatLevel>(5);
        QVERIFY2(!Integration::targetToJson(t, &o), QStringLiteral("targets.threat_level 越界序数应被拒发").toUtf8().constData());
        t.threatLevel = origThreat;
        const Core::TargetStatus origStatus = t.status;
        t.status = static_cast<Core::TargetStatus>(7);
        QVERIFY2(!Integration::targetToJson(t, &o), QStringLiteral("targets.status 越界序数应被拒发").toUtf8().constData());
        t.status = origStatus;
    }
    {
        Core::MissionInfo m;
        QVERIFY(Integration::missionFromJson(validMissionJson(), &m));
        const Core::MissionType origType = m.type;
        m.type = static_cast<Core::MissionType>(5);
        QJsonObject o;
        QVERIFY2(!Integration::missionToJson(m, &o), QStringLiteral("missions.type 越界序数应被拒发").toUtf8().constData());
        m.type = origType;
        const Core::MissionStatus origStatus = m.status;
        m.status = static_cast<Core::MissionStatus>(9);
        QVERIFY2(!Integration::missionToJson(m, &o), QStringLiteral("missions.status 越界序数应被拒发").toUtf8().constData());
        m.status = origStatus;
    }
    {
        Core::DeviceInfo d;
        QVERIFY(Integration::deviceFromJson(validDeviceJson(), &d));
        const Core::DeviceType origType = d.type;
        d.type = static_cast<Core::DeviceType>(8);
        QJsonObject o;
        QVERIFY2(!Integration::deviceToJson(d, &o), QStringLiteral("devices.type 越界序数应被拒发").toUtf8().constData());
        d.type = origType;
        const Core::DeviceStatus origStatus = d.status;
        d.status = static_cast<Core::DeviceStatus>(7);
        QVERIFY2(!Integration::deviceToJson(d, &o), QStringLiteral("devices.status 越界序数应被拒发").toUtf8().constData());
        d.status = origStatus;
    }
    {
        Core::AlarmInfo a;
        QVERIFY(Integration::alarmFromJson(validAlarmJson(), &a));
        const Core::AlarmLevel origLevel = a.level;
        a.level = static_cast<Core::AlarmLevel>(5);
        QJsonObject o;
        QVERIFY2(!Integration::alarmToJson(a, &o), QStringLiteral("alarms.level 越界序数应被拒发").toUtf8().constData());
        a.level = origLevel;
    }
    {
        Integration::DetectionRecordData r;
        QVERIFY(Integration::detectionRecordFromJson(validDetectionJson(), &r));
        const int origReview = r.review;
        r.review = 3;
        QJsonObject o;
        QVERIFY2(!Integration::detectionRecordToJson(r, &o), QStringLiteral("detection_records.review 越界序数应被拒发").toUtf8().constData());
        r.review = origReview;
    }
}

QTEST_APPLESS_MAIN(IntegrationEntityMappingTest)

#include "integration_entity_mapping_test.moc"
