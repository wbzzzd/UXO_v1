#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Core/Simulation/SimulationWorkflow.h"

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>

class SituationView;
class StatusBarWidget;
class NavigationWidget;
class VideoStreamPanel;
class LeftPanelWidget;
class RightPanelWidget;
class AlertPanel;
class DetectionControlPanel;
class BatchOperationBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_actionNewTask();
    void on_actionOpenPlan();
    void on_actionSavePlan();
    void on_actionExit();
    void on_actionViewLeftPanel();
    void on_actionViewRightPanel();
    void on_actionViewStatusBar();
    void on_actionSystemSettings();
    void on_actionAbout();
    void onNavigationChanged(int index);
    void onTargetSelected(const Core::TargetInfo& target);
    void onTargetDoubleClicked(const Core::TargetInfo& target);
    // 三步操作仅请求本地模拟工作流变更。
    void onConfirmSimulationRequested();
    void onStartSimulationDisposalRequested();
    void onCompleteSimulationDisposalRequested();
    // 刷新只重绘现有内存状态，不重置选择和日志。
    void onRefreshSimulationRequested();

private:
    void setupUi();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createMainLayout();
    void createConnections();
    void loadMockData();
    void requestSelectedTargetStatus(Core::TargetStatus requestedStatus);
    void refreshSelectedTarget();
    void refreshSimulationLog();

    NavigationWidget *m_navigationWidget;
    VideoStreamPanel *m_videoStreamPanel;
    LeftPanelWidget *m_leftPanel;
    RightPanelWidget *m_rightPanel;
    AlertPanel *m_alertPanel;
    DetectionControlPanel *m_detectionControlPanel;
    BatchOperationBar *m_batchOperationBar;
    StatusBarWidget *m_statusBarWidget;

    QSplitter *m_mainSplitter;
    QSplitter *m_centerSplitter;
    QWidget *m_centerArea;

    // 工作流持有唯一可变模拟目标集合；任务和设备仅供刷新只读面板。
    Core::Simulation::SimulationWorkflow m_simulationWorkflow;
    QVector<Core::MissionInfo> m_missions;
    QVector<Core::DeviceInfo> m_devices;

    bool m_leftPanelVisible;
    bool m_rightPanelVisible;
};

#endif
