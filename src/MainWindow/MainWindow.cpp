#include "MainWindow/MainWindow.h"
#include "MainWindow/SituationView.h"
#include "MainWindow/StatusBarWidget.h"
#include "MainWindow/LeftPanelWidget.h"
#include "MainWindow/NavigationWidget.h"
#include "MainWindow/VideoStreamPanel.h"
#include "MainWindow/RightPanelWidget.h"
#include "MainWindow/AlertPanel.h"
#include "MainWindow/DetectionControlPanel.h"
#include "MainWindow/BatchOperationBar.h"
#include "MainWindow/DecisionSuggestionPanel.h"
#include "MainWindow/DecisionView.h"
#include "MainWindow/MosPlanningController.h"

#include "Core/Data/Types.h"
#include "Core/Simulation/DemoScenarioProvider.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Common/GlobalStyle.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_navigationWidget(nullptr)
    , m_videoStreamPanel(nullptr)
    , m_leftPanel(nullptr)
    , m_rightPanel(nullptr)
    , m_alertPanel(nullptr)
    , m_detectionControlPanel(nullptr)
    , m_batchOperationBar(nullptr)
    , m_statusBarWidget(nullptr)
    , m_mainToolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_centerSplitter(nullptr)
    , m_centerArea(nullptr)
    , m_pageStack(nullptr)
    , m_situationPage(nullptr)
    , m_decisionView(nullptr)
    , m_mosController(nullptr)
    , m_leftPanelVisible(true)
    , m_rightPanelVisible(true)
{
    setupUi();
    loadMockData();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("排弹抢修指挥系统 V1.0"));
    setMinimumSize(1280, 720);
    resize(1920, 1080);

    setStyleSheet(GlobalStyle::getMainWindowStyle());

    // MOS P0 控制器在 UI 构造前创建，确保 createConnections 可接线
    m_mosController = new Core::MOS::MosPlanningController(this);
    m_mosController->setObjectName(QStringLiteral("mosPlanningController"));

    createMenuBar();
    createToolBar();
    createMainLayout();
    createStatusBar();
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
    viewMenu->addAction(tr("视角 → 顶视"), this, [this]() {
        if (m_rightPanel && m_rightPanel->situationView()) {
            m_rightPanel->situationView()->setCameraView("top");
        }
    });
    viewMenu->addAction(tr("视角 → 侧视"), this, [this]() {
        if (m_rightPanel && m_rightPanel->situationView()) {
            m_rightPanel->situationView()->setCameraView("side");
        }
    });

    QMenu *toolMenu = menuBar->addMenu(tr("工具(&T)"));
    toolMenu->addAction(tr("系统设置"), this, SLOT(on_actionSystemSettings()));
    toolMenu->addAction(tr("历史回放"), this, []() {});
    toolMenu->addAction(tr("日志查看"), this, []() {});
    toolMenu->addAction(tr("数据同步"), this, []() {});

    QMenu *deviceMenu = menuBar->addMenu(tr("设备(&D)"));
    deviceMenu->addAction(tr("打开设备控制台"), this, []() {});

    QMenu *helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(tr("关于"), this, SLOT(on_actionAbout()));
}

void MainWindow::createToolBar()
{
    m_mainToolBar = addToolBar(tr("工具栏"));
    m_mainToolBar->setObjectName(QStringLiteral("mainSituationToolBar"));
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setFixedHeight(32);

    QLabel *label = new QLabel(tr("图层控制"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px; font-size: 12px;");
    m_mainToolBar->addWidget(label);

    m_mainToolBar->addSeparator();

    label = new QLabel(tr("测量工具"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px; font-size: 12px;");
    m_mainToolBar->addWidget(label);

    m_mainToolBar->addSeparator();

    label = new QLabel(tr("坐标拾取"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px; font-size: 12px;");
    m_mainToolBar->addWidget(label);

    m_mainToolBar->addSeparator();

    QAction *resetViewAction = m_mainToolBar->addAction(tr("视角复位"), this, [this]() {
        if (m_rightPanel && m_rightPanel->situationView()) {
            m_rightPanel->situationView()->resetCameraView();
        }
    });
    resetViewAction->setObjectName("resetViewAction");

    m_mainToolBar->addSeparator();

    label = new QLabel(tr("同步状态"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px; font-size: 12px;");
    m_mainToolBar->addWidget(label);

    m_mainToolBar->addSeparator();

    label = new QLabel(tr("书签"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px; font-size: 12px;");
    m_mainToolBar->addWidget(label);

    m_mainToolBar->addSeparator();

    label = new QLabel(tr("设备控制台"));
    label->setStyleSheet("color: #AAAAAA; padding: 4px; font-size: 12px;");
    m_mainToolBar->addWidget(label);
}

void MainWindow::createMainLayout()
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #1E1E1E;");

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_navigationWidget = new NavigationWidget(centralWidget);
    mainLayout->addWidget(m_navigationWidget);

    // P0 页面栈：index 0 = 态势工作区（左+中+右），index 1 = MOS 决策页
    m_pageStack = new QStackedWidget(centralWidget);
    m_pageStack->setObjectName(QStringLiteral("mainPageStack"));

    // 态势工作区页：包裹左面板 + 中心区 + 右面板，作为页面栈 index 0
    m_situationPage = new QWidget(m_pageStack);
    m_situationPage->setObjectName(QStringLiteral("situationWorkspacePage"));
    QHBoxLayout *situationLayout = new QHBoxLayout(m_situationPage);
    situationLayout->setContentsMargins(0, 0, 0, 0);
    situationLayout->setSpacing(0);

    m_leftPanel = new LeftPanelWidget(m_situationPage);
    situationLayout->addWidget(m_leftPanel);

    m_centerArea = new QWidget(m_situationPage);
    QVBoxLayout *centerLayout = new QVBoxLayout(m_centerArea);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_centerSplitter = new QSplitter(Qt::Vertical, m_centerArea);
    m_centerSplitter->setStyleSheet(QString(
        "QSplitter::handle { background-color: %1; height: 1px; }")
        .arg(GlobalStyle::Colors::Border));

    m_videoStreamPanel = new VideoStreamPanel(m_centerSplitter);
    m_centerSplitter->addWidget(m_videoStreamPanel);

    QWidget *infoArea = new QWidget(m_centerSplitter);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoArea);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(0);

    m_centerSplitter->setStretchFactor(0, 3);
    m_centerSplitter->setStretchFactor(1, 2);

    QWidget *infoHeader = new QWidget(infoArea);
    infoHeader->setFixedHeight(28);
    infoHeader->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::Border));
    QHBoxLayout *infoHeaderLayout = new QHBoxLayout(infoHeader);
    infoHeaderLayout->setContentsMargins(8, 0, 8, 0);

    QLabel *infoTitle = new QLabel("信息面板", infoHeader);
    infoTitle->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::BodySize));
    infoHeaderLayout->addWidget(infoTitle);
    infoHeaderLayout->addStretch();
    infoLayout->addWidget(infoHeader);

    QSplitter *infoSplitter = new QSplitter(Qt::Horizontal, infoArea);
    infoSplitter->setStyleSheet(QString(
        "QSplitter::handle { background-color: %1; width: 1px; }")
        .arg(GlobalStyle::Colors::Border));

    m_alertPanel = new AlertPanel(infoSplitter);
    infoSplitter->addWidget(m_alertPanel);

    m_detectionControlPanel = new DetectionControlPanel(infoSplitter);
    infoSplitter->addWidget(m_detectionControlPanel);

    infoSplitter->setStretchFactor(0, 1);
    infoSplitter->setStretchFactor(1, 1);

    infoLayout->addWidget(infoSplitter, 1);

    m_batchOperationBar = new BatchOperationBar(infoArea);
    infoLayout->addWidget(m_batchOperationBar);

    m_centerSplitter->addWidget(infoArea);

    centerLayout->addWidget(m_centerSplitter, 1);

    // 中心区加入态势工作区布局
    situationLayout->addWidget(m_centerArea, 1);

    // 右面板加入态势工作区布局
    m_rightPanel = new RightPanelWidget(m_situationPage);
    situationLayout->addWidget(m_rightPanel);

    // MOS 决策页：页面栈 index 1
    m_decisionView = new DecisionView;
    m_decisionView->setObjectName(QStringLiteral("mosDecisionPage"));
    m_pageStack->addWidget(m_situationPage);
    m_pageStack->addWidget(m_decisionView);
    m_pageStack->setCurrentIndex(0);
    mainLayout->addWidget(m_pageStack, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::createStatusBar()
{
    constexpr int kStatusBarHostHeight = 28;
    constexpr int kStatusBarContentHeight = 22;

    m_statusBarWidget = new StatusBarWidget(this);
    // QStatusBar 自带上下内边距，宿主高度应大于内容高度，避免内容越过窗口底边。
    m_statusBarWidget->setFixedHeight(kStatusBarContentHeight);
    statusBar()->addWidget(m_statusBarWidget, 1);
    statusBar()->setFixedHeight(kStatusBarHostHeight);
}

void MainWindow::createConnections()
{
    connect(m_navigationWidget, &NavigationWidget::navigationChanged,
            this, &MainWindow::onNavigationChanged);

    connect(m_leftPanel, &LeftPanelWidget::targetSelected,
            this, &MainWindow::onTargetSelected);
    connect(m_leftPanel, &LeftPanelWidget::targetDoubleClicked,
            this, &MainWindow::onTargetDoubleClicked);
    connect(m_leftPanel, &LeftPanelWidget::refreshSimulationRequested,
            this, &MainWindow::onRefreshSimulationRequested);

    connect(m_rightPanel, &RightPanelWidget::targetClicked,
            this, [this](const QString& targetId) {
        qDebug() << "Target clicked in 3D view:" << targetId;
    });

    connect(m_detectionControlPanel,
            &DetectionControlPanel::confirmSimulationRequested,
            this,
            &MainWindow::onConfirmSimulationRequested);
    connect(m_detectionControlPanel,
            &DetectionControlPanel::startSimulationDisposalRequested,
            this,
            &MainWindow::onStartSimulationDisposalRequested);
    connect(m_detectionControlPanel,
            &DetectionControlPanel::completeSimulationDisposalRequested,
            this,
            &MainWindow::onCompleteSimulationDisposalRequested);

    // MOS P0 决策页接线：controller 单一状态通知 <-> DecisionView 请求（本地合成 fixture）
    connect(m_mosController, &Core::MOS::MosPlanningController::mosStateChanged,
            this, [this]() {
        if (m_decisionView) {
            m_decisionView->setSnapshot(m_mosController->snapshot());
        }
    });
    connect(m_mosController, &Core::MOS::MosPlanningController::replanActivityChanged,
            m_decisionView, &DecisionView::setPlanning);
    connect(m_decisionView, &DecisionView::replanRequested,
            this, [this]() {
        m_mosController->requestReplan(m_decisionView->currentObstacles(),
                                        m_decisionView->currentParams());
    });
    connect(m_decisionView, &DecisionView::tierSelected,
            m_mosController, &Core::MOS::MosPlanningController::selectTier);
    connect(m_decisionView, &DecisionView::generatorApplied,
            this, [this](const Core::MOS::MosGeneratorParams &genParams, qint32 seed) {
        const auto runwayParams = m_decisionView->currentParams();
        const auto obstacles = Core::MOS::MosFixtureGenerator::generate(runwayParams, genParams, seed);
        // 仅替换障碍物，不自动规划；用户点击重新规划按钮后才执行 MOS 规划
        m_mosController->replaceObstacles(obstacles, runwayParams);
    });
    connect(m_decisionView, &DecisionView::exportRequested,
            this, [this](const QString &path) {
        m_mosController->exportFixture(path);
    });
}

// 加载模拟演示场景数据（来自 DemoScenarioProvider，不连接真实设备）
void MainWindow::loadMockData()
{
    // 从本地模拟场景提供者获取数据
    Core::Simulation::DemoScenario scenario = Core::Simulation::DemoScenarioProvider::create();
    m_simulationWorkflow.reset(scenario.targets);
    m_missions = scenario.missions;
    m_devices = scenario.devices;

    // 左侧目标列表始终读取工作流中的权威副本。
    m_leftPanel->setTargets(m_simulationWorkflow.targets());
    m_leftPanel->setMissions(m_missions);
    m_leftPanel->setDevices(m_devices);

    // 右侧面板：设备状态
    m_rightPanel->setDevices(m_devices);

    // 三维态势图：添加模拟目标标记
    SituationView *sv = m_rightPanel->situationView();
    if (sv) {
        for (const Core::TargetInfo& t : m_simulationWorkflow.targets()) {
            sv->addTargetMarker(t.id, t.position);
        }
    }

    // 状态栏：设备在线数、最低电量和模拟模式标识
    int onlineCount = 0;
    int minBattery = 100;
    for (const Core::DeviceInfo& dev : m_devices) {
        if (dev.status == Core::DeviceStatus::Online || dev.status == Core::DeviceStatus::Idle || dev.status == Core::DeviceStatus::Busy) {
            onlineCount++;
        }
        if (dev.batteryLevel < minBattery) {
            minBattery = static_cast<int>(dev.batteryLevel);
        }
    }
    m_statusBarWidget->updateDeviceStatus(onlineCount, m_devices.size());
    m_statusBarWidget->setMinBatteryLevel(minBattery);
    m_statusBarWidget->setSimulationMode(true);

    // 告警面板：添加模拟告警
    for (int i = 0; i < 3; ++i) {
        Core::AlarmInfo alarm;
        alarm.id = QString("ALM-DEMO-%1").arg(i + 1, 3, 10, QChar('0'));
        alarm.level = (i == 0) ? Core::AlarmLevel::Warning : Core::AlarmLevel::Info;
        alarm.message = QString("模拟告警 %1: %2").arg(i + 1).arg(
            (i == 0) ? QStringLiteral("模拟设备电量偏低") : QStringLiteral("模拟目标待确认"));
        alarm.createTime = QDateTime::currentDateTime().addSecs(-i * 60);
        m_alertPanel->addAlert(alarm);
        m_statusBarWidget->addAlarm(alarm.message);
    }

    // 初始保持未选择状态，同时保留模拟任务详情。
    if (!m_missions.isEmpty()) {
        m_rightPanel->decisionPanel()->setMission(m_missions.first());
    }
    refreshSelectedTarget();
    refreshSimulationLog();

    // MOS 决策页初始 seed-42 replan：用默认跑道参数与默认生成器参数产出 revision 1
    // 已提交快照，避免启动时显示 inert 空状态。createConnections 已接线 mosStateChanged
    // -> setSnapshot，requestReplan 同步触发 worker 完成后会自动刷新视图。
    if (m_decisionView && m_mosController) {
        Core::MOS::MosRunwayParams runwayParams;
        Core::MOS::MosGeneratorParams genParams;
        const qint32 seed = 42;
        const auto obstacles = Core::MOS::MosFixtureGenerator::generate(runwayParams, genParams, seed);
        m_mosController->requestReplan(obstacles, runwayParams);
    }
}

void MainWindow::onNavigationChanged(int index)
{
    qDebug() << "Navigation changed to:" << index;

    // P0 路由：导航 index 2（决策）切到 MOS 决策页，其余 index 保持态势工作区
    if (!m_pageStack) {
        return;
    }
    m_pageStack->setCurrentIndex(index == 2 ? 1 : 0);

    // 态势页遗留工具栏仅在态势工作区可见时显示；MOS 决策页（index 2）独占 MOS 工具栏
    if (m_mainToolBar) {
        m_mainToolBar->setVisible(index != 2);
    }
}

void MainWindow::onTargetSelected(const Core::TargetInfo& target)
{
    // 面板传入对象只提供标识，展示始终回读工作流中的权威目标。
    if (!m_simulationWorkflow.selectTarget(target.id)) {
        refreshSimulationLog();
        return;
    }

    refreshSelectedTarget();
    refreshSimulationLog();
}

void MainWindow::onTargetDoubleClicked(const Core::TargetInfo& target)
{
    // 双击复用选择流程，工作流会保证重复选择不追加日志。
    onTargetSelected(target);
}

void MainWindow::onConfirmSimulationRequested()
{
    requestSelectedTargetStatus(Core::TargetStatus::Confirmed);
}

void MainWindow::onStartSimulationDisposalRequested()
{
    requestSelectedTargetStatus(Core::TargetStatus::Disposing);
}

void MainWindow::onCompleteSimulationDisposalRequested()
{
    requestSelectedTargetStatus(Core::TargetStatus::Disposed);
}

void MainWindow::onRefreshSimulationRequested()
{
    // 刷新使用当前内存状态，不重新加载场景或清空工作流。
    m_leftPanel->setTargets(m_simulationWorkflow.targets());
    m_leftPanel->setMissions(m_missions);
    m_leftPanel->setDevices(m_devices);
    m_rightPanel->setDevices(m_devices);
    refreshSelectedTarget();
    refreshSimulationLog();
}

void MainWindow::requestSelectedTargetStatus(Core::TargetStatus requestedStatus)
{
    if (!m_simulationWorkflow.requestSelectedTargetStatus(requestedStatus)) {
        // 被拒绝时仅刷新日志，所有状态展示保持不变。
        refreshSimulationLog();
        return;
    }

    const Core::TargetInfo *target = m_simulationWorkflow.selectedTarget();
    if (target == nullptr) {
        return;
    }

    m_leftPanel->updateTargetStatus(target->id, target->status);
    refreshSelectedTarget();
    refreshSimulationLog();
}

void MainWindow::refreshSelectedTarget()
{
    const Core::TargetInfo *target = m_simulationWorkflow.selectedTarget();
    if (target == nullptr) {
        // 决策面板构造时已是未选择态，此处避免覆盖已加载的模拟任务详情。
        m_detectionControlPanel->showNoSelection();
        return;
    }

    m_rightPanel->setTarget(*target);
    m_detectionControlPanel->setSelectedTarget(*target);
}

void MainWindow::refreshSimulationLog()
{
    // 日志完全来自工作流的进程内记录。
    m_detectionControlPanel->setOperationLog(m_simulationWorkflow.logEntries());
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
    m_leftPanelVisible = !m_leftPanelVisible;
    m_leftPanel->setVisible(m_leftPanelVisible);
}

void MainWindow::on_actionViewRightPanel()
{
    m_rightPanelVisible = !m_rightPanelVisible;
    m_rightPanel->setVisible(m_rightPanelVisible);
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
