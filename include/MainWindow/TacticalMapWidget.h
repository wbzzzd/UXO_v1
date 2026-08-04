#ifndef MAINWINDOW_TACTICALMAPWIDGET_H
#define MAINWINDOW_TACTICALMAPWIDGET_H

#include <QWidget>
#include <QHash>
#include <QString>
#include "Core/Data/Types.h"

class QGraphicsView;
class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsTextItem;
class QTimer;

// 2D 战术地图控件: 米坐标系 (0-5000m), 占位卫星底图, 目标红点 + 脉冲动画 + ID 标签
// 用于探测阶段态势页中心区, 替代原 Qt3D 三维视图
class TacticalMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TacticalMapWidget(QWidget *parent = nullptr);
    ~TacticalMapWidget();

    // 全量替换目标列表 (重置场景)
    void setTargets(const QVector<Core::TargetInfo>& targets);
    // 增量插入单个目标 (探测阶段脚本驱动用)
    void addTarget(const Core::TargetInfo& target);
    // 清空所有目标
    void clearTargets();
    // 程序化选中目标 (与目标表双向高亮)
    void setSelectedTarget(const QString& targetId);

signals:
    // 用户点击地图上的目标点时发出, 携带目标 ID
    void targetClicked(const QString& targetId);

private slots:
    // 脉冲动画 tick: 周期性放大并淡出脉冲环
    void onPulseTick();

private:
    void setupUi();
    // 将米坐标 (0-5000) 映射到场景坐标
    QPointF mapToScene(const QVector3D& pos) const;

    QGraphicsView *m_view;
    QGraphicsScene *m_scene;
    QTimer *m_pulseTimer;

    // 单个目标的图形元素集合
    struct TargetGraphics {
        QGraphicsEllipseItem *dot;       // 中心红点
        QGraphicsEllipseItem *pulse;     // 脉冲环 (动画)
        QGraphicsTextItem *label;        // ID 标签
        QString id;
    };

    QHash<QString, TargetGraphics> m_items;
    QString m_selectedId;
    int m_pulsePhase;  // 脉冲相位 0..N, 用于驱动动画
};

#endif
