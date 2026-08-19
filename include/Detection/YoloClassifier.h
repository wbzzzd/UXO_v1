#ifndef YOLO_CLASSIFIER_H
#define YOLO_CLASSIFIER_H

#include <onnxruntime_cxx_api.h>
#include <QImage>
#include <QString>
#include <memory>
#include <string>
#include <vector>
#include "Detection/DetectionTypes.h"

// YOLOv8-cls UXO 分类器 - 多尺度 TTA 的 ONNX 推理。
//
// 输入  : 512x512 QImage + patch 网格位置 (row, col) + 可选 UXO 紧包框
// 过程  : 对每个尺度 [0.75, 1.0, 1.5, 2.0]:
//           以 UXO 紧包框中心裁剪 (无效时回退格子中心),
//           边长 = scale * base (base = 紧包框最长边, 无效时取 128), 收敛到 [0, 512]
//           缩放到 224x224
//           对每个翻转 [false, true]:
//             按需水平翻转
//             仅 /255 缩放 -> [1, 3, 224, 224]
//             (ImageNet 归一化已嵌入 ONNX 图内, 外部再做 mean/std 属双重
//              归一化, 会把输入推离训练分布、丧失判别力 - 见 preprocessCrop)
//             ONNX 推理 -> [1, 9] 概率 (图内已含 softmax)
//         对 8 次 TTA 结果取平均
//         取过阈的最优非背景类
// 输出  : ClassificationResult (TTA 平均概率 + 最优类)
class YoloClassifier {
public:
    // env       : Ort::Env 引用 (生命周期须长于本对象)
    // modelPath : yolov8_cls_224.onnx 路径
    // threshold : 非背景类的最低概率阈值 (如 0.05)
    YoloClassifier(Ort::Env& env, const QString& modelPath, float threshold);
    ~YoloClassifier();

    // 对单个 patch 做多尺度 TTA 分类。
    // image : 512x512 原图
    // row, col : patch 网格位置 [0, 3]
    // targetRect : UXO 在 512 域内的紧包框 (与红框同源);
    //              有效时裁剪以其为中心, 否则以格子中心为准
    ClassificationResult classify(const QImage& image, int row, int col,
                                  const QRect& targetRect = QRect());

    bool isLoaded() const { return m_session != nullptr; }

private:
    Ort::Env& m_env;
    std::unique_ptr<Ort::Session> m_session;

    std::string m_inputName;
    std::string m_outputName;

    float m_threshold;

    // 将 224x224 RGB888 裁剪图转为 [1, 3, 224, 224] NCHW float 张量,
    // 仅做 /255 缩放 - ImageNet 归一化已嵌入 ONNX 图, 外部不得重复归一化。
    void preprocessCrop(const QImage& crop, std::vector<float>& output);

    // 单次 ONNX 推理。inputData 须为 [1, 3, 224, 224],
    // outputData 接收 9 个类别概率。
    void runInference(const float* inputData, float* outputData);
};

#endif // YOLO_CLASSIFIER_H
