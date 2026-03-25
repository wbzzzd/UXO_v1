#include "MainWindow/MainWindow.h"
#include "MainWindow/SituationView.h"
#include "MainWindow/DeviceControlView.h"
#include "MainWindow/DecisionView.h"
#include "MainWindow/StatusBarWidget.h"
#include "MainWindow/LeftPanelWidget.h"


#include "Core/Data/Types.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSplitter>
#include <QDebug>

#include "Common/MockDataGenerator.h"
#include "Core/Data/Types.h"

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

    setStyleSheet(R"(
        QMainWindow {
            background-color: #1E1E1E;
        }
        QMenuBar {
            background-color: #2D2D2D;
            color: #FFFFFF;
            border-bottom: 1px solid #3C3C3C;
        }
        QMenuBar::item:selected {
            background-color: #3C3C3C;
        }
        QMenu {
            background-color: #2D2D2D;
            color: #FFFFFF;
            border: 1px solid #3C3C3C;
        }
        QMenu::item:selected {
            background-color: #4A7A4C;
        }
        QToolBar {
            background-color: #2D2D2D;
            border: none;
            spacing: 8px;
            padding: 4px;
        }
        QToolBar QLabel {
            color: #AAAAAA;
        }
        QDockWidget {
            background-color: #252526;
            color: #FFFFFF;
            titlebar-close-icon: url(close.png);
        }
        QDockWidget::title {
            background-color: #2D2D2D;
            padding: 4px;
        }
        QDockWidget::close-button, QDockWidget::float-button {
            background-color: #2D2D2D;
        }
    )");

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
    QAction *leftPanelAction = viewMenu->addAction(tr("显示左侧面板"), this, SLOT(on_actionViewLeftPanel()));
    leftPanelAction->setCheckable(true);
    leftPanelAction->setChecked(true);
    QAction *rightPanelAction = viewMenu->addAction(tr("显示右侧面板"), this, SLOT(on_actionViewRightPanel()));
    rightPanelAction->setCheckable(true);
    rightPanelAction->setChecked(true);
    viewMenu->addAction(tr("显示状态栏"), this, SLOT(on_actionViewStatusBar()));
    viewMenu->addSeparator();
    viewMenu->addAction(tr("视角 → 顶视"), this, []() {});
    viewMenu->addAction(tr("视角 → 侧视"), this, []() {});

    QMenu *toolMenu = menuBar->addMenu(tr("工具(&T)"));
    toolMenu->addAction(tr("系统设置"), this, SLOT(on_actionSystemSettings()));
    toolMenu->addAction(tr("历史回放"), this, SLOT(on_actionViewLeftPanel()));
    toolMenu->addAction(tr("日志查看"), this, SLOT(on_actionViewLeftPanel()));
    toolMenu->addAction(tr("数据同步"), this, SLOT(on_actionViewLeftPanel()));

    QMenu *deviceMenu = menuBar->addMenu(tr("设备(&D)"));
    deviceMenu->addAction(tr("打开设备控制台"), this, SLOT(on_actionViewLeftPanel()));

    QMenu *helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(tr("关于"), this, SLOT(on_actionAbout()));
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("工具栏"));
    toolBar->setMovable(false);
    toolBar->setFixedHeight(48);

    QLabel *label = new QLabel(tr("图层控制"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px;");
    toolBar->addWidget(label);

    toolBar->addSeparator();

    label = new QLabel(tr("测量工具"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px;");
    toolBar->addWidget(label);

    toolBar->addSeparator();

    label = new QLabel(tr("坐标拾取"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px;");
    toolBar->addWidget(label);

    toolBar->addSeparator();

    QAction *resetViewAction = toolBar->addAction(tr("视角复位"), this, [this]() {
        if (m_situationView) {
            m_situationView->setCameraView("top");
        }
    });
    resetViewAction->setObjectName("resetViewAction");

    toolBar->addSeparator();

    label = new QLabel(tr("同步状态"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px;");
    toolBar->addWidget(label);

    toolBar->addSeparator();

    label = new QLabel(tr("书签"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px;");
    toolBar->addWidget(label);

    toolBar->addSeparator();

    label = new QLabel(tr("设备控制台"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px;");
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
    m_leftDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_leftDock->setMinimumWidth(320);
    m_leftDock->setMaximumWidth(320);

    LeftPanelWidget *leftPanel = new LeftPanelWidget(m_leftDock);
    m_leftDock->setWidget(leftPanel);

    connect(leftPanel, &LeftPanelWidget::targetSelected, this, [this, leftPanel](const Core::TargetInfo& target) {
        if (m_situationView) {
            m_situationView->highlightTarget(target.id, true);
            m_situationView->focusOnTarget(target.position);
        }
        qDebug() << "Target selected:" << target.id << target.typeName;
    });

    connect(leftPanel, &LeftPanelWidget::targetDoubleClicked, this, [this](const Core::TargetInfo& target) {
        if (m_situationView) {
            m_situationView->focusOnTarget(target.position);
        }
    });

    addDockWidget(Qt::LeftDockWidgetArea, m_leftDock);

    m_rightDock = new QDockWidget(tr("详情"), this);
    m_rightDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_rightDock->setMinimumWidth(360);
    m_rightDock->setMaximumWidth(360);

    m_decisionView = new DecisionView(m_rightDock);
    m_rightDock->setWidget(m_decisionView);

    addDockWidget(Qt::RightDockWidgetArea, m_rightDock);

    m_situationView = new SituationView(this);

    m_centralSplitter = new QSplitter(Qt::Horizontal, this);
    m_centralSplitter->addWidget(m_situationView);
    m_centralSplitter->setStretchFactor(0, 1);
    setCentralWidget(m_centralSplitter);
}

void MainWindow::createConnections()
{
}

void MainWindow::on_actionNewTask()
{
    QMessageBox::information(this, tr("新建任务"), tr("新建任务功能"));
}

void MainWindow::on_actionOpenPlan()
{
    QMessageBox::information(this, tr("打开预案"), tr("打开预案功能"));
}

void MainWindow::on_actionSavePlan()
{
    QMessageBox::information(this, tr("保存方案"), tr("保存方案功能"));
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
    QMessageBox::information(this, tr("系统设置"), tr("系统设置功能"));
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
