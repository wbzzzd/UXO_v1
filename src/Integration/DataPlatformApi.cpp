#include "Integration/DataPlatformApi.h"

// REQ-012 数据平台接入（W5 五实体 CRUD + 健康检查）实现。
// 结构：匿名 namespace 通用助手（内部错误构造/查询串拼装/单条与列表收发模板）+ 21 个契约方法。
// 语义钉点见头文件与契约 v1.0.3 §2/§3/§6/§7；服务端错误一律透传 W2 ApiError（field+code 为准）。

#include <QJsonArray>
#include <QUrlQuery>

namespace Integration {

namespace {

// 客户端内部形状错误（2xx 载荷解析失败、信封/行/回显不符契约形状、本地实体无法序列化）：
// 单条 InvalidResponse、httpStatus=0，与 W2 网络/超时内部错误同构（客户端内部错误恰 1 条）。
void setShapeError(ApiError *error, const QString &message)
{
    ApiErrorItem item;
    item.code = ApiErrorCode::InvalidResponse;
    item.message = message;
    error->httpStatus = 0;
    error->items.clear();
    error->items.append(item);
}

// 整型过滤项（含布尔过滤的 0/1 形式）：>= 0 时出串，-1 表示未设置（各词表 code 均 >= 0）。
void addIntFilter(QVector<QPair<QString, QString>> *filters, const char *name, int value)
{
    if (value >= 0) {
        filters->append(qMakePair(QString::fromLatin1(name), QString::number(value)));
    }
}

// 字符串过滤项：非空时出串，空串表示未设置。
void addStringFilter(QVector<QPair<QString, QString>> *filters, const char *name, const QString &value)
{
    if (!value.isEmpty()) {
        filters->append(qMakePair(QString::fromLatin1(name), value));
    }
}

// 布尔过滤项（§7：查询串仅接受 true/false 字面量，0/1 被服务端判 400 invalid_format）：
// API 层哨兵 -1 未设 / 0 false / 1 true，出串时转为字面量。
void addBoolFilter(QVector<QPair<QString, QString>> *filters, const char *name, int value)
{
    if (value >= 0) {
        filters->append(qMakePair(QString::fromLatin1(name),
                                  value ? QStringLiteral("true") : QStringLiteral("false")));
    }
}

// 列表路径与查询串：page/page_size 恒出串（含越界值——越界判定属服务端，
// §7 pagination_invalid 400 由服务端返回，本层不做本地预校验）；过滤参数仅已设置项出串。
QString buildListPath(const QString &resource, const PageQuery &pageQuery,
                      const QVector<QPair<QString, QString>> &filters)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("page"), QString::number(pageQuery.page));
    query.addQueryItem(QStringLiteral("page_size"), QString::number(pageQuery.pageSize));
    for (const QPair<QString, QString> &f : filters) {
        query.addQueryItem(f.first, f.second);
    }
    return QStringLiteral("/api/v1/") + resource + QLatin1Char('?') + query.toString();
}

// 单条查询（GET /api/v1/<resource>/<id>）：2xx 裸对象经 W3 fromJson 解析；
// 非 2xx 透传契约错误；2xx 但形状不符判 InvalidResponse。
template <typename T>
bool doGet(HttpClient &http, const QString &resource, const QString &id,
           bool (*fromJson)(const QJsonObject &, T *), T *out, ApiError *error)
{
    const HttpResponse resp = http.get(QStringLiteral("/api/v1/") + resource + QLatin1Char('/') + id);
    if (!resp.success) {
        *error = resp.error;
        return false;
    }
    if (!fromJson(resp.body, out)) {
        setShapeError(error, QStringLiteral("单条响应不符实体形状"));
        return false;
    }
    return true;
}

// 创建（POST→201 回显）与全量更新（PUT→200 回显）公共骨架：
// 请求体由 W3 toJson 生成完整资源表示（§6.0 PUT 请求体与响应同形）；回显经 W3 fromJson 解析。
// toJson 拒绝（本地实体字段越界/非法 UUID 等）判 InvalidResponse（httpStatus=0，请求未发出）。
template <typename T>
bool doWrite(HttpClient &http, const QString &path, bool usePut,
             bool (*toJson)(const T &, QJsonObject *), const T &entity,
             bool (*fromJson)(const QJsonObject &, T *), T *echo, ApiError *error)
{
    QJsonObject body;
    if (!toJson(entity, &body)) {
        setShapeError(error, QStringLiteral("实体无法序列化为完整资源表示"));
        return false;
    }
    const HttpResponse resp = usePut ? http.put(path, body) : http.post(path, body);
    if (!resp.success) {
        *error = resp.error;
        return false;
    }
    if (!fromJson(resp.body, echo)) {
        setShapeError(error, QStringLiteral("回显响应不符实体形状"));
        return false;
    }
    return true;
}

// 列表查询（GET 信封 {"data":[...],"meta":{...}}）：信封解析沿 W2 parseListEnvelope；
// 实体列表必带分页 meta（§7），缺 meta、行非对象或行形状不符均判 InvalidResponse。
template <typename T>
bool doList(HttpClient &http, const QString &path,
            bool (*fromJson)(const QJsonObject &, T *),
            PagedResult<T> *out, ApiError *error)
{
    const HttpResponse resp = http.get(path);
    if (!resp.success) {
        *error = resp.error;
        return false;
    }
    ListEnvelope envelope;
    if (!parseListEnvelope(resp.body, &envelope) || !envelope.hasPagination) {
        setShapeError(error, QStringLiteral("列表响应不符分页信封形状"));
        return false;
    }
    QVector<T> items;
    items.reserve(envelope.data.size());
    for (const QJsonValue &row : envelope.data) {
        if (!row.isObject()) {
            setShapeError(error, QStringLiteral("列表行非 JSON 对象"));
            return false;
        }
        T item;
        if (!fromJson(row.toObject(), &item)) {
            setShapeError(error, QStringLiteral("列表行不符实体形状"));
            return false;
        }
        items.append(item);
    }
    out->items = items;
    out->page = envelope.page;
    out->pageSize = envelope.pageSize;
    out->total = envelope.total;
    return true;
}

} // namespace

DataPlatformApi::DataPlatformApi(int timeoutMs)
    : http_(timeoutMs)
{
}

bool DataPlatformApi::health(HealthStatus *out, ApiError *error)
{
    const HttpResponse resp = http_.get(QStringLiteral("/api/v1/health"));
    if (resp.success) {
        // 200 路径：§3 专属形状 {"status","database"}（非错误信封、非实体裸对象），逐字段校验
        const QJsonValue status = resp.body.value(QStringLiteral("status"));
        const QJsonValue database = resp.body.value(QStringLiteral("database"));
        if (resp.httpStatus == 200 && status.isString() && database.isString()) {
            out->httpStatus = resp.httpStatus;
            out->status = status.toString();
            out->database = database.toString();
            return true;
        }
        setShapeError(error, QStringLiteral("健康检查响应形状不符 §3"));
        return false;
    }
    // 503 路径：§3 钉点形状 {"status":"unavailable","database":"unreachable"} 非错误信封，
    // W2 对非 2xx 统一按契约错误体解析、形状不符回退 InvalidResponse，原响应体不可得，
    // 故 503 按 §3 钉点形状硬填（就绪判定由调用方检查 status/database 字段值）。
    if (resp.httpStatus == 503) {
        out->httpStatus = 503;
        out->status = QStringLiteral("unavailable");
        out->database = QStringLiteral("unreachable");
        return true;
    }
    *error = resp.error;
    return false;
}

// —— 隐患目标（§6.1） ——

bool DataPlatformApi::listTargets(const TargetFilter &filter, const PageQuery &pageQuery,
                                  PagedResult<Core::TargetInfo> *out, ApiError *error)
{
    QVector<QPair<QString, QString>> filters;
    addIntFilter(&filters, "type", filter.type);
    addIntFilter(&filters, "status", filter.status);
    addIntFilter(&filters, "threat_level", filter.threatLevel);
    return doList(http_, buildListPath(QStringLiteral("targets"), pageQuery, filters),
                  &targetFromJson, out, error);
}

bool DataPlatformApi::getTarget(const QString &id, Core::TargetInfo *out, ApiError *error)
{
    return doGet(http_, QStringLiteral("targets"), id, &targetFromJson, out, error);
}

bool DataPlatformApi::createTarget(const Core::TargetInfo &target, Core::TargetInfo *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/targets"), false,
                   &targetToJson, target, &targetFromJson, echo, error);
}

bool DataPlatformApi::updateTarget(const Core::TargetInfo &target, Core::TargetInfo *echo, ApiError *error)
{
    // §6.0：PUT 路径 id 与请求体 id 必须一致——路径 id 取自实体自身，结构性保证不错配
    return doWrite(http_, QStringLiteral("/api/v1/targets/") + target.id, true,
                   &targetToJson, target, &targetFromJson, echo, error);
}

// —— 设备（§6.2） ——

bool DataPlatformApi::listDevices(const DeviceFilter &filter, const PageQuery &pageQuery,
                                  PagedResult<Core::DeviceInfo> *out, ApiError *error)
{
    QVector<QPair<QString, QString>> filters;
    addIntFilter(&filters, "type", filter.type);
    addIntFilter(&filters, "status", filter.status);
    return doList(http_, buildListPath(QStringLiteral("devices"), pageQuery, filters),
                  &deviceFromJson, out, error);
}

bool DataPlatformApi::getDevice(const QString &id, Core::DeviceInfo *out, ApiError *error)
{
    return doGet(http_, QStringLiteral("devices"), id, &deviceFromJson, out, error);
}

bool DataPlatformApi::createDevice(const Core::DeviceInfo &device, Core::DeviceInfo *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/devices"), false,
                   &deviceToJson, device, &deviceFromJson, echo, error);
}

bool DataPlatformApi::updateDevice(const Core::DeviceInfo &device, Core::DeviceInfo *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/devices/") + device.id, true,
                   &deviceToJson, device, &deviceFromJson, echo, error);
}

// —— 处置任务（§6.3） ——

bool DataPlatformApi::listMissions(const MissionFilter &filter, const PageQuery &pageQuery,
                                   PagedResult<Core::MissionInfo> *out, ApiError *error)
{
    QVector<QPair<QString, QString>> filters;
    addIntFilter(&filters, "type", filter.type);
    addIntFilter(&filters, "status", filter.status);
    addStringFilter(&filters, "target_id", filter.targetId);
    addStringFilter(&filters, "device_id", filter.deviceId);
    return doList(http_, buildListPath(QStringLiteral("missions"), pageQuery, filters),
                  &missionFromJson, out, error);
}

bool DataPlatformApi::getMission(const QString &id, Core::MissionInfo *out, ApiError *error)
{
    return doGet(http_, QStringLiteral("missions"), id, &missionFromJson, out, error);
}

bool DataPlatformApi::createMission(const Core::MissionInfo &mission, Core::MissionInfo *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/missions"), false,
                   &missionToJson, mission, &missionFromJson, echo, error);
}

bool DataPlatformApi::updateMission(const Core::MissionInfo &mission, Core::MissionInfo *echo, ApiError *error)
{
    // §6.0：PUT 路径 id 与请求体 id 必须一致——路径 id 取自实体自身，结构性保证不错配
    return doWrite(http_, QStringLiteral("/api/v1/missions/") + mission.id, true,
                   &missionToJson, mission, &missionFromJson, echo, error);
}

// —— 告警（§6.4） ——

bool DataPlatformApi::listAlarms(const AlarmFilter &filter, const PageQuery &pageQuery,
                                 PagedResult<Core::AlarmInfo> *out, ApiError *error)
{
    QVector<QPair<QString, QString>> filters;
    addIntFilter(&filters, "level", filter.level);
    addBoolFilter(&filters, "is_handled", filter.isHandled); // §7 布尔过滤查询串仅接受 true/false
    addStringFilter(&filters, "target_id", filter.targetId);
    addStringFilter(&filters, "device_id", filter.deviceId);
    return doList(http_, buildListPath(QStringLiteral("alarms"), pageQuery, filters),
                  &alarmFromJson, out, error);
}

bool DataPlatformApi::getAlarm(const QString &id, Core::AlarmInfo *out, ApiError *error)
{
    return doGet(http_, QStringLiteral("alarms"), id, &alarmFromJson, out, error);
}

bool DataPlatformApi::createAlarm(const Core::AlarmInfo &alarm, Core::AlarmInfo *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/alarms"), false,
                   &alarmToJson, alarm, &alarmFromJson, echo, error);
}

bool DataPlatformApi::updateAlarm(const Core::AlarmInfo &alarm, Core::AlarmInfo *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/alarms/") + alarm.id, true,
                   &alarmToJson, alarm, &alarmFromJson, echo, error);
}

// —— 检测记录（§6.5） ——

bool DataPlatformApi::listDetectionRecords(const DetectionRecordFilter &filter, const PageQuery &pageQuery,
                                           PagedResult<DetectionRecordData> *out, ApiError *error)
{
    QVector<QPair<QString, QString>> filters;
    addIntFilter(&filters, "review", filter.review);
    addStringFilter(&filters, "class_name", filter.className);
    addBoolFilter(&filters, "has_anomaly", filter.hasAnomaly); // §7 布尔过滤查询串仅接受 true/false
    addStringFilter(&filters, "target_id", filter.targetId);
    return doList(http_, buildListPath(QStringLiteral("detection-records"), pageQuery, filters),
                  &detectionRecordFromJson, out, error);
}

bool DataPlatformApi::getDetectionRecord(const QString &id, DetectionRecordData *out, ApiError *error)
{
    return doGet(http_, QStringLiteral("detection-records"), id, &detectionRecordFromJson, out, error);
}

bool DataPlatformApi::createDetectionRecord(const DetectionRecordData &record, DetectionRecordData *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/detection-records"), false,
                   &detectionRecordToJson, record, &detectionRecordFromJson, echo, error);
}

bool DataPlatformApi::updateDetectionRecord(const DetectionRecordData &record, DetectionRecordData *echo, ApiError *error)
{
    return doWrite(http_, QStringLiteral("/api/v1/detection-records/") + record.id, true,
                   &detectionRecordToJson, record, &detectionRecordFromJson, echo, error);
}

// W1 骨架占位（保留）：返回库版本标识。
const char* dataPlatformApiVersion()
{
    return "0.1.0-skeleton";
}

} // namespace Integration
