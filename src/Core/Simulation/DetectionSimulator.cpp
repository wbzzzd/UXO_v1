// 检测模拟器实现
// 根据视频播放位置触发预设检测，输出 DetectionResult
// 不暴露时间点--公开接口只有 detectionOccurred(DetectionResult)

#include "Core/Simulation/DetectionSimulator.h"

#include <QDebug>

namespace Core::Simulation {

DetectionSimulator::DetectionSimulator(QObject *parent)
    : QObject(parent)
    , m_nextIndex(0)
    , m_running(false)
{
}

void DetectionSimulator::loadDetections(const QVector<DetectionEntry>& detections)
{
    m_detections = detections;
    reset();
}

void DetectionSimulator::start()
{
    m_running = true;
}

void DetectionSimulator::stop()
{
    m_running = false;
}

void DetectionSimulator::reset()
{
    m_running = false;
    m_nextIndex = 0;
}

bool DetectionSimulator::isRunning() const
{
    return m_running;
}

void DetectionSimulator::onPositionChanged(qint64 positionMs)
{
    if (!m_running) {
        return;
    }

    // 遍历所有已到达但尚未触发的检测条目
    // （可能因 seek 跳过多个，需全部触发）
    while (m_nextIndex < m_detections.size()) {
        const DetectionEntry& entry = m_detections[m_nextIndex];
        if (positionMs < entry.videoPositionMs) {
            break;  // 还没到触发时间
        }

        // 构造检测结果：类型 + 画面红框（归一化坐标）+ 置信度
        // 红框由左上角坐标和尺寸组成
        DetectionResult result;
        result.type = entry.type;
        result.videoRect = QRectF(entry.videoBoxPos.x(), entry.videoBoxPos.y(),
                                   entry.videoBoxSize.width(), entry.videoBoxSize.height());
        result.confidence = entry.confidence;

        emit detectionOccurred(result);
        ++m_nextIndex;
    }
}

}
