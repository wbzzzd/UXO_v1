// REQ-012 W2 HTTP 基础设施单元测试（TDD RED 阶段：实现为占位，本套件预期部分失败）。
// 依据契约 v1.0.3：
//   1. 基址解析：默认 http://127.0.0.1:8080（仅回环）；UXO_API_HOST/UXO_API_PORT 覆盖；
//      端口须为 1–65535 的整数，非法值（缺失/非整数/越界/为空）回退默认 8080；
//   2. 列表统一信封（§1.7/§7）：实体列表 {"data": [...], "meta": {...}} 有分页，
//      词表/映射列表 {"data": [...]} 无 meta 无分页；meta 整数须为 JSON 整数（§1.3 类型严格）；
//   3. 空页语义（§7）：page 超出实际页数时 data 为空数组、meta 照常返回。
// 纯逻辑测试，不发起真实网络请求。

#include "Integration/HttpClient.h"

#include <QtTest>
#include <QJsonDocument>

namespace {

// RAII 环境变量保护：测试内修改，析构时恢复原值，避免用例间污染
class EnvGuard {
public:
    explicit EnvGuard(const char *name)
        : name_(name)
        , had_(qEnvironmentVariableIsSet(name))
        , value_(qEnvironmentVariable(name))
    {
    }
    ~EnvGuard()
    {
        if (had_) {
            qputenv(name_, value_.toUtf8());
        } else {
            qunsetenv(name_);
        }
    }

private:
    const char *name_;
    bool had_;
    QString value_;
};

} // namespace

class IntegrationHttpClientTest : public QObject
{
    Q_OBJECT

private slots:
    void baseAddressDefaultsToLoopback();
    void baseAddressEnvOverride();
    void baseAddressPortBoundariesAreValid();
    void baseAddressInvalidPortFallsBackToDefault();
    void parseEntityListEnvelope();
    void parseVocabularyListEnvelope();
    void parseEmptyPageEnvelope();
    void parseEnvelopeRejectsMalformedBodies();
};

// === 默认基址：仅回环 127.0.0.1:8080（安全边界 A4：默认值仅回环） ===
void IntegrationHttpClientTest::baseAddressDefaultsToLoopback()
{
    EnvGuard hostGuard("UXO_API_HOST");
    EnvGuard portGuard("UXO_API_PORT");
    qunsetenv("UXO_API_HOST");
    qunsetenv("UXO_API_PORT");

    const QUrl base = Integration::resolveBaseAddress();
    QCOMPARE(base.scheme(), QStringLiteral("http"));
    QCOMPARE(base.host(), QStringLiteral("127.0.0.1"));
    QCOMPARE(base.port(), 8080);
    QCOMPARE(base.toString(), QStringLiteral("http://127.0.0.1:8080"));
}

// === 环境变量覆盖：UXO_API_HOST / UXO_API_PORT 显式指定；空主机名视为未设置 ===
void IntegrationHttpClientTest::baseAddressEnvOverride()
{
    EnvGuard hostGuard("UXO_API_HOST");
    EnvGuard portGuard("UXO_API_PORT");
    qputenv("UXO_API_HOST", "192.168.10.20");
    qputenv("UXO_API_PORT", "9000");

    const QUrl base = Integration::resolveBaseAddress();
    QCOMPARE(base.scheme(), QStringLiteral("http"));
    QCOMPARE(base.host(), QStringLiteral("192.168.10.20"));
    QCOMPARE(base.port(), 9000);

    // 空主机名视为未设置，回退默认回环；端口覆盖不受影响
    qputenv("UXO_API_HOST", "");
    const QUrl fallbackHost = Integration::resolveBaseAddress();
    QCOMPARE(fallbackHost.host(), QStringLiteral("127.0.0.1"));
    QCOMPARE(fallbackHost.port(), 9000);
}

// === 端口边界：1 与 65535 均为合法覆盖值 ===
void IntegrationHttpClientTest::baseAddressPortBoundariesAreValid()
{
    EnvGuard hostGuard("UXO_API_HOST");
    EnvGuard portGuard("UXO_API_PORT");
    qunsetenv("UXO_API_HOST");

    const int ports[] = {1, 65535};
    for (int port : ports) {
        qputenv("UXO_API_PORT", QByteArray::number(port));
        const QUrl base = Integration::resolveBaseAddress();
        const QByteArray ctx = "UXO_API_PORT=" + QByteArray::number(port) + " 应被视为合法端口";
        QVERIFY2(base.port() == port, ctx.constData());
        QCOMPARE(base.host(), QStringLiteral("127.0.0.1"));
    }
}

// === 非法端口（越界/非整数/为空）回退默认 8080，且不影响主机覆盖 ===
void IntegrationHttpClientTest::baseAddressInvalidPortFallsBackToDefault()
{
    EnvGuard hostGuard("UXO_API_HOST");
    EnvGuard portGuard("UXO_API_PORT");
    qputenv("UXO_API_HOST", "10.0.0.5");

    const QByteArray badPorts[] = {"0", "65536", "-1", "abc", "80.5", ""};
    for (const QByteArray &port : badPorts) {
        qputenv("UXO_API_PORT", port);
        const QUrl base = Integration::resolveBaseAddress();
        const QByteArray ctx = "UXO_API_PORT=[" + port + "] 应回退默认端口 8080";
        QVERIFY2(base.port() == 8080, ctx.constData());
        // 非法端口不影响主机覆盖
        QCOMPARE(base.host(), QStringLiteral("10.0.0.5"));
    }
}

// === 实体列表信封：data + meta 分页三元组（契约 §7 示例同形） ===
void IntegrationHttpClientTest::parseEntityListEnvelope()
{
    const QByteArray body = R"({
        "data": [
            {"id": "3f2a9c1e-8b47-4d62-9a35-1c7d2e5f4a90"},
            {"id": "9d4c8f2b-1e3a-4b7c-8f5d-0a2b6c4e8d10"}
        ],
        "meta": {"page": 2, "page_size": 50, "total": 123}
    })";

    Integration::ListEnvelope envelope;
    QVERIFY(Integration::parseListEnvelope(QJsonDocument::fromJson(body).object(), &envelope));
    QVERIFY(envelope.hasPagination);
    QCOMPARE(envelope.data.size(), 2);
    QCOMPARE(envelope.page, 2);
    QCOMPARE(envelope.pageSize, 50);
    QCOMPARE(envelope.total, 123);
}

// === 词表列表信封：仅 data，无 meta 无分页（契约 §4.1 示例形状） ===
void IntegrationHttpClientTest::parseVocabularyListEnvelope()
{
    const QByteArray body = R"({
        "data": [
            {
                "vocabulary": "target_type",
                "items": [
                    {"code": 0, "name_en": "Unknown", "name_cn": "未知"},
                    {"code": 1, "name_en": "AirBomb", "name_cn": "航弹"}
                ]
            }
        ]
    })";

    Integration::ListEnvelope envelope;
    QVERIFY(Integration::parseListEnvelope(QJsonDocument::fromJson(body).object(), &envelope));
    QVERIFY(!envelope.hasPagination);
    QCOMPARE(envelope.data.size(), 1);
    QCOMPARE(envelope.page, 0);
    QCOMPARE(envelope.pageSize, 0);
    QCOMPARE(envelope.total, 0);
}

// === 空页语义：page 超出实际页数时 data 为空数组、meta 照常（契约 §7） ===
void IntegrationHttpClientTest::parseEmptyPageEnvelope()
{
    const QByteArray body = R"({"data": [], "meta": {"page": 7, "page_size": 20, "total": 0}})";

    Integration::ListEnvelope envelope;
    QVERIFY(Integration::parseListEnvelope(QJsonDocument::fromJson(body).object(), &envelope));
    QVERIFY(envelope.hasPagination);
    QCOMPARE(envelope.data.size(), 0);
    QCOMPARE(envelope.page, 7);
    QCOMPARE(envelope.pageSize, 20);
    QCOMPARE(envelope.total, 0);
}

// === 非法信封：缺 data / data 非数组 / meta 形状或整数类型不符 → 解析失败 ===
void IntegrationHttpClientTest::parseEnvelopeRejectsMalformedBodies()
{
    const QByteArray bodies[] = {
        R"({})",                                                           // 缺 data 键
        R"({"data": {}})",                                                 // data 非数组
        R"({"data": [], "meta": []})",                                     // meta 非对象
        R"({"data": [], "meta": {"page_size": 20, "total": 1}})",          // meta 缺 page
        R"({"data": [], "meta": {"page": "1", "page_size": 20, "total": 1}})", // page 为字符串（§1.3 类型严格）
        R"({"data": [], "meta": {"page": 1, "page_size": 20}})",           // meta 缺 total
    };
    for (const QByteArray &body : bodies) {
        Integration::ListEnvelope envelope;
        const QByteArray ctx = "应当解析失败: " + body;
        QVERIFY2(!Integration::parseListEnvelope(QJsonDocument::fromJson(body).object(), &envelope),
                 ctx.constData());
    }
}

QTEST_APPLESS_MAIN(IntegrationHttpClientTest)

#include "integration_http_client_test.moc"
