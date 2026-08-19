#include "Detection/PatchCoreDetector.h"
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <limits>

PatchCoreDetector::PatchCoreDetector(Ort::Env& env, const QString& modelPath,
                                     float imageMin, float imageMax, float threshold)
    : m_env(env)
    , m_imageMin(imageMin)
    , m_imageMax(imageMax)
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

        // 缓存 I/O 张量名 (拷贝字符串 - 智能指针会释放原始内存)
        Ort::AllocatorWithDefaultOptions allocator;
        m_inputName   = m_session->GetInputNameAllocated(0, allocator).get();
        m_outputName0 = m_session->GetOutputNameAllocated(0, allocator).get();
        m_outputName1 = m_session->GetOutputNameAllocated(1, allocator).get();

        qDebug() << "PatchCore model loaded:" << modelPath
                 << "  in:" << m_inputName.c_str()
                 << "  out0:" << m_outputName0.c_str()
                 << "  out1:" << m_outputName1.c_str();
    } catch (const Ort::Exception& e) {
        qWarning() << "Failed to load PatchCore model:" << e.what();
        m_session.reset();
    }
}

PatchCoreDetector::~PatchCoreDetector() = default;

void PatchCoreDetector::preprocess(const QImage& image, std::vector<float>& output)
{
    // 契约: 本函数按 RGB888 紧凑像素排列直读缓冲区; 输入格式不符时强制转换,
    // 防止上游(Qt 平滑缩放会输出 RGB32)的格式回归悄悄毒化输入张量
    QImage src = image;
    if (src.format() != QImage::Format_RGB888) {
        src = src.convertToFormat(QImage::Format_RGB888);
    }

    // 512x512 RGB888 -> [1, 3, 512, 512] NCHW float, 含 ImageNet 归一化
    // (与 v07 训练/评估门预处理一致: RGB -> [0,1] -> ImageNet 归一化)
    constexpr int C = 3;
    constexpr int H = DetectionConst::IMAGE_SIZE;
    constexpr int W = DetectionConst::IMAGE_SIZE;
    constexpr int channelSize = H * W;
    output.resize(C * channelSize);

    const float inv255 = 1.0f / 255.0f;

    for (int y = 0; y < H; y++) {
        const uchar* scan = src.constScanLine(y);
        for (int x = 0; x < W; x++) {
            float r = scan[x * 3 + 0] * inv255;
            float g = scan[x * 3 + 1] * inv255;
            float b = scan[x * 3 + 2] * inv255;

            r = (r - DetectionConst::IMAGENET_MEAN[0]) / DetectionConst::IMAGENET_STD[0];
            g = (g - DetectionConst::IMAGENET_MEAN[1]) / DetectionConst::IMAGENET_STD[1];
            b = (b - DetectionConst::IMAGENET_MEAN[2]) / DetectionConst::IMAGENET_STD[2];

            // NCHW: 通道优先排布
            output[0 * channelSize + y * W + x] = r;
            output[1 * channelSize + y * W + x] = g;
            output[2 * channelSize + y * W + x] = b;
        }
    }
}

QImage PatchCoreDetector::generateCellHeatmap(const float* mapData, int regionH,
                                              int regionW, int stride,
                                              float minVal, float maxVal)
{
    // 分区区域按原生分辨率渲染; 非 128x128 时缩放对齐 (当前模型 map 为
    // 512x512, 4x4 分区恰为 128x128, 缩放分支仅为形状防御)
    QImage heatmap(regionW, regionH, QImage::Format_ARGB32);
    float range = maxVal - minVal;
    if (range < 1e-6f) range = 1.0f;

    for (int y = 0; y < regionH; y++) {
        QRgb* line = reinterpret_cast<QRgb*>(heatmap.scanLine(y));
        for (int x = 0; x < regionW; x++) {
            float v = (mapData[y * stride + x] - minVal) / range;
            v = std::clamp(v, 0.0f, 1.0f);

            // Jet 伪彩色映射
            float r = std::clamp(1.5f - std::abs(4.0f * v - 3.0f), 0.0f, 1.0f);
            float g = std::clamp(1.5f - std::abs(4.0f * v - 2.0f), 0.0f, 1.0f);
            float b = std::clamp(1.5f - std::abs(4.0f * v - 1.0f), 0.0f, 1.0f);

            line[x] = qRgba(static_cast<int>(r * 255),
                            static_cast<int>(g * 255),
                            static_cast<int>(b * 255), 180);
        }
    }

    if (regionH != DetectionConst::PATCH_SIZE || regionW != DetectionConst::PATCH_SIZE) {
        return heatmap.scaled(DetectionConst::PATCH_SIZE, DetectionConst::PATCH_SIZE,
                              Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return heatmap;
}

QVector<PatchResult> PatchCoreDetector::detect(const QImage& image)
{
    QVector<PatchResult> results;
    if (!m_session) return results;

    // 预处理: 整帧单张 [1, 3, 512, 512]
    std::vector<float> inputData;
    preprocess(image, inputData);

    // 构造输入张量 [1, 3, 512, 512]
    auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> inputShape = {
        1, 3, DetectionConst::IMAGE_SIZE, DetectionConst::IMAGE_SIZE
    };
    auto inputTensor = Ort::Value::CreateTensor<float>(
        memInfo, inputData.data(), inputData.size(),
        inputShape.data(), inputShape.size());

    // 执行推理
    const char* inputNames[]  = { m_inputName.c_str() };
    const char* outputNames[] = { m_outputName0.c_str(), m_outputName1.c_str() };

    std::vector<Ort::Value> outputs;
    try {
        outputs = m_session->Run(
            Ort::RunOptions{nullptr},
            inputNames, &inputTensor, 1,
            outputNames, 2);
    } catch (const Ort::Exception& e) {
        qWarning() << "PatchCore inference failed:" << e.what();
        return results;
    }

    // outputs[0] = pred_score[1]   帧级异常分 (评估门 FP=0/recall=99.6% 验证的量)
    // outputs[1] = anomaly_map[1,1,H,W]
    const float frameScore = outputs[0].GetTensorMutableData<float>()[0];
    const float* maps = outputs[1].GetTensorMutableData<float>();
    const auto mapShape = outputs[1].GetTensorTypeAndShapeInfo().GetShape();
    const int mapH = static_cast<int>(mapShape[2]);
    const int mapW = static_cast<int>(mapShape[3]);
    const int cellH = mapH / DetectionConst::GRID_SIZE;
    const int cellW = mapW / DetectionConst::GRID_SIZE;

    // 全图 min/max 用于热力图显示归一化 (与旧 16 块口径的全局归一化一致)
    float mapMin = std::numeric_limits<float>::max();
    float mapMax = std::numeric_limits<float>::lowest();
    for (int i = 0; i < mapH * mapW; i++) {
        mapMin = std::min(mapMin, maps[i]);
        mapMax = std::max(mapMax, maps[i]);
    }

    // 各 4x4 分区 max + 峰值格定位 (map 峰值即目标位置, 评估门定位 90.2%<=40px)
    float cellMax[DetectionConst::NUM_PATCHES];
    int peakIdx = 0;
    for (int r = 0; r < DetectionConst::GRID_SIZE; r++) {
        for (int c = 0; c < DetectionConst::GRID_SIZE; c++) {
            const int idx = r * DetectionConst::GRID_SIZE + c;
            const float* region = maps + (r * cellH) * mapW + (c * cellW);
            float m = std::numeric_limits<float>::lowest();
            for (int y = 0; y < cellH; y++) {
                const float* line = region + y * mapW;
                for (int x = 0; x < cellW; x++) {
                    m = std::max(m, line[x]);
                }
            }
            cellMax[idx] = m;
            if (m > cellMax[peakIdx]) peakIdx = idx;
        }
    }

    // 帧级判定必须用 pred_score: 实测它与 map.max 双向不等 (干净帧 +0.07/异常帧
    // -0.12), 门验证的是 pred_score; map 仅承担分区定位与热力图
    const float frameNormalized =
        (frameScore - m_imageMin) / (m_imageMax - m_imageMin);
    const bool frameAnomalous = frameNormalized > m_threshold;

    // UXO 紧包框: 与报警判定同阈, 从 amap 峰值连通域推导
    // (红框/YOLO 裁剪/地理定位 videoRect 共用此框)
    QRect targetRect;
    if (frameAnomalous) {
        const float rawThreshold = m_imageMin + m_threshold * (m_imageMax - m_imageMin);
        targetRect = locateTarget(maps, mapH, mapW, rawThreshold, mapMin);
    }

    for (int r = 0; r < DetectionConst::GRID_SIZE; r++) {
        for (int c = 0; c < DetectionConst::GRID_SIZE; c++) {
            const int idx = r * DetectionConst::GRID_SIZE + c;
            PatchResult res;
            res.row = r;
            res.col = c;
            res.rawScore = cellMax[idx];
            if (frameAnomalous && idx == peakIdx) {
                // 峰值格承载帧分数: 帧行为与评估门 (pred_score>阈值) 完全等价,
                // 且该格 normalizedScore 一致地高于阈值
                res.rawScore = std::max(res.rawScore, frameScore);
            }
            res.normalizedScore =
                (res.rawScore - m_imageMin) / (m_imageMax - m_imageMin);
            res.isAnomalous = frameAnomalous && idx == peakIdx;
            if (res.isAnomalous) {
                res.targetRect = targetRect;
            }
            res.heatmap = generateCellHeatmap(maps + (r * cellH) * mapW + (c * cellW),
                                              cellH, cellW, mapW, mapMin, mapMax);
            results.append(res);
        }
    }
    return results;
}

QRect PatchCoreDetector::locateTarget(const float* maps, int mapH, int mapW,
                                      float rawThreshold, float mapMin) const
{
    constexpr int MIN_SIDE = 24;
    constexpr int MARGIN = 10;

    const int n = mapH * mapW;
    int peakPix = 0;
    float peakVal = maps[0];
    for (int i = 1; i < n; i++) {
        if (maps[i] > peakVal) {
            peakVal = maps[i];
            peakPix = i;
        }
    }
    const int peakX = peakPix % mapW;
    const int peakY = peakPix / mapW;

    // 峰值未过阈 (退化场景): 退化为相对电平, 保证连通域至少含峰值像素
    const float level = (peakVal >= rawThreshold)
        ? rawThreshold
        : mapMin + 0.85f * (peakVal - mapMin);

    std::vector<uint8_t> visited(n, 0);
    std::vector<int> stack;
    stack.push_back(peakY * mapW + peakX);
    visited[peakY * mapW + peakX] = 1;
    int minX = peakX, maxX = peakX, minY = peakY, maxY = peakY;
    while (!stack.empty()) {
        const int cur = stack.back();
        stack.pop_back();
        const int cx = cur % mapW;
        const int cy = cur / mapW;
        if (cx < minX) minX = cx;
        if (cx > maxX) maxX = cx;
        if (cy < minY) minY = cy;
        if (cy > maxY) maxY = cy;
        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; k++) {
            const int nx = cx + dx[k];
            const int ny = cy + dy[k];
            if (nx < 0 || nx >= mapW || ny < 0 || ny >= mapH) {
                continue;
            }
            const int ni = ny * mapW + nx;
            if (visited[ni] == 0 && maps[ni] >= level) {
                visited[ni] = 1;
                stack.push_back(ni);
            }
        }
    }

    int x1 = minX - MARGIN;
    int y1 = minY - MARGIN;
    int x2 = maxX + MARGIN;
    int y2 = maxY + MARGIN;
    if (x2 - x1 + 1 < MIN_SIDE) {
        const int cx = (x1 + x2) / 2;
        x1 = cx - MIN_SIDE / 2;
        x2 = x1 + MIN_SIDE - 1;
    }
    if (y2 - y1 + 1 < MIN_SIDE) {
        const int cy = (y1 + y2) / 2;
        y1 = cy - MIN_SIDE / 2;
        y2 = y1 + MIN_SIDE - 1;
    }
    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(mapW - 1, x2);
    y2 = std::min(mapH - 1, y2);
    return QRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}
