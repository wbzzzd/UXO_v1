#pragma once

// REQ-012 数据平台接入（W2 HTTP 基础设施）：契约错误模型。
// 依据契约 v1.0.3 §8：错误体形状 {"errors": [{"field", "code", "message"}]}；
// 判断一律以 field + code 为准，message 仅供展示、不得解析；
// 400 聚合错误（errors 数组多条）完整保留。

#include <QByteArray>
#include <QString>
#include <QVector>

namespace Integration {

// 错误码：契约 v1.0.3 §8 全部 10 个错误码 + 客户端内部错误码。
enum class ApiErrorCode {
    // —— 契约错误码（服务端错误体产生） ——
    InvalidEnum,         // invalid_enum（400）：枚举/词表越界
    OutOfRange,          // out_of_range（400）：数值值域越界（经度/纬度/置信度/电量）
    ForeignKeyViolation, // foreign_key_violation（400）：外键引用不存在
    InvalidFormat,       // invalid_format（400）：UUID/时间戳/JSON 格式非法、类型不符、路径与请求体 id 不一致
    NotNull,             // not_null（400）：必填字段缺失
    DuplicateId,         // duplicate_id（409）：主键重复
    NotFound,            // not_found（404）：资源不存在（含未知词表名、未知路径；field 可为 null）
    PaginationInvalid,   // pagination_invalid（400）：分页参数越界
    InternalError,       // internal_error（500）：服务器内部错误（field 为 null）
    DatabaseUnavailable, // database_unavailable（503）：数据库不可达（field 为 null）
    // —— 客户端内部错误码（无 HTTP 响应，httpStatus 为 0） ——
    NetworkError,    // 网络层失败（连接拒绝等）
    Timeout,         // 请求超时
    InvalidResponse, // 响应体非法 JSON 或形状不符契约（客户端回退判定）
    Unknown,         // 未知错误（RED 阶段占位专用，GREEN 实现后不应出现）
};

// 单条字段级错误（契约 §8 errors 数组元素）。
struct ApiErrorItem {
    // 字段名；契约允许 JSON null（internal_error/database_unavailable/未知路径），
    // 本模型统一以空 QString 表示"无字段"。
    QString field;
    ApiErrorCode code = ApiErrorCode::Unknown;
    // 人类可读中文，仅展示用；严禁参与任何逻辑判断（§8）。
    QString message;
};

// 错误解析结果：服务端一次聚合返回的全部字段级错误逐条完整保留（§8，不截断不合并）。
struct ApiError {
    int httpStatus = 0;          // HTTP 状态码；客户端内部错误（网络/超时）为 0
    QVector<ApiErrorItem> items; // 契约错误体至少 1 条；客户端内部错误恰 1 条
};

// 解析契约错误响应体（§8）。
// 返回 true：body 为契约错误形状（JSON 对象、errors 为非空数组、每项 code 为 10 个契约
//           错误码之一、field 为字符串或 null、message 为字符串）；error->items 完整保留
//           全部字段级错误（400 聚合），error->httpStatus = httpStatus。
// 返回 false：body 非法 JSON 或不符契约形状（非对象/无 errors/数组为空/含未知 code/键缺失）；
//            error 回退为单条 InvalidResponse 内部错误项，error->httpStatus = httpStatus。
bool parseApiError(int httpStatus, const QByteArray &body, ApiError *error);

} // namespace Integration
