// MOS 跑道画布鼠标/滚轮交互实现：从 MosRunwayWidget.cpp 拆分以遵守 250 纯 LOC 上限。
// 命中测试与 paintEvent 共享 MosRunwayInternal 几何映射助手，确保点击区域与绘制对齐。
// 所有交互仅触发信号或更新本地平移/缩放状态，不发起业务、不联网。
// 几何缩放围绕视口中心：变换为 translate(center+pan) × scale(zoom) × translate(-center)，
// 因此 zoom 变化不需要修改 m_panOffset 即可保持视口中心稳定；
// 拖拽平移增量经 clampPan 钳制，保证内容不会被拖出视口。

#include "MainWindow/MosRunwayWidget.h"
#include "MosRunwayWidgetInternal.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>

using namespace MosRunwayInternal;

void MosRunwayWidget::mousePressEvent(QMouseEvent *event)
{
    // 中键启动拖拽平移
    if (event->button() == Qt::MiddleButton) {
        m_dragging = true;
        m_dragStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    // 命中测试经 widget->content 逆变换，与 paintEvent 的 contentTransform 对齐
    const QPointF widgetPt = event->pos();
    const QString targetId = hitTestTarget(widgetPt);
    if (!targetId.isEmpty()) {
        emit targetClicked(targetId);
        return;
    }
    const int tier = hitTestTier(widgetPt);
    if (tier >= 0) {
        emit tierClicked(tier);
    }
}

void MosRunwayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        return;
    }
    // 增量平移：累加本次移动量，并钳制到内容溢出半幅内
    m_panOffset += event->pos() - m_dragStart;
    m_dragStart = event->pos();
    m_panOffset = clampPan(m_panOffset, m_zoom, width(), height());
    emit panXChanged(static_cast<int>(m_panOffset.x()));
    update();
}

void MosRunwayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void MosRunwayWidget::wheelEvent(QWheelEvent *event)
{
    // 角度增量每 120 为一档，步长 0.25，范围 [0.5, 3.0]
    // 几何缩放围绕视口中心（由 contentTransform 实现），与 +/- 按钮共用 setZoomDisplay 路径
    const int steps = event->angleDelta().y() / 120;
    if (steps == 0) {
        return;
    }
    setZoomDisplay(m_zoom + steps * 0.25);
}
