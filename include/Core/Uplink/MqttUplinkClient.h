#ifndef CORE_UPLINK_MQTTUPLINKCLIENT_H
#define CORE_UPLINK_MQTTUPLINKCLIENT_H

// MQTT 上行链路客户端（REQ-015 M4E WP3）
// 基于 Paho MQTT C++ 封装连接、订阅与消息回调，将 Paho 回调线程的事件
// 以 Qt 信号排队转发回 Qt 事件循环；只订阅不发布，不解析业务报文。

#include <memory>

#include <QObject>
#include <QString>

#include <mqtt/callback.h>

namespace mqtt {
class async_client;
}

namespace Core::Uplink {

class MqttUplinkClient : public QObject, private mqtt::callback
{
    Q_OBJECT

public:
    explicit MqttUplinkClient(const QString &host, quint16 port, QObject *parent = nullptr);
    ~MqttUplinkClient() override;

    // 连接 MQTT 服务器（同步阻塞，超时约 3 秒），成功返回 true
    bool connectToBroker();
    // 断开与服务器连接（幂等，未连接时无操作）
    void disconnectFromBroker();
    // 当前是否已连接
    bool isConnected() const;
    // 订阅主题过滤器，成功返回 true（需已连接）
    bool subscribe(const QString &topicFilter, int qos);

signals:
    // 连接建立成功
    void connectionEstablished();
    // 连接丢失或断开，reason 为原因描述
    void connectionDropped(const QString &reason);
    // 收到一条 MQTT 消息（主题 + 文本载荷）
    void messageArrived(const QString &topic, const QString &payload);

private:
    // mqtt::callback：连接建立（Paho 回调线程）
    void connected(const mqtt::string &cause) override;
    // mqtt::callback：连接丢失（Paho 回调线程）
    void connection_lost(const mqtt::string &cause) override;
    // mqtt::callback：消息到达（Paho 回调线程）
    void message_arrived(mqtt::const_message_ptr msg) override;

    std::unique_ptr<mqtt::async_client> m_client;  // Paho 异步客户端
};

}

#endif
