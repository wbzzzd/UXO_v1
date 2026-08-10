#ifndef MAINWINDOW_LEFTPANELWIDGET_H
#define MAINWINDOW_LEFTPANELWIDGET_H

#include <QWidget>
#include <QVector>
#include <QString>
#include "Core/Data/Types.h"

class QTableWidget;
class QTableWidgetItem;
class QLineEdit;
class QPushButton;
class QLabel;

// 左侧目标列表面板：可折叠，仅列威胁目标物（不含任务/设备 tab）。
// 折叠态 40px 窄条，展开态 320px 完整列表。默认折叠。
class LeftPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LeftPanelWidget(QWidget *parent = nullptr);
    ~LeftPanelWidget();

    void setTargets(const QVector<Core::TargetInfo>& targets);
    void updateTargetStatus(const QString& targetId, Core::TargetStatus status);
    void addTargetRow(const Core::TargetInfo& target);
    void selectTargetRow(const QString& targetId);

    // 可折叠控制
    void setCollapsed(bool collapsed);
    bool isCollapsed() const;

signals:
    void targetSelected(const Core::TargetInfo& target);
    void refreshSimulationRequested();
    void collapseChanged(bool collapsed);

public slots:
    void onRefreshTargets();
    void onSearchTextChanged(const QString& text);

protected:
    // 折叠态窄条整体可点击：捕获 m_collapsedLabel 及其子控件的鼠标按下事件，触发展开
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUi();
    void setupTargetList();
    void populateTargetList();
    void updateStatusTabs();
    void appendTargetRow(int row, const Core::TargetInfo& target);
    // 应用折叠/展开的视觉态（宽度、按钮文案、内容可见性）
    void applyCollapseState();

    QTableWidget *m_targetTable;
    QLineEdit *m_searchEdit;

    QPushButton *m_statusTabPending;
    QPushButton *m_statusTabExecuting;
    QPushButton *m_statusTabCompleted;

    QPushButton *m_collapseBtn;
    QWidget *m_collapsedLabel;
    QWidget *m_contentWidget;

    bool m_collapsed;

    QVector<Core::TargetInfo> m_targets;
};

#endif
