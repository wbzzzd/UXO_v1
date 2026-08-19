#ifndef DETECTION_TYPES_H
#define DETECTION_TYPES_H

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

// Pipeline constants - matches Phase 1 Python pipeline (pipeline/inference.py)
namespace DetectionConst {
    // Image / grid dimensions
    constexpr int PATCH_SIZE       = 128;
    constexpr int GRID_SIZE        = 4;
    constexpr int IMAGE_SIZE       = 512;
    constexpr int YOLO_INPUT_SIZE  = 224;
    constexpr int NUM_PATCHES      = GRID_SIZE * GRID_SIZE;   // 16
    constexpr int NUM_CLASSES      = 9;
    constexpr int BG_IDX           = 1;   // "background" index in CLASS_NAMES

    // Default thresholds (normalized space)
    // 0.500 = (image_threshold - image_min) / (image_max - image_min)
    //         = (2.4904 - 1.5988) / (3.3820 - 1.5988)
    // DetectionEngine.initialize() reads patchcore_params.json and computes
    // the actual threshold at runtime; this constant is a documentation fallback.
    constexpr float DEFAULT_PC_THRESHOLD    = 0.508f;
    constexpr float DEFAULT_YOLO_THRESHOLD  = 0.05f;

    // Multi-scale TTA parameters
    inline constexpr float MULTISCALE_SCALES[] = { 0.75f, 1.0f, 1.5f, 2.0f };
    constexpr int NUM_SCALES = 4;
    inline constexpr bool MULTISCALE_FLIPS[] = { false, true };
    constexpr int NUM_FLIPS  = 2;
    constexpr int NUM_TTA    = NUM_SCALES * NUM_FLIPS;   // 8

    // ImageNet normalization
    inline constexpr float IMAGENET_MEAN[] = { 0.485f, 0.456f, 0.406f };
    inline constexpr float IMAGENET_STD[]  = { 0.229f, 0.224f, 0.225f };

    // UXO class names (index order matters - matches model training)
    inline constexpr const char* const CLASS_NAMES[NUM_CLASSES] = {
        "aircraft-bombs",
        "background",
        "fuzes",
        "grenades",
        "landmines",
        "mortars",
        "projectiles",
        "rockets",
        "submunitions"
    };
}

// Result of PatchCore anomaly detection for a single 128x128 patch
struct PatchResult {
    int   row               = 0;       // grid row  [0, 3]
    int   col               = 0;       // grid col  [0, 3]
    float rawScore          = 0.0f;    // raw anomaly score from ONNX model
    float normalizedScore   = 0.0f;    // (raw - image_min) / (image_max - image_min)
    bool  isAnomalous        = false;   // normalizedScore > threshold
    QImage heatmap;                    // 128x128 colored heatmap (ARGB32) for visualization
    QRect targetRect;                  // UXO tight bbox in 512x512 domain (amap peak
                                      // connected region); valid only on anomalous patch
};

// Result of YOLOv8-cls classification for a single anomalous patch
struct ClassificationResult {
    int    patchRow       = 0;
    int    patchCol       = 0;
    int    bestClass      = -1;        // best non-bg class index, -1 if none above threshold
    QString bestClassName;             // human-readable class name
    float  confidence     = 0.0f;     // probability of best class
    float  probs[DetectionConst::NUM_CLASSES] = {};  // all class probabilities (averaged over TTA)
};

// Complete detection result for one image
struct ImageDetectionResult {
    QString   imagePath;
    QImage    originalImage;           // 512x512 original
    QImage    heatmapOverlay;          // 512x512 with heatmap overlaid (semi-transparent)
    QImage    annotatedImage;          // 512x512 original with red detection box (anomalous frames only)
    QVector<PatchResult> patches;              // 16 patch results
    QVector<ClassificationResult> classifications;  // results for anomalous patches only
    bool      hasAnomaly       = false;         // any patch anomalous
    float     maxAnomalyScore  = 0.0f;          // max normalized score across patches
    qint64    processingTimeMs = 0;             // total processing time
    QString   error;                            // empty if no error
    qint64    timestampMs      = 0;             // video frame timestamp (for sorting/timeline)
};

#endif // DETECTION_TYPES_H
