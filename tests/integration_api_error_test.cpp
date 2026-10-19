// REQ-012 W2 契约错误模型单元测试（TDD RED 阶段：实现为占位，本套件预期失败）。
// 依据契约 v1.0.3 §8：
//   1. 10 个契约错误码全部按 field + code 映射（不解析 message 文本）；
//   2. 400 聚合错误完整保留（errors 数组逐条、保序）；
//   3. field 为 JSON null 时（internal_error/database_unavailable/未知路径）映射为"无字段"（空 QString）；
//   4. 非法 JSON / 非契约形状 / 未知错误码 → 整体回退内部 InvalidResponse，httpStatus 原样保留。
// 载荷使用契约 §8 典型载荷原文或同形构造；纯逻辑测试，不发起网络请求。

#include "Integration/ApiError.h"

#include <QtTest>

namespace {

// 单条映射用例：HTTP 状态码 + 错误体 + 期望的 field（空 = 契约 null）与错误码
struct ErrorCase {
    int httpStatus;
    QByteArray body;
    QString expectedField;
    Integration::ApiErrorCode expectedCode;
};

} // namespace

class IntegrationApiErrorTest : public QObject
{
    Q_OBJECT

private slots:
    void parse400AggregatedErrorsPreserved();
    void parseTypicalPayloadsMapByFieldAndCode();
    void messageDoesNotAffectMapping();
    void parseInvalidJsonFallsBackToInvalidResponse();
    void parseNonContractShapeFallsBackToInvalidResponse();
};

// === 400 聚合：§8 示例原文，两条字段级错误完整保留且保序 ===
void IntegrationApiErrorTest::parse400AggregatedErrorsPreserved()
{
    const QByteArray body = R"({"errors": [)"
        R"({"field": "type", "code": "invalid_enum", "message": "type 必须在 0–11 内"},)"
        R"({"field": "longitude", "code": "out_of_range", "message": "longitude 必须在 -180–180 内"}]})";

    Integration::ApiError error;
    QVERIFY(Integration::parseApiError(400, body, &error));
    QCOMPARE(error.httpStatus, 400);
    QCOMPARE(error.items.size(), 2);
    QCOMPARE(error.items.at(0).field, QStringLiteral("type"));
    QCOMPARE(static_cast<int>(error.items.at(0).code),
             static_cast<int>(Integration::ApiErrorCode::InvalidEnum));
    QCOMPARE(error.items.at(1).field, QStringLiteral("longitude"));
    QCOMPARE(static_cast<int>(error.items.at(1).code),
             static_cast<int>(Integration::ApiErrorCode::OutOfRange));
}

// === 10 个契约错误码逐一映射（§8 典型载荷原文 + 同形构造），以 field + code 断言 ===
void IntegrationApiErrorTest::parseTypicalPayloadsMapByFieldAndCode()
{
    const ErrorCase cases[] = {
        // —— §8 典型载荷原文 ——
        {404, R"({"errors": [{"field": "id", "code": "not_found", "message": "资源不存在"}]})",
            QStringLiteral("id"), Integration::ApiErrorCode::NotFound},
        {404, R"({"errors": [{"field": "name", "code": "not_found", "message": "词表不存在"}]})",
            QStringLiteral("name"), Integration::ApiErrorCode::NotFound},
        {404, R"({"errors": [{"field": null, "code": "not_found", "message": "路径不存在"}]})",
            QString(), Integration::ApiErrorCode::NotFound},
        {409, R"({"errors": [{"field": "id", "code": "duplicate_id", "message": "id 已存在"}]})",
            QStringLiteral("id"), Integration::ApiErrorCode::DuplicateId},
        {400, R"({"errors": [{"field": "page", "code": "pagination_invalid", "message": "page 必须 ≥ 1"}]})",
            QStringLiteral("page"), Integration::ApiErrorCode::PaginationInvalid},
        {500, R"({"errors": [{"field": null, "code": "internal_error", "message": "服务器内部错误"}]})",
            QString(), Integration::ApiErrorCode::InternalError},
        {503, R"({"errors": [{"field": null, "code": "database_unavailable", "message": "数据库不可达"}]})",
            QString(), Integration::ApiErrorCode::DatabaseUnavailable},
        // —— §8 未给载荷原文的三个 400 码与两个值域码：按契约错误体形状构造 ——
        {400, R"({"errors": [{"field": "device_id", "code": "foreign_key_violation", "message": "引用的设备不存在"}]})",
            QStringLiteral("device_id"), Integration::ApiErrorCode::ForeignKeyViolation},
        {400, R"({"errors": [{"field": "id", "code": "invalid_format", "message": "id 必须为 UUID"}]})",
            QStringLiteral("id"), Integration::ApiErrorCode::InvalidFormat},
        {400, R"({"errors": [{"field": "type", "code": "not_null", "message": "type 为必填字段"}]})",
            QStringLiteral("type"), Integration::ApiErrorCode::NotNull},
        {400, R"({"errors": [{"field": "type", "code": "invalid_enum", "message": "type 必须在 0–11 内"}]})",
            QStringLiteral("type"), Integration::ApiErrorCode::InvalidEnum},
        {400, R"({"errors": [{"field": "longitude", "code": "out_of_range", "message": "longitude 必须在 -180–180 内"}]})",
            QStringLiteral("longitude"), Integration::ApiErrorCode::OutOfRange},
    };

    for (const auto &c : cases) {
        Integration::ApiError error;
        const bool ok = Integration::parseApiError(c.httpStatus, c.body, &error);
        const QByteArray ctx = "解析失败: status=" + QByteArray::number(c.httpStatus)
                               + " body=" + c.body;
        QVERIFY2(ok, ctx.constData());
        QCOMPARE(error.httpStatus, c.httpStatus);
        QCOMPARE(error.items.size(), 1);
        QCOMPARE(error.items.at(0).field, c.expectedField);
        QCOMPARE(static_cast<int>(error.items.at(0).code),
                 static_cast<int>(c.expectedCode));
    }
}

// === message 仅供展示：不同 message 文本不影响 field + code 映射，message 原文透传 ===
void IntegrationApiErrorTest::messageDoesNotAffectMapping()
{
    struct MsgCase {
        QByteArray body;
        QString expectedMessage;
    };
    const MsgCase cases[] = {
        {R"({"errors": [{"field": "type", "code": "invalid_enum", "message": "type 必须在 0–11 内"}]})",
            QString::fromUtf8("type 必须在 0–11 内")},
        {R"({"errors": [{"field": "type", "code": "invalid_enum", "message": "任意其他展示文本"}]})",
            QString::fromUtf8("任意其他展示文本")},
        {R"({"errors": [{"field": "type", "code": "invalid_enum", "message": ""}]})",
            QString()},
    };

    for (const auto &c : cases) {
        Integration::ApiError error;
        QVERIFY(Integration::parseApiError(400, c.body, &error));
        QCOMPARE(error.items.size(), 1);
        QCOMPARE(error.items.at(0).field, QStringLiteral("type"));
        // 相同 field + code、不同 message：映射结果一致
        QCOMPARE(static_cast<int>(error.items.at(0).code),
                 static_cast<int>(Integration::ApiErrorCode::InvalidEnum));
        // message 逐字透传，仅作展示用途
        QCOMPARE(error.items.at(0).message, c.expectedMessage);
    }
}

// === 非法 JSON：整体回退内部 InvalidResponse，httpStatus 原样保留 ===
void IntegrationApiErrorTest::parseInvalidJsonFallsBackToInvalidResponse()
{
    Integration::ApiError error;
    QVERIFY(!Integration::parseApiError(400, QByteArray("{not-json"), &error));
    QCOMPARE(error.httpStatus, 400);
    QCOMPARE(error.items.size(), 1);
    QVERIFY(error.items.at(0).field.isEmpty());
    QCOMPARE(static_cast<int>(error.items.at(0).code),
             static_cast<int>(Integration::ApiErrorCode::InvalidResponse));
}

// === 非契约形状 / 未知错误码：整体回退内部 InvalidResponse ===
void IntegrationApiErrorTest::parseNonContractShapeFallsBackToInvalidResponse()
{
    const QByteArray bodies[] = {
        R"({})",                                                            // 无 errors 键
        R"({"errors": {}})",                                                // errors 非数组
        R"({"errors": []})",                                                // 空数组（契约错误体至少一条）
        R"({"errors": [{"field": "type", "code": "made_up_code", "message": "x"}]})", // 未知错误码
        R"({"errors": [{"field": "type", "message": "缺 code 键"}]})",       // 缺 code 键
        R"([{"field": "type", "code": "not_found"}])",                      // 顶层非对象
    };
    for (const QByteArray &body : bodies) {
        Integration::ApiError error;
        const QByteArray ctx = "应当判定为非契约形状: " + body;
        QVERIFY2(!Integration::parseApiError(400, body, &error), ctx.constData());
        QCOMPARE(error.items.size(), 1);
        QCOMPARE(static_cast<int>(error.items.at(0).code),
                 static_cast<int>(Integration::ApiErrorCode::InvalidResponse));
    }
}

QTEST_APPLESS_MAIN(IntegrationApiErrorTest)

#include "integration_api_error_test.moc"
