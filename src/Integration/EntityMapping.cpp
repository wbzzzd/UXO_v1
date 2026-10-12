// REQ-012 数据平台接入（W3 实体映射）：TDD GREEN 实现。
// 契约语义钉点与字段表见 EntityMapping.h 文件头注释，本文件不再重复。

#include "Integration/EntityMapping.h"

#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtMath>

namespace {

// === 整数严格性（沿 W2 strictJsonInt 语义）===
// 字符串数字（"12"）、非整数值（1.5）、非数字类型一律拒收；
// Qt5 QJsonValue 数字统一存储 double，整数值 double（含 12.0）按整数接受。
bool strictJsonInt(const QJsonValue &value, int *out)
{
    if (!value.isDouble())
        return false;
    const double d = value.toDouble();
    if (std::fmod(d, 1.0) != 0.0)
        return false;
    if (d < static_cast<double>(INT_MIN) || d > static_cast<double>(INT_MAX))
        return false;
    *out = static_cast<int>(d);
    return true;
}

// 序数值域检查：lo..hi 闭区间
bool intInRange(int v, int lo, int hi)
{
    return v >= lo && v <= hi;
}

// === UUID（契约 §1.5）===
// 仅接受 36 位连字符形（十六进制大小写均可），容忍首尾空白与花括号包裹；
// 32 位无连字符形、35/37 位、非十六进制字符等一律拒收；入实体规范化为 36 位小写。
bool normalizeUuid(const QString &raw, QString *out)
{
    QString s = raw.trimmed();
    if (s.startsWith(QLatin1Char('{')) && s.endsWith(QLatin1Char('}')))
        s = s.mid(1, s.length() - 2);

    if (s.length() != 36)
        return false;
    static const int dashPos[] = { 8, 13, 18, 23 };
    for (int pos : dashPos) {
        if (s.at(pos) != QLatin1Char('-'))
            return false;
    }
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23)
            continue;
        const QChar c = s.at(i);
        const bool hex = (c >= QLatin1Char('0') && c <= QLatin1Char('9'))
                         || (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
                         || (c >= QLatin1Char('A') && c <= QLatin1Char('F'));
        if (!hex)
            return false;
    }
    *out = s.toLower();
    return true;
}

// 必填 UUID：缺失/非字符串/null/非法字符串 → false
bool requiredUuidFromJson(const QJsonObject &json, const QString &key, QString *out)
{
    const QJsonValue value = json.value(key);
    if (value.isUndefined() || value.isNull() || !value.isString())
        return false;
    return normalizeUuid(value.toString(), out);
}

// 可空 UUID：null → 空串；非空必须合法（否则拒收）
bool nullableUuidFromJson(const QJsonObject &json, const QString &key, QString *out)
{
    const QJsonValue value = json.value(key);
    if (value.isNull())
        return true; // 实体字段无 null 态，保持空串
    if (value.isUndefined() || !value.isString())
        return false;
    return normalizeUuid(value.toString(), out);
}

// 可空 UUID 输出：空串 → null；非空必须合法（否则拒发）
QJsonValue nullableUuidToJson(const QString &uuid, bool *ok)
{
    if (uuid.isEmpty()) {
        *ok = true;
        return QJsonValue(QJsonValue::Null);
    }
    QString normalized;
    if (!normalizeUuid(uuid, &normalized)) {
        *ok = false;
        return QJsonValue(QJsonValue::Null);
    }
    *ok = true;
    return QJsonValue(normalized);
}

// === 字符串 ===
// 必填字符串：缺失/null/非字符串 → false；空串接受
bool requiredStringFromJson(const QJsonObject &json, const QString &key, QString *out)
{
    const QJsonValue value = json.value(key);
    if (value.isUndefined() || value.isNull() || !value.isString())
        return false;
    *out = value.toString();
    return true;
}

// 可空字符串：缺失/null → 空串；非字符串 → false
bool nullableStringFromJson(const QJsonObject &json, const QString &key, QString *out)
{
    const QJsonValue value = json.value(key);
    if (value.isNull())
        return true;
    if (value.isUndefined() || !value.isString())
        return false;
    *out = value.toString();
    return true;
}

// 可空字符串输出：空串 → null（QString 不区分 null 与空串）
QJsonValue nullableStringToJson(const QString &s)
{
    if (s.isEmpty())
        return QJsonValue(QJsonValue::Null);
    return QJsonValue(s);
}

// === 时间戳（契约 §1.4）===
// 三输入形式：Z 含/不含小数秒、显式时区偏移（±HH:MM）、无偏移按 UTC；
// 小数秒超过 3 位截断到毫秒（Qt5 QDateTime 仅毫秒精度，.123456 → .123）。
// 手工截断而非先构造 QDateTime 再截断，避免超位小数秒直接解析失败。
bool parseTimestamp(const QString &raw, QDateTime *out)
{
    QString s = raw.trimmed();
    if (s.isEmpty())
        return false;

    // 拆出小数秒部分：'.' 后连续数字，其后必须是偏移段（Z / ±HH:MM / 空）
    QString fractional;
    {
        const int dotPos = s.indexOf(QLatin1Char('.'));
        if (dotPos >= 0) {
            int end = dotPos + 1;
            while (end < s.length() && s.at(end).isDigit())
                ++end;
            fractional = s.mid(dotPos + 1, end - dotPos - 1);
            s = s.left(dotPos) + s.mid(end);
        }
    }

    // 无偏移按 UTC：QDateTime::fromString 无法区分，这里显式补 Z
    QString offsetPart;
    if (s.endsWith(QLatin1Char('Z'), Qt::CaseInsensitive)) {
        offsetPart = QStringLiteral("Z");
        s.chop(1);
    } else {
        const int plus = s.lastIndexOf(QLatin1Char('+'));
        const int minus = s.lastIndexOf(QLatin1Char('-'));
        const int offPos = qMax(plus, minus);
        if (offPos > 10) { // 日期段内也含 '-'，偏移符号必在时间段之后
            offsetPart = s.mid(offPos);
            s = s.left(offPos);
        }
    }

    // 小数秒截断到毫秒
    int msec = 0;
    if (!fractional.isEmpty()) {
        QString ms = fractional.left(3);
        while (ms.length() < 3)
            ms += QLatin1Char('0');
        msec = ms.toInt();
    }

    const QDateTime parsed = QDateTime::fromString(s, QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
    if (!parsed.isValid())
        return false;
    if (offsetPart.isEmpty() || offsetPart == QLatin1String("Z")) {
        *out = QDateTime(parsed.date(), parsed.time(), Qt::UTC);
    } else {
        // 显式时区偏移：手工解析符号与 HH:MM/HHMM/HH 形式后从 UTC 减去偏移
        // （Qt5 "t" 格式符对数字偏移解析不可靠，手工解析更稳妥）
        if (offsetPart.length() < 2)
            return false;
        const QChar sign = offsetPart.at(0);
        if (sign != QLatin1Char('+') && sign != QLatin1Char('-'))
            return false;
        QString digits = offsetPart.mid(1);
        digits.remove(QLatin1Char(':'));
        for (const QChar &c : digits) {
            if (!c.isDigit())
                return false;
        }
        int hh = 0;
        int mm = 0;
        if (digits.length() == 4) {          // HHMM
            hh = digits.left(2).toInt();
            mm = digits.mid(2).toInt();
        } else if (digits.length() == 2) {   // HH
            hh = digits.toInt();
        } else {
            return false;
        }
        if (hh > 23 || mm > 59)
            return false;
        const int magnitude = hh * 3600 + mm * 60;
        const int offsetSeconds = (sign == QLatin1Char('-')) ? -magnitude : magnitude;
        *out = QDateTime(parsed.date(), parsed.time(), Qt::UTC).addSecs(-offsetSeconds);
    }
    if (msec > 0)
        *out = out->addMSecs(msec);
    return true;
}

// 时间戳输出：UTC "Z"，无小数秒省略；无效 QDateTime → false
bool formatTimestamp(const QDateTime &dt, QJsonValue *out)
{
    if (!dt.isValid())
        return false;
    const QDateTime utc = dt.toUTC();
    QString s = utc.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
    if (utc.time().msec() != 0)
        s += QStringLiteral(".%1").arg(utc.time().msec(), 3, 10, QLatin1Char('0'));
    s += QLatin1Char('Z');
    *out = QJsonValue(s);
    return true;
}

// 必填时间戳：缺失/null/非字符串/非法 → false
bool requiredTimestampFromJson(const QJsonObject &json, const QString &key, QDateTime *out)
{
    const QJsonValue value = json.value(key);
    if (value.isUndefined() || value.isNull() || !value.isString())
        return false;
    return parseTimestamp(value.toString(), out);
}

// 可空时间戳：null → 无效 QDateTime；非空必须合法
bool nullableTimestampFromJson(const QJsonObject &json, const QString &key, QDateTime *out)
{
    const QJsonValue value = json.value(key);
    if (value.isNull())
        return true;
    if (value.isUndefined() || !value.isString())
        return false;
    return parseTimestamp(value.toString(), out);
}

// 可空时间戳输出：无效 → null；有效必须可格式化
QJsonValue nullableTimestampToJson(const QDateTime &dt, bool *ok)
{
    if (!dt.isValid()) {
        *ok = true;
        return QJsonValue(QJsonValue::Null);
    }
    QJsonValue value;
    if (!formatTimestamp(dt, &value)) {
        *ok = false;
        return QJsonValue(QJsonValue::Null);
    }
    *ok = true;
    return value;
}

// === battery_level（devices）===
// 实体为 double：先 qRound 四舍五入（round-half-up）再校验 0–100，输出整数。
bool batteryToJson(int *out, double batteryLevel)
{
    const int rounded = qRound(batteryLevel);
    if (!intInRange(rounded, 0, 100))
        return false;
    *out = rounded;
    return true;
}

// === 数值字段 ===
// 数值可空字段：null → 0/0.0；缺失/非数字 → false
bool nullableDoubleFromJson(const QJsonObject &json, const QString &key, double *out)
{
    const QJsonValue value = json.value(key);
    if (value.isNull())
        return true;
    if (value.isUndefined() || !value.isDouble())
        return false;
    *out = value.toDouble();
    return true;
}

// 必填数值：缺失/null/非数字 → false
bool requiredDoubleFromJson(const QJsonObject &json, const QString &key, double *out)
{
    const QJsonValue value = json.value(key);
    if (value.isUndefined() || value.isNull() || !value.isDouble())
        return false;
    *out = value.toDouble();
    return true;
}

// 必填数值且值域 [lo, hi]
bool requiredDoubleInRangeFromJson(const QJsonObject &json, const QString &key,
                                   double lo, double hi, double *out)
{
    if (!requiredDoubleFromJson(json, key, out))
        return false;
    return *out >= lo && *out <= hi;
}

// === 布尔 ===
// 必填布尔：缺失/null/非布尔 → false（"true" 字符串、1 数字均拒收）
bool requiredBoolFromJson(const QJsonObject &json, const QString &key, bool *out)
{
    const QJsonValue value = json.value(key);
    if (value.isUndefined() || value.isNull() || !value.isBool())
        return false;
    *out = value.toBool();
    return true;
}

// === 词表（契约表 4-1）：8 个 Core 枚举序数与 code 一一对应 ===
// 序数即 code，双向（fromJson code 越界拒收 / toJson 序数越界拒发）。

constexpr int kTargetTypeMax = 11;   // 0..11（Unknown..Fuze，12 值）
constexpr int kThreatLevelMax = 4;   // 0..4
constexpr int kTargetStatusMax = 6;  // 0..6
constexpr int kMissionTypeMax = 4;   // 0..4
constexpr int kMissionStatusMax = 8; // 0..8
constexpr int kDeviceTypeMax = 7;    // 0..7
constexpr int kDeviceStatusMax = 6;  // 0..6
constexpr int kAlarmLevelMax = 4;    // 0..4
constexpr int kDetectionReviewMax = 2; // 词表 detection_review 0..2

// fromJson 侧：code → 序数（越界拒收，序数即 code）
bool enumCodeFromJson(const QJsonObject &json, const QString &key, int maxCode, int *out)
{
    int code = 0;
    if (!strictJsonInt(json.value(key), &code))
        return false;
    if (!intInRange(code, 0, maxCode))
        return false;
    *out = code;
    return true;
}

// toJson 侧：序数 → code（越界拒发）
bool enumCodeToJson(int ordinal, int maxCode, QJsonValue *out)
{
    if (!intInRange(ordinal, 0, maxCode))
        return false;
    *out = QJsonValue(ordinal);
    return true;
}

// === QVector3D（position x=经度/y=纬度/z=高度、attitude x=heading/y=pitch/z=roll）===
// 必填数值向量（targets/devices 的坐标与姿态按字段表逐键处理，此处为公共数值读取）
bool vectorComponentFromJson(const QJsonObject &json, const QString &key, double *out)
{
    return requiredDoubleFromJson(json, key, out);
}

} // namespace

namespace Integration {

// === targets（§6.1，17 字段）===

// 实体 → 完整资源表示：全部字段必现、可空字段显式 null；
// 任何不可表示状态（非法 UUID / 越界序数 / 无效时间 / 越界数值）一律拒发。
bool targetToJson(const Core::TargetInfo &target, QJsonObject *json)
{
    // id：非法拒发，合法规范化为 36 位小写输出
    QString id;
    if (!normalizeUuid(target.id, &id))
        return false;

    // 枚举序数即 code，越界拒发
    QJsonValue typeCode;
    if (!enumCodeToJson(static_cast<int>(target.type), kTargetTypeMax, &typeCode))
        return false;
    QJsonValue threatCode;
    if (!enumCodeToJson(static_cast<int>(target.threatLevel), kThreatLevelMax, &threatCode))
        return false;
    QJsonValue statusCode;
    if (!enumCodeToJson(static_cast<int>(target.status), kTargetStatusMax, &statusCode))
        return false;

    // 经纬度值域与 fromJson 一致；confidence 0–1
    const double longitude = static_cast<double>(target.position.x());
    const double latitude = static_cast<double>(target.position.y());
    if (longitude < -180.0 || longitude > 180.0)
        return false;
    if (latitude < -90.0 || latitude > 90.0)
        return false;
    if (target.confidence < 0.0 || target.confidence > 1.0)
        return false;

    // 必填时间戳：无效时间拒发
    QJsonValue detectValue;
    if (!formatTimestamp(target.detectTime, &detectValue))
        return false;
    QJsonValue updateValue;
    if (!formatTimestamp(target.updateTime, &updateValue))
        return false;

    // 组装完整表示：17 键（type_name 为本地展示字段，不输出）
    QJsonObject out;
    out.insert(QStringLiteral("id"), id);
    out.insert(QStringLiteral("type"), typeCode);
    out.insert(QStringLiteral("longitude"), longitude);
    out.insert(QStringLiteral("latitude"), latitude);
    out.insert(QStringLiteral("altitude"), static_cast<double>(target.position.z()));
    out.insert(QStringLiteral("depth"), target.depth);
    out.insert(QStringLiteral("heading"), static_cast<double>(target.attitude.x()));
    out.insert(QStringLiteral("pitch"), static_cast<double>(target.attitude.y()));
    out.insert(QStringLiteral("roll"), static_cast<double>(target.attitude.z()));
    out.insert(QStringLiteral("confidence"), target.confidence);
    out.insert(QStringLiteral("threat_level"), threatCode);
    out.insert(QStringLiteral("status"), statusCode);
    out.insert(QStringLiteral("image_path"), nullableStringToJson(target.imagePath));
    out.insert(QStringLiteral("radar_data_path"), nullableStringToJson(target.radarDataPath));
    out.insert(QStringLiteral("detect_time"), detectValue);
    out.insert(QStringLiteral("update_time"), updateValue);
    out.insert(QStringLiteral("remark"), nullableStringToJson(target.remark));
    *json = out;
    return true;
}

// 响应形状解析：全部契约字段必在场（§1.7），未知字段忽略（§1.3）；
// type_name 回填属 W4 词表层，本层不回填。
bool targetFromJson(const QJsonObject &json, Core::TargetInfo *target)
{
    Core::TargetInfo t;

    // id：必填 UUID，入实体规范化为 36 位小写
    if (!requiredUuidFromJson(json, QStringLiteral("id"), &t.id))
        return false;

    // type：严格整数 0–11
    int typeCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("type"), kTargetTypeMax, &typeCode))
        return false;
    t.type = static_cast<Core::TargetType>(typeCode);

    // 位置：经度 [-180,180] / 纬度 [-90,90] 必填带值域；高度可空 → 0
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    if (!requiredDoubleInRangeFromJson(json, QStringLiteral("longitude"), -180.0, 180.0, &longitude))
        return false;
    if (!requiredDoubleInRangeFromJson(json, QStringLiteral("latitude"), -90.0, 90.0, &latitude))
        return false;
    if (!nullableDoubleFromJson(json, QStringLiteral("altitude"), &altitude))
        return false;
    t.position = QVector3D(static_cast<float>(longitude), static_cast<float>(latitude),
                           static_cast<float>(altitude));

    // depth 可空 → 0；姿态轴向 x=heading、y=pitch、z=roll，null → 0
    if (!nullableDoubleFromJson(json, QStringLiteral("depth"), &t.depth))
        return false;
    double heading = 0.0;
    double pitch = 0.0;
    double roll = 0.0;
    if (!nullableDoubleFromJson(json, QStringLiteral("heading"), &heading))
        return false;
    if (!nullableDoubleFromJson(json, QStringLiteral("pitch"), &pitch))
        return false;
    if (!nullableDoubleFromJson(json, QStringLiteral("roll"), &roll))
        return false;
    t.attitude = QVector3D(static_cast<float>(heading), static_cast<float>(pitch),
                           static_cast<float>(roll));

    // confidence：必填，0–1
    if (!requiredDoubleInRangeFromJson(json, QStringLiteral("confidence"), 0.0, 1.0, &t.confidence))
        return false;

    // threat_level 0–4、status 0–6：严格整数
    int threatCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("threat_level"), kThreatLevelMax, &threatCode))
        return false;
    t.threatLevel = static_cast<Core::ThreatLevel>(threatCode);
    int statusCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("status"), kTargetStatusMax, &statusCode))
        return false;
    t.status = static_cast<Core::TargetStatus>(statusCode);

    // 可空字符串：null → 空串
    if (!nullableStringFromJson(json, QStringLiteral("image_path"), &t.imagePath))
        return false;
    if (!nullableStringFromJson(json, QStringLiteral("radar_data_path"), &t.radarDataPath))
        return false;
    if (!nullableStringFromJson(json, QStringLiteral("remark"), &t.remark))
        return false;

    // 必填时间戳
    if (!requiredTimestampFromJson(json, QStringLiteral("detect_time"), &t.detectTime))
        return false;
    if (!requiredTimestampFromJson(json, QStringLiteral("update_time"), &t.updateTime))
        return false;

    *target = t;
    return true;
}

// === missions（§6.3，16 字段）===

// 实体 → 完整资源表示：result 0 ↔ null 特例（保留字段语义钉点）；
// 任何不可表示状态（非法必填 UUID / 无效必填时间 / 越界序数 / 非空非法 device_id）拒发。
bool missionToJson(const Core::MissionInfo &mission, QJsonObject *json)
{
    // id / target_id：必填 UUID，非法拒发，规范化 36 位小写输出
    QString id;
    if (!normalizeUuid(mission.id, &id))
        return false;
    QString targetId;
    if (!normalizeUuid(mission.targetId, &targetId))
        return false;

    // 枚举序数即 code，越界拒发
    QJsonValue typeCode;
    if (!enumCodeToJson(static_cast<int>(mission.type), kMissionTypeMax, &typeCode))
        return false;
    QJsonValue statusCode;
    if (!enumCodeToJson(static_cast<int>(mission.status), kMissionStatusMax, &statusCode))
        return false;

    // device_id 可空：空串 → null，非空必须合法
    bool deviceOk = false;
    const QJsonValue deviceIdValue = nullableUuidToJson(mission.deviceId, &deviceOk);
    if (!deviceOk)
        return false;

    // 必填时间戳：无效时间拒发
    QJsonValue createTimeValue;
    if (!formatTimestamp(mission.createTime, &createTimeValue))
        return false;
    QJsonValue updateTimeValue;
    if (!formatTimestamp(mission.updateTime, &updateTimeValue))
        return false;

    // 可空时间：无效 → null
    bool planStartOk = false;
    const QJsonValue planStartValue = nullableTimestampToJson(mission.planStartTime, &planStartOk);
    if (!planStartOk)
        return false;
    bool planEndOk = false;
    const QJsonValue planEndValue = nullableTimestampToJson(mission.planEndTime, &planEndOk);
    if (!planEndOk)
        return false;
    bool actualStartOk = false;
    const QJsonValue actualStartValue = nullableTimestampToJson(mission.actualStartTime, &actualStartOk);
    if (!actualStartOk)
        return false;
    bool actualEndOk = false;
    const QJsonValue actualEndValue = nullableTimestampToJson(mission.actualEndTime, &actualEndOk);
    if (!actualEndOk)
        return false;

    // 组装完整表示：16 键；result 0 → null（0 ↔ null 特例）
    QJsonObject out;
    out.insert(QStringLiteral("id"), id);
    out.insert(QStringLiteral("type"), typeCode);
    out.insert(QStringLiteral("target_id"), targetId);
    out.insert(QStringLiteral("priority"), mission.priority);
    out.insert(QStringLiteral("status"), statusCode);
    out.insert(QStringLiteral("assigner"), nullableStringToJson(mission.assigner));
    out.insert(QStringLiteral("assignee"), nullableStringToJson(mission.assignee));
    out.insert(QStringLiteral("device_id"), deviceIdValue);
    out.insert(QStringLiteral("plan_start"), planStartValue);
    out.insert(QStringLiteral("plan_end"), planEndValue);
    out.insert(QStringLiteral("actual_start"), actualStartValue);
    out.insert(QStringLiteral("actual_end"), actualEndValue);
    out.insert(QStringLiteral("result"),
               mission.result == 0 ? QJsonValue(QJsonValue::Null) : QJsonValue(mission.result));
    out.insert(QStringLiteral("create_time"), createTimeValue);
    out.insert(QStringLiteral("update_time"), updateTimeValue);
    out.insert(QStringLiteral("remark"), nullableStringToJson(mission.remark));
    *json = out;
    return true;
}

// 响应形状解析：全部契约字段必在场（§1.7），未知字段忽略（§1.3）；
// result null → 0，其余严格整数透传。
bool missionFromJson(const QJsonObject &json, Core::MissionInfo *mission)
{
    Core::MissionInfo m;

    // id / target_id：必填 UUID，null 拒收
    if (!requiredUuidFromJson(json, QStringLiteral("id"), &m.id))
        return false;
    if (!requiredUuidFromJson(json, QStringLiteral("target_id"), &m.targetId))
        return false;

    // type 0–4、status 0–8：严格整数
    int typeCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("type"), kMissionTypeMax, &typeCode))
        return false;
    m.type = static_cast<Core::MissionType>(typeCode);
    int statusCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("status"), kMissionStatusMax, &statusCode))
        return false;
    m.status = static_cast<Core::MissionStatus>(statusCode);

    // priority：必填严格整数（契约未钉值域，整数严格性即全部约束）
    if (!strictJsonInt(json.value(QStringLiteral("priority")), &m.priority))
        return false;

    // 可空字符串：null → 空串
    if (!nullableStringFromJson(json, QStringLiteral("assigner"), &m.assigner))
        return false;
    if (!nullableStringFromJson(json, QStringLiteral("assignee"), &m.assignee))
        return false;

    // device_id：可空 UUID，null → 空串，非空非法拒收
    if (!nullableUuidFromJson(json, QStringLiteral("device_id"), &m.deviceId))
        return false;

    // 可空时间：null → 无效 QDateTime
    if (!nullableTimestampFromJson(json, QStringLiteral("plan_start"), &m.planStartTime))
        return false;
    if (!nullableTimestampFromJson(json, QStringLiteral("plan_end"), &m.planEndTime))
        return false;
    if (!nullableTimestampFromJson(json, QStringLiteral("actual_start"), &m.actualStartTime))
        return false;
    if (!nullableTimestampFromJson(json, QStringLiteral("actual_end"), &m.actualEndTime))
        return false;

    // result：null → 0（0 ↔ null 特例），非 null 严格整数透传
    const QJsonValue resultValue = json.value(QStringLiteral("result"));
    if (resultValue.isNull()) {
        m.result = 0;
    } else if (!strictJsonInt(resultValue, &m.result)) {
        return false;
    }

    // 必填时间戳
    if (!requiredTimestampFromJson(json, QStringLiteral("create_time"), &m.createTime))
        return false;
    if (!requiredTimestampFromJson(json, QStringLiteral("update_time"), &m.updateTime))
        return false;

    if (!nullableStringFromJson(json, QStringLiteral("remark"), &m.remark))
        return false;

    *mission = m;
    return true;
}

// === devices（§6.2，14 字段）===

// 实体 → 完整资源表示：battery_level 先四舍五入再校验 0–100，输出整数；
// name 为必填字符串，恒输出字符串（空串原样 ""，不输出 null）；
// 任何不可表示状态（非法 UUID / 越界序数 / 无效必填时间 / battery 越界）拒发。
bool deviceToJson(const Core::DeviceInfo &device, QJsonObject *json)
{
    // id：必填 UUID，非法拒发，规范化 36 位小写输出
    QString id;
    if (!normalizeUuid(device.id, &id))
        return false;

    // 枚举序数即 code，越界拒发
    QJsonValue typeCode;
    if (!enumCodeToJson(static_cast<int>(device.type), kDeviceTypeMax, &typeCode))
        return false;
    QJsonValue statusCode;
    if (!enumCodeToJson(static_cast<int>(device.status), kDeviceStatusMax, &statusCode))
        return false;

    // battery_level：qRound 四舍五入（round-half-up）后校验 0–100，越界拒发
    int battery = 0;
    if (!batteryToJson(&battery, device.batteryLevel))
        return false;

    // 必填时间戳：无效时间拒发
    QJsonValue createTimeValue;
    if (!formatTimestamp(device.createTime, &createTimeValue))
        return false;

    // 可空时间：无效 → null
    bool lastOnlineOk = false;
    const QJsonValue lastOnlineValue = nullableTimestampToJson(device.lastOnlineTime, &lastOnlineOk);
    if (!lastOnlineOk)
        return false;

    // 组装完整表示：14 键
    QJsonObject out;
    out.insert(QStringLiteral("id"), id);
    out.insert(QStringLiteral("type"), typeCode);
    out.insert(QStringLiteral("name"), device.name);
    out.insert(QStringLiteral("manufacturer"), nullableStringToJson(device.manufacturer));
    out.insert(QStringLiteral("model"), nullableStringToJson(device.model));
    out.insert(QStringLiteral("serial_number"), nullableStringToJson(device.serialNumber));
    out.insert(QStringLiteral("status"), statusCode);
    out.insert(QStringLiteral("battery_level"), battery);
    out.insert(QStringLiteral("longitude"), static_cast<double>(device.position.x()));
    out.insert(QStringLiteral("latitude"), static_cast<double>(device.position.y()));
    out.insert(QStringLiteral("altitude"), static_cast<double>(device.position.z()));
    out.insert(QStringLiteral("last_online_time"), lastOnlineValue);
    out.insert(QStringLiteral("create_time"), createTimeValue);
    out.insert(QStringLiteral("remark"), nullableStringToJson(device.remark));
    *json = out;
    return true;
}

// 响应形状解析：全部契约字段必在场（§1.7），未知字段忽略（§1.3）；
// name 为必填字符串：null 拒收、空串接受。
bool deviceFromJson(const QJsonObject &json, Core::DeviceInfo *device)
{
    Core::DeviceInfo d;

    // id：必填 UUID，入实体规范化为 36 位小写
    if (!requiredUuidFromJson(json, QStringLiteral("id"), &d.id))
        return false;

    // type 0–7、status 0–6：严格整数
    int typeCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("type"), kDeviceTypeMax, &typeCode))
        return false;
    d.type = static_cast<Core::DeviceType>(typeCode);
    int statusCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("status"), kDeviceStatusMax, &statusCode))
        return false;
    d.status = static_cast<Core::DeviceStatus>(statusCode);

    // name：必填字符串，null 拒收、空串接受
    if (!requiredStringFromJson(json, QStringLiteral("name"), &d.name))
        return false;

    // 可空字符串：null → 空串
    if (!nullableStringFromJson(json, QStringLiteral("manufacturer"), &d.manufacturer))
        return false;
    if (!nullableStringFromJson(json, QStringLiteral("model"), &d.model))
        return false;
    if (!nullableStringFromJson(json, QStringLiteral("serial_number"), &d.serialNumber))
        return false;
    if (!nullableStringFromJson(json, QStringLiteral("remark"), &d.remark))
        return false;

    // battery_level：可空，null → 0.0；非 null 严格整数且 0–100（87.5 拒收）
    const QJsonValue batteryValue = json.value(QStringLiteral("battery_level"));
    if (batteryValue.isNull()) {
        d.batteryLevel = 0.0;
    } else {
        int batteryInt = 0;
        if (!strictJsonInt(batteryValue, &batteryInt) || !intInRange(batteryInt, 0, 100))
            return false;
        d.batteryLevel = static_cast<double>(batteryInt);
    }

    // 位置：经度 [-180,180] / 纬度 [-90,90] 可空带值域，高度可空；null → 0
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    if (!nullableDoubleFromJson(json, QStringLiteral("longitude"), &longitude))
        return false;
    if (longitude < -180.0 || longitude > 180.0)
        return false;
    if (!nullableDoubleFromJson(json, QStringLiteral("latitude"), &latitude))
        return false;
    if (latitude < -90.0 || latitude > 90.0)
        return false;
    if (!nullableDoubleFromJson(json, QStringLiteral("altitude"), &altitude))
        return false;
    d.position = QVector3D(static_cast<float>(longitude), static_cast<float>(latitude),
                           static_cast<float>(altitude));

    // last_online_time：可空时间，null → 无效 QDateTime
    if (!nullableTimestampFromJson(json, QStringLiteral("last_online_time"), &d.lastOnlineTime))
        return false;

    // create_time：必填时间戳
    if (!requiredTimestampFromJson(json, QStringLiteral("create_time"), &d.createTime))
        return false;

    *device = d;
    return true;
}

// === alarms（§6.4，10 字段）===

// 实体 → 完整资源表示：type 为字符串词表 code，恒输出字符串（空串原样 ""，不输出 null）；
// 任何不可表示状态（非法 UUID / 越界 level / 无效必填时间 / 非空非法可空 UUID）拒发。
bool alarmToJson(const Core::AlarmInfo &alarm, QJsonObject *json)
{
    // id：必填 UUID，非法拒发，规范化 36 位小写输出
    QString id;
    if (!normalizeUuid(alarm.id, &id))
        return false;

    // level 序数即 code，越界拒发
    QJsonValue levelCode;
    if (!enumCodeToJson(static_cast<int>(alarm.level), kAlarmLevelMax, &levelCode))
        return false;

    // target_id / device_id 可空：空串 → null，非空必须合法
    bool targetOk = false;
    const QJsonValue targetIdValue = nullableUuidToJson(alarm.targetId, &targetOk);
    if (!targetOk)
        return false;
    bool deviceOk = false;
    const QJsonValue deviceIdValue = nullableUuidToJson(alarm.deviceId, &deviceOk);
    if (!deviceOk)
        return false;

    // 必填时间戳：无效时间拒发
    QJsonValue createTimeValue;
    if (!formatTimestamp(alarm.createTime, &createTimeValue))
        return false;

    // 可空时间：无效 → null
    bool handleTimeOk = false;
    const QJsonValue handleTimeValue = nullableTimestampToJson(alarm.handleTime, &handleTimeOk);
    if (!handleTimeOk)
        return false;

    // 组装完整表示：10 键
    QJsonObject out;
    out.insert(QStringLiteral("id"), id);
    out.insert(QStringLiteral("type"), alarm.type);
    out.insert(QStringLiteral("level"), levelCode);
    out.insert(QStringLiteral("target_id"), targetIdValue);
    out.insert(QStringLiteral("device_id"), deviceIdValue);
    out.insert(QStringLiteral("message"), nullableStringToJson(alarm.message));
    out.insert(QStringLiteral("is_handled"), alarm.isHandled);
    out.insert(QStringLiteral("handle_time"), handleTimeValue);
    out.insert(QStringLiteral("handle_result"), nullableStringToJson(alarm.handleResult));
    out.insert(QStringLiteral("create_time"), createTimeValue);
    *json = out;
    return true;
}

// 响应形状解析：全部契约字段必在场（§1.7），未知字段忽略（§1.3）；
// type 为必填字符串：null 拒收、空串接受；alarm type 词表校验属 W4，本层透传。
bool alarmFromJson(const QJsonObject &json, Core::AlarmInfo *alarm)
{
    Core::AlarmInfo a;

    // id：必填 UUID，入实体规范化为 36 位小写
    if (!requiredUuidFromJson(json, QStringLiteral("id"), &a.id))
        return false;

    // type：必填字符串，null 拒收、空串接受
    if (!requiredStringFromJson(json, QStringLiteral("type"), &a.type))
        return false;

    // level 0–4：严格整数
    int levelCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("level"), kAlarmLevelMax, &levelCode))
        return false;
    a.level = static_cast<Core::AlarmLevel>(levelCode);

    // target_id / device_id：可空 UUID，null → 空串，非空非法拒收
    if (!nullableUuidFromJson(json, QStringLiteral("target_id"), &a.targetId))
        return false;
    if (!nullableUuidFromJson(json, QStringLiteral("device_id"), &a.deviceId))
        return false;

    // message / handle_result：可空字符串，null → 空串
    if (!nullableStringFromJson(json, QStringLiteral("message"), &a.message))
        return false;
    if (!nullableStringFromJson(json, QStringLiteral("handle_result"), &a.handleResult))
        return false;

    // is_handled：必填严格布尔（"true" 字符串、1 数字均拒收）
    if (!requiredBoolFromJson(json, QStringLiteral("is_handled"), &a.isHandled))
        return false;

    // handle_time：可空时间，null → 无效 QDateTime
    if (!nullableTimestampFromJson(json, QStringLiteral("handle_time"), &a.handleTime))
        return false;

    // create_time：必填时间戳
    if (!requiredTimestampFromJson(json, QStringLiteral("create_time"), &a.createTime))
        return false;

    *alarm = a;
    return true;
}

// === detection-records（§6.5，9 字段，DetectionRecordData 载体）===

// 实体 → 完整资源表示：raw_result 必填非空，任意非空 JSON 值保真透传；
// class_name 9 值域校验属 W4，本层按可空字符串透传；
// 任何不可表示状态（非法 UUID / 无效必填时间 / 越界 review / 空 raw_result）拒发。
bool detectionRecordToJson(const DetectionRecordData &record, QJsonObject *json)
{
    // id：必填 UUID，非法拒发，规范化 36 位小写输出
    QString id;
    if (!normalizeUuid(record.id, &id))
        return false;

    // analyzed_at：必填时间戳，无效时间拒发
    QJsonValue analyzedValue;
    if (!formatTimestamp(record.analyzedAt, &analyzedValue))
        return false;

    // review 序数即 code（词表 detection_review 0–2），越界拒发
    QJsonValue reviewCode;
    if (!enumCodeToJson(record.review, kDetectionReviewMax, &reviewCode))
        return false;

    // target_id 可空：空串 → null，非空必须合法
    bool targetOk = false;
    const QJsonValue targetIdValue = nullableUuidToJson(record.targetId, &targetOk);
    if (!targetOk)
        return false;

    // reviewed_at 可空时间：无效 → null
    bool reviewedOk = false;
    const QJsonValue reviewedAtValue = nullableTimestampToJson(record.reviewedAt, &reviewedOk);
    if (!reviewedOk)
        return false;

    // raw_result：必填非空（§6.0），null/undefined 状态不可表示，拒发
    if (record.rawResult.isNull() || record.rawResult.isUndefined())
        return false;

    // 组装完整表示：9 键
    QJsonObject out;
    out.insert(QStringLiteral("id"), id);
    out.insert(QStringLiteral("analyzed_at"), analyzedValue);
    out.insert(QStringLiteral("has_anomaly"), record.hasAnomaly);
    out.insert(QStringLiteral("class_name"), nullableStringToJson(record.className));
    out.insert(QStringLiteral("confidence"), record.confidence);
    out.insert(QStringLiteral("raw_result"), record.rawResult);
    out.insert(QStringLiteral("target_id"), targetIdValue);
    out.insert(QStringLiteral("review"), reviewCode);
    out.insert(QStringLiteral("reviewed_at"), reviewedAtValue);
    *json = out;
    return true;
}

// 响应形状解析：全部契约字段必在场（§1.7），未知字段忽略（§1.3）；
// raw_result 任意非空 JSON 值保真透传（§1.3），null 拒收。
bool detectionRecordFromJson(const QJsonObject &json, DetectionRecordData *record)
{
    DetectionRecordData r;

    // id：必填 UUID，入实体规范化为 36 位小写
    if (!requiredUuidFromJson(json, QStringLiteral("id"), &r.id))
        return false;

    // analyzed_at：必填时间戳，null 拒收
    if (!requiredTimestampFromJson(json, QStringLiteral("analyzed_at"), &r.analyzedAt))
        return false;

    // has_anomaly：必填严格布尔（"true" 字符串、1 数字均拒收）
    if (!requiredBoolFromJson(json, QStringLiteral("has_anomaly"), &r.hasAnomaly))
        return false;

    // class_name：可空字符串（值域校验属 W4），null → 空串
    if (!nullableStringFromJson(json, QStringLiteral("class_name"), &r.className))
        return false;

    // confidence：可空 0–1，null → 0.0
    double confidence = 0.0;
    if (!nullableDoubleFromJson(json, QStringLiteral("confidence"), &confidence))
        return false;
    if (confidence < 0.0 || confidence > 1.0)
        return false;
    r.confidence = confidence;

    // raw_result：必填非空，任意非空 JSON 值保真透传
    const QJsonValue rawValue = json.value(QStringLiteral("raw_result"));
    if (rawValue.isUndefined() || rawValue.isNull())
        return false;
    r.rawResult = rawValue;

    // target_id：可空 UUID，null → 空串，非空非法拒收
    if (!nullableUuidFromJson(json, QStringLiteral("target_id"), &r.targetId))
        return false;

    // review：严格整数 0–2（词表 detection_review）
    int reviewCode = 0;
    if (!enumCodeFromJson(json, QStringLiteral("review"), kDetectionReviewMax, &reviewCode))
        return false;
    r.review = reviewCode;

    // reviewed_at：可空时间，null → 无效 QDateTime
    if (!nullableTimestampFromJson(json, QStringLiteral("reviewed_at"), &r.reviewedAt))
        return false;

    *record = r;
    return true;
}

} // namespace Integration
