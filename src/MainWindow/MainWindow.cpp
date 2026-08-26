#include "MainWindow/MainWindow.h"
#include "MainWindow/StatusBarWidget.h"
#include "MainWindow/LeftPanelWidget.h"
#include "MainWindow/NavigationWidget.h"
#include "MainWindow/VideoStreamPanel.h"
#include "MainWindow/VideoOverlayWidget.h"
#include "MainWindow/TacticalMapWidget.h"
#include "MainWindow/DeviceResourceBar.h"
#include "MainWindow/TargetDetailOverlay.h"
#include "MainWindow/DecisionView.h"
#include "MainWindow/DetectionView.h"
#include "MainWindow/MosPlanningController.h"

#include "Core/Data/Types.h"
#include "Core/Simulation/DemoScenarioProvider.h"
#include "Core/Simulation/DroneTelemetrySimulator.h"
#include "Detection/DetectionEngine.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Common/GlobalStyle.h"
#include "MainWindow/UiIcons.h"

// FA 图标枚举码点（vendored third_party/QtAwesome）
#include "QtAwesome.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QCloseEvent>
#include <QShowEvent>
#include <QStackedWidget>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QtMath>

// PiP 尺寸常量
namespace {
constexpr int kPipWidth = 480;
constexpr int kPipVideoHeight = 270;
constexpr int kPipTitleBarHeight = 24;
constexpr int kPipHeight = kPipVideoHeight + kPipTitleBarHeight;
constexpr int kPipMargin = 12;
constexpr int kMapToolbarHeight = 32;

// 模拟素材文件名（演示用，真实系统从配置加载）；完整路径在 loadMockData 中
// 按 MEDIA_ASSETS_DIR 编译期宏（源码树）优先、安装布局 share/uxo/assets/media
// 回退的方式解析，与 AI 模型目录（DETECTION_ASSETS_DIR）的做法一致
const char* const kVideoFile = "perth_airport_drone_uxo.mp4";
const char* const kSatelliteFile = "shenyang_yuhong_satellite.jpg";

// 视频时长（秒），与 DroneTelemetrySimulator 航线时长一致
constexpr double kVideoDurationSec = 96.0;

// 无人机相机地面覆盖范围（米），用于目标坐标推算
// 简化模型：300m 高度时覆盖约 600m x 400m
constexpr double kFootprintWidthM = 600.0;
constexpr double kFootprintHeightM = 400.0;

/**
 * @brief 浮动面板阴影层（REQ-010 阶段3 纵深 token 的消费端）
 * @details 与浮动面板同几何的无边框底衬：绘制一块与面板同色的矩形并挂
 *          QGraphicsDropShadowEffect，底衬被浮动面板完全遮盖，仅四周投影可见。
 *          不直接给浮动面板本体挂 effect：VideoStreamPanel 子树内的 QVideoWidget
 *          是原生窗口，effect 的栅格渲染路径无法捕获原生子窗口内容（视频
 *          黑屏风险），故用底衬旁路，完全不触碰视频渲染路径。
 */
class FloatingShadowLayer : public QWidget {
public:
    explicit FloatingShadowLayer(QWidget *parent, const QString &fillColor)
        : QWidget(parent)
    {
        // 纯栅格底衬 + QSS 背景填充（WA_StyledBackground，同 TargetDetailOverlay 根容器做法）
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(QStringLiteral("background-color:%1;").arg(fillColor));
        // 深度 token（design-system.md §9）：浮动面板档投影
        auto *effect = new QGraphicsDropShadowEffect(this);
        effect->setBlurRadius(GlobalStyle::Elevation::OverlayBlurRadius);
        effect->setOffset(0, GlobalStyle::Elevation::OverlayOffsetY);
        effect->setColor(QColor(0, 0, 0, GlobalStyle::Elevation::OverlayShadowAlpha));
        setGraphicsEffect(effect);
    }
};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_navigationWidget(nullptr)
    , m_leftPanel(nullptr)
    , m_statusBarWidget(nullptr)
    , m_tacticalMap(nullptr)
    , m_deviceResourceBar(nullptr)
    , m_targetDetailOverlay(nullptr)
    , m_mapContainer(nullptr)
    , m_videoPiP(nullptr)
    , m_pipShadowLayer(nullptr)
    , m_overlayShadowLayer(nullptr)
    , m_mapToolbar(nullptr)
    , m_resetViewBtn(nullptr)
    , m_startBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_droneSimulator(nullptr)
    , m_detectionEngine(nullptr)
    , m_pageStack(nullptr)
    , m_situationPage(nullptr)
    , m_decisionView(nullptr)
    , m_mosController(nullptr)
    , m_currentDroneLat(0.0)
    , m_currentDroneLng(0.0)
    , m_currentDroneAlt(0.0)
    , m_currentDroneHeading(0.0)
    , m_targetCounter(0)
    , m_leftPanelVisible(true)
    , m_pipMinimized(false)
    , m_pipVisible(true)
    , m_videoIsMain(false)
{
    // 字体图标库初始化（幂等）：必须先于 setupUi 中的图标创建；失败时控件降级为纯文本
    UiIcons::init();

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
    createMainLayout();

    // 创建遥测模拟器与 AI 检测引擎（在 createConnections 之前，供信号槽接线）
    m_droneSimulator = new Core::Simulation::DroneTelemetrySimulator(this);
    m_detectionEngine = new DetectionEngine(this);
    m_detectionEngine->setObjectName(QStringLiteral("detectionEngine"));

    createMapToolbar();
    createStatusBar();
    createConnections();

    // AI 检测引擎: 加载 ONNX 模型（createConnections 之后 error 信号已接线，
    // 失败时 initialize 返回 false 并 emit error -> 状态栏告警）
    // 模型目录: 优先编译期源码树路径（开发期）；源码树不在时回退安装布局
    // bin/../share/uxo/assets/models（对应根 CMakeLists 的 install 规则）
    QString modelsDir = QStringLiteral(DETECTION_ASSETS_DIR) + QStringLiteral("/models");
    if (!QFileInfo::exists(modelsDir + QStringLiteral("/patchcore_params.json"))) {
        modelsDir = QCoreApplication::applicationDirPath()
                        + QStringLiteral("/../share/uxo/assets/models");
    }
    m_detectionEngine->initialize(modelsDir + QStringLiteral("/patchcore_512.onnx"),
                                  modelsDir + QStringLiteral("/yolov8_cls_224.onnx"),
                                  modelsDir + QStringLiteral("/patchcore_params.json"));

    repositionFloatingWidgets();
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
    viewMenu->addAction(tr("显示状态栏"), this, SLOT(on_actionViewStatusBar()));
    QAction *pipAction = viewMenu->addAction(tr("显示视频画中画"), this, SLOT(on_actionViewVideoPiP()));
    pipAction->setCheckable(true);
    pipAction->setChecked(true);

    QMenu *toolMenu = menuBar->addMenu(tr("工具(&T)"));
    toolMenu->addAction(tr("系统设置"), this, SLOT(on_actionSystemSettings()));
    toolMenu->addAction(tr("历史回放"), this, []() {});
    toolMenu->addAction(tr("日志查看"), this, []() {});

    QMenu *helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(tr("关于"), this, SLOT(on_actionAbout()));
}

void MainWindow::createMainLayout()
{
    QWidget *centralWidget = new QWidget(this);
    // 中央容器底色改走全局 QSS containerBg 属性：
    // 原无选择器样式表会级联到全部子孙控件，强制覆盖子孙的背景/悬停样式
    centralWidget->setProperty("containerBg", "main");

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. 左侧导航栏 (80px)
    m_navigationWidget = new NavigationWidget(centralWidget);
    mainLayout->addWidget(m_navigationWidget);

    // P0 页面栈：index 0 = 态势工作区（左+中），index 1 = MOS 决策页
    m_pageStack = new QStackedWidget(centralWidget);
    m_pageStack->setObjectName(QStringLiteral("mainPageStack"));

    // 态势工作区页：包裹左面板 + 中心区
    m_situationPage = new QWidget(m_pageStack);
    m_situationPage->setObjectName(QStringLiteral("situationWorkspacePage"));
    QHBoxLayout *situationLayout = new QHBoxLayout(m_situationPage);
    situationLayout->setContentsMargins(0, 0, 0, 0);
    situationLayout->setSpacing(0);

    // 2. 左 pane: 可折叠目标列表
    m_leftPanel = new LeftPanelWidget(m_situationPage);
    situationLayout->addWidget(m_leftPanel);

    // 3. 中心区: 设备资源条 + 地图工具栏 + 地图主舞台
    QWidget *centerArea = new QWidget(m_situationPage);
    QVBoxLayout *centerLayout = new QVBoxLayout(centerArea);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    // 3a. 设备资源条 (36px)
    m_deviceResourceBar = new DeviceResourceBar(centerArea);
    centerLayout->addWidget(m_deviceResourceBar);

    // 3b. 地图工具栏 (32px)
    m_mapToolbar = new QWidget(centerArea);
    // 工具栏底色：全局 QSS containerBg="toolbar"（构造期静态属性；
    // 与 statusTabs 同模式：property 先于 attribute、且在 addWidget 之前设置）
    m_mapToolbar->setProperty("containerBg", "toolbar");
    m_mapToolbar->setAttribute(Qt::WA_StyledBackground, true);
    m_mapToolbar->setFixedHeight(kMapToolbarHeight);
    centerLayout->addWidget(m_mapToolbar);

    // 3c. 地图主舞台容器
    m_mapContainer = new QWidget(centerArea);
    m_mapContainer->setObjectName(QStringLiteral("mapContainer"));
    // 地图舞台底色：全局 QSS containerBg="main"（构造期静态属性，无需 repolish）
    m_mapContainer->setAttribute(Qt::WA_StyledBackground, true);
    m_mapContainer->setProperty("containerBg", QStringLiteral("main"));
    m_mapContainer->installEventFilter(this);

    m_tacticalMap = new TacticalMapWidget(m_mapContainer);
    m_tacticalMap->setObjectName(QStringLiteral("tacticalMap"));

    m_videoPiP = new VideoStreamPanel(m_mapContainer);
    m_videoPiP->setObjectName(QStringLiteral("videoPiP"));

    m_targetDetailOverlay = new TargetDetailOverlay(m_mapContainer);
    m_targetDetailOverlay->setObjectName(QStringLiteral("targetDetailOverlay"));

    // 浮动面板阴影层（深度 token 消费端）：PiP 与目标详情浮层的投影底衬，
    // 默认隐藏，由 repositionFloatingWidgets 按可见性同步几何与显隐
    m_pipShadowLayer = new FloatingShadowLayer(m_mapContainer, GlobalStyle::Colors::Background);
    m_pipShadowLayer->setObjectName(QStringLiteral("pipShadowLayer"));
    m_pipShadowLayer->hide();
    m_overlayShadowLayer = new FloatingShadowLayer(m_mapContainer, GlobalStyle::Colors::PanelBackground);
    m_overlayShadowLayer->setObjectName(QStringLiteral("overlayShadowLayer"));
    m_overlayShadowLayer->hide();
    // 浮层经关闭按钮自行隐藏不经过 reposition，需事件过滤同步其阴影层
    m_targetDetailOverlay->installEventFilter(this);

    centerLayout->addWidget(m_mapContainer, 1);

    // 中心区加入态势工作区布局
    situationLayout->addWidget(centerArea, 1);

    // 探测页：页面栈 index 1
    m_detectionView = new DetectionView;
    m_detectionView->setObjectName(QStringLiteral("detectionPage"));

    // MOS 决策页：页面栈 index 2
    m_decisionView = new DecisionView;
    m_decisionView->setObjectName(QStringLiteral("mosDecisionPage"));
    m_pageStack->addWidget(m_situationPage);    // index 0
    m_pageStack->addWidget(m_detectionView);     // index 1
    m_pageStack->addWidget(m_decisionView);      // index 2
    m_pageStack->setCurrentIndex(0);
    mainLayout->addWidget(m_pageStack, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::createMapToolbar()
{
    QHBoxLayout *layout = new QHBoxLayout(m_mapToolbar);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(4);

    // 探测控制按钮: [重置] [开始] [结束]
    // 扁平工具栏按钮：全局 QSS btnVariant="flat"（替代原共享 btnStyle 内联，下方 7 个按钮同款）
    // 图标状态色对齐 flat QSS 文字色：常规=TextPrimary(%3)、禁用=TextDisabled(%10)
    const auto flatIcon = [](int glyph) {
        return UiIcons::icon(glyph,
                             GlobalStyle::Colors::TextPrimary,
                             QColor(),
                             GlobalStyle::Colors::TextDisabled);
    };
    m_resetBtn = new QPushButton(QStringLiteral("重置"), m_mapToolbar);
    m_resetBtn->setObjectName(QStringLiteral("mapToolbarReset"));
    m_resetBtn->setProperty("btnVariant", QStringLiteral("flat"));
    m_resetBtn->setIconSize(QSize(12, 12));
    m_resetBtn->setIcon(flatIcon(fa::fa_rotate_right));
    layout->addWidget(m_resetBtn);

    m_startBtn = new QPushButton(QStringLiteral("开始"), m_mapToolbar);
    m_startBtn->setObjectName(QStringLiteral("mapToolbarStart"));
    m_startBtn->setProperty("btnVariant", QStringLiteral("flat"));
    m_startBtn->setIconSize(QSize(12, 12));
    m_startBtn->setIcon(flatIcon(fa::fa_play));
    layout->addWidget(m_startBtn);

    m_stopBtn = new QPushButton(QStringLiteral("结束"), m_mapToolbar);
    m_stopBtn->setObjectName(QStringLiteral("mapToolbarStop"));
    m_stopBtn->setProperty("btnVariant", QStringLiteral("flat"));
    m_stopBtn->setIconSize(QSize(12, 12));
    m_stopBtn->setIcon(flatIcon(fa::fa_stop));
    layout->addWidget(m_stopBtn);

    layout->addSpacing(8);

    m_resetViewBtn = new QPushButton(QStringLiteral("视角复位"), m_mapToolbar);
    m_resetViewBtn->setObjectName(QStringLiteral("mapToolbarResetView"));
    m_resetViewBtn->setEnabled(false);
    m_resetViewBtn->setProperty("btnVariant", QStringLiteral("flat"));
    m_resetViewBtn->setIconSize(QSize(12, 12));
    m_resetViewBtn->setIcon(flatIcon(fa::fa_expand));
    layout->addWidget(m_resetViewBtn);

    auto *layerBtn = new QPushButton(QStringLiteral("图层"), m_mapToolbar);
    layerBtn->setObjectName(QStringLiteral("mapToolbarLayer"));
    layerBtn->setEnabled(false);
    layerBtn->setProperty("btnVariant", QStringLiteral("flat"));
    layerBtn->setIconSize(QSize(12, 12));
    layerBtn->setIcon(flatIcon(fa::fa_layer_group));
    layout->addWidget(layerBtn);

    auto *measureBtn = new QPushButton(QStringLiteral("测量"), m_mapToolbar);
    measureBtn->setObjectName(QStringLiteral("mapToolbarMeasure"));
    measureBtn->setEnabled(false);
    measureBtn->setProperty("btnVariant", QStringLiteral("flat"));
    measureBtn->setIconSize(QSize(12, 12));
    measureBtn->setIcon(flatIcon(fa::fa_ruler));
    layout->addWidget(measureBtn);

    auto *pickCoordBtn = new QPushButton(QStringLiteral("坐标拾取"), m_mapToolbar);
    pickCoordBtn->setObjectName(QStringLiteral("mapToolbarPickCoord"));
    pickCoordBtn->setEnabled(false);
    pickCoordBtn->setProperty("btnVariant", QStringLiteral("flat"));
    pickCoordBtn->setIconSize(QSize(12, 12));
    pickCoordBtn->setIcon(flatIcon(fa::fa_location_crosshairs));
    layout->addWidget(pickCoordBtn);

    layout->addStretch();

    auto *simTag = new QLabel(QStringLiteral("[模拟]"), m_mapToolbar);
    simTag->setObjectName(QStringLiteral("mapToolbarSimTag"));
    // 模拟角标：caption 排版 + 中威胁语义色（labelRole/textColor 组合，见词汇表约定 3）
    simTag->setProperty("labelRole", QStringLiteral("caption"));
    simTag->setProperty("textColor", QStringLiteral("medium"));
    layout->addWidget(simTag);

    // 探测控制按钮信号
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartDetection);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopDetection);
    connect(m_resetBtn, &QPushButton::clicked, this, &MainWindow::onResetDetection);
    connect(m_resetViewBtn, &QPushButton::clicked, this, &MainWindow::onResetViewClicked);
}

void MainWindow::createStatusBar()
{
    constexpr int kStatusBarHostHeight = 28;
    constexpr int kStatusBarContentHeight = 22;

    m_statusBarWidget = new StatusBarWidget(this);
    m_statusBarWidget->setFixedHeight(kStatusBarContentHeight);
    statusBar()->addWidget(m_statusBarWidget, 1);
    statusBar()->setFixedHeight(kStatusBarHostHeight);
}

void MainWindow::createConnections()
{
    connect(m_navigationWidget, &NavigationWidget::navigationChanged,
            this, &MainWindow::onNavigationChanged);

    // 目标列表 -> 三向联动
    connect(m_leftPanel, &LeftPanelWidget::targetSelected,
            this, [this](const Core::TargetInfo& target) {
        onSelectTargetEverywhere(target.id);
    });
    connect(m_leftPanel, &LeftPanelWidget::refreshSimulationRequested,
            this, &MainWindow::onRefreshSimulationRequested);

    // 视频抽帧 -> AI 检测引擎异步分析
    connect(m_videoPiP, &VideoStreamPanel::frameExtracted,
            m_detectionEngine, &DetectionEngine::analyzeFrame);

    // AI 分析结果 -> 目标生成 + 四区同步
    connect(m_detectionEngine, &DetectionEngine::imageAnalyzed,
            this, &MainWindow::onFrameAnalyzed);

    // 引擎错误 -> 状态栏告警
    connect(m_detectionEngine, &DetectionEngine::error,
            this, [this](const QString& message) {
        m_statusBarWidget->addAlarm(QStringLiteral("[AI] 检测引擎错误: %1").arg(message));
    });

    // 无人机遥测 -> 地图无人机标记 + 航迹 + 视频 HUD
    connect(m_droneSimulator, &Core::Simulation::DroneTelemetrySimulator::telemetryUpdated,
            this, &MainWindow::onTelemetryUpdated);

    // 视频结束 -> 停止模拟器
    connect(m_videoPiP, &VideoStreamPanel::videoEnded, this, &MainWindow::onVideoEnded);

    // 三向联动: 地图红点点击
    connect(m_tacticalMap, &TacticalMapWidget::targetClicked,
            this, &MainWindow::onSelectTargetEverywhere);

    // 设备资源条
    connect(m_deviceResourceBar, &DeviceResourceBar::deviceSelected,
            this, &MainWindow::onDeviceSelected);

    // PiP 标题栏按钮
    connect(m_videoPiP, &VideoStreamPanel::swapRequested, this, &MainWindow::onPipSwapClicked);
    connect(m_videoPiP, &VideoStreamPanel::minimizeRequested, this, &MainWindow::onPipMinimizeClicked);
    connect(m_videoPiP, &VideoStreamPanel::closeRequested, this, &MainWindow::onPipCloseClicked);

    // 目标详情浮层操作
    connect(m_targetDetailOverlay, &TargetDetailOverlay::createTaskRequested,
            this, &MainWindow::onCreateTaskRequested);
    connect(m_targetDetailOverlay, &TargetDetailOverlay::assignDeviceRequested,
            this, &MainWindow::onAssignDeviceRequested);
    connect(m_targetDetailOverlay, &TargetDetailOverlay::viewHistoryRequested,
            this, &MainWindow::onViewHistoryRequested);

    // 探测页: 人工校验 -> 工作流状态流转; 选中结果行 -> 三向联动
    connect(m_detectionView, &DetectionView::targetConfirmed,
            this, &MainWindow::onTargetConfirmed);
    connect(m_detectionView, &DetectionView::targetRejected,
            this, &MainWindow::onTargetRejected);
    connect(m_detectionView, &DetectionView::resultSelected,
            this, &MainWindow::onSelectTargetEverywhere);

    // MOS P0 决策页接线
    // mosStateChanged 不自动触发 setSnapshot：档位选择 emit 该信号后同步全量重建会
    // 在 PlanCardWidget 信号链中删除发信控件。改为 requestReplan/replaceObstacles 后
    // 显式推送快照，档位选择仅刷新视觉态。
    m_decisionView->setMosController(m_mosController);
    connect(m_mosController, &Core::MOS::MosPlanningController::replanActivityChanged,
            m_decisionView, &DecisionView::setPlanning);
    connect(m_decisionView, &DecisionView::replanRequested,
            this, [this]() {
        m_mosController->requestReplan(m_decisionView->currentObstacles(),
                                        m_decisionView->currentParams());
        m_decisionView->setSnapshot(m_mosController->snapshot());
    });
    connect(m_decisionView, &DecisionView::tierSelected,
            this, [this](int tierIndex) {
        m_mosController->selectTier(tierIndex);
        m_decisionView->selectTier(tierIndex);
    });
    connect(m_decisionView, &DecisionView::generatorApplied,
            this, [this](const Core::MOS::MosGeneratorParams &genParams, qint32 seed) {
        const auto runwayParams = m_decisionView->currentParams();
        const auto obstacles = Core::MOS::MosFixtureGenerator::generate(runwayParams, genParams, seed);
        // 仅替换障碍物，不自动规划；用户点击重新规划按钮后才执行 MOS 规划
        m_mosController->replaceObstacles(obstacles, runwayParams);
        m_decisionView->setSnapshot(m_mosController->snapshot());
    });
}

// 加载模拟演示场景：设备/任务/航线/卫星图/视频
// 空起步：不自动播放，等待用户点击 [开始]
void MainWindow::loadMockData()
{
    Core::Simulation::DemoScenario scenario = Core::Simulation::DemoScenarioProvider::create();

    m_simulationWorkflow.reset({});
    m_missions = scenario.missions;
    m_devices = scenario.devices;

    m_leftPanel->setTargets(m_simulationWorkflow.targets());
    m_deviceResourceBar->setDevices(m_devices);

    // 状态栏
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

    // 媒体目录: 优先编译期源码树路径（开发期）；源码树不在时回退安装布局
    // bin/../share/uxo/assets/media（对应根 CMakeLists 的 install 规则）
    QString mediaDir = QStringLiteral(MEDIA_ASSETS_DIR) + QStringLiteral("/media");
    if (!QFileInfo::exists(mediaDir + QStringLiteral("/") + QString::fromUtf8(kSatelliteFile))) {
        mediaDir = QCoreApplication::applicationDirPath()
                       + QStringLiteral("/../share/uxo/assets/media");
    }

    // 地图: 卫星底图 + 机场边界
    m_tacticalMap->setAirportBounds(scenario.airportBounds);
    m_tacticalMap->setSatelliteImage(mediaDir + QStringLiteral("/") + QString::fromUtf8(kSatelliteFile));

    // 视频: 加载演示视频文件（与卫星底图同一 media 目录）
    m_videoPiP->loadVideo(mediaDir + QStringLiteral("/") + QString::fromUtf8(kVideoFile));

    // 无人机遥测模拟器: 加载航线
    m_droneSimulator->loadRoute(scenario.droneRoute, kVideoDurationSec);

    // MOS 初始规划：seed=42 生成确定性 fixture，同步触发首次 replan 后显式推送快照
    if (m_decisionView && m_mosController) {
        Core::MOS::MosRunwayParams runwayParams;
        Core::MOS::MosGeneratorParams genParams;
        const qint32 seed = 42;
        const auto obstacles = Core::MOS::MosFixtureGenerator::generate(runwayParams, genParams, seed);
        m_mosController->requestReplan(obstacles, runwayParams);
        m_decisionView->setSnapshot(m_mosController->snapshot());
    }
}

// [开始]: 播放视频 + 启动遥测模拟器（视频抽帧定时器随播放自动启动）
void MainWindow::onStartDetection()
{
    m_videoPiP->play();
    m_droneSimulator->start();
    m_statusBarWidget->addAlarm(QStringLiteral("探测已开始"));
}

// [结束]: 停止视频并回 0s + 停止遥测模拟器（保留目标/地图/侧栏/选中状态）
void MainWindow::onStopDetection()
{
    m_videoPiP->pause();
    m_videoPiP->seek(0);
    m_droneSimulator->stop();
    m_statusBarWidget->addAlarm(QStringLiteral("探测已结束，视频回 0s"));
}

// [重置]: 停止视频 + 复位遥测模拟器 + 清空目标/检测框/航迹/探测页结果
void MainWindow::onResetDetection()
{
    m_videoPiP->pause();
    m_videoPiP->seek(0);
    m_droneSimulator->reset();

    // 清空目标
    m_simulationWorkflow.reset({});
    m_leftPanel->setTargets({});
    m_tacticalMap->clearTargets();
    m_tacticalMap->clearDroneTrack();
    m_videoPiP->overlay()->clear();
    m_targetDetailOverlay->reset();
    m_detectionView->clearResults();

    m_targetCounter = 0;
    m_evidenceByTargetId.clear();
    m_currentDroneLat = 0.0;
    m_currentDroneLng = 0.0;
    m_currentDroneAlt = 0.0;
    m_currentDroneHeading = 0.0;

    repositionFloatingWidgets();
    m_statusBarWidget->addAlarm(QStringLiteral("探测已重置"));
}

// 无人机遥测更新: 地图无人机标记 + 航迹 + 视频 HUD
void MainWindow::onTelemetryUpdated(double lat, double lng, double alt, double heading)
{
    m_currentDroneLat = lat;
    m_currentDroneLng = lng;
    m_currentDroneAlt = alt;
    m_currentDroneHeading = heading;

    // 地图: 更新无人机位置 + 添加航迹点
    m_tacticalMap->setDronePosition(lat, lng, heading);
    m_tacticalMap->addTrackPoint(lat, lng);

    // 视频 HUD: 更新遥测显示
    m_videoPiP->overlay()->setTelemetry(lat, lng, alt, heading);
}

// AI 分析结果落地: 异常帧生成目标 + 四区同步; 正常帧仅记录到探测页
void MainWindow::onFrameAnalyzed(const ImageDetectionResult& result)
{
    // 正常帧: 仅记录到探测页（无目标生成）
    if (!result.hasAnomaly) {
        m_detectionView->onFrameAnalyzed(result, QString());
        return;
    }

    // 两阶段语义：PatchCore 标记异常后，还需 YOLO 分类确认才生成目标；
    // 未获确认的异常帧与正常帧一样仅记录到探测页（对齐 Python 参考管线）
    bool yoloConfirmed = false;
    for (const ClassificationResult& c : result.classifications) {
        if (c.bestClass >= 0) {
            yoloConfirmed = true;
            break;
        }
    }
    if (!yoloConfirmed) {
        m_detectionView->onFrameAnalyzed(result, QString());
        return;
    }

    // 异常帧: 生成目标 ID + TargetInfo
    m_targetCounter++;
    const QString targetId = QStringLiteral("T-%1").arg(m_targetCounter, 3, 10, QLatin1Char('0'));
    const Core::TargetInfo target = createDetectedTarget(result, targetId);

    // 1. 目标表插行 + 地图红点 + 工作流目标集
    m_leftPanel->addTargetRow(target);
    m_tacticalMap->addTarget(target);
    m_simulationWorkflow.addTarget(target);

    // 2. 探测页记录异常帧（先入工作流，避免行选中联动查不到目标）
    m_detectionView->onFrameAnalyzed(result, targetId);

    // 3. 状态栏告警
    m_statusBarWidget->addAlarm(
        QStringLiteral("[AI] 探测到目标 %1（%2）").arg(targetId, target.typeName));

    // 4. 证据冻结: AI 红框标注图（无则回退热力图叠加图）+ 时间戳，不手工标注
    DetectionEvidence evidence;
    evidence.annotatedImage = result.annotatedImage.isNull()
                                   ? result.heatmapOverlay
                                   : result.annotatedImage;
    evidence.captureTime = target.detectTime;
    evidence.videoPositionMs = result.timestampMs;
    evidence.provenance = QStringLiteral("[AI] PatchCore+YOLO 自动检测");
    m_evidenceByTargetId.insert(targetId, evidence);
}

// 视频播放结束: 停止遥测模拟器
void MainWindow::onVideoEnded()
{
    m_droneSimulator->stop();
    m_statusBarWidget->addAlarm(QStringLiteral("视频播放结束，探测完成"));
}

// 三向联动: 目标表/地图/视频框 任一选中 -> 同步其他两区
void MainWindow::onSelectTargetEverywhere(const QString& targetId)
{
    if (targetId.isEmpty()) {
        return;
    }

    // 同步目标表（阻断信号避免递归）
    m_leftPanel->blockSignals(true);
    m_leftPanel->selectTargetRow(targetId);
    m_leftPanel->blockSignals(false);

    // 同步地图
    m_tacticalMap->setSelectedTarget(targetId);

    // 显示目标详情浮层 + 证据快照
    if (m_simulationWorkflow.selectTarget(targetId)) {
        const Core::TargetInfo *selected = m_simulationWorkflow.selectedTarget();
        if (selected != nullptr) {
            m_targetDetailOverlay->showTarget(*selected);
            auto it = m_evidenceByTargetId.find(targetId);
            if (it != m_evidenceByTargetId.end()) {
                m_targetDetailOverlay->setEvidence(it->annotatedImage, it->captureTime,
                                                    it->videoPositionMs, it->provenance);
            } else {
                m_targetDetailOverlay->clearEvidence();
            }
            repositionFloatingWidgets();
        }
    }
}

// 根据 TargetType 获取中文显示名称
QString MainWindow::targetTypeName(Core::TargetType type) const
{
    switch (type) {
    case Core::TargetType::AntiRunwayMine:
        return QStringLiteral("反跑道雷");
    case Core::TargetType::AirBomb:
        return QStringLiteral("航弹");
    case Core::TargetType::ClusterBomb:
        return QStringLiteral("子母弹");
    case Core::TargetType::IED:
        return QStringLiteral("简易爆炸装置");
    case Core::TargetType::Rocket:
        return QStringLiteral("火箭弹");
    case Core::TargetType::Mortar:
        return QStringLiteral("迫击炮弹");
    case Core::TargetType::Grenade:
        return QStringLiteral("手榴弹");
    case Core::TargetType::Projectile:
        return QStringLiteral("投射物");
    case Core::TargetType::Fuze:
        return QStringLiteral("引信");
    default:
        return QStringLiteral("未知目标");
    }
}

// 根据检测框归一化坐标 + 无人机遥测(含航向)推算目标地面经纬度
// 简化投影模型: 无人机垂直俯视, 画面中心对准无人机地面投影点
// 坐标基: 视频X正向=相机右舷(camera-right), 视频Y正向=相机下方(camera-down, 画面底部);
//         航向 0=北, 顺时针。画面顶部对应航向正前方, 故 forwardOffset=(0.5-cy)*H。
void MainWindow::calculateTargetCoord(double droneLat, double droneLng, double droneAlt,
                                       double heading,
                                       const QRectF& videoRect,
                                       double& outLat, double& outLng) const
{
    // 检测框中心归一化坐标
    double cx = videoRect.x() + videoRect.width() / 2.0;
    double cy = videoRect.y() + videoRect.height() / 2.0;

    // 画面中心偏移 -> 相机坐标系地面偏移（米）
    // rightOffset: 正=相机右舷(画面右); forwardOffset: 正=航向正前方(画面顶部)
    // 画面 y 向下增加, 正前方在画面顶部, 故 forwardOffset 取 0.5-cy
    double rightOffset = (cx - 0.5) * kFootprintWidthM;
    double forwardOffset = (0.5 - cy) * kFootprintHeightM;

    // 按当前航向把相机系偏移旋转到东/北地面系(米)
    // 航向 h(度, 0=北, 顺时针): east=right*cos(h)+forward*sin(h); north=-right*sin(h)+forward*cos(h)
    double hRad = heading * M_PI / 180.0;
    double cosH = qCos(hRad);
    double sinH = qSin(hRad);
    double east = rightOffset * cosH + forwardOffset * sinH;   // 正=东
    double north = -rightOffset * sinH + forwardOffset * cosH; // 正=北

    // 地面偏移(米) -> 经纬度偏移
    // 1 度纬度 ≈ 111000m; 1 度经度 ≈ 111000m * cos(lat) (纬度相关经度比例)
    double deltaLat = north / 111000.0;
    double deltaLng = east / (111000.0 * qCos(droneLat * M_PI / 180.0));

    outLat = droneLat + deltaLat;
    outLng = droneLng + deltaLng;
}

Core::TargetType MainWindow::targetTypeFromClassName(const QString& className) const
{
    if (className == QLatin1String("aircraft-bombs")) {
        return Core::TargetType::AirBomb;
    }
    if (className == QLatin1String("landmines")) {
        return Core::TargetType::AntiRunwayMine;
    }
    if (className == QLatin1String("rockets")) {
        return Core::TargetType::Rocket;
    }
    if (className == QLatin1String("submunitions")) {
        return Core::TargetType::ClusterBomb;
    }
    if (className == QLatin1String("mortars")) {
        return Core::TargetType::Mortar;
    }
    if (className == QLatin1String("grenades")) {
        return Core::TargetType::Grenade;
    }
    if (className == QLatin1String("projectiles")) {
        return Core::TargetType::Projectile;
    }
    if (className == QLatin1String("fuzes")) {
        return Core::TargetType::Fuze;
    }
    return Core::TargetType::Other;
}

Core::TargetInfo MainWindow::createDetectedTarget(const ImageDetectionResult& result,
                                                  const QString& targetId) const
{
    // 置信度最高的有效分类；onFrameAnalyzed 已按 YOLO 确认门控，
    // 无分类分支仅为防御性兜底，置信度不再回退异常分数（此前会显示 >100%）
    const ClassificationResult *bestClass = nullptr;
    for (const ClassificationResult& c : result.classifications) {
        if (c.bestClass >= 0 && (bestClass == nullptr || c.confidence > bestClass->confidence)) {
            bestClass = &c;
        }
    }
    const PatchResult *worstPatch = nullptr;
    for (const PatchResult& p : result.patches) {
        if (p.isAnomalous && (worstPatch == nullptr || p.normalizedScore > worstPatch->normalizedScore)) {
            worstPatch = &p;
        }
    }

    Core::TargetInfo target;
    target.id = targetId;
    if (bestClass != nullptr) {
        target.type = targetTypeFromClassName(bestClass->bestClassName);
        target.confidence = bestClass->confidence;
    } else {
        target.type = Core::TargetType::Other;
        target.confidence = 0.0f;
    }
    target.typeName = targetTypeName(target.type);

    // UXO 紧包框 -> 归一化视频框（与引擎红框同源）；无有效框时回退格子框（4x4 网格，每格 0.25）
    const PatchResult *selPatch = nullptr;
    if (bestClass != nullptr) {
        for (const PatchResult& p : result.patches) {
            if (p.row == bestClass->patchRow && p.col == bestClass->patchCol) {
                selPatch = &p;
                break;
            }
        }
    }
    if (selPatch == nullptr) {
        selPatch = worstPatch;
    }
    QRectF videoRect(0.0, 0.0, 0.25, 0.25);
    if (selPatch != nullptr && selPatch->targetRect.isValid()) {
        const qreal kInvImage = 1.0 / static_cast<qreal>(DetectionConst::IMAGE_SIZE);
        videoRect = QRectF(static_cast<qreal>(selPatch->targetRect.x()) * kInvImage,
                           static_cast<qreal>(selPatch->targetRect.y()) * kInvImage,
                           static_cast<qreal>(selPatch->targetRect.width()) * kInvImage,
                           static_cast<qreal>(selPatch->targetRect.height()) * kInvImage);
    } else {
        const int gridRow = (bestClass != nullptr) ? bestClass->patchRow
                            : ((worstPatch != nullptr) ? worstPatch->row : 0);
        const int gridCol = (bestClass != nullptr) ? bestClass->patchCol
                            : ((worstPatch != nullptr) ? worstPatch->col : 0);
        videoRect = QRectF(static_cast<qreal>(gridCol) / 4.0,
                           static_cast<qreal>(gridRow) / 4.0,
                           0.25, 0.25);
    }

    // 推算目标地面坐标（经纬度）
    double targetLat = 0.0;
    double targetLng = 0.0;
    calculateTargetCoord(m_currentDroneLat, m_currentDroneLng, m_currentDroneAlt,
                         m_currentDroneHeading, videoRect, targetLat, targetLng);

    target.position = QVector3D(targetLng, targetLat, 0.0f);
    target.status = Core::TargetStatus::Detected;
    target.threatLevel = Core::ThreatLevel::High;
    target.detectTime = QDateTime::currentDateTime();
    target.remark = QStringLiteral("[AI] 视频抽帧分析");
    return target;
}

void MainWindow::onTargetConfirmed(const QString& targetId)
{
    if (m_simulationWorkflow.selectTarget(targetId)
            && m_simulationWorkflow.requestSelectedTargetStatus(Core::TargetStatus::Confirmed)) {
        m_leftPanel->updateTargetStatus(targetId, Core::TargetStatus::Confirmed);
    }
}

void MainWindow::onTargetRejected(const QString& targetId)
{
    if (m_simulationWorkflow.selectTarget(targetId)
            && m_simulationWorkflow.markSelectedTargetFalseAlarm()) {
        m_leftPanel->updateTargetStatus(targetId, Core::TargetStatus::FalseAlarm);
    }
}

void MainWindow::onNavigationChanged(int index)
{
    qDebug() << "Navigation changed to:" << index;

    // 路由：导航 index 1（探测）切到探测页，index 2（决策）切到 MOS 决策页，
    // 其余 index 保持态势工作区
    if (!m_pageStack) {
        return;
    }
    int page = (index == 1) ? 1 : (index == 2) ? 2 : 0;
    m_pageStack->setCurrentIndex(page);
}

void MainWindow::onRefreshSimulationRequested()
{
    m_leftPanel->setTargets(m_simulationWorkflow.targets());
    m_deviceResourceBar->setDevices(m_devices);
}

void MainWindow::onCreateTaskRequested(const Core::TargetInfo& target)
{
    Q_UNUSED(target)
}

void MainWindow::onAssignDeviceRequested(const Core::TargetInfo& target)
{
    Q_UNUSED(target)
}

void MainWindow::onViewHistoryRequested(const Core::TargetInfo& target)
{
    Q_UNUSED(target)
}

void MainWindow::onDeviceSelected(const Core::DeviceInfo& device)
{
    // 选中设备即重新打开视频 PiP：恢复可见且非最小化状态，更新标题并重排浮层。
    m_pipVisible = true;
    m_pipMinimized = false;
    m_videoPiP->setDeviceTitle(QStringLiteral("%1 %2").arg(device.id, device.name));
    repositionFloatingWidgets();
    m_statusBarWidget->addAlarm(QStringLiteral("[模拟] 已切换视频源: %1").arg(device.id));
}

void MainWindow::onPipMinimizeClicked()
{
    m_pipMinimized = !m_pipMinimized;
    repositionFloatingWidgets();
}

void MainWindow::onPipCloseClicked()
{
    m_pipVisible = false;
    repositionFloatingWidgets();
}

void MainWindow::onPipSwapClicked()
{
    m_videoIsMain = !m_videoIsMain;
    repositionFloatingWidgets();
}

void MainWindow::onResetViewClicked()
{
    m_statusBarWidget->addAlarm(QStringLiteral("[模拟] 视角已复位"));
}

void MainWindow::repositionFloatingWidgets()
{
    if (m_mapContainer == nullptr) {
        return;
    }
    const int cw = m_mapContainer->width();
    const int ch = m_mapContainer->height();

    QWidget *mainWidget = m_videoIsMain ? static_cast<QWidget *>(m_videoPiP) : static_cast<QWidget *>(m_tacticalMap);
    QWidget *pipWidget = m_videoIsMain ? static_cast<QWidget *>(m_tacticalMap) : static_cast<QWidget *>(m_videoPiP);

    if (mainWidget != nullptr) {
        if (mainWidget == m_videoPiP) {
            m_videoPiP->setMinimized(false);
        }
        mainWidget->setGeometry(0, 0, cw, ch);
        mainWidget->raise();
        mainWidget->show();
    }

    if (!m_pipVisible) {
        if (pipWidget != nullptr) pipWidget->hide();
    } else if (m_pipMinimized) {
        if (pipWidget == m_videoPiP) {
            m_videoPiP->setMinimized(true);
        }
        const int pipH = kPipTitleBarHeight;
        pipWidget->setGeometry(kPipMargin, ch - pipH - kPipMargin,
                               kPipWidth, pipH);
        pipWidget->show();
        pipWidget->raise();
    } else {
        if (pipWidget == m_videoPiP) {
            m_videoPiP->setMinimized(false);
        }
        pipWidget->setGeometry(kPipMargin, ch - kPipHeight - kPipMargin,
                               kPipWidth, kPipHeight);
        pipWidget->show();
        pipWidget->raise();
    }

    // PiP 阴影层同步：仅常规模式（视频为浮窗）投影；交换模式下地图浮于原生
    // 视频之上，底衬投影会被原生窗口遮挡，此轮不投影（design-system.md §9 已知限制）
    if (m_pipShadowLayer != nullptr) {
        if (pipWidget == m_videoPiP && m_pipVisible) {
            m_pipShadowLayer->setGeometry(pipWidget->geometry());
            m_pipShadowLayer->show();
            m_pipShadowLayer->raise();
            pipWidget->raise();
        } else {
            m_pipShadowLayer->hide();
        }
    }

    if (m_targetDetailOverlay != nullptr && m_targetDetailOverlay->isVisible()) {
        const int overlayW = m_targetDetailOverlay->width();
        const int overlayH = m_targetDetailOverlay->sizeHint().height();
        m_targetDetailOverlay->setGeometry(cw - overlayW - kPipMargin,
                                             kPipMargin,
                                             overlayW,
                                             overlayH);
        // 详情浮层阴影层：同几何跟随，先抬阴影层再抬浮层，保证浮层盖在投影之上
        if (m_overlayShadowLayer != nullptr) {
            m_overlayShadowLayer->setGeometry(m_targetDetailOverlay->geometry());
            m_overlayShadowLayer->show();
            m_overlayShadowLayer->raise();
        }
        m_targetDetailOverlay->raise();
    } else if (m_overlayShadowLayer != nullptr) {
        m_overlayShadowLayer->hide();
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_mapContainer && event->type() == QEvent::Resize) {
        repositionFloatingWidgets();
    }
    // 详情浮层经关闭按钮/重置自行隐藏时不经过 repositionFloatingWidgets，
    // 在 Hide 事件同步隐藏其阴影层，避免残留无面板的悬浮投影
    if (watched == m_targetDetailOverlay && event->type() == QEvent::Hide) {
        if (m_overlayShadowLayer != nullptr) {
            m_overlayShadowLayer->hide();
        }
    }
    return QMainWindow::eventFilter(watched, event);
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

void MainWindow::on_actionViewStatusBar()
{
    statusBar()->setVisible(!statusBar()->isVisible());
}

void MainWindow::on_actionViewVideoPiP()
{
    m_pipVisible = !m_pipVisible;
    if (m_pipVisible) {
        m_pipMinimized = false;
    }
    repositionFloatingWidgets();
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

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    repositionFloatingWidgets();
}
