#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QSplitter>

class SituationView;
class DeviceControlView;
class DecisionView;
class StatusBarWidget;

namespace Ui {
class MainWindow;
}

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

private:
    void setupUi();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createDockWidgets();
    void createConnections();

    SituationView *m_situationView;
    DeviceControlView *m_deviceControlView;
    DecisionView *m_decisionView;
    StatusBarWidget *m_statusBarWidget;

    QDockWidget *m_leftDock;
    QDockWidget *m_rightDock;

    QSplitter *m_centralSplitter;
};

#endif
