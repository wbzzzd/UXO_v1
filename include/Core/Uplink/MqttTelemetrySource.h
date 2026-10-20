#ifndef CORE_UPLINK_MQTTTELEMETRYSOURCE_H
#define CORE_UPLINK_MQTTTELEMETRYSOURCE_H

// 真实链路遥测数据源（REQ-015 M4E WP3）
// 通过 MQTT 订阅机场网关转发的无人机 OSD 与事件报文（报文格式见
// scripts/sim_telemetry.py），解析后以 ITelemetrySource 统一信号发布。
// 只读订阅，不发布任何下行命令。

#include "Core/Uplink/ITelemetrySource.h"

namespace Core::Uplink {

class MqttUplinkClient;

class MqttTelemetrySource : public ITelemetrySource
{
    Q_OBJECT

public:
    // 默认连接本机 MQTT 服务器（127.0.0.1:1883，与 docker-compose 部署一致）
    explicit MqttTelemetrySource(QObject *parent = nullptr);
    // 指定 MQTT 服务器地址与端口
    MqttTelemetrySource(const QString &host, quint16 port, QObject *parent = nullptr);
    ~MqttTelemetrySource() override = default;

    // 启动：置运行意图并尝试连接；连接结果经 linkStateChanged 上报
    void start() override;
    // 停止：清除运行意图并断开连接
    void stop() override;
    // 真实链路无内部可重放状态，重置为空操作
    void reset() override;
    bool isRunning() const override;

private slots:
    // 底层连接建立：完成主题订阅并进入 Connected
    void onClientConnectionEstablished();
    // 底层连接断开
    void onClientConnectionDropped(const QString &reason);
    // 底层收到 MQTT 消息：按主题分发给 OSD/事件解析
    void onClientMessageArrived(const QString &topic, const QString &payload);

private:
    // 订阅契约主题（OSD/events），供连接建立回调与 start() 已连补订路径复用
    void subscribeContractTopics();
    // 解析 OSD 遥测报文，有效则发 telemetryUpdated
    void handleOsdPayload(const QString &payload);
    // 解析事件报文（HMS 等），有效则发 alertReceived
    void handleEventsPayload(const QString &payload);
    // 链路状态变化（去重）并上报
    void setLinkState(LinkState state);

    bool m_running;             // 运行意图（start 后 stop 前，与连接状态解耦）
    LinkState m_linkState;      // 当前链路状态
    MqttUplinkClient *m_client; // 底层 MQTT 客户端（QObject 子对象）
};

}

#endif
