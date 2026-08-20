#ifndef DETECTION_ENGINE_H
#define DETECTION_ENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <memory>
#include "Detection/DetectionTypes.h"

class PatchCoreDetector;
class YoloClassifier;

// DetectionEngine - UXO 检测的顶层编排器。
//
// 持有 PatchCoreDetector 与 YoloClassifier, 通过 QtConcurrent 异步执行分析,
// 结果就绪后以信号抛出。
//
// 流水线:
//   1. PatchCore: 512x512 整帧 -> 16 个分区 -> 异常分 + 热力图
//   2. YOLOv8-cls: 对每个异常分区做多尺度 TTA 分类
//   3. 生成热力图叠加图用于可视化
//
// 用法:
//   DetectionEngine engine;
//   engine.initialize("patchcore_512.onnx", "yolov8_cls_224.onnx", "patchcore_params.json");
//   connect(&engine, &DetectionEngine::imageAnalyzed, ...);
//   engine.analyzeImage("/path/to/image.jpg");
class DetectionEngine : public QObject {
    Q_OBJECT
public:
    explicit DetectionEngine(QObject* parent = nullptr);
    ~DetectionEngine();

    // 以模型路径与 PatchCore 后处理参数初始化。
    // paramsPath 为含 image_min / image_max / image_threshold 的 JSON 文件。
    // 失败时返回 false 并 emit error()。
    bool initialize(const QString& patchcoreModelPath,
                    const QString& yoloModelPath,
                    const QString& paramsPath);

    bool isInitialized() const;

    // 异步单图分析。结果经 imageAnalyzed() 或 error() 抛出。
    void analyzeImage(const QString& imagePath);

    void analyzeFrame(const QImage& frame, qint64 timestampMs);

    // 异步批量分析。每张图 emit imageAnalyzed(), 期间 emit
    // batchProgress(completed, total), 结束 emit batchFinished()。
    void analyzeBatch(const QStringList& imagePaths);

    // 请求取消当前批量任务。后台线程在图像间检查该标志并停止。
    void cancel();

signals:
    void imageAnalyzed(const ImageDetectionResult& result);
    void batchProgress(int completed, int total);
    void batchFinished();
    void error(const QString& message);

private:
    struct Private;
    std::unique_ptr<Private> d;

    // 同步分析 (由后台线程调用)。
    ImageDetectionResult doAnalyze(const QString& imagePath);
    ImageDetectionResult doAnalyzeImage(const QImage& image, qint64 timestampMs);

    QImage generateOverlay(const QImage& original,
                           const QVector<PatchResult>& patches);

    QImage generateAnnotatedImage(const QImage& original,
                                  const QVector<PatchResult>& patches,
                                  const QVector<ClassificationResult>& classifications);
};

#endif // DETECTION_ENGINE_H
