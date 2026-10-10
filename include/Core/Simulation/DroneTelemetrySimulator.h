#ifndef CORE_SIMULATION_DRONETELEMETRYSIMULATOR_H
#define CORE_SIMULATION_DRONETELEMETRYSIMULATOR_H

// 无人机遥测模拟器
// 沿预设航线（经纬度航点序列）持续输出 GPS 坐标（经纬度、高度、航向）。
// 定时器驱动，start/stop/reset 控制接口。
// 已挂接 Core::Uplink::ITelemetrySource 统一遥测源契约，由 TelemetrySourceManager
// 与真实链路源（MqttTelemetrySource）互斥调度。

#include "Core/Simulation/DemoScenarioProvider.h"
#include "Core/Uplink/ITelemetrySource.h"

#include <QTimer>
#include <QElapsedTimer>

namespace Core::Simulation {

class DroneTelemetrySimulator : public Core::Uplink::ITelemetrySource
{
    Q_OBJECT

public:
    explicit DroneTelemetrySimulator(QObject *parent = nullptr);

    // 加载航线航点序列和总时长（秒）
    // 总时长应与视频时长一致，航段时长均分
    void loadRoute(const QVector<DroneWaypoint>& route, double totalDurationSec);

    // 控制接口（ITelemetrySource 契约；由 TelemetrySourceManager 统一调度）
    void start() override;
    void stop() override;
    void reset() override;

    // 当前是否处于运行中
    bool isRunning() const override;

    // 遥测更新信号继承自 ITelemetrySource：
    // void telemetryUpdated(const Core::Uplink::TelemetryData &data);
    // 经 TelemetrySourceManager 转发至 MainWindow（战术地图无人机标记与航迹）。

private slots:
    void onTick();

private:
    QVector<DroneWaypoint> m_route;
    double m_totalDurationSec;     // 航线总时长（秒）
    QTimer *m_timer;               // 定时器（100ms 间隔）
    QElapsedTimer m_elapsed;       // 计时器（start 时启动，提供精确流逝时间）
    bool m_running;
};

}

#endif
