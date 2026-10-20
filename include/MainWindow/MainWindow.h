#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Core/Simulation/SimulationWorkflow.h"
#include "Core/Uplink/TelemetryTypes.h"
#include "Detection/DetectionTypes.h"

#include <QMainWindow>
#include <QImage>
#include <QHash>
#include <QDateTime>

class QPushButton;
class QLabel;
class QButtonGroup;

class QStackedWidget;
class StatusBarWidget;
class NavigationWidget;
class VideoStreamPanel;
class LeftPanelWidget;
class TacticalMapWidget;
class DeviceResourceBar;
class TargetDetailOverlay;
class AlertPanel;
class EventLogWidget;
class DecisionView;
class DetectionView;
class DetectionEngine;

namespace Core::MOS {
class MosPlanningController;
}

namespace Core::Uplink {
class TelemetrySourceManager;
}

namespace Core::TrackStore {
class TrackStore;
}

// 主窗口: 态势页布局
// 导航(80px) | 左pane(可折叠40/320px) | 中心区(设备资源条36px + 地图主舞台 + 告警与日志条带140px)
// 地图主舞台上浮动: 视频PiP(左下480x270) + 目标详情浮层(右上340px) + 数据源徽标(左上)
// 探测工具栏: [本地模拟|真实链路] [重置][开始][结束] 驱动无人机遥测+AI 检测（视频抽帧）
// 遥测数据源经 TelemetrySourceManager 统一管理: 本地模拟 / 真实链路（REQ-015 WP3）
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_actionNewTask();
    void on_actionOpenPlan();
    void on_actionSavePlan();
    void on_actionExit();
    void on_actionViewLeftPanel();
    void on_actionViewStatusBar();
    void on_actionViewVideoPiP();
    void on_actionSystemSettings();
    void on_actionAbout();
    void onNavigationChanged(int index);

    // 探测工具栏控制
    void onStartDetection();
    void onStopDetection();
    void onResetDetection();

    // 数据源回调（TelemetrySourceManager 统一转发，REQ-015 WP3）
    void onTelemetryUpdated(const Core::Uplink::TelemetryData& data);
    void onSourceSegmentClicked(int id);  // 分段控件切换数据源
    void onSourceModeChanged(Core::Uplink::SourceMode mode);
    void onLinkStateChanged(Core::Uplink::LinkState state);
    // HMS 告警事件: 冻结级别映射 -> 告警面板 + 事件日志双区分发 + 状态栏轻提示
    void onAlertReceived(const Core::Uplink::AlertEvent &event);
    void onVideoEnded();

    // AI 检测引擎回调
    void onFrameAnalyzed(const ImageDetectionResult& result);

    // 探测页人工校验回调
    void onTargetConfirmed(const QString& targetId);
    void onTargetRejected(const QString& targetId);

    // 三向联动: 目标表/地图/视频框 任一选中 -> 同步其他两区
    void onSelectTargetEverywhere(const QString& targetId);

    // 旧有回调
    void onRefreshSimulationRequested();
    void onPipMinimizeClicked();
    void onPipCloseClicked();
    void onPipSwapClicked();
    void onResetViewClicked();
    void onDeviceSelected(const Core::DeviceInfo& device);
    void onCreateTaskRequested(const Core::TargetInfo& target);
    void onAssignDeviceRequested(const Core::TargetInfo& target);
    void onViewHistoryRequested(const Core::TargetInfo& target);

private:
    void setupUi();
    void createMenuBar();
    void createStatusBar();
    void createMainLayout();
    void createMapToolbar();
    void createConnections();
    void loadMockData();
    void repositionFloatingWidgets();
    // 刷新数据源徽标文案/语义色（模拟 [模拟] / 真实链路按链路状态显示）
    void updateSourceBadge();
    // 刷新数据源分段控件与 [开始] 可用性（运行中/连接中锁定切换）
    void updateSourceControlStates();

    // 根据 TargetType 获取中文显示名称
    QString targetTypeName(Core::TargetType type) const;
    // 根据检测框归一化坐标 + 无人机遥测(含航向)推算目标地面经纬度
    void calculateTargetCoord(double droneLat, double droneLng, double droneAlt,
                              double heading,
                              const QRectF& videoRect,
                              double& outLat, double& outLng) const;
    // YOLOv8-cls 类别名 -> TargetType
    Core::TargetType targetTypeFromClassName(const QString& className) const;
    // 从 AI 分析结果构造 TargetInfo: 类型/置信度取最优分类, 坐标由异常 patch 推算
    Core::TargetInfo createDetectedTarget(const ImageDetectionResult& result,
                                          const QString& targetId) const;

    // 检测证据记录（内存中冻结，不持久化）
    struct DetectionEvidence {
        QImage annotatedImage;
        QDateTime captureTime;
        qint64 videoPositionMs;
        QString provenance;
    };

    NavigationWidget *m_navigationWidget;
    LeftPanelWidget *m_leftPanel;
    StatusBarWidget *m_statusBarWidget;

    TacticalMapWidget *m_tacticalMap;
    DeviceResourceBar *m_deviceResourceBar;
    TargetDetailOverlay *m_targetDetailOverlay;

    QWidget *m_mapContainer;
    VideoStreamPanel *m_videoPiP;
    // 本地演示视频路径（loadMockData 解析后缓存；数据源切回本地模拟时重载，REQ-015 WP5 裁决⑥）
    QString m_localVideoPath;
    // 浮动面板阴影底衬（PiP 与目标详情浮层的投影层，见 MainWindow.cpp 匿名命名空间）
    QWidget *m_pipShadowLayer;
    QWidget *m_overlayShadowLayer;
    QWidget *m_mapToolbar;
    QPushButton *m_resetViewBtn;
    QPushButton *m_startBtn;       // 探测工具栏 [开始]
    QPushButton *m_stopBtn;        // 探测工具栏 [结束]
    QPushButton *m_resetBtn;       // 探测工具栏 [重置]
    // 数据源分段控件: [本地模拟|真实链路] 互斥切换（REQ-015 WP3）
    QButtonGroup *m_sourceGroup;         // 互斥组，按钮 id=SourceMode 枚举值
    QPushButton *m_simSourceBtn;         // 分段 [本地模拟]
    QPushButton *m_realSourceBtn;        // 分段 [真实链路]
    QLabel *m_sourceBadge;               // 地图舞台左上数据源徽标

    // 「告警与日志」区（REQ-015 WP4）: 态势页地图下方条带，告警面板与事件日志各半宽
    AlertPanel *m_alertPanel;       // 左半: HMS 告警列表（复活孤儿组件）
    EventLogWidget *m_eventLog;     // 右半: 只读追加式事件日志

    Core::Uplink::TelemetrySourceManager *m_telemetryManager;  // 遥测数据源管理器（持有模拟/真实源）
    Core::TrackStore::TrackStore *m_trackStore = nullptr;  // WP6 落库门面，未启用时保持 nullptr
    DetectionEngine *m_detectionEngine;

    // P0 最小页面栈：index 0 = 态势工作区，index 1 = MOS 决策页
    QStackedWidget *m_pageStack;
    QWidget *m_situationPage;
    DecisionView *m_decisionView;
    DetectionView *m_detectionView;
    // MOS P0 规划控制器（应用拥有，本地合成 fixture 驱动）
    Core::MOS::MosPlanningController *m_mosController;

    // 工作流持有唯一可变模拟目标集合；任务和设备仅供刷新只读面板。
    Core::Simulation::SimulationWorkflow m_simulationWorkflow;
    QVector<Core::MissionInfo> m_missions;
    QVector<Core::DeviceInfo> m_devices;

    // 当前无人机遥测（用于检测时坐标推算）
    double m_currentDroneLat;
    double m_currentDroneLng;
    double m_currentDroneAlt;
    double m_currentDroneHeading;  // 最新航向(度, 0=北, 顺时针)

    int m_targetCounter;  // 目标 ID 生成计数器

    // 真实链路状态判定标志（区分连接失败回退与曾连接后断链，见 onLinkStateChanged）
    bool m_realLinkConnectingSeen;  // 本轮进入真实链路后是否见过 Connecting
    bool m_realLinkEverConnected;   // 本轮真实链路是否成功连接过

    QHash<QString, DetectionEvidence> m_evidenceByTargetId;

    bool m_leftPanelVisible;
    bool m_pipMinimized;
    bool m_pipVisible;
    bool m_videoIsMain;
};

#endif
