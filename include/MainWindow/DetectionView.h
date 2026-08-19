#ifndef DETECTIONVIEW_H
#define DETECTIONVIEW_H

// 探测页：AI 自动检测工作区。
// 三栏布局：左侧检测结果表(自动填充) + 中心证据查看器(干净原图) + 右侧异常热力图/目标详情/状态时间线。
// 底部确认操作条：[确认] [拒绝] 人工二次校验，状态机 Detected -> Confirmed/Rejected。
// 检测结果由 MainWindow 从 DetectionEngine 转发，本类不拥有 Engine。
// 探测源标注"AI 分析"，与态势页遥测"模拟"标识区分（功能文档 §6.6）。

#include "Detection/DetectionTypes.h"

#include <QWidget>
#include <QVector>
#include <QDateTime>

class QTableWidget;
class QLabel;
class QPushButton;
class QSplitter;

enum class DetectionReview {
    Pending,
    Confirmed,
    Rejected
};

// 单帧分析结果 + 人工校验状态
struct DetectionRecord {
    ImageDetectionResult result;
    QString targetId;   // 异常帧对应的目标 ID；正常帧为空
    QDateTime analyzedAt;
    DetectionReview review = DetectionReview::Pending;
    QDateTime reviewedAt;   // 人工确认/拒绝时间，动作发生时记录（时间线展示用）
};

class DetectionView : public QWidget
{
    Q_OBJECT
public:
    explicit DetectionView(QWidget *parent = nullptr);
    ~DetectionView() override;

    // MainWindow 转发单帧分析结果；异常帧 targetId 为四区同步创建的目标 ID
    void onFrameAnalyzed(const ImageDetectionResult &result, const QString &targetId);
    // 清空全部结果（[重置] 时由 MainWindow 调用）
    void clearResults();

signals:
    void targetConfirmed(const QString &targetId);
    void targetRejected(const QString &targetId);
    // 选中异常结果行时联动态势页（目标表/地图/详情浮层）
    void resultSelected(const QString &targetId);

private slots:
    void onResultSelected(int row, int column);
    void onConfirmClicked();
    void onRejectClicked();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUi();
    void displayRecord(int index);
    void updateActionBar();
    void updateSummaryLabel();
    void showEmptyState();
    QString reviewText(DetectionReview review) const;

    QTableWidget *m_resultTable;
    QLabel *m_viewerLabel;
    QLabel *m_classLabel;
    QSplitter *m_splitter;
    QPushButton *m_confirmBtn;
    QPushButton *m_rejectBtn;
    QLabel *m_statusLabel;
    QLabel *m_summaryLabel;
    QLabel *m_detailLabel;
    QLabel *m_heatmapLabel;
    QLabel *m_timelineLabel;

    QVector<DetectionRecord> m_records;
    QImage m_currentImage;   // 中栏当前原图（resize 时重绘）
    QImage m_currentHeatmap; // 右栏当前热力图（resize 时重绘）
    int m_currentIndex;
};

#endif // DETECTIONVIEW_H
