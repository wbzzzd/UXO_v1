// 无人机遥测模拟器实现
// 沿预设航线插值输出 GPS 遥测数据，定时器驱动（100ms 间隔）

#include "Core/Simulation/DroneTelemetrySimulator.h"

#include <QtMath>
#include <QDebug>

namespace Core::Simulation {

// 定时器间隔（毫秒），平衡流畅度与 CPU 开销
static constexpr int TIMER_INTERVAL_MS = 100;

DroneTelemetrySimulator::DroneTelemetrySimulator(QObject *parent)
    : QObject(parent)
    , m_totalDurationSec(96.0)
    , m_timer(nullptr)
    , m_running(false)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(TIMER_INTERVAL_MS);
    connect(m_timer, &QTimer::timeout, this, &DroneTelemetrySimulator::onTick);
}

void DroneTelemetrySimulator::loadRoute(const QVector<DroneWaypoint>& route, double totalDurationSec)
{
    m_route = route;
    m_totalDurationSec = totalDurationSec;
    reset();
}

void DroneTelemetrySimulator::start()
{
    if (m_route.size() < 2) {
        return;
    }
    m_running = true;
    m_elapsed.start();
    m_timer->start();
    // 立即发出首帧遥测（航线起点）
    onTick();
}

void DroneTelemetrySimulator::stop()
{
    m_running = false;
    m_timer->stop();
}

void DroneTelemetrySimulator::reset()
{
    m_running = false;
    m_timer->stop();
    m_elapsed.invalidate();
}

bool DroneTelemetrySimulator::isRunning() const
{
    return m_running;
}

void DroneTelemetrySimulator::onTick()
{
    if (!m_running || m_route.size() < 2) {
        return;
    }

    // 计算当前流逝时间（秒）
    double elapsedSec = m_elapsed.elapsed() / 1000.0;

    // 到达航线终点：停止定时器，输出最后一个航点
    if (elapsedSec >= m_totalDurationSec) {
        const DroneWaypoint& last = m_route.last();
        emit telemetryUpdated(last.lat, last.lng, last.alt, last.heading);
        m_timer->stop();
        m_running = false;
        return;
    }

    // 计算当前在航线中的进度 [0.0, 1.0)
    double progress = elapsedSec / m_totalDurationSec;

    // 将进度映射到航段：N 个航点有 N-1 个航段，均分时长
    int segmentCount = m_route.size() - 1;
    double segProgress = progress * segmentCount;
    int segIndex = static_cast<int>(segProgress);
    if (segIndex >= segmentCount) {
        segIndex = segmentCount - 1;
    }
    double frac = segProgress - segIndex;  // 当前航段内插值因子 [0,1)

    // 线性插值经纬度、高度
    const DroneWaypoint& p0 = m_route[segIndex];
    const DroneWaypoint& p1 = m_route[segIndex + 1];

    double lat = p0.lat + (p1.lat - p0.lat) * frac;
    double lng = p0.lng + (p1.lng - p0.lng) * frac;
    double alt = p0.alt + (p1.alt - p0.alt) * frac;

    // 航向插值：处理 0-360 度环绕（取最短路径）
    double h0 = p0.heading;
    double h1 = p1.heading;
    double dHeading = h1 - h0;
    if (dHeading > 180.0) {
        dHeading -= 360.0;
    } else if (dHeading < -180.0) {
        dHeading += 360.0;
    }
    double heading = h0 + dHeading * frac;
    // 归一化到 [0, 360)
    heading = std::fmod(heading + 360.0, 360.0);

    emit telemetryUpdated(lat, lng, alt, heading);
}

}
