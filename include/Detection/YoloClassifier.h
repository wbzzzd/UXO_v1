#ifndef YOLO_CLASSIFIER_H
#define YOLO_CLASSIFIER_H

#include <onnxruntime_cxx_api.h>
#include <QImage>
#include <QString>
#include <memory>
#include <string>
#include <vector>
#include "Detection/DetectionTypes.h"

// YOLOv8-cls UXO classifier - runs ONNX inference with multi-scale TTA.
//
// Input  : 512x512 QImage + patch grid position (row, col) + optional UXO bbox
// Process: For each scale in [0.75, 1.0, 1.5, 2.0]:
//            Crop centered on UXO bbox center (fallback: patch-cell center),
//            size = scale * base (base = bbox max side, fallback: 128), clamped to [0, 512]
//            Resize to 224x224
//            For each flip in [false, true]:
//              Apply horizontal flip if needed
//              ImageNet normalize -> [1, 3, 224, 224]
//              Run ONNX inference -> [1, 9] probabilities (already softmaxed)
//          Average probabilities over 8 TTA runs
//          Find best non-background class above threshold
// Output : ClassificationResult with averaged probs and best class
class YoloClassifier {
public:
    // env       : Ort::Env reference (must outlive this object)
    // modelPath : path to yolov8_cls_224.onnx
    // threshold : minimum probability for non-bg class (e.g. 0.05)
    YoloClassifier(Ort::Env& env, const QString& modelPath, float threshold);
    ~YoloClassifier();

    // Classify a single patch using multi-scale TTA.
    // image : 512x512 original image
    // row, col : patch grid position [0, 3]
    // targetRect : UXO tight bbox in 512 domain (same source as red box);
    //              crop centers on it when valid, else on the patch-cell center
    ClassificationResult classify(const QImage& image, int row, int col,
                                  const QRect& targetRect = QRect());

    bool isLoaded() const { return m_session != nullptr; }

private:
    Ort::Env& m_env;
    std::unique_ptr<Ort::Session> m_session;

    std::string m_inputName;
    std::string m_outputName;

    float m_threshold;

    // Preprocess a 224x224 RGB888 crop into [1, 3, 224, 224] NCHW float tensor
    // with ImageNet normalization.
    void preprocessCrop(const QImage& crop, std::vector<float>& output);

    // Run single ONNX inference. inputData must be [1, 3, 224, 224].
    // outputData receives 9 class probabilities.
    void runInference(const float* inputData, float* outputData);
};

#endif // YOLO_CLASSIFIER_H
