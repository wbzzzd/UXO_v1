#include "Detection/DetectionEngine.h"
#include "Detection/PatchCoreDetector.h"
#include "Detection/YoloClassifier.h"

#include <onnxruntime_cxx_api.h>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QElapsedTimer>
#include <QDebug>

#include <atomic>

struct DetectionEngine::Private {
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<PatchCoreDetector> patchcore;
    std::unique_ptr<YoloClassifier> yolo;
    std::atomic<bool> cancelled{false};
    bool initialized = false;
    QFutureWatcher<void>* batchWatcher = nullptr;
};

DetectionEngine::DetectionEngine(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    d->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "Detection");
}

DetectionEngine::~DetectionEngine()
{
    cancel();
    if (d->batchWatcher) {
        d->batchWatcher->waitForFinished();
    }
}

bool DetectionEngine::initialize(const QString& patchcoreModelPath,
                                 const QString& yoloModelPath,
                                 const QString& paramsPath)
{
    // 从 JSON 读取 PatchCore 后处理参数
    QFile paramsFile(paramsPath);
    if (!paramsFile.open(QIODevice::ReadOnly)) {
        emit error(QString("Cannot open params file: %1").arg(paramsPath));
        return false;
    }

    auto doc = QJsonDocument::fromJson(paramsFile.readAll());
    if (!doc.isObject()) {
        emit error("Invalid params JSON format");
        return false;
    }
    auto obj = doc.object();

    float imageMin       = static_cast<float>(obj["image_min"].toDouble());
    float imageMax       = static_cast<float>(obj["image_max"].toDouble());
    float imageThreshold = static_cast<float>(obj["image_threshold"].toDouble());

    // 原始阈值换算到归一化空间
    float normalizedThreshold = (imageThreshold - imageMin) / (imageMax - imageMin);

    qDebug() << "PatchCore params: image_min=" << imageMin
             << " image_max=" << imageMax
             << " image_threshold=" << imageThreshold
             << " -> normalized threshold=" << normalizedThreshold;

    // 创建检测器
    d->patchcore = std::make_unique<PatchCoreDetector>(
        *d->env, patchcoreModelPath, imageMin, imageMax, normalizedThreshold);

    d->yolo = std::make_unique<YoloClassifier>(
        *d->env, yoloModelPath, DetectionConst::DEFAULT_YOLO_THRESHOLD);

    if (!d->patchcore->isLoaded() || !d->yolo->isLoaded()) {
        emit error("Failed to load one or both ONNX models");
        d->patchcore.reset();
        d->yolo.reset();
        return false;
    }

    d->initialized = true;
    return true;
}

bool DetectionEngine::isInitialized() const
{
    return d->initialized;
}

void DetectionEngine::analyzeImage(const QString& imagePath)
{
    auto* watcher = new QFutureWatcher<ImageDetectionResult>(this);
    connect(watcher, &QFutureWatcher<ImageDetectionResult>::finished, this, [this, watcher]() {
        auto result = watcher->result();
        if (!result.error.isEmpty()) {
            emit error(result.error);
        } else {
            emit imageAnalyzed(result);
        }
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([this, imagePath]() {
        return doAnalyze(imagePath);
    }));
}

void DetectionEngine::analyzeFrame(const QImage& frame, qint64 timestampMs)
{
    auto* watcher = new QFutureWatcher<ImageDetectionResult>(this);
    connect(watcher, &QFutureWatcher<ImageDetectionResult>::finished, this, [this, watcher]() {
        auto result = watcher->result();
        if (!result.error.isEmpty()) {
            emit error(result.error);
        } else {
            emit imageAnalyzed(result);
        }
        watcher->deleteLater();
    });

    QImage frameCopy = frame;
    watcher->setFuture(QtConcurrent::run([this, frameCopy, timestampMs]() {
        return doAnalyzeImage(frameCopy, timestampMs);
    }));
}

void DetectionEngine::analyzeBatch(const QStringList& imagePaths)
{
    d->cancelled = false;

    if (!d->batchWatcher) {
        d->batchWatcher = new QFutureWatcher<void>(this);
        connect(d->batchWatcher, &QFutureWatcher<void>::finished, this, [this]() {
            emit batchFinished();
        });
    }

    d->batchWatcher->setFuture(QtConcurrent::run([this, imagePaths]() {
        int total = imagePaths.size();
        for (int i = 0; i < total; ++i) {
            if (d->cancelled) break;

            auto result = doAnalyze(imagePaths[i]);

            if (!result.error.isEmpty()) {
                emit error(result.error);
            } else {
                emit imageAnalyzed(result);
            }
            emit batchProgress(i + 1, total);
        }
    }));
}

void DetectionEngine::cancel()
{
    d->cancelled = true;
}

ImageDetectionResult DetectionEngine::doAnalyze(const QString& imagePath)
{
    QImage image(imagePath);
    if (image.isNull()) {
        ImageDetectionResult result;
        result.imagePath = imagePath;
        result.error = QString("Failed to load image: %1").arg(imagePath);
        return result;
    }

    auto result = doAnalyzeImage(image, 0);
    result.imagePath = imagePath;
    return result;
}

ImageDetectionResult DetectionEngine::doAnalyzeImage(const QImage& image, qint64 timestampMs)
{
    ImageDetectionResult result;
    result.timestampMs = timestampMs;

    QElapsedTimer timer;
    timer.start();

    QImage img = image;
    if (img.size() != QSize(DetectionConst::IMAGE_SIZE, DetectionConst::IMAGE_SIZE)) {
        img = img.scaled(DetectionConst::IMAGE_SIZE, DetectionConst::IMAGE_SIZE,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    // 必须在 scaled 之后转换: Qt 平滑缩放内部走 32 位路径, 会把 RGB888
    // 输入悄悄回退成 RGB32 输出; 先转换会让下游按 RGB888 紧凑排列读张量时错位
    img = img.convertToFormat(QImage::Format_RGB888);
    result.originalImage = img;

    auto patches = d->patchcore->detect(img);
    result.patches = patches;

    for (const auto& p : patches) {
        if (p.normalizedScore > result.maxAnomalyScore) {
            result.maxAnomalyScore = p.normalizedScore;
        }
        if (p.isAnomalous) {
            result.hasAnomaly = true;
        }
    }

    if (result.hasAnomaly) {
        for (const auto& p : patches) {
            if (p.isAnomalous) {
                auto cls = d->yolo->classify(img, p.row, p.col, p.targetRect);
                result.classifications.append(cls);
            }
        }
    }

    result.heatmapOverlay = generateOverlay(img, patches);
    if (result.hasAnomaly) {
        result.annotatedImage = generateAnnotatedImage(img, patches,
                                                       result.classifications);
    }

    result.processingTimeMs = timer.elapsed();

    return result;
}

QImage DetectionEngine::generateOverlay(const QImage& original,
                                        const QVector<PatchResult>& patches)
{
    QImage result = original.convertToFormat(QImage::Format_ARGB32);
    QPainter painter(&result);
    painter.setOpacity(0.45);
    for (const auto& p : patches) {
        if (!p.heatmap.isNull()) {
            painter.drawImage(p.col * DetectionConst::PATCH_SIZE,
                              p.row * DetectionConst::PATCH_SIZE,
                              p.heatmap);
        }
    }
    painter.end();
    return result;
}

QImage DetectionEngine::generateAnnotatedImage(const QImage& original,
                                               const QVector<PatchResult>& patches,
                                               const QVector<ClassificationResult>& classifications)
{
    // 选框格与 MainWindow::createDetectedTarget 的 videoRect 同源:
    // 置信度最高的 YOLO 确认格优先, 无确认时回退异常分最高的格
    const ClassificationResult *bestClass = nullptr;
    for (const auto& c : classifications) {
        if (c.bestClass >= 0 && (bestClass == nullptr || c.confidence > bestClass->confidence)) {
            bestClass = &c;
        }
    }
    const PatchResult *worstPatch = nullptr;
    for (const auto& p : patches) {
        if (p.isAnomalous && (worstPatch == nullptr || p.normalizedScore > worstPatch->normalizedScore)) {
            worstPatch = &p;
        }
    }
    const PatchResult *selected = nullptr;
    if (bestClass != nullptr) {
        for (const auto& p : patches) {
            if (p.row == bestClass->patchRow && p.col == bestClass->patchCol) {
                selected = &p;
                break;
            }
        }
    }
    if (selected == nullptr) {
        selected = worstPatch;
    }
    if (selected == nullptr) {
        return QImage();
    }

    QImage result = original;
    QPainter painter(&result);
    // 警示红检测框色: 证据图红框的既定约定色, 提取为局部常量避免魔法数
    const QColor kBBoxColor(255, 45, 45);
    QPen pen(kBBoxColor);
    pen.setWidth(4);
    painter.setPen(pen);
    if (selected->targetRect.isValid()) {
        painter.drawRect(selected->targetRect);
    } else {
        const int inset = 2;
        painter.drawRect(selected->col * DetectionConst::PATCH_SIZE + inset,
                         selected->row * DetectionConst::PATCH_SIZE + inset,
                         DetectionConst::PATCH_SIZE - 2 * inset,
                         DetectionConst::PATCH_SIZE - 2 * inset);
    }
    painter.end();
    return result;
}
