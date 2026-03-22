#include "MainWindow/MainWindow.h"
#include "MainWindow/SituationView.h"
#include "MainWindow/DeviceControlView.h"
#include "MainWindow/DecisionView.h"
#include "MainWindow/StatusBarWidget.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_situationView(nullptr)
    , m_deviceControlView(nullptr)
    , m_decisionView(nullptr)
    , m_statusBarWidget(nullptr)
    , m_leftDock(nullptr)
    , m_rightDock(nullptr)
    , m_centralSplitter(nullptr)
{
    setupUi();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("排弹抢修指挥系统 V1.0"));
    setMinimumSize(1280, 720);
    resize(1920, 1080);

    createMenuBar();
    createToolBar();
    createStatusBar();
    createDockWidgets();
    createConnections();
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = this->menuBar();

    QMenu *fileMenu = menuBar->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("新建任务"), this, SLOT(on_actionNewTask()), QKeySequence::New);
    fileMenu->addAction(tr("打开预案"), this, SLOT(on_actionOpenPlan()), QKeySequence::Open);
    fileMenu->addAction(tr("保存方案"), this, SLOT(on_actionSavePlan()), QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, SLOT(on_actionExit()), QKeySequence::Quit);

    QMenu *viewMenu = menuBar->addMenu(tr("视图(&V)"));
    viewMenu->addAction(tr("显示左侧面板"), this, SLOT(on_actionViewLeftPanel()));
    viewMenu->addAction(tr("显示右侧面板"), this, SLOT(on_actionViewRightPanel()));
    viewMenu->addAction(tr("显示状态栏"), this, SLOT(on_actionViewStatusBar()));

    QMenu *toolMenu = menuBar->addMenu(tr("工具(&T)"));
    toolMenu->addAction(tr("系统设置"), this, SLOT(on_actionSystemSettings()));
    toolMenu->addAction(tr("历史回放"), this, SLOT(on_actionViewLeftPanel()));

    QMenu *helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(tr("关于"), this, SLOT(on_actionAbout()));
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("工具栏"));
    toolBar->setMovable(false);
    toolBar->setFixedHeight(48);

    QLabel *label = new QLabel(tr("图层控制"));
    toolBar->addWidget(label);

    toolBar->addSeparator();

    label = new QLabel(tr("测量工具"));
    toolBar->addWidget(label);
}

void MainWindow::createStatusBar()
{
    m_statusBarWidget = new StatusBarWidget(this);
    setStatusBar(m_statusBarWidget);
}

void MainWindow::createDockWidgets()
{
    m_leftDock = new QDockWidget(tr("目标/任务/设备"), this);
    m_leftDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, m_leftDock);

    m_rightDock = new QDockWidget(tr("详情"), this);
    m_rightDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::RightDockWidgetArea, m_rightDock);

    m_centralSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_centralSplitter);
}

void MainWindow::createConnections()
{
}

void MainWindow::on_actionNewTask()
{
}

void MainWindow::on_actionOpenPlan()
{
}

void MainWindow::on_actionSavePlan()
{
}

void MainWindow::on_actionExit()
{
    close();
}

void MainWindow::on_actionViewLeftPanel()
{
    m_leftDock->setVisible(!m_leftDock->isVisible());
}

void MainWindow::on_actionViewRightPanel()
{
    m_rightDock->setVisible(!m_rightDock->isVisible());
}

void MainWindow::on_actionViewStatusBar()
{
    statusBar()->setVisible(!statusBar()->isVisible());
}

void MainWindow::on_actionSystemSettings()
{
}

void MainWindow::on_actionAbout()
{
    QMessageBox::about(this, tr("关于"),
        tr("排弹抢修指挥系统 V1.0\n\n")
        + tr("一套用于空军场站的智能化排爆管理软件系统。\n\n")
        + tr("© 2024 All Rights Reserved"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton ret = QMessageBox::question(this,
        tr("确认退出"),
        tr("确定要退出系统吗？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}
