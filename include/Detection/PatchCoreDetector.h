#ifndef PATCHCORE_DETECTOR_H
#define PATCHCORE_DETECTOR_H

#include <onnxruntime_cxx_api.h>
#include <QImage>
#include <QString>
#include <QVector>
#include <memory>
#include <string>
#include <vector>
#include "Detection/DetectionTypes.h"

// PatchCore 异常检测器 - 整帧单次 ONNX 推理 (v07 frame512 口径).
//
// Input  : 512x512 QImage (Format_RGB888)
// Process: 整帧推理 [1, 3, 512, 512] -> pred_score[1] + anomaly_map[1,1,H,W]
//          帧级判定用 pred_score (评估门 FP=0/recall=99.6% 验证的量);
//          anomaly_map 按 4x4 分区聚合 -> 16 格, 峰值格承载帧分数并唯一置旗
// Output : 16 PatchResults (保持 UI 网格契约: row/col 0-3, 128x128 热力图)
class PatchCoreDetector {
public:
    // env        : Ort::Env reference (must outlive this object)
    // modelPath  : path to patchcore_512.onnx
    // imageMin   : post-processor param (raw score min for normalization)
    // imageMax   : post-processor param (raw score max for normalization)
    // threshold  : normalized score threshold (e.g. 0.5)
    PatchCoreDetector(Ort::Env& env, const QString& modelPath,
                      float imageMin, float imageMax, float threshold);
    ~PatchCoreDetector();

    // Detect anomalies in a 512x512 image. Returns 16 PatchResults.
    QVector<PatchResult> detect(const QImage& image);

    bool isLoaded() const { return m_session != nullptr; }

private:
    Ort::Env& m_env;
    std::unique_ptr<Ort::Session> m_session;

    // I/O tensor names (stored as std::string to keep memory alive)
    std::string m_inputName;
    std::string m_outputName0;   // pred_score
    std::string m_outputName1;   // anomaly_map

    float m_imageMin;
    float m_imageMax;
    float m_threshold;

    // Preprocess 512x512 image into [1, 3, 512, 512] NCHW float tensor
    // with ImageNet normalization.
    void preprocess(const QImage& image, std::vector<float>& output);

    // 从整帧 map 的分区区域生成 128x128 彩色热力图 (ARGB32).
    // stride 为整帧 map 行跨度 (区域是 map 的视图, 行内存不连续);
    // minVal/maxVal 用于显示归一化 (全图范围).
    QImage generateCellHeatmap(const float* mapData, int regionH, int regionW,
                               int stride, float minVal, float maxVal);

    // UXO 紧包框: 以全图峰值像素为种子, 在 >= rawThreshold 的连通域上取外接矩形,
    // 外扩边距并保底最小尺寸 (红框/YOLO 裁剪/地理定位共用; 512 域坐标)
    QRect locateTarget(const float* maps, int mapH, int mapW,
                       float rawThreshold, float mapMin) const;
};

#endif // PATCHCORE_DETECTOR_H
