#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>

namespace Core { struct TargetInfo; }

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

private:
    void setupUi();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createMainLayout();
    void createConnections();
    void loadMockData();

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

    bool m_leftPanelVisible;
    bool m_rightPanelVisible;
};

#endif
