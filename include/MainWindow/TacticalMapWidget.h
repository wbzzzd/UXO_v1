#ifndef MAINWINDOW_TACTICALMAPWIDGET_H
#define MAINWINDOW_TACTICALMAPWIDGET_H

// 2D 战术地图控件: 卫星底图 + 经纬度坐标系 + 目标红点(脉冲) + 无人机标记/航迹
// 用于探测阶段态势页中心区, 替代原 Qt3D 三维视图
// 坐标系: WGS84 经纬度, 机场区域由 setAirportBounds 设定

#include <QWidget>
#include <QHash>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QRectF>
#include "Core/Data/Types.h"
#include "Core/Simulation/DemoScenarioProvider.h"

class QGraphicsView;
class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsTextItem;
class QGraphicsPolygonItem;
class QGraphicsPathItem;
class QTimer;

class TacticalMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TacticalMapWidget(QWidget *parent = nullptr);
    ~TacticalMapWidget();

    // 设置机场区域边界（经纬度），用于坐标转换和卫星图对齐
    void setAirportBounds(const Core::Simulation::AirportBounds& bounds);
    // 加载卫星底图（本地图片文件路径）
    void setSatelliteImage(const QString& path);

    // 全量替换目标列表 (重置场景)
    void setTargets(const QVector<Core::TargetInfo>& targets);
    // 增量插入单个目标 (探测阶段检测驱动)
    void addTarget(const Core::TargetInfo& target);
    // 清空所有目标
    void clearTargets();
    // 程序化选中目标 (与目标表/视频框双向高亮)
    void setSelectedTarget(const QString& targetId);
    // 当前选中目标 ID (空串表示未选中)
    QString selectedTargetId() const;
    // 当前地图目标数
    int targetCount() const;

    // 无人机标记: 更新位置（经纬度+航向），蓝色三角标记
    void setDronePosition(double lat, double lng, double heading);
    // 添加航迹点（经纬度），虚线连接
    void addTrackPoint(double lat, double lng);
    // 清除无人机标记和航迹
    void clearDroneTrack();

signals:
    // 用户点击地图上的目标点时发出, 携带目标 ID
    void targetClicked(const QString& targetId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPulseTick();

private:
    void setupUi();
    // 经纬度 -> 场景坐标转换
    QPointF latLngToScene(double lat, double lng) const;
    // TargetInfo.position(x=lng, y=lat) -> 场景坐标
    QPointF mapToScene(const QVector3D& pos) const;
    // 根据目标威胁等级获取标记颜色
    QColor threatColor(const Core::TargetInfo& target) const;
    // 判断目标是否为待检测状态
    bool isPending(const Core::TargetInfo& target) const;
    // 加载卫星底图
    void loadSatelliteBackground();

    QGraphicsView *m_view;
    QGraphicsScene *m_scene;
    QTimer *m_pulseTimer;

    Core::Simulation::AirportBounds m_bounds;  // 机场区域边界
    QRectF m_imageRect;  // 卫星底图在场景中的显示矩形（aspect-fit 居中, 含信箱偏移）

    // 无人机标记和航迹
    QGraphicsPolygonItem *m_droneMarker;
    QGraphicsTextItem *m_droneLabel;
    QGraphicsPathItem *m_trackPath;
    QVector<QPointF> m_trackPoints;

    // 单个目标的图形元素集合
    struct TargetGraphics {
        QGraphicsEllipseItem *dot;       // 中心标记点
        QGraphicsEllipseItem *glow;      // 发光外圈 (仅已检测目标)
        QGraphicsEllipseItem *pulse;     // 脉冲环 (仅已检测目标)
        QGraphicsTextItem *label;        // ID 标签
        QString id;
        bool pending;
        QColor color;
    };

    QHash<QString, TargetGraphics> m_items;
    QString m_selectedId;
    int m_pulsePhase;
};

#endif
