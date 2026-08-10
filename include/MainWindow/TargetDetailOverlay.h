#ifndef MAINWINDOW_TARGETDETAILOVERLAY_H
#define MAINWINDOW_TARGETDETAILOVERLAY_H

#include <QWidget>
#include <QImage>

#include "Core/Data/Types.h"

class QLabel;
class QPushButton;

// 目标详情研判浮层：选中目标时在地图右上角浮现，关闭后地图恢复干净。
// 设计语义：威胁是整体研判对象，选中即看详情+做研判，3 个操作为模拟反馈不改状态机。
// 视觉与交互对齐 docs/ui/prototypes/situation/index.html 的 .target-detail-overlay。
class TargetDetailOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit TargetDetailOverlay(QWidget *parent = nullptr);
    ~TargetDetailOverlay();

    // 显示指定目标的详情。TargetStatus::Detected/Confirmed/Pending/Disposing/Disposed
    // 视为"已检测"，显示详情+操作；其它视为"待检测"，仅显示提示。
    void showTarget(const Core::TargetInfo &target);

    // 当前展示的目标 ID（无则空串）。
    QString currentTargetId() const;

    // 设置冻结证据快照（标注截图 + 元数据），仅选中已检测目标时由 MainWindow 调用。
    // 空 QImage 视为无证据，显示占位态。
    void setEvidence(const QImage &annotatedImage, const QDateTime &captureTime,
                     qint64 videoPositionMs, const QString &provenance);
    // 清除证据快照，恢复无证据占位态。
    void clearEvidence();
    // 重置浮层：清除当前目标与证据，回到无目标初始态并隐藏。
    void reset();

signals:
    // 3 个模拟研判操作信号，上层接收后给纯文本反馈，不改状态机。
    void createTaskRequested(const Core::TargetInfo &target);
    void assignDeviceRequested(const Core::TargetInfo &target);
    void viewHistoryRequested(const Core::TargetInfo &target);

private slots:
    void onCloseClicked();
    void onCreateTaskClicked();
    void onAssignDeviceClicked();
    void onViewHistoryClicked();

private:
    void setupUi();
    // 刷新详情行与操作区可见性（依据 m_target.status）
    void refreshDetail();
    // 刷新证据视口图像/占位切换（依据 m_hasEvidence）
    void refreshEvidence();
    // 威胁等级中文文案
    QString threatText(Core::ThreatLevel level) const;
    // 威胁等级样式类（high/medium/unknown）
    QString threatClass(Core::ThreatLevel level) const;
    // 设置反馈文本（带"模拟"前缀）
    void setFeedback(const QString &text);

protected:
    // 关闭按钮是浮动子控件，需随尺寸变化重定位到右上角
    void resizeEvent(QResizeEvent *event) override;

    Core::TargetInfo m_target;
    bool m_hasTarget;
    bool m_hasEvidence;

    QWidget *m_evidenceContainer;   // 证据区容器（视口 + 元数据行）
    QWidget *m_evidenceViewport;    // 证据图像视口（316x180，#161616 底）
    QLabel *m_evidenceImageLabel;   // 冻结标注截图（保持宽高比）
    QLabel *m_evidencePlaceholder;  // 无证据占位文本
    QLabel *m_frozenChip;           // 冻结标识 chip（左上角）

    QLabel *m_idLabel;          // 目标 ID
    QLabel *m_typeLabel;        // 类型标签（带威胁色）
    QLabel *m_threatValue;      // 威胁等级
    QLabel *m_confValue;        // 置信度
    QLabel *m_coordValue;       // 坐标(WGS84 经度/纬度)
    QLabel *m_deviceValue;      // 检测设备
    QLabel *m_distValue;        // 距跑道
    QLabel *m_captureTimeValue; // 证据捕获时间
    QLabel *m_videoTimeValue;   // 证据视频时间
    QLabel *m_provenanceValue;  // 证据来源标注
    QWidget *m_detailContainer; // 详情行容器
    QWidget *m_actionsContainer;// 3 操作按钮容器
    QLabel *m_pendingMsg;       // 待检测提示
    QLabel *m_feedback;         // 操作反馈
    QPushButton *m_closeBtn;
    QPushButton *m_createTaskBtn;
    QPushButton *m_assignDeviceBtn;
    QPushButton *m_viewHistoryBtn;
};

#endif  // MAINWINDOW_TARGETDETAILOVERLAY_H
