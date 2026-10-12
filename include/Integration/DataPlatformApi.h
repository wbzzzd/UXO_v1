#pragma once

// REQ-012 数据平台接入（W5 五实体 CRUD + 健康检查）：契约 v1.0.3 §2/§3/§6/§7 适配面。
// - 五实体（targets/devices/missions/alarms/detection-records）单条查询、列表（分页+起步过滤集）、
//   创建（POST→201 回显）、全量更新（PUT→200 回显）；v1 无删除端点。
// - 请求体由 W3 toJson 生成完整资源表示（§6.0 PUT 请求体与响应同形）；回显经 W3 fromJson 解析。
// - 更新路径 id 取自实体自身（§6.0 要求路径 id 与请求体 id 一致，本层结构性保证不错配）。
// - 健康检查为专属响应形状（§3：非错误信封），200/503 两路径均解析入 HealthStatus。
// - 错误模型沿 W2 ApiError（field+code 为准，不解析 message；400 聚合完整保留）。

#include "Integration/ApiError.h"
#include "Integration/EntityMapping.h"
#include "Integration/HttpClient.h"

#include "Core/Data/Types.h"

#include <QString>
#include <QVector>

namespace Integration {

// 健康检查结果（契约 §3）：httpStatus 为 200 或 503；status/database 为响应体两字段值。
struct HealthStatus {
    int httpStatus = 0;
    QString status;
    QString database;
};

// 分页参数（契约 §7：page 1 起；page_size 1–100）。越界由服务端判 400 pagination_invalid。
struct PageQuery {
    int page = 1;
    int pageSize = 20;
};

// 分页结果：meta 三字段原样带回（total 为过滤后跨页总数）。
template <typename T>
struct PagedResult {
    QVector<T> items;
    int page = 0;
    int pageSize = 0;
    int total = 0;
};

// 起步过滤集（契约 §7，精确匹配 AND 组合）：
// 整数过滤以 -1 表示未设置（各词表 code 均 ≥0），字符串以空串表示未设置；
// 未设置的参数不出现在查询串。布尔过滤（is_handled/has_anomaly）以 -1 未设 / 0 false / 1 true 表示。
struct TargetFilter {
    int type = -1;
    int status = -1;
    int threatLevel = -1;
};

struct DeviceFilter {
    int type = -1;
    int status = -1;
};

struct MissionFilter {
    int type = -1;
    int status = -1;
    QString targetId;
    QString deviceId;
};

struct AlarmFilter {
    int level = -1;
    int isHandled = -1;
    QString targetId;
    QString deviceId;
};

struct DetectionRecordFilter {
    int review = -1;
    QString className;
    int hasAnomaly = -1;
    QString targetId;
};

// 数据平台 API 适配面（同步；单席位低频，契约 §1.8）。
// 返回 true = 契约成功路径（2xx 且响应解析成功）；false = error 携带契约/内部错误（health 例外见下）。
class DataPlatformApi {
public:
    explicit DataPlatformApi(int timeoutMs = 5000);

    // 健康检查（§3）：200 与 503 均为契约健康形状，解析入 out 并返回 true
    // （就绪判定 = out.status=="ok" && out.database=="connected"，由调用方检查）；
    // 传输失败/非健康形状 → false 且 error 有效。
    bool health(HealthStatus *out, ApiError *error);

    // 隐患目标（§6.1）
    bool listTargets(const TargetFilter &filter, const PageQuery &pageQuery,
                     PagedResult<Core::TargetInfo> *out, ApiError *error);
    bool getTarget(const QString &id, Core::TargetInfo *out, ApiError *error);
    bool createTarget(const Core::TargetInfo &target, Core::TargetInfo *echo, ApiError *error);
    bool updateTarget(const Core::TargetInfo &target, Core::TargetInfo *echo, ApiError *error);

    // 设备（§6.2）
    bool listDevices(const DeviceFilter &filter, const PageQuery &pageQuery,
                     PagedResult<Core::DeviceInfo> *out, ApiError *error);
    bool getDevice(const QString &id, Core::DeviceInfo *out, ApiError *error);
    bool createDevice(const Core::DeviceInfo &device, Core::DeviceInfo *echo, ApiError *error);
    bool updateDevice(const Core::DeviceInfo &device, Core::DeviceInfo *echo, ApiError *error);

    // 处置任务（§6.3）
    bool listMissions(const MissionFilter &filter, const PageQuery &pageQuery,
                      PagedResult<Core::MissionInfo> *out, ApiError *error);
    bool getMission(const QString &id, Core::MissionInfo *out, ApiError *error);
    bool createMission(const Core::MissionInfo &mission, Core::MissionInfo *echo, ApiError *error);
    bool updateMission(const Core::MissionInfo &mission, Core::MissionInfo *echo, ApiError *error);

    // 告警（§6.4）
    bool listAlarms(const AlarmFilter &filter, const PageQuery &pageQuery,
                    PagedResult<Core::AlarmInfo> *out, ApiError *error);
    bool getAlarm(const QString &id, Core::AlarmInfo *out, ApiError *error);
    bool createAlarm(const Core::AlarmInfo &alarm, Core::AlarmInfo *echo, ApiError *error);
    bool updateAlarm(const Core::AlarmInfo &alarm, Core::AlarmInfo *echo, ApiError *error);

    // 检测记录（§6.5）
    bool listDetectionRecords(const DetectionRecordFilter &filter, const PageQuery &pageQuery,
                              PagedResult<DetectionRecordData> *out, ApiError *error);
    bool getDetectionRecord(const QString &id, DetectionRecordData *out, ApiError *error);
    bool createDetectionRecord(const DetectionRecordData &record, DetectionRecordData *echo, ApiError *error);
    bool updateDetectionRecord(const DetectionRecordData &record, DetectionRecordData *echo, ApiError *error);

private:
    HttpClient http_; // 含 QNAM 成员，本类自然不可复制
};

// W1 骨架占位（保留）：返回库版本标识。
const char* dataPlatformApiVersion();

} // namespace Integration
