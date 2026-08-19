#include "MainWindow/DetectionView.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "Common/GlobalStyle.h"

namespace {

// YOLO 类名 -> 中文类型名（显示用；TargetType 枚举映射在 MainWindow）
QString uxoTypeDisplayName(const QString &className)
{
    if (className == QStringLiteral("aircraft-bombs")) return QStringLiteral("航弹");
    if (className == QStringLiteral("landmines")) return QStringLiteral("反跑道雷");
    if (className == QStringLiteral("rockets")) return QStringLiteral("火箭弹");
    if (className == QStringLiteral("submunitions")) return QStringLiteral("子母弹");
    if (className == QStringLiteral("mortars")) return QStringLiteral("迫击炮弹");
    if (className == QStringLiteral("grenades")) return QStringLiteral("手榴弹");
    if (className == QStringLiteral("projectiles")) return QStringLiteral("投射物");
    if (className == QStringLiteral("fuzes")) return QStringLiteral("引信");
    return QStringLiteral("未分类");
}

QString formatVideoTime(qint64 timestampMs)
{
    const qint64 totalSec = timestampMs / 1000;
    return QStringLiteral("%1:%2")
        .arg(totalSec / 60, 2, 10, QLatin1Char('0'))
        .arg(totalSec % 60, 2, 10, QLatin1Char('0'));
}

// 取置信度最高的有效分类
const ClassificationResult *bestClassification(const ImageDetectionResult &result)
{
    const ClassificationResult *best = nullptr;
    for (const auto &c : result.classifications) {
        if (c.bestClass >= 0 && (best == nullptr || c.confidence > best->confidence)) {
            best = &c;
        }
    }
    return best;
}

// Top-3 非 background 分类文本
QString top3ClassificationText(const ImageDetectionResult &result)
{
    const ClassificationResult *cls = bestClassification(result);
    if (cls == nullptr) {
        return QStringLiteral("分类: 无（未达分类阈值或无异常）");
    }

    QVector<QPair<int, float>> ranked;
    for (int i = 0; i < DetectionConst::NUM_CLASSES; ++i) {
        if (i == DetectionConst::BG_IDX) continue;
        ranked.append({i, cls->probs[i]});
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    QStringList parts;
    const int count = qMin(3, ranked.size());
    for (int i = 0; i < count; ++i) {
        parts << QStringLiteral("%1 %2%")
                     .arg(QString::fromLatin1(DetectionConst::CLASS_NAMES[ranked[i].first]))
                     .arg(static_cast<int>(ranked[i].second * 100));
    }
    return QStringLiteral("分类: ") + parts.join(QStringLiteral("  |  "));
}

constexpr int kColTargetId = 0;
constexpr int kColType = 1;
constexpr int kColThreat = 2;
constexpr int kColConfidence = 3;
constexpr int kColTime = 4;
constexpr int kColStatus = 5;
constexpr int kColSource = 6;
constexpr int kColCount = 7;

} // namespace

DetectionView::DetectionView(QWidget *parent)
    : QWidget(parent)
    , m_resultTable(nullptr)
    , m_viewerLabel(nullptr)
    , m_classLabel(nullptr)
    , m_splitter(nullptr)
    , m_confirmBtn(nullptr)
    , m_rejectBtn(nullptr)
    , m_statusLabel(nullptr)
    , m_summaryLabel(nullptr)
    , m_detailLabel(nullptr)
    , m_heatmapLabel(nullptr)
    , m_timelineLabel(nullptr)
    , m_currentIndex(-1)
{
    setupUi();
}

DetectionView::~DetectionView() = default;

void DetectionView::setupUi()
{
    setStyleSheet(QString("background-color: %1;")
                  .arg(GlobalStyle::Colors::Background));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部摘要条：AI 检测标识 + 计数
    auto *topBar = new QWidget(this);
    topBar->setFixedHeight(32);
    topBar->setStyleSheet(
        QString("background-color: %1; border-bottom: 1px solid %2;")
            .arg(GlobalStyle::Colors::ToolbarBackground)
            .arg(GlobalStyle::Colors::Border));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(12, 4, 12, 4);
    topLayout->setSpacing(8);

    auto *aiTag = new QLabel(QStringLiteral("[AI] 自动检测"), topBar);
    aiTag->setStyleSheet(
        QString("color: %1; font-size: 12px; font-weight: bold; border: none;")
            .arg(GlobalStyle::Colors::ThreatMedium));
    topLayout->addWidget(aiTag);
    topLayout->addStretch();

    m_summaryLabel = new QLabel(topBar);
    m_summaryLabel->setStyleSheet(
        QString("color: %1; font-size: 12px; border: none;")
            .arg(GlobalStyle::Colors::TextSecondary));
    topLayout->addWidget(m_summaryLabel);

    mainLayout->addWidget(topBar);

    // 三栏 splitter：结果表(360) | 证据查看器(弹性) | 详情(380)
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setStyleSheet(
        QString("QSplitter::handle { background-color: %1; }")
            .arg(GlobalStyle::Colors::Border));

    // 左栏：检测结果表
    auto *leftPanel = new QWidget(m_splitter);
    leftPanel->setStyleSheet(
        QString("background-color: %1;")
            .arg(GlobalStyle::Colors::PanelBackground));
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    auto *leftTitle = new QLabel(QStringLiteral("检测结果"), leftPanel);
    leftTitle->setStyleSheet(
        QString("color: %1; font-size: %2px; font-weight: bold; "
                "padding: 8px 12px; border-bottom: 1px solid %3;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::TitleSize)
            .arg(GlobalStyle::Colors::Border));
    leftLayout->addWidget(leftTitle);

    m_resultTable = new QTableWidget(0, kColCount, leftPanel);
    m_resultTable->setObjectName(QStringLiteral("detectionResultTable"));
    m_resultTable->setHorizontalHeaderLabels({
        QStringLiteral("目标 ID"),
        QStringLiteral("类型"),
        QStringLiteral("威胁"),
        QStringLiteral("置信度"),
        QStringLiteral("时间"),
        QStringLiteral("状态"),
        QStringLiteral("探测源"),
    });
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->verticalHeader()->hide();
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setStyleSheet(
        QString("QTableWidget { background-color: %1; color: %2; border: none; "
                "font-size: %3px; gridline-color: %4; }"
                "QTableWidget::item { padding: 3px 6px; }"
                "QTableWidget::item:selected { background-color: %5; }"
                "QHeaderView::section { background-color: %6; color: %2; "
                "border: none; border-bottom: 1px solid %4; padding: 4px 6px; }")
            .arg(GlobalStyle::Colors::PanelBackground)
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::CaptionSize)
            .arg(GlobalStyle::Colors::Border)
            .arg(GlobalStyle::Colors::SelectionBackground)
            .arg(GlobalStyle::Colors::ToolbarBackground));
    leftLayout->addWidget(m_resultTable, 1);

    // 中栏：证据查看器
    auto *centerPanel = new QWidget(m_splitter);
    centerPanel->setStyleSheet(
        QString("background-color: %1;")
            .arg(GlobalStyle::Colors::Background));
    auto *centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_viewerLabel = new QLabel(centerPanel);
    m_viewerLabel->setAlignment(Qt::AlignCenter);
    m_viewerLabel->setMinimumSize(400, 400);
    m_viewerLabel->setStyleSheet(
        QString("background-color: %1; color: %2;")
            .arg(GlobalStyle::Colors::Background)
            .arg(GlobalStyle::Colors::TextSecondary));
    centerLayout->addWidget(m_viewerLabel, 1);

    m_classLabel = new QLabel(centerPanel);
    m_classLabel->setStyleSheet(
        QString("color: %1; font-size: %2px; padding: 6px 12px; "
                "background-color: %3; border-top: 1px solid %4;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::BodySize)
            .arg(GlobalStyle::Colors::ToolbarBackground)
            .arg(GlobalStyle::Colors::Border));
    centerLayout->addWidget(m_classLabel);

    // 右栏：目标详情 + 状态时间线
    auto *rightPanel = new QWidget(m_splitter);
    rightPanel->setStyleSheet(
        QString("background-color: %1;")
            .arg(GlobalStyle::Colors::PanelBackground));
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto *detailTitle = new QLabel(QStringLiteral("目标详情"), rightPanel);
    detailTitle->setStyleSheet(
        QString("color: %1; font-size: %2px; font-weight: bold; "
                "padding: 8px 12px; border-bottom: 1px solid %3;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::TitleSize)
            .arg(GlobalStyle::Colors::Border));
    rightLayout->addWidget(detailTitle);

    m_detailLabel = new QLabel(rightPanel);
    m_detailLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setTextFormat(Qt::PlainText);
    m_detailLabel->setStyleSheet(
        QString("color: %1; font-size: %2px; padding: 8px 12px;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::BodySize));
    rightLayout->addWidget(m_detailLabel, 2);

    auto *heatmapTitle = new QLabel(QStringLiteral("异常热力图"), rightPanel);
    heatmapTitle->setStyleSheet(
        QString("color: %1; font-size: %2px; padding: 6px 12px; "
                "border-top: 1px solid %3;")
            .arg(GlobalStyle::Colors::TextSecondary)
            .arg(GlobalStyle::Fonts::CaptionSize)
            .arg(GlobalStyle::Colors::Border));
    rightLayout->addWidget(heatmapTitle);

    m_heatmapLabel = new QLabel(rightPanel);
    m_heatmapLabel->setAlignment(Qt::AlignCenter);
    m_heatmapLabel->setMinimumSize(200, 200);
    m_heatmapLabel->setStyleSheet(
        QString("color: %1; font-size: %2px; background-color: %3;")
            .arg(GlobalStyle::Colors::TextSecondary)
            .arg(GlobalStyle::Fonts::CaptionSize)
            .arg(GlobalStyle::Colors::Background));
    rightLayout->addWidget(m_heatmapLabel, 2);

    auto *timelineTitle = new QLabel(QStringLiteral("状态时间线"), rightPanel);
    timelineTitle->setStyleSheet(
        QString("color: %1; font-size: %2px; padding: 6px 12px; "
                "border-top: 1px solid %3;")
            .arg(GlobalStyle::Colors::TextSecondary)
            .arg(GlobalStyle::Fonts::CaptionSize)
            .arg(GlobalStyle::Colors::Border));
    rightLayout->addWidget(timelineTitle);

    m_timelineLabel = new QLabel(rightPanel);
    m_timelineLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_timelineLabel->setWordWrap(true);
    m_timelineLabel->setTextFormat(Qt::PlainText);
    m_timelineLabel->setStyleSheet(
        QString("color: %1; font-size: %2px; padding: 4px 12px 8px 12px;")
            .arg(GlobalStyle::Colors::TextSecondary)
            .arg(GlobalStyle::Fonts::CaptionSize));
    rightLayout->addWidget(m_timelineLabel, 1);

    m_splitter->addWidget(leftPanel);
    m_splitter->addWidget(centerPanel);
    m_splitter->addWidget(rightPanel);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    leftPanel->setFixedWidth(360);
    rightPanel->setFixedWidth(380);

    mainLayout->addWidget(m_splitter, 1);

    // 底部确认操作条
    auto *actionBar = new QWidget(this);
    actionBar->setFixedHeight(44);
    actionBar->setStyleSheet(
        QString("background-color: %1; border-top: 1px solid %2;")
            .arg(GlobalStyle::Colors::ToolbarBackground)
            .arg(GlobalStyle::Colors::Border));
    auto *actionLayout = new QHBoxLayout(actionBar);
    actionLayout->setContentsMargins(12, 6, 12, 6);
    actionLayout->setSpacing(8);

    m_confirmBtn = new QPushButton(QStringLiteral("确认"), actionBar);
    m_confirmBtn->setObjectName(QStringLiteral("detectionConfirmButton"));
    m_confirmBtn->setStyleSheet(GlobalStyle::getButtonStyle(true));
    m_confirmBtn->setEnabled(false);

    m_rejectBtn = new QPushButton(QStringLiteral("拒绝"), actionBar);
    m_rejectBtn->setObjectName(QStringLiteral("detectionRejectButton"));
    m_rejectBtn->setStyleSheet(GlobalStyle::getButtonStyle(false));
    m_rejectBtn->setEnabled(false);

    m_statusLabel = new QLabel(actionBar);
    m_statusLabel->setStyleSheet(
        QString("color: %1; font-size: 12px; border: none;")
            .arg(GlobalStyle::Colors::TextSecondary));

    actionLayout->addWidget(m_confirmBtn);
    actionLayout->addWidget(m_rejectBtn);
    actionLayout->addSpacing(16);
    actionLayout->addWidget(m_statusLabel, 1);

    mainLayout->addWidget(actionBar);

    connect(m_resultTable, &QTableWidget::cellClicked,
            this, &DetectionView::onResultSelected);
    connect(m_confirmBtn, &QPushButton::clicked,
            this, &DetectionView::onConfirmClicked);
    connect(m_rejectBtn, &QPushButton::clicked,
            this, &DetectionView::onRejectClicked);

    showEmptyState();
    updateSummaryLabel();
}

void DetectionView::onFrameAnalyzed(const ImageDetectionResult &result,
                                    const QString &targetId)
{
    DetectionRecord record;
    record.result = result;
    record.targetId = targetId;
    record.analyzedAt = QDateTime::currentDateTime();
    m_records.append(record);

    const int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    const bool anomalous = result.hasAnomaly;
    const ClassificationResult *cls = bestClassification(result);
    const QString typeName = anomalous
        ? (cls != nullptr ? uxoTypeDisplayName(cls->bestClassName)
                          : QStringLiteral("未分类"))
        : QStringLiteral("正常");

    auto *idItem = new QTableWidgetItem(anomalous ? targetId : QStringLiteral("--"));
    auto *typeItem = new QTableWidgetItem(typeName);
    auto *threatItem = new QTableWidgetItem(anomalous ? QStringLiteral("高")
                                                       : QStringLiteral("--"));
    auto *confItem = new QTableWidgetItem(
        cls != nullptr
            ? QStringLiteral("%1%").arg(static_cast<int>(cls->confidence * 100))
            : QStringLiteral("--"));
    auto *timeItem = new QTableWidgetItem(formatVideoTime(result.timestampMs));
    auto *statusItem = new QTableWidgetItem(anomalous ? QStringLiteral("已发现")
                                                       : QStringLiteral("--"));
    auto *sourceItem = new QTableWidgetItem(QStringLiteral("AI 分析"));

    const QColor typeColor = anomalous ? QColor(GlobalStyle::Colors::DangerRed)
                                       : QColor(GlobalStyle::Colors::PrimaryGreen);
    typeItem->setForeground(typeColor);
    if (anomalous) {
        threatItem->setForeground(QColor(GlobalStyle::Colors::DangerRed));
        statusItem->setForeground(QColor(GlobalStyle::Colors::DangerRed));
    }

    m_resultTable->setItem(row, kColTargetId, idItem);
    m_resultTable->setItem(row, kColType, typeItem);
    m_resultTable->setItem(row, kColThreat, threatItem);
    m_resultTable->setItem(row, kColConfidence, confItem);
    m_resultTable->setItem(row, kColTime, timeItem);
    m_resultTable->setItem(row, kColStatus, statusItem);
    m_resultTable->setItem(row, kColSource, sourceItem);

    m_resultTable->scrollToBottom();
    updateSummaryLabel();
}

void DetectionView::clearResults()
{
    m_records.clear();
    m_resultTable->setRowCount(0);
    m_currentIndex = -1;
    m_currentImage = QImage();
    m_currentHeatmap = QImage();
    showEmptyState();
    updateSummaryLabel();
    m_confirmBtn->setEnabled(false);
    m_rejectBtn->setEnabled(false);
}

void DetectionView::onResultSelected(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0 || row >= m_records.size()) {
        return;
    }
    displayRecord(row);
}

void DetectionView::displayRecord(int index)
{
    m_currentIndex = index;
    const DetectionRecord &record = m_records[index];
    const ImageDetectionResult &result = record.result;

    // 中栏显示红框标注图（异常帧）；热力图叠加图独立展示在右侧模块
    m_currentImage = (result.hasAnomaly && !result.annotatedImage.isNull())
                         ? result.annotatedImage
                         : result.originalImage;
    if (!m_currentImage.isNull()) {
        QPixmap pm = QPixmap::fromImage(m_currentImage);
        pm = pm.scaled(m_viewerLabel->size(), Qt::KeepAspectRatio,
                       Qt::SmoothTransformation);
        m_viewerLabel->setPixmap(pm);
    }

    m_currentHeatmap = result.heatmapOverlay;
    if (!m_currentHeatmap.isNull()) {
        QPixmap hpm = QPixmap::fromImage(m_currentHeatmap);
        hpm = hpm.scaled(m_heatmapLabel->size(), Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
        m_heatmapLabel->setPixmap(hpm);
    } else {
        m_heatmapLabel->setPixmap(QPixmap());
        m_heatmapLabel->setText(QStringLiteral("无热力图"));
    }

    m_classLabel->setText(top3ClassificationText(result));

    // 目标详情
    const ClassificationResult *cls = bestClassification(result);
    QStringList detailLines;
    detailLines << QStringLiteral("目标 ID: %1")
                      .arg(record.targetId.isEmpty()
                               ? QStringLiteral("--")
                               : record.targetId);
    detailLines << QStringLiteral("类型: %1").arg(
        result.hasAnomaly
            ? (cls != nullptr ? uxoTypeDisplayName(cls->bestClassName)
                              : QStringLiteral("未分类"))
            : QStringLiteral("正常"));
    detailLines << QStringLiteral("威胁等级: %1").arg(
        result.hasAnomaly ? QStringLiteral("高") : QStringLiteral("--"));
    detailLines << QStringLiteral("置信度: %1").arg(
        cls != nullptr
            ? QStringLiteral("%1%").arg(static_cast<int>(cls->confidence * 100))
            : QStringLiteral("--"));
    detailLines << QStringLiteral("最大异常分: %1")
                      .arg(result.maxAnomalyScore, 0, 'f', 4);
    detailLines << QStringLiteral("帧时间: %1").arg(
        formatVideoTime(result.timestampMs));
    detailLines << QStringLiteral("推理耗时: %1 ms").arg(result.processingTimeMs);
    detailLines << QStringLiteral("探测源: AI 分析");
    m_detailLabel->setText(detailLines.join(QLatin1Char('\n')));

    // 状态时间线
    QStringList timeline;
    timeline << QStringLiteral("%1  已发现（AI）")
                   .arg(record.analyzedAt.toString(QStringLiteral("HH:mm:ss")));
    if (record.review == DetectionReview::Confirmed) {
        timeline << QStringLiteral("%1  已确认（人工）")
                       .arg(record.reviewedAt.toString(QStringLiteral("HH:mm:ss")));
    } else if (record.review == DetectionReview::Rejected) {
        timeline << QStringLiteral("%1  已拒绝（人工）")
                       .arg(record.reviewedAt.toString(QStringLiteral("HH:mm:ss")));
    }
    m_timelineLabel->setText(timeline.join(QLatin1Char('\n')));

    updateActionBar();

    if (!record.targetId.isEmpty()) {
        emit resultSelected(record.targetId);
    }
}

void DetectionView::onConfirmClicked()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_records.size()) {
        return;
    }
    DetectionRecord &record = m_records[m_currentIndex];
    if (record.targetId.isEmpty() || record.review != DetectionReview::Pending) {
        return;
    }
    record.review = DetectionReview::Confirmed;
    record.reviewedAt = QDateTime::currentDateTime();
    if (auto *statusItem = m_resultTable->item(m_currentIndex, kColStatus)) {
        statusItem->setText(QStringLiteral("已确认"));
    }
    emit targetConfirmed(record.targetId);
    displayRecord(m_currentIndex);
}

void DetectionView::onRejectClicked()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_records.size()) {
        return;
    }
    DetectionRecord &record = m_records[m_currentIndex];
    if (record.targetId.isEmpty() || record.review != DetectionReview::Pending) {
        return;
    }
    record.review = DetectionReview::Rejected;
    record.reviewedAt = QDateTime::currentDateTime();
    if (auto *statusItem = m_resultTable->item(m_currentIndex, kColStatus)) {
        statusItem->setText(QStringLiteral("已拒绝"));
    }
    emit targetRejected(record.targetId);
    displayRecord(m_currentIndex);
}

void DetectionView::updateActionBar()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_records.size()) {
        m_confirmBtn->setEnabled(false);
        m_rejectBtn->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("未选中结果"));
        return;
    }

    const DetectionRecord &record = m_records[m_currentIndex];
    const bool reviewable = !record.targetId.isEmpty()
        && record.review == DetectionReview::Pending;
    m_confirmBtn->setEnabled(reviewable);
    m_rejectBtn->setEnabled(reviewable);

    m_statusLabel->setText(
        QStringLiteral("当前目标: %1 · 状态: %2")
            .arg(record.targetId.isEmpty() ? QStringLiteral("--")
                                           : record.targetId)
            .arg(record.targetId.isEmpty() ? QStringLiteral("正常帧")
                                           : reviewText(record.review)));
}

void DetectionView::updateSummaryLabel()
{
    int anomalyCount = 0;
    for (const auto &r : m_records) {
        if (r.result.hasAnomaly) anomalyCount++;
    }
    m_summaryLabel->setText(
        QStringLiteral("已分析 %1 帧 · 异常 %2")
            .arg(m_records.size())
            .arg(anomalyCount));
}

void DetectionView::showEmptyState()
{
    m_viewerLabel->setPixmap(QPixmap());
    m_viewerLabel->setText(QStringLiteral("等待检测结果"));
    m_heatmapLabel->setPixmap(QPixmap());
    m_heatmapLabel->setText(QStringLiteral("无热力图"));
    m_classLabel->setText(QStringLiteral("分类: --"));
    m_detailLabel->setText(QStringLiteral("--"));
    m_timelineLabel->setText(QStringLiteral("--"));
    m_statusLabel->setText(QStringLiteral("等待检测结果"));
}

QString DetectionView::reviewText(DetectionReview review) const
{
    switch (review) {
    case DetectionReview::Confirmed: return QStringLiteral("已确认");
    case DetectionReview::Rejected: return QStringLiteral("已拒绝");
    case DetectionReview::Pending:
    default: return QStringLiteral("已发现");
    }
}

void DetectionView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!m_currentImage.isNull() && m_viewerLabel != nullptr) {
        QPixmap pm = QPixmap::fromImage(m_currentImage);
        pm = pm.scaled(m_viewerLabel->size(), Qt::KeepAspectRatio,
                       Qt::SmoothTransformation);
        m_viewerLabel->setPixmap(pm);
    }
    if (!m_currentHeatmap.isNull() && m_heatmapLabel != nullptr) {
        QPixmap hpm = QPixmap::fromImage(m_currentHeatmap);
        hpm = hpm.scaled(m_heatmapLabel->size(), Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
        m_heatmapLabel->setPixmap(hpm);
    }
}
