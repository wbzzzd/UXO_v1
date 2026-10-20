// MQTT 上行链路客户端实现（REQ-015 M4E WP3）
// 同步 connect/subscribe + 超时兜底；Paho 回调经 QMetaObject::invokeMethod
// 排队回 Qt 事件循环，保证上层信号全部在 Qt 主线程触发。

#include "Core/Uplink/MqttUplinkClient.h"

#include <chrono>

#include <QCoreApplication>
#include <QDebug>

#include <mqtt/async_client.h>

namespace Core::Uplink {

namespace {

// 连接 MQTT 服务器的同步等待超时（毫秒）
constexpr int CONNECT_TIMEOUT_MS = 3000;
// 断开连接的同步等待超时（毫秒）
constexpr int DISCONNECT_TIMEOUT_MS = 1000;
// 订阅主题的同步等待超时（毫秒）
constexpr int SUBSCRIBE_TIMEOUT_MS = 2000;
// MQTT keep-alive 心跳间隔（秒）
constexpr int KEEP_ALIVE_SEC = 60;
// MQTT 协议层连接超时（秒）
constexpr int CONNECT_TIMEOUT_SEC = 3;

}

MqttUplinkClient::MqttUplinkClient(const QString &host, quint16 port, QObject *parent)
    : QObject(parent)
{
    const QString serverUri = QStringLiteral("tcp://%1:%2").arg(host).arg(port);
    // 客户端 ID 附进程号，避免同机多实例互踢
    const QString clientId = QStringLiteral("UXOMissionControl-%1")
                                 .arg(QCoreApplication::applicationPid());
    // 第三参传 nullptr：使用内存会话（无持久化），断开后不保留订阅状态
    m_client = std::make_unique<mqtt::async_client>(serverUri.toStdString(),
                                                     clientId.toStdString(),
                                                     nullptr);
    m_client->set_callback(*this);
}

MqttUplinkClient::~MqttUplinkClient()
{
    // 析构时尽力同步断开，避免 Paho 回调线程访问已销毁对象
    try {
        if (m_client && m_client->is_connected()) {
            m_client->disconnect()->wait_for(std::chrono::milliseconds(DISCONNECT_TIMEOUT_MS));
        }
    } catch (...) {
        // 析构阶段不允许抛出异常
    }
}

bool MqttUplinkClient::connectToBroker()
{
    if (!m_client) {
        return false;
    }
    if (m_client->is_connected()) {
        return true;
    }

    try {
        mqtt::connect_options opts;
        opts.set_clean_session(true);
        opts.set_keep_alive_interval(KEEP_ALIVE_SEC);
        opts.set_connect_timeout(CONNECT_TIMEOUT_SEC);

        if (!m_client->connect(opts)->wait_for(std::chrono::milliseconds(CONNECT_TIMEOUT_MS))) {
            qWarning() << "[MqttUplinkClient] 连接 MQTT 服务器超时";
            // 超时后连接可能仍在后台进行，尽力清理半开连接
            try {
                m_client->disconnect()->wait_for(std::chrono::milliseconds(DISCONNECT_TIMEOUT_MS));
            } catch (...) {
                // 忽略清理阶段异常
            }
            return false;
        }
        return true;
    } catch (const mqtt::exception &e) {
        qWarning() << "[MqttUplinkClient] 连接 MQTT 服务器失败:" << e.what();
        return false;
    } catch (const std::exception &e) {
        qWarning() << "[MqttUplinkClient] 连接 MQTT 服务器异常:" << e.what();
        return false;
    }
}

void MqttUplinkClient::disconnectFromBroker()
{
    if (!m_client || !m_client->is_connected()) {
        return;
    }

    try {
        m_client->disconnect()->wait_for(std::chrono::milliseconds(DISCONNECT_TIMEOUT_MS));
    } catch (const mqtt::exception &e) {
        qWarning() << "[MqttUplinkClient] 断开 MQTT 连接失败:" << e.what();
    } catch (...) {
        // 忽略断开阶段异常
    }
}

bool MqttUplinkClient::isConnected() const
{
    return m_client && m_client->is_connected();
}

bool MqttUplinkClient::subscribe(const QString &topicFilter, int qos)
{
    if (!m_client || !m_client->is_connected()) {
        return false;
    }

    try {
        return m_client->subscribe(topicFilter.toStdString(), qos)
                   ->wait_for(std::chrono::milliseconds(SUBSCRIBE_TIMEOUT_MS));
    } catch (const mqtt::exception &e) {
        qWarning() << "[MqttUplinkClient] 订阅主题失败:" << e.what();
        return false;
    } catch (...) {
        return false;
    }
}

void MqttUplinkClient::connected(const mqtt::string &cause)
{
    Q_UNUSED(cause)
    // Paho 回调线程 → 排队回 Qt 事件循环
    QMetaObject::invokeMethod(this, [this]() { emit connectionEstablished(); },
                              Qt::QueuedConnection);
}

void MqttUplinkClient::connection_lost(const mqtt::string &cause)
{
    // 拷贝为 QString 值再捕获，避免跨线程访问 Paho 内部字符串
    const QString reason = QString::fromStdString(cause);
    QMetaObject::invokeMethod(this, [this, reason]() { emit connectionDropped(reason); },
                              Qt::QueuedConnection);
}

void MqttUplinkClient::message_arrived(mqtt::const_message_ptr msg)
{
    // 拷贝为 QString 值再捕获，避免跨线程访问 Paho 内部缓冲
    const QString topic = QString::fromStdString(msg->get_topic());
    const QString payload = QString::fromStdString(msg->get_payload_str());
    QMetaObject::invokeMethod(
        this, [this, topic, payload]() { emit messageArrived(topic, payload); },
        Qt::QueuedConnection);
}

}
