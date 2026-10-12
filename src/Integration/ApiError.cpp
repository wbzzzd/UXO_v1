#include "Integration/ApiError.h"

// REQ-012 W2 契约错误模型（TDD GREEN）：真实解析实现。
// 依据契约 v1.0.3 §8：
//   - 错误体形状 {"errors": [{"field", "code", "message"}]}，判断一律以 field + code 为准；
//   - 10 个契约错误码逐一映射，message 仅展示、原文透传、不参与逻辑；
//   - 400 聚合错误完整保留（逐条、保序）；
//   - 非法 JSON / 非契约形状 / 未知 code → 整体回退单条 InvalidResponse，httpStatus 保留入参。

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace Integration {

namespace {

// 契约 §8 code 字符串 → 错误码映射表（全部 10 码；未命中即未知码）
struct CodeMapping {
    const char *text;
    ApiErrorCode code;
};

const CodeMapping kCodeMappings[] = {
    {"invalid_enum", ApiErrorCode::InvalidEnum},
    {"out_of_range", ApiErrorCode::OutOfRange},
    {"foreign_key_violation", ApiErrorCode::ForeignKeyViolation},
    {"invalid_format", ApiErrorCode::InvalidFormat},
    {"not_null", ApiErrorCode::NotNull},
    {"duplicate_id", ApiErrorCode::DuplicateId},
    {"not_found", ApiErrorCode::NotFound},
    {"pagination_invalid", ApiErrorCode::PaginationInvalid},
    {"internal_error", ApiErrorCode::InternalError},
    {"database_unavailable", ApiErrorCode::DatabaseUnavailable},
};

bool mapContractCode(const QString &text, ApiErrorCode *out)
{
    for (const CodeMapping &mapping : kCodeMappings) {
        if (text == QLatin1String(mapping.text)) {
            *out = mapping.code;
            return true;
        }
    }
    return false;
}

} // namespace

bool parseApiError(int httpStatus, const QByteArray &body, ApiError *error)
{
    // 统一回退：非法 JSON / 非契约形状 / 未知 code → 单条 InvalidResponse，httpStatus 保留入参
    const auto fallback = [httpStatus, error]() {
        error->httpStatus = httpStatus;
        error->items.clear();
        ApiErrorItem item;
        item.code = ApiErrorCode::InvalidResponse;
        item.message = QStringLiteral("响应体不符合契约错误模型");
        error->items.append(item);
    };

    // 顶层必须为 JSON 对象（非法 JSON 产生空文档，顶层数组均在此拒绝）
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        fallback();
        return false;
    }

    // errors 必须为非空数组（契约错误体至少一条）
    const QJsonValue errors = doc.object().value(QStringLiteral("errors"));
    if (!errors.isArray()) {
        fallback();
        return false;
    }
    const QJsonArray entries = errors.toArray();
    if (entries.isEmpty()) {
        fallback();
        return false;
    }

    // 逐条校验并按序保留：field 字符串或 null、code 为已知 10 码、message 字符串（可为空串）
    QVector<ApiErrorItem> items;
    const int count = entries.size();
    for (int i = 0; i < count; ++i) {
        const QJsonValue entry = entries.at(i);
        if (!entry.isObject()) {
            fallback();
            return false;
        }
        const QJsonObject object = entry.toObject();

        ApiErrorItem item;

        // field：字符串（字段名）或 JSON null（internal_error/database_unavailable/未知路径 → 空字段）；
        // 键缺失或其他类型均视为非契约形状。
        const QJsonValue field = object.value(QStringLiteral("field"));
        if (field.isString()) {
            item.field = field.toString();
        } else if (field.isNull()) {
            item.field = QString();
        } else {
            fallback();
            return false;
        }

        // code：必须为字符串且命中 10 个契约错误码之一
        const QJsonValue code = object.value(QStringLiteral("code"));
        if (!code.isString() || !mapContractCode(code.toString(), &item.code)) {
            fallback();
            return false;
        }

        // message：必须为字符串（可为空串），仅展示用、原文透传
        const QJsonValue message = object.value(QStringLiteral("message"));
        if (!message.isString()) {
            fallback();
            return false;
        }
        item.message = message.toString();

        items.append(item);
    }

    error->httpStatus = httpStatus;
    error->items = items;
    return true;
}

} // namespace Integration
