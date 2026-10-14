#pragma once

// REQ-012 数据平台接入（W3 实体映射）：契约 JSON ⇄ Core 实体双向映射。
// 依据契约 v1.0.3 §6（五实体字段表）与 §1.3–§1.7（类型严格、时间戳、UUID、响应包装）：
//   - toJson 输出完整资源表示（与响应同形：全部字段必现、可空字段显式 null，§1.7），
//     可直接用作 POST/PUT 请求体（§6.0 PUT 请求体即完整资源表示）；
//   - fromJson 按响应形状解析：要求全部契约字段在场（§1.7 响应不缺字段），未知字段忽略（§1.3）。
//
// 映射语义钉点：
//   - 整数严格性沿 W2 strictJsonInt 语义：字符串数字（"12"）与非整数值（1.5）拒收。
//     Qt5 QJsonValue 数字统一存储为 double，12 与 12.0 解析后不可区分，整数值 double 按
//     整数接受；契约 §1.3 对 12.0 字面量的拒绝由服务端执行（客户端解析后已无法区分）。
//   - 姿态角轴向：attitude.x=heading、y=pitch、z=roll（与 position x=经度/y=纬度/z=高度 同构）。
//   - 数值可空字段（targets 的 altitude/depth/heading/pitch/roll、devices 的坐标与
//     battery_level、detection-records 的 confidence）：实体字段无 null 态，toJson 恒输出数值，
//     fromJson 接受 null → 0/0.0。唯一例外 missions.result：0 ↔ null（保留字段语义钉点）。
//   - 可空字符串：QString 不区分 null 与空串，空值 toJson 输出 null、fromJson null → 空 QString；
//     必填字符串（devices.name、alarms.type）fromJson null 拒收，空串接受且 toJson 原样输出 ""。
//   - battery_level：契约为整数 0–100，toJson 先四舍五入再校验值域（87.5→88、100.4→100、101 拒）。
//   - UUID：fromJson 接受 36 位（十六进制大小写均可）与花括号两形式、容忍首尾空白（§1.5），
//     入实体时规范化为 36 位小写；toJson 统一输出 36 位小写，非法字符串拒收/拒发。
//   - 时间戳：fromJson 接受三形式（§1.4：Z 含/不含小数秒、显式时区偏移、无偏移按 UTC）；
//     toJson 输出 ISO 8601 UTC Z，无小数秒时省略。Qt5 QDateTime 仅毫秒精度：超过 3 位的
//     小数秒被截断到毫秒（.123456 → .123），契约 6 位上限与超位舍入由服务端存储保证。
//   - 枚举/词表对齐：8 个 Core 枚举 58 个值的序数与词表 code 一一对应（契约表 4-1），
//     双向越界一律拒收；detection-records.review 为 int 载体（词表 detection_review 0–2）。
//   - Core::TargetInfo::typeName 为本地展示字段，契约无对应键：toJson 不输出、fromJson 不回填
//     （词表 name_cn 回填属 W4 词表缓存层）。
//   - detection-records 的 class_name 9 值域校验依赖表 5-1 正典（W4），本层按字符串透传。
//   - raw_result 必填非空（§6.0 必填=非空）：任意非空 JSON 值结构保真透传（§1.3），null 拒收。
//   - Core::DetectionResult 为检测模拟器输出（type/红框/置信度 3 字段），无法承载
//     detection-records 九字段载荷，映射使用 DetectionRecordData 载体（契约 §6.5 字段 1:1）。

#include "Core/Data/Types.h"

#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace Integration {

// 检测记录载体（契约 §6.5 九字段 1:1；Core::DetectionResult 承载不了完整载荷，见文件头注释）
struct DetectionRecordData {
    QString id;                           // 主键 UUID（客户端生成）
    QDateTime analyzedAt;                 // analyzed_at（必填时间戳）
    bool hasAnomaly = false;              // has_anomaly（必填，严格布尔）
    QString className;                    // class_name（可空；9 值域校验属 W4）
    double confidence = 0.0;              // confidence（可空 0–1；null → 0.0）
    QJsonValue rawResult{QJsonObject()};  // raw_result（必填非空；任意非空 JSON 值保真，默认空对象）
    QString targetId;                     // target_id（可空 UUID）
    int review = 0;                       // review（词表 detection_review 0–2）
    QDateTime reviewedAt;                 // reviewed_at（可空时间戳）
};

// 隐患目标（targets，契约 §6.1，17 字段）
bool targetToJson(const Core::TargetInfo &target, QJsonObject *json);
bool targetFromJson(const QJsonObject &json, Core::TargetInfo *target);

// 设备（devices，契约 §6.2，14 字段）
bool deviceToJson(const Core::DeviceInfo &device, QJsonObject *json);
bool deviceFromJson(const QJsonObject &json, Core::DeviceInfo *device);

// 处置任务（missions，契约 §6.3，16 字段）
bool missionToJson(const Core::MissionInfo &mission, QJsonObject *json);
bool missionFromJson(const QJsonObject &json, Core::MissionInfo *mission);

// 告警（alarms，契约 §6.4，10 字段）
bool alarmToJson(const Core::AlarmInfo &alarm, QJsonObject *json);
bool alarmFromJson(const QJsonObject &json, Core::AlarmInfo *alarm);

// 检测记录（detection-records，契约 §6.5，9 字段）
bool detectionRecordToJson(const DetectionRecordData &record, QJsonObject *json);
bool detectionRecordFromJson(const QJsonObject &json, DetectionRecordData *record);

} // namespace Integration
