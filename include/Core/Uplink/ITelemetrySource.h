#ifndef CORE_UPLINK_ITELEMETRYSOURCE_H
#define CORE_UPLINK_ITELEMETRYSOURCE_H

// 遥测数据源抽象接口（REQ-015 WP3）
// 本地模拟源（DroneTelemetrySimulator）与真实链路源（MqttTelemetrySource）
// 共同实现，由 TelemetrySourceManager 统一调度，UI 只面向管理器。

#include "Core/Uplink/TelemetryTypes.h"

#include <QByteArray>
#include <QObject>
#include <QString>

namespace Core::Uplink {

class ITelemetrySource : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~ITelemetrySource() override = default;

    // 启动/停止数据源
    virtual void start() = 0;
    virtual void stop() = 0;

    // 复位：语义由各实现定义（模拟源=清航线进度回起点；真实源=无操作）
    virtual void reset() = 0;

    // 当前是否处于运行中
    virtual bool isRunning() const = 0;

signals:
    // 遥测更新：两种来源输出同一 TelemetryData 形状
    void telemetryUpdated(const Core::Uplink::TelemetryData &data);

    // 告警事件（仅真实源发出，如机身/负载 HMS 事件；模拟源不发）
    // WP4 起携带结构化 AlertEvent（code/level/text），UI 负责级别映射与 [HMS] 前缀
    void alertReceived(const Core::Uplink::AlertEvent &event);

    // 链路状态变化（仅真实源发出；模拟源不进入链路状态机）
    void linkStateChanged(Core::Uplink::LinkState state);

    // 原始报文直通（REQ-015 WP6 TrackStore）：真实源收到契约主题（osd/events）
    // 报文时原样转发主题与 UTF-8 载荷字节，供落库旁路 verbatim 消费（不做语义
    // 加工，帧有效性过滤与列映射由消费方自裁决）；模拟源不发射——模拟模式
    // 落库零写入天然达成。QString/QByteArray 为内建元类型，队列投递免注册。
    void rawMessageReceived(const QString &topic, const QByteArray &payload);
};

}

#endif
