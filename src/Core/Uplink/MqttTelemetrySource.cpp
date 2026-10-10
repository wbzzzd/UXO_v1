// 真实链路遥测数据源实现（REQ-012 M4E WP3）
// 订阅机场网关 MQTT 主题，解析 Cloud API 信封（契约见 docs/interfaces/接口规格-MSDK对接.md）：
// OSD 提取经纬度/高度/航向发 telemetryUpdated；events 按 method 分发 HMS 告警与通用事件。

#include "Core/Uplink/MqttTelemetrySource.h"

#include <cmath>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Core/Uplink/MqttUplinkClient.h"

namespace Core::Uplink {

namespace {
// 契约 §5：客户端按通配符订阅两种上行主题
constexpr char TOPIC_OSD_FILTER[] = "thing/product/+/osd";
constexpr char TOPIC_EVENTS_FILTER[] = "thing/product/+/events";
// 契约 §1：OSD QoS 0（允许丢帧保最新），events QoS 1（必达）
constexpr int OSD_QOS = 0;
constexpr int EVENTS_QOS = 1;
// 契约 §3：MSDK 未定位时返回 (0,0)——双零坐标视为无效帧丢弃
constexpr double INVALID_COORD_EPS = 1e-7;
} // namespace

MqttTelemetrySource::MqttTelemetrySource(QObject *parent)
    : MqttTelemetrySource(QStringLiteral("127.0.0.1"), 1883, parent)
{
}

MqttTelemetrySource::MqttTelemetrySource(const QString &host, quint16 port, QObject *parent)
    : ITelemetrySource(parent)
    , m_running(false)
    , m_linkState(LinkState::Disconnected)
    , m_client(new MqttUplinkClient(host, port, this))
{
    // Paho 回调线程信号跨线程排队投递所需的元数据注册
    qRegisterMetaType<TelemetryData>();
    qRegisterMetaType<LinkState>();
    // WP4（feature §6.4 / 接口规格 §4）：alertReceived 改携自定义 AlertEvent 结构。
    // Paho 回调在 Paho 线程，信号经队列连接跨线程投递自定义类型必须在运行期
    // qRegisterMetaType 注册（Q_DECLARE_METATYPE 仅为编译期声明，队列连接按
    // 类型名查元数据表，未注册会在投递点静默失败）。
    qRegisterMetaType<AlertEvent>();

    connect(m_client, &MqttUplinkClient::connectionEstablished,
            this, &MqttTelemetrySource::onClientConnectionEstablished);
    connect(m_client, &MqttUplinkClient::connectionDropped,
            this, &MqttTelemetrySource::onClientConnectionDropped);
    connect(m_client, &MqttUplinkClient::messageArrived,
            this, &MqttTelemetrySource::onClientMessageArrived);
}

void MqttTelemetrySource::start()
{
    m_running = true;
    if (m_client->isConnected()) {
        setLinkState(LinkState::Connected);
        return;
    }
    setLinkState(LinkState::Connecting);
    // 同步连接失败（拒绝/超时）立即回 Disconnected；成功后的建立事件经队列回调上报
    if (!m_client->connectToBroker()) {
        setLinkState(LinkState::Disconnected);
    }
}

void MqttTelemetrySource::stop()
{
    m_running = false;
    m_client->disconnectFromBroker();
    setLinkState(LinkState::Disconnected);
}

void MqttTelemetrySource::reset()
{
    // 真实链路无内部可重放状态（重放起点即 stop/start 语义），重置为空操作
}

bool MqttTelemetrySource::isRunning() const
{
    return m_running;
}

void MqttTelemetrySource::onClientConnectionEstablished()
{
    if (!m_running) {
        return;
    }
    // 连接建立后订阅契约主题；单项失败仅告警不中断（另一主题仍可用）
    if (!m_client->subscribe(QString::fromLatin1(TOPIC_OSD_FILTER), OSD_QOS)) {
        qWarning() << "[MqttTelemetrySource] OSD 主题订阅失败:" << TOPIC_OSD_FILTER;
    }
    if (!m_client->subscribe(QString::fromLatin1(TOPIC_EVENTS_FILTER), EVENTS_QOS)) {
        qWarning() << "[MqttTelemetrySource] events 主题订阅失败:" << TOPIC_EVENTS_FILTER;
    }
    setLinkState(LinkState::Connected);
}

void MqttTelemetrySource::onClientConnectionDropped(const QString &reason)
{
    Q_UNUSED(reason);
    setLinkState(LinkState::Disconnected);
}

void MqttTelemetrySource::onClientMessageArrived(const QString &topic, const QString &payload)
{
    // 按主题后缀分发（通配订阅前缀为 thing/product/{sn}）
    if (topic.endsWith(QStringLiteral("/osd"))) {
        // WP6 TrackStore：契约主题原始报文 verbatim 直通落库旁路（在语义解析
        // 之前发射，保真优先；帧有效性过滤与列映射由消费方自裁决）
        emit rawMessageReceived(topic, payload.toUtf8());
        handleOsdPayload(payload);
    } else if (topic.endsWith(QStringLiteral("/events"))) {
        emit rawMessageReceived(topic, payload.toUtf8());
        handleEventsPayload(payload);
    }
    // 其他主题（如网关 state 拓扑，契约 §5 注明非契约不消费）忽略
}

void MqttTelemetrySource::handleOsdPayload(const QString &payload)
{
    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject data = doc.object().value(QStringLiteral("data")).toObject();

    // 契约 §2/§3：经纬度缺省或 null 即未定位，丢弃该帧
    const QJsonValue latValue = data.value(QStringLiteral("latitude"));
    const QJsonValue lngValue = data.value(QStringLiteral("longitude"));
    if (!latValue.isDouble() || !lngValue.isDouble()) {
        return;
    }
    const double latitude = latValue.toDouble();
    const double longitude = lngValue.toDouble();
    // 契约 §3：MSDK 未定位哨兵值 (0,0) 视为无效帧
    if (std::fabs(latitude) <= INVALID_COORD_EPS && std::fabs(longitude) <= INVALID_COORD_EPS) {
        return;
    }

    // 高度：优先相对起飞点高度 height，缺省回退海拔 elevation，再缺省 0
    const QJsonValue heightValue = data.value(QStringLiteral("height"));
    const QJsonValue elevationValue = data.value(QStringLiteral("elevation"));
    double altitude = 0.0;
    if (heightValue.isDouble()) {
        altitude = heightValue.toDouble();
    } else if (elevationValue.isDouble()) {
        altitude = elevationValue.toDouble();
    }

    const QJsonValue headingValue = data.value(QStringLiteral("attitude_head"));
    const double heading = headingValue.isDouble() ? headingValue.toDouble() : 0.0;

    emit telemetryUpdated(TelemetryData{latitude, longitude, altitude, heading});
}

void MqttTelemetrySource::handleEventsPayload(const QString &payload)
{
    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject root = doc.object();
    const QString method = root.value(QStringLiteral("method")).toString();
    if (method.isEmpty()) {
        return;
    }
    const QJsonObject data = root.value(QStringLiteral("data")).toObject();

    if (method == QStringLiteral("hms")) {
        // 契约 §4：data.list[] 每项至少含 code（字符串）与 level（0/1/2 → 提示/警告/严重）；
        // 可选字段 args/component_index/sensor_index 为原样透传字段（非可读描述），
        // 故正文沿用级别文案（事件正文显示 code + 文案见 feature §6.4）。
        const QJsonArray list = data.value(QStringLiteral("list")).toArray();
        for (const QJsonValue &item : list) {
            const QJsonObject obj = item.toObject();
            const QJsonValue codeValue = obj.value(QStringLiteral("code"));
            QString code;
            if (codeValue.isString()) {
                code = codeValue.toString();
            } else if (codeValue.isDouble()) {
                // 数值码防御性兼容（契约定义为字符串）
                code = QString::number(static_cast<qint64>(codeValue.toDouble()));
            }
            if (code.isEmpty()) {
                continue;
            }
            // WP4（feature §6.4）：level 数值直传不再折叠进文案，级别→告警档位映射
            // 冻结在 UI 转换槽；字段缺失、非整数或越界（非 0/1/2）按无级别 -1 处理
            const int rawLevel = obj.value(QStringLiteral("level")).toInt(-1);
            const int level = (rawLevel >= 0 && rawLevel <= 2) ? rawLevel : -1;
            QString levelText;
            switch (level) {
            case 0: levelText = QStringLiteral("提示"); break;
            case 1: levelText = QStringLiteral("警告"); break;
            case 2: levelText = QStringLiteral("严重"); break;
            default: levelText = QStringLiteral("未知"); break;
            }
            emit alertReceived(
                AlertEvent{code, level, QStringLiteral("HMS 告警（%1）").arg(levelText)});
        }
        return;
    }

    if (method == QStringLiteral("device_exit_hms")) {
        // 网关既定事件：机身 HMS 告警解除（data 为空，模拟器附录 A 亦发此事件）；
        // 解除通知无级别语义，level=-1
        emit alertReceived(AlertEvent{method, -1, QStringLiteral("机身 HMS 告警已解除")});
        return;
    }

    // 契约 §4：其他 method 允许出现——按通用事件透传，不做语义消费（无级别，level=-1）
    emit alertReceived(AlertEvent{method, -1, QStringLiteral("事件：%1").arg(method)});
}

void MqttTelemetrySource::setLinkState(LinkState state)
{
    if (m_linkState == state) {
        return;
    }
    m_linkState = state;
    emit linkStateChanged(state);
}

} // namespace Core::Uplink
