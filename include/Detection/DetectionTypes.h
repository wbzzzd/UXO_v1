#ifndef DETECTION_TYPES_H
#define DETECTION_TYPES_H

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

// 流水线常量 - 与 Phase 1 Python 流水线 (pipeline/inference.py) 保持一致
namespace DetectionConst {
    // 图像 / 网格尺寸
    constexpr int PATCH_SIZE       = 128;
    constexpr int GRID_SIZE        = 4;
    constexpr int IMAGE_SIZE       = 512;
    constexpr int YOLO_INPUT_SIZE  = 224;
    constexpr int NUM_PATCHES      = GRID_SIZE * GRID_SIZE;   // 16
    constexpr int NUM_CLASSES      = 9;
    constexpr int BG_IDX           = 1;   // CLASS_NAMES 中 "background" 的索引

    // 默认阈值（归一化空间）
    // 0.500 = (image_threshold - image_min) / (image_max - image_min)
    //         = (2.4904 - 1.5988) / (3.3820 - 1.5988)
    // 运行时由 DetectionEngine.initialize() 读取 patchcore_params.json 计算
    // 实际阈值; 本常量仅作文档级回退值。
    constexpr float DEFAULT_PC_THRESHOLD    = 0.500f;
    constexpr float DEFAULT_YOLO_THRESHOLD  = 0.05f;

    // 多尺度 TTA 参数
    inline constexpr float MULTISCALE_SCALES[] = { 0.75f, 1.0f, 1.5f, 2.0f };
    constexpr int NUM_SCALES = 4;
    inline constexpr bool MULTISCALE_FLIPS[] = { false, true };
    constexpr int NUM_FLIPS  = 2;
    constexpr int NUM_TTA    = NUM_SCALES * NUM_FLIPS;   // 8

    // ImageNet 归一化参数 (仅 PatchCore 预处理使用;
    // YOLO 的归一化已嵌入 ONNX 图, 外部不得重复归一化)
    inline constexpr float IMAGENET_MEAN[] = { 0.485f, 0.456f, 0.406f };
    inline constexpr float IMAGENET_STD[]  = { 0.229f, 0.224f, 0.225f };

    // UXO 类别名 (索引顺序不可变 - 与模型训练一致)
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

// PatchCore 对单个 128x128 patch 的异常检测结果
struct PatchResult {
    int   row               = 0;       // 网格行 [0, 3]
    int   col               = 0;       // 网格列 [0, 3]
    float rawScore          = 0.0f;    // ONNX 模型输出的原始异常分
    float normalizedScore   = 0.0f;    // (raw - image_min) / (image_max - image_min)
    bool  isAnomalous        = false;   // normalizedScore > threshold
    QImage heatmap;                    // 128x128 彩色热力图 (ARGB32), 用于可视化
    QRect targetRect;                  // UXO 在 512x512 域内的紧包框 (amap 峰值
                                       // 连通域); 仅异常 patch 有效
};

// YOLOv8-cls 对单个异常 patch 的分类结果
struct ClassificationResult {
    int    patchRow       = 0;
    int    patchCol       = 0;
    int    bestClass      = -1;        // 最优非背景类索引, 无过阈类时为 -1
    QString bestClassName;             // 可读类名
    float  confidence     = 0.0f;     // 最优类概率
    float  probs[DetectionConst::NUM_CLASSES] = {};  // 全部类别概率 (TTA 平均)
};

// 单张图像的完整检测结果
struct ImageDetectionResult {
    QString   imagePath;
    QImage    originalImage;           // 512x512 原图
    QImage    heatmapOverlay;          // 512x512 热力图叠加 (半透明)
    QImage    annotatedImage;          // 512x512 原图 + 红色检测框 (仅异常帧)
    QVector<PatchResult> patches;              // 16 个 patch 结果
    QVector<ClassificationResult> classifications;  // 仅异常 patch 的结果
    bool      hasAnomaly       = false;         // 任一 patch 异常
    float     maxAnomalyScore  = 0.0f;          // 全部 patch 的最大归一化分
    qint64    processingTimeMs = 0;             // 总处理耗时
    QString   error;                            // 无错误时为空
    qint64    timestampMs      = 0;             // 视频帧时间戳 (用于排序/时间线)
};

#endif // DETECTION_TYPES_H
