// REQ-012 W6 契约一致性套件：对本地数据平台实例（默认 http://127.0.0.1:8080，UXO_API_HOST/
// UXO_API_PORT 覆盖）断言契约 v1.0.3 §8 错误模型适配面的剩余缺口（W2/W5 已覆盖
// pagination_invalid/duplicate_id/foreign_key_violation/not_found 主路径）：
//   - invalid_enum：class_name 过滤参数非表 5-1 值（§7 行 429）；
//   - out_of_range：longitude 值域越界（§8 错误码表：经度 -180–180）；
//   - not_null：POST detection-records 缺必填 raw_result（§6.5）；
//   - invalid_format：路径 UUID 非法（§1.5）、布尔过滤 is_handled 仅 true/false（§7 行 429）、
//     类型严格性——整数字段拒字符串/布尔字面量（§1.3；12.0 字面量分支因 Qt5 JSON 数值
//     无整/浮点区分而无法在客户端表达，由服务端保障，见 EntityMapping.h 头注释钉点）；
//   - 400 聚合错误：errors 数组多条完整保留、不截断不合并（§8）；
//   - 404 变体：未知词表名 field="name"、未知路径 field=null（§8 典型载荷）；
//   - PUT 路径 id 与请求体 id 不一致 → invalid_format（§6.0；v1.0.3 判定顺序：路径 id
//     合法且资源存在后才进入请求体校验）。
// 非法载荷经 W2 HttpClient 原始 get/post/put 构造（类型化 DataPlatformApi 结构性阻却
// 非法类型与越界枚举）；合法基线由 W3 targetToJson 生成完整资源表示后翻转单字段。
// internal_error(500)/database_unavailable(503)/健康 503 需停库破坏性操作，不进自动套件
// （手动核验流程见 DEVELOPMENT.md 测试表登记，W6 收口时裁定）。
// 错误断言只看 httpStatus + code + field，不解析 message（§8）。

#include "Integration/DataPlatformApi.h"
#include "Integration/HttpClient.h"

#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QUuid>
#include <QVector3D>
#include <QtTest>

using namespace Integration;

namespace {

// 每次生成全新 36 位小写 UUID（主键客户端生成；行跨运行持久累积，断言用存在性/成员资格）
static QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// 当前 UTC 时间 ISO 8601 Z 含毫秒（原始载荷用；契约 §1.4 三形式之 Z 形式）
static QString isoNow()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

// 错误项集合中是否存在指定 code 的条目（不解析 message）
static bool hasErrorCode(const ApiError &error, ApiErrorCode code)
{
    for (const ApiErrorItem &item : error.items) {
        if (item.code == code)
            return true;
    }
    return false;
}

// 错误项集合中是否存在指定 code + field 的条目（§8：field+code 为定位依据；field=null 为空串）
static bool hasErrorItem(const ApiError &error, ApiErrorCode code, const QString &field)
{
    for (const ApiErrorItem &item : error.items) {
        if (item.code == code && item.field == field)
            return true;
    }
    return false;
}

// targets 起步正典值（与 W5 同源：type=1 航弹、威胁 3、状态 1、契约示例坐标/置信度/备注）
static Core::TargetInfo makeTarget()
{
    Core::TargetInfo target;
    target.id = newId();
    target.type = static_cast<decltype(target.type)>(1);
    target.position = QVector3D(116.397128f, 39.916527f, 43.5f);
    target.depth = 0.0;
    target.attitude = QVector3D(0.0f, 0.0f, 0.0f);
    target.confidence = 0.87;
    target.threatLevel = static_cast<decltype(target.threatLevel)>(3);
    target.status = static_cast<decltype(target.status)>(1);
    target.imagePath = QStringLiteral("/assets/targets/") + target.id + QStringLiteral(".jpg");
    target.detectTime = QDateTime::currentDateTimeUtc();
    target.updateTime = QDateTime::currentDateTimeUtc();
    target.remark = QStringLiteral("跑道东侧疑似航弹");
    return target;
}

// 合法 targets 请求体基线：W3 targetToJson 输出完整资源表示（§1.7 全字段必现），
// 调用方在此基础上翻转单字段构造各类非法载荷
static bool makeTargetBaselineJson(QJsonObject *json)
{
    return targetToJson(makeTarget(), json);
}

} // namespace

class IntegrationContractTest : public QObject
{
    Q_OBJECT

private slots:
    // 实例缺席探测：健康端点传输层失败（连不上/超时）按 S4 环境跳过整套件；其余失败继续执行
    void initTestCase()
    {
        HealthStatus hs;
        ApiError probeError;
        if (!api.health(&hs, &probeError)) {
            for (const ApiErrorItem &item : probeError.items) {
                if (item.code == ApiErrorCode::NetworkError || item.code == ApiErrorCode::Timeout)
                    QSKIP("本地数据平台实例未运行（S4 环境跳过）");
            }
        }
    }

    // (1) 健康检查原始路径锚点：GET /api/v1/health → 200 + 专属形状两字段（§3）
    void healthRawOk()
    {
        const HttpResponse resp = http.get(QStringLiteral("/api/v1/health"));
        QVERIFY2(resp.success, "health 原始 GET 应成功");
        QCOMPARE(resp.httpStatus, 200);
        QCOMPARE(resp.body.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
        QCOMPARE(resp.body.value(QStringLiteral("database")).toString(), QStringLiteral("connected"));
    }

    // (2) invalid_enum：class_name 过滤参数非表 5-1 值 → 400 field=class_name（§7 行 429）
    void errorInvalidEnumClassNameFilter()
    {
        const HttpResponse resp =
            http.get(QStringLiteral("/api/v1/detection-records?class_name=not-a-model-class"));
        QVERIFY(!resp.success);
        QCOMPARE(resp.httpStatus, 400);
        QVERIFY2(hasErrorItem(resp.error, ApiErrorCode::InvalidEnum, QStringLiteral("class_name")),
                 "应含 code=invalid_enum field=class_name");
    }

    // (3) out_of_range：longitude=200 越出 -180–180 → 400 field=longitude（§8 错误码表与示例）
    void errorOutOfRangeLongitude()
    {
        QJsonObject body;
        QVERIFY(makeTargetBaselineJson(&body));
        body[QStringLiteral("longitude")] = 200.0;
        const HttpResponse resp = http.post(QStringLiteral("/api/v1/targets"), body);
        QVERIFY(!resp.success);
        QCOMPARE(resp.httpStatus, 400);
        QVERIFY2(hasErrorItem(resp.error, ApiErrorCode::OutOfRange, QStringLiteral("longitude")),
                 "应含 code=out_of_range field=longitude");
    }

    // (4) not_null：POST detection-records 缺必填 raw_result → 400 field=raw_result（§6.5；
    //     其余必填 id/analyzed_at/has_anomaly 已在场，raw_result 故意缺失）
    void errorNotNullRawResult()
    {
        QJsonObject body;
        body[QStringLiteral("id")] = newId();
        body[QStringLiteral("analyzed_at")] = isoNow();
        body[QStringLiteral("has_anomaly")] = true;
        const HttpResponse resp = http.post(QStringLiteral("/api/v1/detection-records"), body);
        QVERIFY(!resp.success);
        QCOMPARE(resp.httpStatus, 400);
        QVERIFY2(hasErrorItem(resp.error, ApiErrorCode::NotNull, QStringLiteral("raw_result")),
                 "应含 code=not_null field=raw_result");
    }

    // (5) invalid_format 两个变体：
    //     (a) 路径 UUID 非法（§1.5；契约未给该场景 field 示例，确定性断言限 code+状态码）；
    //     (b) 布尔过滤 is_handled=0 非 true/false 字面量 → 400 field=is_handled
    //         （§7 行 429，实现期 curl 双形式实测钉死）
    void errorInvalidFormatVariants()
    {
        Core::TargetInfo out;
        ApiError typedErr;
        QVERIFY(!api.getTarget(QStringLiteral("not-a-uuid"), &out, &typedErr));
        QCOMPARE(typedErr.httpStatus, 400);
        QVERIFY2(hasErrorCode(typedErr, ApiErrorCode::InvalidFormat),
                 "路径 UUID 非法应含 code=invalid_format");

        const HttpResponse resp = http.get(QStringLiteral("/api/v1/alarms?is_handled=0"));
        QVERIFY(!resp.success);
        QCOMPARE(resp.httpStatus, 400);
        QVERIFY2(hasErrorItem(resp.error, ApiErrorCode::InvalidFormat, QStringLiteral("is_handled")),
                 "应含 code=invalid_format field=is_handled");
    }

    // (6) 类型严格性（§1.3）：整数字段 threat_level 拒字符串 "3" 与布尔 true 字面量 →
    //     400 invalid_format field=threat_level（12.0 字面量 Qt5 JSON 不可表达，见文件头注释）
    void errorTypeStrictness()
    {
        QJsonObject stringBody;
        QVERIFY(makeTargetBaselineJson(&stringBody));
        stringBody[QStringLiteral("threat_level")] = QStringLiteral("3");
        const HttpResponse stringResp = http.post(QStringLiteral("/api/v1/targets"), stringBody);
        QVERIFY(!stringResp.success);
        QCOMPARE(stringResp.httpStatus, 400);
        QVERIFY2(hasErrorItem(stringResp.error, ApiErrorCode::InvalidFormat,
                              QStringLiteral("threat_level")),
                 "字符串充整数字段应含 invalid_format field=threat_level");

        QJsonObject boolBody;
        QVERIFY(makeTargetBaselineJson(&boolBody));
        boolBody[QStringLiteral("threat_level")] = true;
        const HttpResponse boolResp = http.post(QStringLiteral("/api/v1/targets"), boolBody);
        QVERIFY(!boolResp.success);
        QCOMPARE(boolResp.httpStatus, 400);
        QVERIFY2(hasErrorItem(boolResp.error, ApiErrorCode::InvalidFormat,
                              QStringLiteral("threat_level")),
                 "布尔充整数字段应含 invalid_format field=threat_level");
    }

    // (7) 400 聚合：type=99（invalid_enum，§8 示例 type 0–11）与 longitude=200（out_of_range）
    //     同体两错一次返回，errors 数组 ≥2 且两码并置完整保留（§8：预校验收集全部字段级错误）；
    //     W3 客户端枚举越界拒发，故必须经原始 POST 构造
    void errorAggregatePreserved()
    {
        QJsonObject body;
        QVERIFY(makeTargetBaselineJson(&body));
        body[QStringLiteral("type")] = 99;
        body[QStringLiteral("longitude")] = 200.0;
        const HttpResponse resp = http.post(QStringLiteral("/api/v1/targets"), body);
        QVERIFY(!resp.success);
        QCOMPARE(resp.httpStatus, 400);
        QVERIFY2(resp.error.items.size() >= 2, "聚合错误应至少 2 条，不得截断合并");
        QVERIFY2(hasErrorItem(resp.error, ApiErrorCode::InvalidEnum, QStringLiteral("type")),
                 "应含 invalid_enum field=type");
        QVERIFY2(hasErrorItem(resp.error, ApiErrorCode::OutOfRange, QStringLiteral("longitude")),
                 "应含 out_of_range field=longitude");
    }

    // (8) 404 变体：未知词表名 field="name"、未知路径 field=null（模型统一为空 QString）
    void errorNotFoundVariants()
    {
        const HttpResponse vocabResp = http.get(QStringLiteral("/api/v1/vocabularies/no_such_vocab"));
        QVERIFY(!vocabResp.success);
        QCOMPARE(vocabResp.httpStatus, 404);
        QVERIFY2(hasErrorItem(vocabResp.error, ApiErrorCode::NotFound, QStringLiteral("name")),
                 "未知词表名应含 not_found field=name");

        const HttpResponse pathResp = http.get(QStringLiteral("/api/v1/no-such-path"));
        QVERIFY(!pathResp.success);
        QCOMPARE(pathResp.httpStatus, 404);
        QVERIFY2(hasErrorItem(pathResp.error, ApiErrorCode::NotFound, QString()),
                 "未知路径应含 not_found field=null");
    }

    // (9) PUT 路径 id ≠ 请求体 id → 400 invalid_format field=id（§6.0）；
    //     v1.0.3 判定顺序：路径 id 合法且资源存在（先经类型化创建真实行）才进入请求体校验
    void errorPutIdMismatch()
    {
        const Core::TargetInfo target = makeTarget();
        ApiError createErr;
        Core::TargetInfo echo;
        QVERIFY2(api.createTarget(target, &echo, &createErr), "前置类型化创建应成功");

        QJsonObject body;
        QVERIFY2(targetToJson(echo, &body), "回显实体应可再序列化");
        body[QStringLiteral("id")] = newId(); // 请求体 id 与路径错配
        const HttpResponse resp = http.put(QStringLiteral("/api/v1/targets/") + target.id, body);
        QVERIFY(!resp.success);
        QCOMPARE(resp.httpStatus, 400);
        QVERIFY2(hasErrorItem(resp.error, ApiErrorCode::InvalidFormat, QStringLiteral("id")),
                 "应含 code=invalid_format field=id");
    }


private:
    // 原始 HTTP 通道（非法载荷构造）；类型化适配面（合法创建与 UUID 路径用例）
    HttpClient http;
    DataPlatformApi api;
};

QTEST_GUILESS_MAIN(IntegrationContractTest)

#include "integration_contract_test.moc"
