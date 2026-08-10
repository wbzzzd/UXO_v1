#ifndef CORE_SIMULATION_DRONETELEMETRYSIMULATOR_H
#define CORE_SIMULATION_DRONETELEMETRYSIMULATOR_H

// 无人机遥测模拟器
// 沿预设航线（经纬度航点序列）持续输出 GPS 坐标（经纬度、高度、航向）。
// 定时器驱动，start/stop/reset 控制接口。
// 接口贴合真实遥测源——真实系统替换为接收 MAVLink 或其他遥测协议即可。

#include "Core/Simulation/DemoScenarioProvider.h"

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

namespace Core::Simulation {

class DroneTelemetrySimulator : public QObject
{
    Q_OBJECT

public:
    explicit DroneTelemetrySimulator(QObject *parent = nullptr);

    // 加载航线航点序列和总时长（秒）
    // 总时长应与视频时长一致，航段时长均分
    void loadRoute(const QVector<DroneWaypoint>& route, double totalDurationSec);

    // 控制接口（由探测工具栏 [开始]/[结束]/[重置] 调用）
    void start();
    void stop();
    void reset();

    // 当前是否处于运行中
    bool isRunning() const;

signals:
    // 无人机遥测更新：经纬度、高度、航向
    // MainWindow 接收后更新战术地图无人机标记和航迹
    void telemetryUpdated(double lat, double lng, double alt, double heading);

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
