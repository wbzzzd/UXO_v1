#ifndef MAINWINDOW_MOSRUNWAYWIDGET_H
#define MAINWINDOW_MOSRUNWAYWIDGET_H

// MOS 跑道俯视图 QPainter 画布：自绘跑道/刻度/目标圆圈/候选档位矩形。
// 仅按传入的 MosPlanningSnapshot 副本渲染，不持有业务状态、不发起规划、不联网。
// 所有几何与坐标均为合成本地 fixture 语义，非真实跑道或真实目标。

#include "Core/MOS/MosPlanningSession.h"
#include <QPointF>
#include <QString>
#include <QWidget>

class QTransform;
class QTimer;

class MosRunwayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MosRunwayWidget(QWidget *parent = nullptr);
    ~MosRunwayWidget() override;

    // 用快照副本刷新渲染（被动入口，由父页面调用）
    void setSnapshot(const Core::MOS::MosPlanningSnapshot &snapshot);
    // 设置当前选中档位（-1 表示无选中）
    void setSelectedTier(int tierIndex);
    // 设置当前选中目标 ID（空串表示无选中）
    void setSelectedTargetId(const QString &id);
    // 设置几何缩放系数（0.5..3.0，围绕视口中心缩放内容几何，与视口字体缩放独立）
    void setZoomDisplay(double zoom);
    // 设置视口缩放系数（由父页面 applyViewportScale 传入，仅影响字体/笔宽/标签布局）
    void setViewportScale(double scale);
    // 读取当前视口缩放系数
    double viewportScale() const { return m_viewportScale; }
    // 读取当前几何缩放系数
    double zoom() const { return m_zoom; }
    // 按增量调整几何缩放（钳制 0.5..3.0，围绕视口中心），父页面 +/- 按钮与滚轮共用此路径
    void zoomBy(double delta);
    // 复位几何缩放为 1.0 并清零平移偏移
    void resetView();

    // 设置 X 方向平移偏移（像素，由滚动条驱动；不发射 panXChanged 以防信号环）
    void setPanX(int px);
    // 读取当前 X 方向平移偏移（像素）
    int panX() const { return static_cast<int>(m_panOffset.x()); }
    // X 方向平移半幅（像素，对称 [−range, +range]；zoom<=1 时为 0）
    int panRangeX() const;

    // === 以下为测试与命中测试共享的只读访问 API（不构成调试接口）===
    // 当前内容像素 -> widget 像素的仿射变换（含 zoom 与 pan）
    QTransform contentTransform() const;
    // 内容像素坐标 -> widget 像素坐标
    QPointF mapContentToWidget(const QPointF &contentPt) const;
    // widget 像素坐标 -> 内容像素坐标（命中测试用）
    QPointF mapWidgetToContent(const QPointF &widgetPt) const;
    // 命中障碍物目标 ID（无命中返回空串）
    QString hitTestTarget(const QPointF &widgetPt) const;
    // 命中档位矩形序号（无命中返回 -1）
    int hitTestTier(const QPointF &widgetPt) const;

signals:
    // 点击目标圆圈时发出，父页面据此联动左面板与状态栏
    void targetClicked(const QString &targetId);
    // 点击候选档位矩形时发出
    void tierClicked(int tierIndex);
    // 缩放系数变化时发出（滚轮或父页面 setZoomDisplay 触发）
    void zoomChanged(double zoom);
    // X 平移偏移变化时发出（拖拽、缩放、复位触发；setPanX 不发射以防信号环）
    void panXChanged(int px);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    Core::MOS::MosPlanningSnapshot m_snapshot;
    int m_selectedTier{-1};
    QString m_selectedTargetId;
    double m_zoom{1.0};
    // 视口缩放系数（由父页面 applyViewportScale 写入，影响字体/笔宽/标签布局）
    double m_viewportScale{1.0};
    // 中键拖拽平移偏移（像素）
    QPointF m_panOffset;
    // 拖拽起始点（用于增量计算）
    QPointF m_dragStart;
    bool m_dragging{false};
    // 选中目标脉冲动画驱动（纯视觉，50ms 间隔，仅调用 update）
    QTimer *m_pulseTimer{nullptr};
    int m_pulsePhase{0}; // 0..19 循环递增，驱动正弦 alpha
};

#endif // MAINWINDOW_MOSRUNWAYWIDGET_H
