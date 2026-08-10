#ifndef CORE_SIMULATION_DETECTIONSIMULATOR_H
#define CORE_SIMULATION_DETECTIONSIMULATOR_H

// 检测模拟器
// 处理视频画面，在目标出现时输出检测结果（类型 + 画面红框 + 置信度）。
// 内部用视频位置驱动检测时机，但接口不暴露时间点概念--只输出检测结果。
// 接口贴合真实检测器--真实系统替换为 AI 推理引擎即可。

#include "Core/Simulation/DemoScenarioProvider.h"
#include "Core/Data/Types.h"

#include <QObject>

namespace Core::Simulation {

class DetectionSimulator : public QObject
{
    Q_OBJECT

public:
    explicit DetectionSimulator(QObject *parent = nullptr);

    // 加载检测数据条目（内部用视频位置驱动检测时机）
    void loadDetections(const QVector<DetectionEntry>& detections);

    // 控制接口（由探测工具栏 [开始]/[结束]/[重置] 调用）
    void start();
    void stop();
    void reset();

    // 当前是否处于运行中
    bool isRunning() const;

signals:
    // 检测到目标：输出检测结果（类型 + 画面红框 + 置信度）
    // 不暴露时间点概念，MainWindow 接收后推算目标坐标并同步四区
    void detectionOccurred(const Core::DetectionResult& result);

public slots:
    // 由 VideoStreamPanel::positionChanged(qint64 ms) 连接
    // 检查是否到达下一个检测位置，到达则发出 detectionOccurred
    void onPositionChanged(qint64 positionMs);

private:
    QVector<DetectionEntry> m_detections;
    int m_nextIndex;   // 下一个待触发的检测条目索引
    bool m_running;
};

}

#endif
