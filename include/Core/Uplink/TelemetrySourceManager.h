#ifndef CORE_UPLINK_TELEMETRYSOURCEMANAGER_H
#define CORE_UPLINK_TELEMETRYSOURCEMANAGER_H

// 遥测数据源管理器（REQ-015 WP3）
// 持有本地模拟源与真实链路源，按 SourceMode 切换当前源并向 UI 转发统一信号；
// 真实链路停滞判定（>5s 无遥测）在此实现；断链不自动切回模拟（由 UI 裁决，
// 见 docs/features/m4e-drone-integration.md §6.1）。

#include "Core/Uplink/TelemetryTypes.h"

#include <QByteArray>
#include <QObject>

class QTimer;

namespace Core::Simulation { class DroneTelemetrySimulator; }

namespace Core::Uplink {

class MqttTelemetrySource;

class TelemetrySourceManager : public QObject
{
    Q_OBJECT

public:
    explicit TelemetrySourceManager(QObject *parent = nullptr);
    ~TelemetrySourceManager() override;

    // 切换数据源：停旧源、重挂信号转发；若旧源在运行则新源自动启动（保持原运行状态）。
    // 任务进行中是否允许切换由 UI 层裁决（管理器不感知任务状态）。
    void setMode(SourceMode mode);
    SourceMode mode() const;

    // 当前真实链路状态（模拟模式下恒为 Disconnected，无意义）
    LinkState linkState() const;

    // 访问模拟源（供 loadRoute 等模拟特有操作）；真实源不对外暴露
    Core::Simulation::DroneTelemetrySimulator *simulationSource() const;

    // 控制接口：转发到当前源
    void start();
    void stop();
    void reset();

    // 当前源是否运行中
    bool isRunning() const;

signals:
    // 以下信号均为当前源信号的转发（真实源信号已在其内部回到主线程）
    void telemetryUpdated(const Core::Uplink::TelemetryData &data);
    void alertReceived(const Core::Uplink::AlertEvent &event);
    // WP6 TrackStore：原始报文直通转发（模拟源无此信号，转发自然静默）
    void rawMessageReceived(const QString &topic, const QByteArray &payload);
    void modeChanged(Core::Uplink::SourceMode mode);
    void linkStateChanged(Core::Uplink::LinkState state);

private slots:
    void onSourceTelemetry(const Core::Uplink::TelemetryData &data);
    void onSourceLinkState(Core::Uplink::LinkState state);
    void onStallTimeout();

private:
    Core::Simulation::DroneTelemetrySimulator *m_simSource;  // 模拟源（持有）
    MqttTelemetrySource *m_realSource;                       // 真实链路源（持有）
    SourceMode m_mode;
    LinkState m_linkState;
    QTimer *m_stallTimer;  // 真实链路 >5s 无遥测判停滞（超时发出 Stalled）
};

}

#endif
