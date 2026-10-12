#include "Integration/HttpClient.h"

// REQ-012 W2 HTTP 基础设施（TDD GREEN）：真实实现。
// 语义由测试钉死：
//   - 默认基址 http://127.0.0.1:8080 仅回环；非回环必须由 UXO_API_HOST 显式指定（空串视为未设置）；
//   - UXO_API_PORT 须为 1–65535 的整数字面量，非法值（缺失/非整数/越界/为空）回退 8080，
//     且不影响 UXO_API_HOST 覆盖生效；
//   - 列表信封 meta 的 page/page_size/total 用严格整数域校验（字符串 "1"、浮点 1.5 均拒绝）；
//   - 同步请求：QEventLoop 等待 finished（非忙等），QTimer 以 timeoutMs 竞争、先到者胜，
//     超时先触发则 abort 请求并返回 Timeout 内部错误。

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QVariant>

#include <cmath>
#include <limits>

namespace Integration {

namespace {

// 默认端口（与默认回环主机 127.0.0.1 共同构成安全边界：默认部署不指向任何外部地址）
constexpr int kDefaultPort = 8080;

// 请求方法（契约 v1 端点方法仅 GET/POST/PUT，无 DELETE）
enum class HttpMethod {
    Get,
    Post,
    Put,
};

// 构造客户端内部错误（网络/超时/响应非法）：httpStatus 恒 0，恰 1 条
ApiError internalError(ApiErrorCode code, const QString &message)
{
    ApiError error;
    error.httpStatus = 0;
    ApiErrorItem item;
    item.code = code;
    item.message = message;
    error.items.append(item);
    return error;
}

// 完整请求 URL：基址（无路径）+ 调用方拼好的路径与查询串（含 /api/v1 前缀）
QUrl requestUrl(const QString &pathAndQuery)
{
    return QUrl(resolveBaseAddress().toString() + pathAndQuery);
}

// JSON 整数严格校验（契约 §1.3 类型严格；备忘录第 1 条：Qt JSON 数字是 double，
// 禁止 toInt() 静默截断）：键必须存在且为整数值（isDouble 且无小数部分且在 int 域内）。
// 字符串 "1"、null、缺失、浮点 1.5、超 int 域的值均拒绝。
bool strictJsonInt(const QJsonObject &object, const QString &key, int *out)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return false;
    }
    const double d = value.toDouble();
    if (std::floor(d) != d
        || d < static_cast<double>(std::numeric_limits<int>::min())
        || d > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    *out = static_cast<int>(d);
    return true;
}

// 同步执行单个请求。
// 选型理由：指挥席为单席位低频访问（契约 §1.8），同步阻塞式接口避免异步回调链复杂度；
// QEventLoop 等待 QNetworkReply::finished（事件循环等待、非忙等、非线程休眠），
// QTimer 超时与之竞争、先到者胜；超时先触发则 abort 请求并返回 Timeout。
HttpResponse runSync(QNetworkAccessManager *nam, int timeoutMs,
                     const QNetworkRequest &request, HttpMethod method,
                     const QByteArray &body)
{
    QNetworkReply *reply = nullptr;
    switch (method) {
    case HttpMethod::Get:
        reply = nam->get(request);
        break;
    case HttpMethod::Post:
        reply = nam->post(request, body);
        break;
    case HttpMethod::Put:
        reply = nam->put(request, body);
        break;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();
    // 定时器仍活跃 = finished 先到；定时器已停 = 超时先到
    const bool timedOut = !timer.isActive();
    timer.stop();

    HttpResponse response;
    if (timedOut) {
        reply->abort();
        reply->deleteLater();
        response.error = internalError(ApiErrorCode::Timeout,
                                        QStringLiteral("请求超时（%1 毫秒）").arg(timeoutMs));
        return response;
    }

    const QByteArray rawBody = reply->readAll();
    // 传输层失败（连接拒绝/DNS 等）时无 HTTP 状态码，attribute 为无效值 → 0
    const int status =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    if (status == 0) {
        // 网络层失败：无 HTTP 响应，httpStatus 为 0
        response.error = internalError(ApiErrorCode::NetworkError,
                                        QStringLiteral("网络层请求失败（无 HTTP 响应）"));
        return response;
    }

    response.httpStatus = status;
    if (status >= 200 && status <= 299) {
        // 2xx：响应体必须为合法 JSON 对象（契约 §1.7 裸对象/包装对象）
        const QJsonDocument doc = QJsonDocument::fromJson(rawBody);
        if (doc.isObject()) {
            response.body = doc.object();
            response.success = true;
        } else {
            // 2xx 但响应体非 JSON 对象：内部 InvalidResponse（httpStatus 保留实际状态码）
            response.error = internalError(ApiErrorCode::InvalidResponse,
                                            QStringLiteral("2xx 响应体不是 JSON 对象"));
            response.error.httpStatus = status;
        }
        return response;
    }

    // 4xx/5xx：按契约 §8 解析错误体（契约形状保留条目；非契约形状由 parseApiError
    // 自行回退 InvalidResponse，httpStatus 均保留）
    parseApiError(status, rawBody, &response.error);
    return response;
}

} // namespace

QUrl resolveBaseAddress()
{
    // 主机：UXO_API_HOST 非空时覆盖默认回环；空串视为未设置
    QString host = QStringLiteral("127.0.0.1");
    const QString envHost = qEnvironmentVariable("UXO_API_HOST");
    if (!envHost.isEmpty()) {
        host = envHost;
    }

    // 端口：UXO_API_PORT 须为 1–65535 的整数字面量。
    // toLongLong 为全串解析："abc"/"80.5"/"" 均解析失败（ok=false）；
    // "-1"/"0"/"65536" 解析成功但越界，由 1–65535 域校验拒绝；两者均回退默认 8080。
    int port = kDefaultPort;
    const QString envPort = qEnvironmentVariable("UXO_API_PORT");
    bool ok = false;
    const long long parsed = envPort.toLongLong(&ok);
    if (ok && parsed >= 1 && parsed <= 65535) {
        port = static_cast<int>(parsed);
    }

    // 基址不含路径（/api/v1 前缀由请求构造方拼接）
    QUrl base;
    base.setScheme(QStringLiteral("http"));
    base.setHost(host);
    base.setPort(port);
    return base;
}

bool parseListEnvelope(const QJsonObject &body, ListEnvelope *envelope)
{
    // 输出先复位：失败时保持零值，成功时填入
    *envelope = ListEnvelope();

    // data 必须为数组（缺失、非数组均拒绝）
    const QJsonValue data = body.value(QStringLiteral("data"));
    if (!data.isArray()) {
        return false;
    }

    // meta 缺失 → 词表/映射列表形态（契约 §1.7）：无分页，仍算成功
    const QJsonValue meta = body.value(QStringLiteral("meta"));
    if (meta.isUndefined()) {
        envelope->data = data.toArray();
        envelope->hasPagination = false;
        return true;
    }

    // meta 存在 → 必须为对象，且 page/page_size/total 三个键全为 JSON 整数（§1.3 类型严格）
    if (!meta.isObject()) {
        return false;
    }
    const QJsonObject metaObject = meta.toObject();
    int page = 0;
    int pageSize = 0;
    int total = 0;
    if (!strictJsonInt(metaObject, QStringLiteral("page"), &page)
        || !strictJsonInt(metaObject, QStringLiteral("page_size"), &pageSize)
        || !strictJsonInt(metaObject, QStringLiteral("total"), &total)) {
        return false;
    }

    envelope->data = data.toArray();
    envelope->hasPagination = true;
    envelope->page = page;
    envelope->pageSize = pageSize;
    envelope->total = total;
    return true;
}

HttpClient::HttpClient(int timeoutMs)
    : timeoutMs_(timeoutMs)
{
}

HttpResponse HttpClient::get(const QString &pathAndQuery)
{
    return runSync(&nam_, timeoutMs_, QNetworkRequest(requestUrl(pathAndQuery)),
                   HttpMethod::Get, QByteArray());
}

HttpResponse HttpClient::post(const QString &path, const QJsonObject &body)
{
    QNetworkRequest request(requestUrl(path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    return runSync(&nam_, timeoutMs_, request, HttpMethod::Post,
                   QJsonDocument(body).toJson(QJsonDocument::Compact));
}

HttpResponse HttpClient::put(const QString &path, const QJsonObject &body)
{
    QNetworkRequest request(requestUrl(path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    return runSync(&nam_, timeoutMs_, request, HttpMethod::Put,
                   QJsonDocument(body).toJson(QJsonDocument::Compact));
}

} // namespace Integration
