#ifndef MAINWINDOW_DETECTIONCONTROLPANEL_H
#define MAINWINDOW_DETECTIONCONTROLPANEL_H

#include "Core/Simulation/SimulationWorkflow.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QTextEdit;

class DetectionControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DetectionControlPanel(QWidget *parent = nullptr);
    ~DetectionControlPanel();

    void showNoSelection();
    void setSelectedTarget(const Core::TargetInfo &target);
    void setOperationLog(
        const QVector<Core::Simulation::SimulationOperationLogEntry> &entries);

signals:
    void confirmSimulationRequested();
    void startSimulationDisposalRequested();
    void completeSimulationDisposalRequested();

private:
    void setupUi();
    void updateActionAvailability(Core::TargetStatus status);

    QLabel *m_titleLabel;
    QLabel *m_targetLabel;
    QLabel *m_statusLabel;
    QPushButton *m_confirmButton;
    QPushButton *m_startButton;
    QPushButton *m_completeButton;
    QTextEdit *m_operationLog;
};

#endif
