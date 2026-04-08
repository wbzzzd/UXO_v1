/**
 * @file SituationView.h
 * @brief 态势显示视图头文件
 * @details 3D机场场景显示控件，使用AirportSceneFactory创建场景
 */
#ifndef SITUATIONVIEW_H
#define SITUATIONVIEW_H

#include <QWidget>
#include <QString>
#include <QVector3D>
#include <QToolBar>
#include <QLabel>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/Qt3DWindow>

#include "Core/Data/AirportData.h"
#include "Core/3D/AirportSceneFactory.h"

class SituationView : public QWidget
{
    Q_OBJECT

public:
    explicit SituationView(QWidget *parent = nullptr);
    ~SituationView();

    void setCameraView(const QString &view);
    void resetCameraView();
    void addTargetMarker(const QString &targetId, const QVector3D &position);
    void removeTargetMarker(const QString &targetId);
    void updateTargetPosition(const QString &targetId, const QVector3D &position);
    void clearAllTargets();
    void focusOnTarget(const QVector3D &position);
    void highlightTarget(const QString &targetId, bool highlight);

    /**
     * @brief 加载机场数据并重新渲染
     * @param airportData 机场数据
     */
    void loadAirportData(const Core::AirportData& airportData);

signals:
    void targetClicked(const QString &targetId);
    void targetDoubleClicked(const QString &targetId);

private:
    void setup3DView();
    void setupToolBar();
    void updateZoomLabel();

    Qt3DExtras::Qt3DWindow *m_3dWindow;
    Qt3DCore::QEntity *m_rootEntity;
    Qt3DRender::QCamera *m_camera;
    Qt3DExtras::QOrbitCameraController *m_cameraController;
    
    QToolBar *m_toolBar;           ///< 3D视图工具栏
    QLabel *m_zoomLabel;            ///< 缩放级别显示标签
    QLabel *m_positionLabel;        ///< 相机位置显示标签
    
    Core::AirportData m_airportData;           ///< 当前机场数据
    Core::AirportSceneFactory* m_sceneFactory; ///< 3D场景工厂
};

#endif