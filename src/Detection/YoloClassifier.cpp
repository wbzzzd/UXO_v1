#include "Detection/YoloClassifier.h"
#include <QDebug>
#include <cstring>
#include <algorithm>

YoloClassifier::YoloClassifier(Ort::Env& env, const QString& modelPath, float threshold)
    : m_env(env)
    , m_threshold(threshold)
{
    try {
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(2);
        opts.SetInterOpNumThreads(1);
        opts.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_ALL);

        m_session = std::make_unique<Ort::Session>(
            m_env, modelPath.toStdString().c_str(), opts);

        Ort::AllocatorWithDefaultOptions allocator;
        m_inputName  = m_session->GetInputNameAllocated(0, allocator).get();
        m_outputName = m_session->GetOutputNameAllocated(0, allocator).get();

        qDebug() << "YOLO classifier model loaded:" << modelPath
                 << "  in:" << m_inputName.c_str()
                 << "  out:" << m_outputName.c_str();
    } catch (const Ort::Exception& e) {
        qWarning() << "Failed to load YOLO model:" << e.what();
        m_session.reset();
    }
}

YoloClassifier::~YoloClassifier() = default;

void YoloClassifier::preprocessCrop(const QImage& crop, std::vector<float>& output)
{
    // 224x224 RGB888 -> [1, 3, 224, 224] NCHW float, 仅 /255 缩放。
    // 导出工具链已在 onnx 图内嵌入 ImageNet 归一化（bench onnx-v2-verify 语义），
    // 外部再除 mean/std 属双重归一化，会把输入推离训练分布、丧失判别力。
    constexpr int C = 3;
    constexpr int H = DetectionConst::YOLO_INPUT_SIZE;
    constexpr int W = DetectionConst::YOLO_INPUT_SIZE;
    constexpr int channelSize = H * W;
    output.resize(C * channelSize);

    const float inv255 = 1.0f / 255.0f;

    for (int y = 0; y < H; y++) {
        const uchar* scan = crop.constScanLine(y);
        for (int x = 0; x < W; x++) {
            output[0 * channelSize + y * W + x] = scan[x * 3 + 0] * inv255;
            output[1 * channelSize + y * W + x] = scan[x * 3 + 1] * inv255;
            output[2 * channelSize + y * W + x] = scan[x * 3 + 2] * inv255;
        }
    }
}

void YoloClassifier::runInference(const float* inputData, float* outputData)
{
    constexpr int tensorSize = 3 * DetectionConst::YOLO_INPUT_SIZE * DetectionConst::YOLO_INPUT_SIZE;

    auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> inputShape = {
        1, 3, DetectionConst::YOLO_INPUT_SIZE, DetectionConst::YOLO_INPUT_SIZE
    };
    auto inputTensor = Ort::Value::CreateTensor<float>(
        memInfo, const_cast<float*>(inputData), tensorSize,
        inputShape.data(), inputShape.size());

    const char* inputNames[]  = { m_inputName.c_str() };
    const char* outputNames[] = { m_outputName.c_str() };

    try {
        auto outputs = m_session->Run(
            Ort::RunOptions{nullptr},
            inputNames, &inputTensor, 1,
            outputNames, 1);
        const float* data = outputs[0].GetTensorMutableData<float>();
        std::memcpy(outputData, data, DetectionConst::NUM_CLASSES * sizeof(float));
    } catch (const Ort::Exception& e) {
        qWarning() << "YOLO inference failed:" << e.what();
        std::memset(outputData, 0, DetectionConst::NUM_CLASSES * sizeof(float));
    }
}

ClassificationResult YoloClassifier::classify(const QImage& image, int row, int col,
                                              const QRect& targetRect)
{
    ClassificationResult result;
    result.patchRow = row;
    result.patchCol = col;

    if (!m_session) return result;

    // 裁剪中心: 优先 UXO 紧包框中心 (与红框同源), 无效时回退格子中心
    int centerX = 0;
    int centerY = 0;
    int baseSize = DetectionConst::PATCH_SIZE;
    if (targetRect.isValid()) {
        centerX = targetRect.center().x();
        centerY = targetRect.center().y();
        // TTA 最小档 0.75x 需保底上下文, 避免过小裁剪放大噪声
        baseSize = std::max(std::max(targetRect.width(), targetRect.height()),
                            DetectionConst::PATCH_SIZE / 2);
    } else {
        centerX = col * DetectionConst::PATCH_SIZE + DetectionConst::PATCH_SIZE / 2;
        centerY = row * DetectionConst::PATCH_SIZE + DetectionConst::PATCH_SIZE / 2;
    }

    // Accumulate probabilities over all TTA runs
    float avgProbs[DetectionConst::NUM_CLASSES] = {};
    int numRuns = 0;

    for (int s = 0; s < DetectionConst::NUM_SCALES; s++) {
        float scale = DetectionConst::MULTISCALE_SCALES[s];
        int cropSize = static_cast<int>(baseSize * scale);
        int half = cropSize / 2;

        // Clamp crop to image bounds [0, 512]
        int x1 = std::max(0, centerX - half);
        int y1 = std::max(0, centerY - half);
        int x2 = std::min(DetectionConst::IMAGE_SIZE, centerX + half);
        int y2 = std::min(DetectionConst::IMAGE_SIZE, centerY + half);

        QImage crop = image.copy(x1, y1, x2 - x1, y2 - y1);
        crop = crop.scaled(DetectionConst::YOLO_INPUT_SIZE, DetectionConst::YOLO_INPUT_SIZE,
                           Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        crop = crop.convertToFormat(QImage::Format_RGB888);

        for (int f = 0; f < DetectionConst::NUM_FLIPS; f++) {
            QImage input = crop;
            if (DetectionConst::MULTISCALE_FLIPS[f]) {
                input = input.mirrored(true, false);
            }

            std::vector<float> inputData;
            preprocessCrop(input, inputData);

            float probs[DetectionConst::NUM_CLASSES];
            runInference(inputData.data(), probs);

            for (int c = 0; c < DetectionConst::NUM_CLASSES; c++) {
                avgProbs[c] += probs[c];
            }
            numRuns++;
        }
    }

    // Average
    for (int c = 0; c < DetectionConst::NUM_CLASSES; c++) {
        avgProbs[c] /= numRuns;
        result.probs[c] = avgProbs[c];
    }

    // Find best non-bg class above threshold
    int bestClass = -1;
    float bestProb = m_threshold;
    for (int c = 0; c < DetectionConst::NUM_CLASSES; c++) {
        if (c == DetectionConst::BG_IDX) continue;
        if (avgProbs[c] > bestProb) {
            bestProb = avgProbs[c];
            bestClass = c;
        }
    }

    result.bestClass = bestClass;
    result.confidence = bestClass >= 0 ? avgProbs[bestClass] : 0.0f;
    if (bestClass >= 0) {
        result.bestClassName = QString(DetectionConst::CLASS_NAMES[bestClass]);
    }

    return result;
}
