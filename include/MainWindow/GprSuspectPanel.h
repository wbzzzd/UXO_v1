#ifndef MAINWINDOW_GPRSUSPECTPANEL_H
#define MAINWINDOW_GPRSUSPECTPANEL_H

// 雷达页疑似目标面板（REQ-012 / gpr-display.md §5.3、§6.2）：
// 列表列：ID / 道索引 / 深度(m) / 幅度；标题含 [模拟][GPR自动检测] 标注（§6.4）。
// [确认目标]/[标记误报] 仅在选中行时可用（§6.3 禁用态）；
// 检测列表为空或无选中时按钮保持禁用。
// 本面板只发意图信号；目标注入由 GprView/MainWindow 经 SimulationWorkflow 处理
// （§5.3 状态所有权）。

#include "GPR/GprThresholdDetector.h"   // GPR::GprSuspect

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

class GprSuspectPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GprSuspectPanel(QWidget *parent = nullptr);
    ~GprSuspectPanel() override;

    // 替换全部候选并清空选中；空列表进入"无检测结果"禁用态
    void setSuspects(const QList<GPR::GprSuspect> &suspects);

    // 当前选中候选 ID（GprSuspect::id，1 基）；无选中返回 0
    int selectedSuspectId() const;

signals:
    void suspectSelected(int suspectId);
    void confirmRequested(int suspectId);
    void falsePositiveRequested(int suspectId);

private slots:
    void onRowSelectionChanged();
    void onConfirmClicked();
    void onFalsePositiveClicked();

private:
    void setupUi();
    void updateActionState();

    // objectName 契约（gpr_view UI 契约测试，§7.3）：
    //   自身 gprSuspectPanel；gprSuspectTable / gprConfirmButton / gprFalsePositiveButton
    QLabel *m_titleLabel = nullptr;          // 标题（含 [模拟][GPR自动检测] 标注）
    QTableWidget *m_table = nullptr;
    QPushButton *m_confirmBtn = nullptr;     // [确认目标]
    QPushButton *m_falsePositiveBtn = nullptr; // [标记误报]
    QList<GPR::GprSuspect> m_suspects;       // 与表格行一一对应
};

#endif // MAINWINDOW_GPRSUSPECTPANEL_H
