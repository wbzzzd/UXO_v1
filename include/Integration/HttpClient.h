#pragma once

// REQ-012 数据平台接入（W2 HTTP 基础设施）：同步 HTTP 客户端、基址解析与列表统一信封。
// 依据契约 v1.0.3：基址 http://{UXO_API_HOST}:{UXO_API_PORT}（默认 http://127.0.0.1:8080，
// 仅回环为默认部署形态）；路径前缀 /api/v1；列表信封见 §1.7/§7。

#include "Integration/ApiError.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>

namespace Integration {

// 服务层基址解析：
// - UXO_API_HOST 非空时覆盖默认主机（放开非回环属部署侧显式决策；默认值与硬编码仅回环）；
// - UXO_API_PORT 为 1–65535 的整数时覆盖默认端口，否则（缺失/非整数/越界/为空）回退默认 8080；
// - 基址不含路径，/api/v1 前缀由请求构造方拼接（W3+）。
QUrl resolveBaseAddress();

// HTTP 响应结构：success = 收到 2xx 且响应体为合法 JSON 对象；
// 失败时 error 携带契约错误（4xx/5xx 错误体，§8）或客户端内部错误（网络/超时/响应非法）。
struct HttpResponse {
    int httpStatus = 0;  // HTTP 状态码；网络失败/超时为 0
    QJsonObject body;    // success 时有效（契约 §1.7：单资源裸对象 / 列表包装对象）
    bool success = false;
    ApiError error;      // !success 时有效
};

// 列表统一信封（契约 §1.7/§7）：
// - 实体列表：{"data": [...], "meta": {"page", "page_size", "total"}}（有分页）；
// - 词表与映射列表：{"data": [...]}（无 meta、无分页，固定正典集合）。
struct ListEnvelope {
    QJsonArray data;             // 列表行（行内容由 W3+ 映射层解释）
    bool hasPagination = false;  // 实体列表为 true；词表/映射列表为 false
    int page = 0;                // hasPagination 时有效（1 起）
    int pageSize = 0;            // hasPagination 时有效
    int total = 0;               // 过滤后总数（跨页）
};

// 解析列表统一信封。
// 返回 true：body 为 JSON 对象且含 "data" 数组；若含 "meta"，则 meta 须为对象且
//           page/page_size/total 均为 JSON 整数（契约 §1.3 类型严格），填入分页字段；
//           meta 缺失时 hasPagination = false（词表/映射列表形态）。
// 返回 false：缺 "data"、data 非数组、meta 存在但形状或整数类型不符。
bool parseListEnvelope(const QJsonObject &body, ListEnvelope *envelope);

// 同步 HTTP 客户端（契约 v1 端点方法仅为 GET/POST/PUT，无 DELETE）。
// 同步选型说明：指挥席为单席位低频访问（契约 §1.8），同步阻塞式接口可避免异步回调链复杂度；
// 内部以 QEventLoop 等待 QNetworkReply::finished 信号（非忙等、非线程休眠），QTimer 超时
// 与之竞争、先到者胜，超时后 abort 该请求并返回 Timeout 内部错误。
class HttpClient {
public:
    explicit HttpClient(int timeoutMs = 5000); // 默认 5 秒：单席位低频场景的宽松上限

    // pathAndQuery 形如 "/api/v1/targets?type=1&page=1"（含 /api/v1 前缀，由调用方拼接）。
    HttpResponse get(const QString &pathAndQuery);
    HttpResponse post(const QString &path, const QJsonObject &body);
    HttpResponse put(const QString &path, const QJsonObject &body);

private:
    QNetworkAccessManager nam_; // Qt 网络入口；因 QObject 成员，HttpClient 自然不可复制
    int timeoutMs_;
};

} // namespace Integration
