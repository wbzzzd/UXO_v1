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

// DetectionEngine - top-level orchestrator for UXO detection.
//
// Owns PatchCoreDetector and YoloClassifier, runs analysis asynchronously
// via QtConcurrent, and emits signals when results are ready.
//
// Pipeline:
//   1. PatchCore: 512x512 image -> 16 patches -> anomaly scores + heatmaps
//   2. YOLOv8-cls: for each anomalous patch, multi-scale TTA classification
//   3. Generate heatmap overlay for visualization
//
// Usage:
//   DetectionEngine engine;
//   engine.initialize("patchcore_512.onnx", "yolov8_cls_224.onnx", "patchcore_params.json");
//   connect(&engine, &DetectionEngine::imageAnalyzed, ...);
//   engine.analyzeImage("/path/to/image.jpg");
class DetectionEngine : public QObject {
    Q_OBJECT
public:
    explicit DetectionEngine(QObject* parent = nullptr);
    ~DetectionEngine();

    // Initialize with model paths and PatchCore post-processor params.
    // paramsPath is a JSON file with image_min, image_max, image_threshold.
    // Returns false and emits error() on failure.
    bool initialize(const QString& patchcoreModelPath,
                    const QString& yoloModelPath,
                    const QString& paramsPath);

    bool isInitialized() const;

    // Asynchronous single-image analysis. Emits imageAnalyzed() or error().
    void analyzeImage(const QString& imagePath);

    void analyzeFrame(const QImage& frame, qint64 timestampMs);

    // Asynchronous batch analysis. Emits imageAnalyzed() for each image,
    // batchProgress(completed, total), and batchFinished().
    void analyzeBatch(const QStringList& imagePaths);

    // Request cancellation of the current batch. The background thread
    // checks this flag between images and stops.
    void cancel();

signals:
    void imageAnalyzed(const ImageDetectionResult& result);
    void batchProgress(int completed, int total);
    void batchFinished();
    void error(const QString& message);

private:
    struct Private;
    std::unique_ptr<Private> d;

    // Synchronous analysis (called from background thread).
    ImageDetectionResult doAnalyze(const QString& imagePath);
    ImageDetectionResult doAnalyzeImage(const QImage& image, qint64 timestampMs);

    QImage generateOverlay(const QImage& original,
                           const QVector<PatchResult>& patches);

    QImage generateAnnotatedImage(const QImage& original,
                                  const QVector<PatchResult>& patches,
                                  const QVector<ClassificationResult>& classifications);
};

#endif // DETECTION_ENGINE_H
