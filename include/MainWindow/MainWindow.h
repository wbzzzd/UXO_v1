#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Core/Simulation/SimulationWorkflow.h"

#include <QMainWindow>
#include <QImage>
#include <QHash>
#include <QDateTime>

class QPushButton;
class QLabel;

class StatusBarWidget;
class NavigationWidget;
class VideoStreamPanel;
class LeftPanelWidget;
class TacticalMapWidget;
class DeviceResourceBar;
class TargetDetailOverlay;

namespace Core::Simulation {
class DroneTelemetrySimulator;
class DetectionSimulator;
}

// 主窗口: 态势页布局
// 导航(80px) | 左pane(可折叠40/320px) | 中心区(设备资源条36px + 地图主舞台)
// 地图主舞台上浮动: 视频PiP(左下480x270) + 目标详情浮层(右上340px)
// 探测工具栏: [重置][开始][结束] 驱动无人机遥测+检测模拟器
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

    // 模拟器回调
    void onTelemetryUpdated(double lat, double lng, double alt, double heading);
    void onDetectionOccurred(const Core::DetectionResult& result);
    void onVideoEnded();

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

    // 根据 TargetType 获取中文显示名称
    QString targetTypeName(Core::TargetType type) const;
    // 根据检测框归一化坐标 + 无人机遥测(含航向)推算目标地面经纬度
    void calculateTargetCoord(double droneLat, double droneLng, double droneAlt,
                              double heading,
                              const QRectF& videoRect,
                              double& outLat, double& outLng) const;
    // 在证据截图上绘制归一化检测框和目标标签
    void annotateEvidenceImage(QImage& image, const Core::DetectionResult& result,
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
    QWidget *m_mapToolbar;
    QPushButton *m_resetViewBtn;
    QPushButton *m_startBtn;       // 探测工具栏 [开始]
    QPushButton *m_stopBtn;        // 探测工具栏 [结束]
    QPushButton *m_resetBtn;       // 探测工具栏 [重置]

    Core::Simulation::DroneTelemetrySimulator *m_droneSimulator;
    Core::Simulation::DetectionSimulator *m_detectionSimulator;

    Core::Simulation::SimulationWorkflow m_simulationWorkflow;
    QVector<Core::MissionInfo> m_missions;
    QVector<Core::DeviceInfo> m_devices;

    // 当前无人机遥测（用于检测时坐标推算）
    double m_currentDroneLat;
    double m_currentDroneLng;
    double m_currentDroneAlt;
    double m_currentDroneHeading;  // 最新航向(度, 0=北, 顺时针)

    int m_targetCounter;  // 目标 ID 生成计数器

    QHash<QString, DetectionEvidence> m_evidenceByTargetId;

    bool m_leftPanelVisible;
    bool m_pipMinimized;
    bool m_pipVisible;
    bool m_videoIsMain;
};

#endif
