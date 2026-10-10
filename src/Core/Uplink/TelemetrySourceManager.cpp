// 遥测数据源管理器实现（REQ-012 M4E WP3）
// 持有模拟/真实两源并按 SourceMode 互斥切换；真实链路 >5s 无遥测判停滞；
// 断链不自动切回模拟（由 UI 裁决，见 docs/features/m4e-drone-integration.md §6.1）。

#include "Core/Uplink/TelemetrySourceManager.h"

#include <QTimer>

#include "Core/Simulation/DroneTelemetrySimulator.h"
#include "Core/Uplink/ITelemetrySource.h"
#include "Core/Uplink/MqttTelemetrySource.h"

namespace Core::Uplink {

namespace {
// 真实链路停滞判定窗口：>5s 无 OSD 报文判停滞（契约 §3.4 / UI 契约 §6.3）
constexpr int STALL_TIMEOUT_MS = 5000;
} // namespace

TelemetrySourceManager::TelemetrySourceManager(QObject *parent)
    : QObject(parent)
    , m_simSource(new Core::Simulation::DroneTelemetrySimulator(this))
    , m_realSource(new MqttTelemetrySource(this))
    , m_mode(SourceMode::Simulation)
    , m_linkState(LinkState::Disconnected)
    , m_stallTimer(new QTimer(this))
{
    // 跨线程信号投递（真实源回调自 Paho 线程排队回主线程）所需元数据注册
    qRegisterMetaType<TelemetryData>();
    qRegisterMetaType<SourceMode>();
    qRegisterMetaType<LinkState>();
    // WP4（feature §6.4）：转发信号 alertReceived 携自定义 AlertEvent，
    // 队列连接跨线程投递自定义类型须运行期注册，与上述三类型同理
    qRegisterMetaType<AlertEvent>();

    m_stallTimer->setSingleShot(true);
    m_stallTimer->setInterval(STALL_TIMEOUT_MS);
    connect(m_stallTimer, &QTimer::timeout, this, &TelemetrySourceManager::onStallTimeout);

    // 挂当前源（默认模拟源）信号转发：遥测经槽转发，告警信号直通，链路状态驱动停滞计时
    ITelemetrySource *source = m_simSource;
    connect(source, &ITelemetrySource::telemetryUpdated,
            this, &TelemetrySourceManager::onSourceTelemetry);
    connect(source, &ITelemetrySource::alertReceived,
            this, &TelemetrySourceManager::alertReceived);
    connect(source, &ITelemetrySource::linkStateChanged,
            this, &TelemetrySourceManager::onSourceLinkState);
}

TelemetrySourceManager::~TelemetrySourceManager() = default;

void TelemetrySourceManager::setMode(SourceMode mode)
{
    if (m_mode == mode) {
        return;
    }

    // 停旧源：断开其信号转发并停止运行（数据源互斥，UI 只见当前源信号）
    ITelemetrySource *oldSource = (m_mode == SourceMode::Simulation)
                                      ? static_cast<ITelemetrySource *>(m_simSource)
                                      : static_cast<ITelemetrySource *>(m_realSource);
    const bool wasRunning = oldSource->isRunning();
    disconnect(oldSource, nullptr, this, nullptr);
    oldSource->stop();
    m_stallTimer->stop();

    // 离开真实链路时链路状态回 Disconnected（模拟模式无链路状态语义）
    if (m_linkState != LinkState::Disconnected) {
        m_linkState = LinkState::Disconnected;
        emit linkStateChanged(m_linkState);
    }

    m_mode = mode;

    // 挂新源信号转发
    ITelemetrySource *newSource = (m_mode == SourceMode::Simulation)
                                      ? static_cast<ITelemetrySource *>(m_simSource)
                                      : static_cast<ITelemetrySource *>(m_realSource);
    connect(newSource, &ITelemetrySource::telemetryUpdated,
            this, &TelemetrySourceManager::onSourceTelemetry);
    connect(newSource, &ITelemetrySource::alertReceived,
            this, &TelemetrySourceManager::alertReceived);
    connect(newSource, &ITelemetrySource::linkStateChanged,
            this, &TelemetrySourceManager::onSourceLinkState);

    // 保持原运行状态：旧源在跑则新源自动启动
    if (wasRunning) {
        newSource->start();
    }

    emit modeChanged(mode);
}

SourceMode TelemetrySourceManager::mode() const
{
    return m_mode;
}

LinkState TelemetrySourceManager::linkState() const
{
    return m_linkState;
}

Core::Simulation::DroneTelemetrySimulator *TelemetrySourceManager::simulationSource() const
{
    return m_simSource;
}

void TelemetrySourceManager::start()
{
    if (m_mode == SourceMode::Simulation) {
        m_simSource->start();
    } else {
        m_realSource->start();
    }
}

void TelemetrySourceManager::stop()
{
    if (m_mode == SourceMode::Simulation) {
        m_simSource->stop();
    } else {
        m_realSource->stop();
    }
}

void TelemetrySourceManager::reset()
{
    if (m_mode == SourceMode::Simulation) {
        m_simSource->reset();
    } else {
        m_realSource->reset();
    }
}

bool TelemetrySourceManager::isRunning() const
{
    return (m_mode == SourceMode::Simulation) ? m_simSource->isRunning()
                                              : m_realSource->isRunning();
}

void TelemetrySourceManager::onSourceTelemetry(const TelemetryData &data)
{
    // 真实链路：每帧遥测重置停滞计时；停滞后收到遥测即时恢复 Connected
    if (m_mode == SourceMode::RealLink) {
        m_stallTimer->start();
        if (m_linkState == LinkState::Stalled) {
            m_linkState = LinkState::Connected;
            emit linkStateChanged(m_linkState);
        }
    }
    emit telemetryUpdated(data);
}

void TelemetrySourceManager::onSourceLinkState(LinkState state)
{
    if (m_linkState == state) {
        return;
    }
    m_linkState = state;
    emit linkStateChanged(state);

    // Connected 起算停滞窗口；断链停止计时（断链态停滞判定无意义）
    if (state == LinkState::Connected) {
        m_stallTimer->start();
    } else if (state == LinkState::Disconnected) {
        m_stallTimer->stop();
    }
}

void TelemetrySourceManager::onStallTimeout()
{
    // 仅真实链路 Connected 态判停滞（模拟模式无停滞语义）
    if (m_mode == SourceMode::RealLink && m_linkState == LinkState::Connected) {
        m_linkState = LinkState::Stalled;
        emit linkStateChanged(m_linkState);
    }
}

} // namespace Core::Uplink
